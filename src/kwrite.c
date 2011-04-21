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
#include "kinteger.h"
#include "kpair.h"
#include "kstring.h"
#include "ksymbol.h"
#include "kstate.h"
#include "kerror.h"
#include "ktable.h"

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
    klispE_throw(K, msg);
}

/* TODO: check for return codes and throw error if necessary */

void kw_print_bigint(klisp_State *K, TValue bigint)
{
    int32_t size = kbigint_print_size(bigint, 10) + 
	((kbigint_negativep(bigint))? 1 : 0);
	
    krooted_tvs_push(K, bigint);
    TValue buf_str = kstring_new_s(K, size);
    krooted_tvs_push(K, buf_str);

    /* write backwards so we can use printf later */
    char *buf = kstring_buf(buf_str) + size - 1;

    TValue copy = kbigint_copy(K, bigint);
    krooted_vars_push(K, &copy);

    /* must work with positive bigint to get the digits */
    if (kbigint_negativep(bigint))
	kbigint_invert_sign(K, copy);

    while(kbigint_has_digits(K, copy)) {
	int32_t digit = kbigint_remove_digit(K, copy, 10);
	/* write backwards so we can use printf later */
	/* XXX: use to_digit function */
	*buf-- = '0' + digit;
    }
    if (kbigint_negativep(bigint))
	*buf-- = '-';

    kw_printf(K, "%s", buf+1);

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_vars_pop(K);
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

    if (!K->write_displayp)
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
			
    if (!K->write_displayp)
	kw_printf(K, "\"");
}

/*
** Mark initialization and clearing
*/
/* GC: root is rooted */
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
/* GC: root is rooted */
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

#if KTRACK_NAMES
/* Assumes obj has a name */
TValue kw_get_name(klisp_State *K, TValue obj)
{
    const TValue *node = klispH_get(tv2table(K->name_table),
				    obj);
    klisp_assert(node != &kfree);
    return *node;
}

void kw_print_name(klisp_State *K, TValue obj)
{
    kw_printf(K, ": %s", ksymbol_buf(kw_get_name(K, obj)));
}
#endif /* KTRACK_NAMES */

#if KTRACK_SI
/* Assumes obj has a si */
void kw_print_si(klisp_State *K, TValue obj)
{
    TValue si = kget_source_info(K, obj);
    /* should be either a string or an improper list of 2 pairs,
       with a string and 2 fixints, just check if pair */
    klisp_assert(kstringp(si) || kpairp(si));
    
    kw_printf(K, " @ ");
    /* this is a hack, would be better to change the interface of 
       kw_print_string */
    bool saved_displayp = K->write_displayp; 
    K->write_displayp = true; /* avoid "s and escapes */
    if (ttisstring(si)) {
	kw_print_string(K, si);
    } else {
	TValue str = kcar(si);
	int32_t row = ivalue(kcadr(si));
	int32_t col = ivalue(kcddr(si));
	kw_print_string(K, str);
	kw_printf(K, " (row: %d, col: %d)", row, col);
    }
    K->write_displayp = saved_displayp;
}
#endif /* KTRACK_SI */

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
    case K_TBIGINT:
	kw_print_bigint(K, obj);
	break;
    case K_TNIL:
	kw_printf(K, "()");
	break;
    case K_TCHAR: {
	if (K->write_displayp) {
	    kw_printf(K, "%c", chvalue(obj));
	} else {
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
	}
	break;
    }
    case K_TBOOLEAN:
	kw_printf(K, "#%c", bvalue(obj)? 't' : 'f');
	break;
    case K_TSYMBOL:
	if (khas_ext_rep(obj)) {
	    /* TEMP: access symbol structure directly */
	    kw_printf(K, "%s", ksymbol_buf(obj));
	} else {
	    kw_printf(K, "#[symbol]");
	}
	break;
    case K_TINERT:
	kw_printf(K, "#inert");
	break;
    case K_TIGNORE:
	kw_printf(K, "#ignore");
	break;
/* unreadable objects */
    case K_TEOF:
	kw_printf(K, "#[eof]");
	break;
    case K_TENVIRONMENT:
	kw_printf(K, "#[environment");
	#if KTRACK_NAMES
	if (khas_name(obj)) {
	    kw_print_name(K, obj);
	}
	#endif
	kw_printf(K, "]");
	break;
    case K_TCONTINUATION:
	kw_printf(K, "#[continuation");
	#if KTRACK_NAMES
	if (khas_name(obj)) {
	    kw_print_name(K, obj);
	}
	#endif
	kw_printf(K, "]");
	break;
    case K_TOPERATIVE:
	kw_printf(K, "#[operative");
	#if KTRACK_NAMES
	if (khas_name(obj)) {
	    kw_print_name(K, obj);
	}
	#endif
	#if KTRACK_SI
	if (khas_si(obj))
	    kw_print_si(K, obj);
	#endif
	kw_printf(K, "]");
	break;
    case K_TAPPLICATIVE:
	kw_printf(K, "#[applicative");
	#if KTRACK_NAMES
	if (khas_name(obj)) {
	    kw_print_name(K, obj);
	}
	#endif
	#if KTRACK_SI
	if (khas_si(obj))
	    kw_print_si(K, obj);
	#endif
	kw_printf(K, "]");
	break;
    case K_TENCAPSULATION:
	/* TODO try to get the name */
	kw_printf(K, "#[encapsulation]");
	break;
    case K_TPROMISE:
	/* TODO try to get the name */
	kw_printf(K, "#[promise]");
	break;
    case K_TPORT:
	/* TODO try to get the name/ I/O direction / filename */
	kw_printf(K, "#[%s port", kport_is_input(obj)? "input" : "output");
	#if KTRACK_NAMES
	if (khas_name(obj)) {
	    kw_print_name(K, obj);
	}
	#endif
	kw_printf(K, "]");
	break;
    default:
	/* shouldn't happen */
	kwrite_error(K, "unknown object type");
	/* avoid warning */
	return;
    }
}


/* GC: obj is rooted */
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
		} else { /* pair with an assigned number */
		    kw_printf(K, "#%" PRId32 "#", ivalue(mark));
		    middle_list = true;
		}
		break;
	    }
	    case K_TSTRING: {
		if (kstring_emptyp(obj)) {
		    kw_printf(K, "\"\"");
		} else {
		    TValue mark = kget_mark(obj);
		    if (K->write_displayp || ttisboolean(mark)) { 
                        /* simple string (only once) or in display
			   (show all strings) */
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
    /* GC: root obj */
    krooted_tvs_push(K, obj);

    kw_set_initial_marks(K, obj);
    kwrite_fsm(K, obj);
    kw_flush(K);
    kw_clear_marks(K, obj);

    krooted_tvs_pop(K);
}

void kwrite_newline(klisp_State *K)
{
    kw_printf(K, "\n");
    kw_flush(K);
}
