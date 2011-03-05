/*
** kwrite.c
** Writer for the Kernel Programming Language
** See Copyright Notice in klisp.h
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "kwrite.h"
#include "kobject.h"
#include "kpair.h"
#include "kstring.h"
#include "ksymbol.h"
#include "kstate.h"
#include "kerror.h"

/*
** Stack for the write FSM
** 
*/
#define push_data(ks_, data_) (ks_spush(ks_, data_))
#define pop_data(ks_) (ks_sdpop(ks_))
#define get_data(ks_) (ks_sget(ks_))
#define data_is_empty(ks_) (ks_sisempty(ks_))

/* macro for printing */
#define kw_printf(ks_, ...) fprintf((ks_)->curr_out, __VA_ARGS__)
#define kw_flush(ks_) fflush((ks_)->curr_out)

void kwrite_error(klisp_State *K, char *msg)
{
    klispE_throw(K, msg, true);
}

/*
** Helper for printing strings (correcly escapes backslashes and
** double quotes & prints embedded '\0's). It includes the surrounding
** double quotes.
*/
void kw_print_string(klisp_State *K, TValue str)
{
    int size = kstring_size(str);
    char *buf = kstring_buf(str);
    char *ptr = buf;
    int i = 0;

    kw_printf(K, "\"");

    while (i < size) {
	/* find the longest printf-able substring to avoid calling printf
	 for every char */
	for (ptr = buf; i < size && *ptr != '\0' 
		 && *ptr != '\\' && *ptr != '"'; i++, ptr++)
	    ;

	/* NOTE: this work even if ptr == buf (which can only happen the 
	 first or last time) */
	char ch = *ptr;
	*ptr = '\0';
	kw_printf(K, "%s", buf);
	*ptr = ch;

	while(i < size && (*ptr == '\0' || *ptr == '\\' || *ptr == '"')) {
	    if (*ptr == '\0')
		kw_printf(K, "%c", '\0'); /* this may not show in the terminal */
	    else 
		kw_printf(K, "\\%c", *ptr);
	    i++;
	    ptr++;
	}
	buf = ptr;
    }
			
    kw_printf(K, "\"");
}

/*
** Mark initialization and clearing
*/
void kw_clear_marks(klisp_State *K, TValue root)
{
    
    assert(ks_sisempty(K));
    push_data(K, root);

    while(!data_is_empty(K)) {
	TValue obj = get_data(K);
	pop_data(K);
	
	if (ttispair(obj)) {
	    if (kis_marked(obj)) {
		kunmark(obj);
		push_data(K, kcdr(obj));
		push_data(K, kcar(obj));
	    }
	} else if (ttisstring(obj) && (kis_marked(obj))) {
	    kunmark(obj);
	}
    }
    assert(ks_sisempty(K));
}

/*
** NOTE: 
**   - The objects that appear more than once are marked with a -1.
**   that way, the first time they are found in write, a shared def
**   token will be generated and the mark updated with the number;
**   from then on, the writer will generate a shared ref each time
**   it appears again.
**   - The objects that appear only once are marked with a #t to 
**   find repetitions and to allow unmarking after write
*/

void kw_set_initial_marks(klisp_State *K, TValue root)
{
    assert(ks_sisempty(K));
    push_data(K, root);
    
    while(!data_is_empty(K)) {
	TValue obj = get_data(K);
	pop_data(K);

	if (ttispair(obj)) {
	    if (kis_unmarked(obj)) {
		kmark(obj); /* this mark just means visited */
		push_data(K, kcdr(obj));
		push_data(K, kcar(obj));
	    } else {
		/* this mark means it will need a ref number */
		kset_mark(obj, i2tv(-1));
	    }
	} else if (ttisstring(obj)) {
	    if (kis_unmarked(obj)) {
		kmark(obj); /* this mark just means visited */
	    } else {
		/* this mark means it will need a ref number */
		kset_mark(obj, i2tv(-1));
	    }
	}
	/* all other types of object don't matter */
    }
    assert(ks_sisempty(K));
}

