/*
** kwrite.c
** Writer for the Kernel Programming Language
** See Copyright Notice in klisp.h
*/

#include <stdio.h>
/* XXX for malloc */
#include <stdlib.h>
/* TODO: use a generalized alloc function */
/* TEMP: for out of mem errors */
#include <assert.h>
#include <inttypes.h>

#include "kwrite.h"
#include "kobject.h"
#include "kpair.h"
#include "kstring.h"
#include "ksymbol.h"

/* TODO: move to the global state */
FILE *kwrite_file = NULL;
/* TEMP: for now use fixints for shared refs */
int32_t kw_shared_count;

/*
** Stack for the write FSM
** 
*/

/* TODO: move to the global state */
TValue *kw_dstack;
int kw_dstack_size;
int kw_dstack_i;

/* TEMP: for now stacks are fixed size, use asserts to check */
#define STACK_INIT_SIZE 1024

#define push_data(data_) ({ assert(kw_dstack_i < kw_dstack_size);	\
	    kw_dstack[kw_dstack_i++] = data_; })
#define pop_data() (--kw_dstack_i)
#define get_data() (kw_dstack[kw_dstack_i-1])
#define data_is_empty() (kw_dstack_i == 0)
#define clear_data() (kw_dstack_i = 0)

/* macro for printing */
#define kw_printf(...) fprintf(kwrite_file, __VA_ARGS__)
#define kw_flush() fflush(kwrite_file)

/*
** Helper for printing strings (correcly escapes backslashes and
** double quotes & prints embedded '\0's). It includes the surrounding
** double quotes.
*/
void kw_print_string(TValue str)
{
    int size = kstring_size(str);
    char *buf = kstring_buf(str);
    char *ptr = buf;
    int i = 0;

    kw_printf("\"");

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
	printf("%s", buf);
	*ptr = ch;

	while(i < size && (*ptr == '\0' || *ptr == '\\' || *ptr == '"')) {
	    if (*ptr == '\0')
		printf("%c", '\0'); /* this may not show in the terminal */
	    else 
		printf("\\%c", *ptr);
	    i++;
	    ptr++;
	}
	buf = ptr;
    }
			
    kw_printf("\"");
}

/*
** Writer initialization
*/
void kwrite_init()
{
    assert(kwrite_file != NULL);

    /* XXX: for now use a fixed size for stack */
    kw_dstack_size = STACK_INIT_SIZE;
    clear_data();
    kw_dstack = malloc(STACK_INIT_SIZE*sizeof(TValue));
    assert(kw_dstack != NULL);
}

/*
** Mark initialization and clearing
*/
void kw_clear_marks(TValue root)
{
    push_data(root);

    while(!data_is_empty()) {
	TValue obj = get_data();
	pop_data();
	
	if (ttispair(obj)) {
	    if (kis_marked(obj)) {
		kunmark(obj);
		push_data(kcdr(obj));
		push_data(kcar(obj));
	    }
	} else if (ttisstring(obj) && (kis_marked(obj))) {
	    kunmark(obj);
	}
    }
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

void kw_set_initial_marks(TValue root)
{
    push_data(root);
    
    while(!data_is_empty()) {
	TValue obj = get_data();
	pop_data();

	if (ttispair(obj)) {
	    if (kis_unmarked(obj)) {
		kmark(obj); /* this mark just means visited */
		push_data(kcdr(obj));
		push_data(kcar(obj));
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
}

/*
** Writes all values except strings and pairs
*/
void kwrite_simple(TValue obj)
{
    switch(ttype(obj)) {
    case K_TSTRING:
	/* this shouldn't happen */
	assert(0);
    case K_TEINF:
	kw_printf("#e%cinfinity", tv_equal(obj, KEPINF)? '+' : '-');
	break;
    case K_TFIXINT:
	kw_printf("%" PRId32, ivalue(obj));
	break;
    case K_TNIL:
	kw_printf("()");
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
	kw_printf("#\\%s", ch_ptr);
	break;
    }
    case K_TBOOLEAN:
	kw_printf("#%c", bvalue(obj)? 't' : 'f');
	break;
    case K_TSYMBOL:
	/* TEMP: access symbol structure directly */
	/* TEMP: for now assume all symbols have external representations */
	kw_printf("%s", ksymbol_buf(obj));
	break;
    case K_TINERT:
	kw_printf("#inert");
	break;
    case K_TIGNORE:
	kw_printf("#ignore");
	break;
    case K_TEOF:
	kw_printf("[eof]");
	break;
    default:
	/* shouldn't happen */
	assert(0);
    }
}


void kwrite_fsm()
{
    bool middle_list = false;
    while (!data_is_empty()) {
	TValue obj = get_data();
	pop_data();

	if (middle_list) {
	    if (ttisnil(obj)) { /* end of list */
		kw_printf(")");
		/* middle_list = true; */
	    } else if (ttispair(obj) && ttisboolean(kget_mark(obj))) {
		push_data(kcdr(obj));
		push_data(kcar(obj));
		kw_printf(" ");
		middle_list = false;
	    } else { /* improper list is the same as shared ref */
		kw_printf(" . ");
		push_data(KNIL);
		push_data(obj);
		middle_list = false;
	    }
	} else { /* if (middle_list) */
	    switch(ttype(obj)) {
	    case K_TPAIR: {
		TValue mark = kget_mark(obj);
		if (ttisboolean(mark)) { /* simple pair (only once) */
		    kw_printf("(");
		    push_data(kcdr(obj));
		    push_data(kcar(obj));
		    middle_list = false;
		} else if (ivalue(mark) < 0) { /* pair with no assigned # */
		    /* TEMP: for now only fixints in shared refs */
		    assert(kw_shared_count >= 0);
		    kset_mark(obj, i2tv(kw_shared_count));
		    kw_printf("#%" PRId32 "=(", kw_shared_count);
		    kw_shared_count++;
		    push_data(kcdr(obj));
		    push_data(kcar(obj));
		    middle_list = false;
		} else { /* string with an assigned number */
		    kw_printf("#%" PRId32 "#", ivalue(mark));
		    middle_list = true;
		}
		break;
	    }
	    case K_TSTRING: {
		if (kstring_is_empty(obj)) {
		    kw_printf("\"\"");
		} else {
		    TValue mark = kget_mark(obj);
		    if (ttisboolean(mark)) { /* simple string (only once) */
			kw_print_string(obj);
		    } else if (ivalue(mark) < 0) { /* string with no assigned # */
			/* TEMP: for now only fixints in shared refs */
			assert(kw_shared_count >= 0);
			kset_mark(obj, i2tv(kw_shared_count));
			kw_printf("#%" PRId32 "=", kw_shared_count);
			kw_shared_count++;
			kw_print_string(obj);
		    } else { /* string with an assigned number */
			kw_printf("#%" PRId32 "#", ivalue(mark));
		    }
		}
		middle_list = true;
		break;
	    }
	    default:
		kwrite_simple(obj);
		middle_list = true;
	    }
	}
    }
    return;
}

/*
** Writer Main function
*/
void kwrite(TValue obj)
{
    assert(data_is_empty());

    kw_shared_count = 0;
    kw_set_initial_marks(obj);

    push_data(obj);
    kwrite_fsm();
    kw_flush();

    kw_clear_marks(obj);

    assert(data_is_empty());
    return;
}

void knewline()
{
    kw_printf("\n");
    kw_flush();
    return;
}