/*
** Writes all values except strings and pairs
*/
void kwrite_simple(klisp_State *K, TValue obj)
{
    switch(ttype(obj)) {
    case K_TSTRING:
	/* shouldn't happen */
	kwrite_error(K, "string type found in kwrite-simple");
	/* avoid warning */
	return;
    case K_TEINF:
	kw_printf(K, "#e%cinfinity", tv_equal(obj, KEPINF)? '+' : '-');
	break;
    case K_TFIXINT:
	kw_printf(K, "%" PRId32, ivalue(obj));
	break;
    case K_TNIL:
	kw_printf(K, "()");
	break;
    case K_TCHAR: {
	char ch_buf[4];
	char ch = chvalue(obj);
	char *ch_ptr;

	if (ch == '\n') {
	    ch_ptr = "newline";
	} else if (ch == ' ') {
	    ch_ptr = "space";
	} else {
	    ch_buf[0] = ch;
	    ch_buf[1] = '\0';
	    ch_ptr = ch_buf;
	}
	kw_printf(K, "#\\%s", ch_ptr);
	break;
    }
    case K_TBOOLEAN:
	kw_printf(K, "#%c", bvalue(obj)? 't' : 'f');
	break;
    case K_TSYMBOL:
	/* TEMP: access symbol structure directly */
	/* TEMP: for now assume all symbols have external representations */
	kw_printf(K, "%s", ksymbol_buf(obj));
	break;
    case K_TINERT:
	kw_printf(K, "#inert");
	break;
    case K_TIGNORE:
	kw_printf(K, "#ignore");
	break;
/* unreadable objects */
    case K_TEOF:
	kw_printf(K, "[eof]");
	break;
    case K_TENVIRONMENT:
	kw_printf(K, "[environment]");
	break;
    case K_TCONTINUATION:
	kw_printf(K, "[continuation]");
	break;
    case K_TOPERATIVE:
	kw_printf(K, "[operative]");
	break;
    case K_TAPPLICATIVE:
	kw_printf(K, "[applicative]");
	break;
    default:
	/* shouldn't happen */
	kwrite_error(K, "unknown object type");
	/* avoid warning */
	return;
    }
}


void kwrite_fsm(klisp_State *K, TValue obj)
{
    /* NOTE: a fixint is more than enough for output */
    int32_t kw_shared_count = 0;

    assert(ks_sisempty(K));
    push_data(K, obj);

    bool middle_list = false;
    while (!data_is_empty(K)) {
	TValue obj = get_data(K);
	pop_data(K);

	if (middle_list) {
	    if (ttisnil(obj)) { /* end of list */
		kw_printf(K, ")");
		/* middle_list = true; */
	    } else if (ttispair(obj) && ttisboolean(kget_mark(obj))) {
		push_data(K, kcdr(obj));
		push_data(K, kcar(obj));
		kw_printf(K, " ");
		middle_list = false;
	    } else { /* improper list is the same as shared ref */
		kw_printf(K, " . ");
		push_data(K, KNIL);
		push_data(K, obj);
		middle_list = false;
	    }
	} else { /* if (middle_list) */
	    switch(ttype(obj)) {
	    case K_TPAIR: {
		TValue mark = kget_mark(obj);
		if (ttisboolean(mark)) { /* simple pair (only once) */
		    kw_printf(K, "(");
		    push_data(K, kcdr(obj));
		    push_data(K, kcar(obj));
		    middle_list = false;
		} else if (ivalue(mark) < 0) { /* pair with no assigned # */
		    /* TEMP: for now only fixints in shared refs */
		    assert(kw_shared_count >= 0);

		    kset_mark(obj, i2tv(kw_shared_count));
		    kw_printf(K, "#%" PRId32 "=(", kw_shared_count);
		    kw_shared_count++;
		    push_data(K, kcdr(obj));
		    push_data(K, kcar(obj));
		    middle_list = false;
		} else { /* string with an assigned number */
		    kw_printf(K, "#%" PRId32 "#", ivalue(mark));
		    middle_list = true;
		}
		break;
	    }
	    case K_TSTRING: {
		if (kstring_is_empty(obj)) {
		    kw_printf(K, "\"\"");
		} else {
		    TValue mark = kget_mark(obj);
		    if (ttisboolean(mark)) { /* simple string (only once) */
			kw_print_string(K, obj);
		    } else if (ivalue(mark) < 0) { /* string with no assigned # */
			/* TEMP: for now only fixints in shared refs */
			assert(kw_shared_count >= 0);
			kset_mark(obj, i2tv(kw_shared_count));
			kw_printf(K, "#%" PRId32 "=", kw_shared_count);
			kw_shared_count++;
			kw_print_string(K, obj);
		    } else { /* string with an assigned number */
			kw_printf(K, "#%" PRId32 "#", ivalue(mark));
		    }
		}
		middle_list = true;
		break;
	    }
	    default:
		kwrite_simple(K, obj);
		middle_list = true;
	    }
	}
    }

    assert(ks_sisempty(K));
}

/*
** Writer Main function
*/
void kwrite(klisp_State *K, TValue obj)
{
    kw_set_initial_marks(K, obj);
    kwrite_fsm(K, obj);
    kw_flush(K);
    kw_clear_marks(K, obj);
}

void knewline(klisp_State *K)
{
    kw_printf(K, "\n");
    kw_flush(K);
}
