/*
** kread.c
** Reader for the Kernel Programming Language
** See Copyright Notice in klisp.h
*/


/*
** TODO:
**
** - Read mutable/immutable objects (cons function should be a parameter)
**    this is needed because some functions (like load) return immutable objs
** - Decent error handling mechanism
**
*/

#include <stdio.h>
/* XXX for malloc */
#include <stdlib.h>
/* TODO: use a generalized alloc function */
/* TEMP: for out of mem errors */
#include <assert.h>

#include "kread.h"
#include "kobject.h"
#include "kpair.h"
#include "ktoken.h"

/* TODO: move to the global state */
/* TODO: replace the list with a hashtable */
TValue shared_dict = KNIL_;
FILE *kread_file = NULL;
char *kread_filename = NULL;


/*
** Stacks for the read FSM
**
** The state stack is never empty while read is in process and
** selects the action to be performed on the next read token.
**
** The data saved in the data stack changes according to state:
** ST_FIRST_LIST: pair representing the first pair of the list
**   with source info of the '(' token.
** ST_MIDDLE_LIST, ST_LAST_ILIST: two elements, first below, second on top:
**   - a pair with car: first pair of the list (with source info 
**     corrected to car of list) and cdr: source info of the '(' token that 
**     started the [i]list.
**   - another pair, that is the last pair of the list so far. 
** ST_PAST_LAST_ILIST: a pair with car: first pair and cdr: source
**   info as above (but no pair with last pair).
** ST_SHARED_DEF: a pair with car: shared def token and cdr: source
**   info of the shared def token.
** 
*/

/* TODO: move to the global state */
typedef enum {
    ST_READ, ST_SHARED_DEF, ST_LAST_ILIST, ST_PAST_LAST_ILIST,
    ST_FIRST_LIST, ST_MIDDLE_LIST 
} state_t;

state_t *sstack;
int sstack_size;
int sstack_i;

TValue *dstack;
int dstack_size;
int dstack_i;

/* TEMP: for now stacks are fixed size, use asserts to check */
#define STACK_INIT_SIZE 1024

#define push_state(st_) ({ assert(sstack_i < sstack_size);	\
	    sstack[sstack_i++] = (st_); })
#define pop_state() (--sstack_i)
#define get_state() (sstack[sstack_i-1])
#define clear_state() (sstack_i = 0)

#define push_data(data_) ({ assert(dstack_i < dstack_size);	\
	    dstack[dstack_i++] = (data_); })
#define pop_data() (--dstack_i)
#define get_data() (dstack[dstack_i-1])
#define clear_data() (dstack_i = 0)

/*
** Error management
*/
TValue kread_error(char *str)
{
    /* TODO: Decide on error handling mechanism for reader (& tokenizer) */
    printf("READ ERROR: %s\n", str);
    return KEOF;
}

/*
** Reader initialization
*/
void kread_init()
{
    assert(kread_file != NULL);
    assert(kread_filename != NULL);

    ktok_file = kread_file;
    ktok_source_info.filename = kread_filename;
    /* XXX: For now just hardcode it to 8 spaces tab-stop */
    ktok_source_info.tab_width = 8;
    ktok_init();
    ktok_reset_source_info();

    /* XXX: for now use a fixed size for stacks */
    sstack_size = STACK_INIT_SIZE;
    clear_state();
    sstack = malloc(STACK_INIT_SIZE*sizeof(state_t));
    assert(sstack != NULL);

    dstack_size = STACK_INIT_SIZE;
    clear_data();
    dstack = malloc(STACK_INIT_SIZE*sizeof(TValue));
    assert(dstack != NULL);
}

/*
** Shared Reference Management (srfi-38) 
*/

/* This is called after kread to clear the shared alist */
void clear_shared_dict() 
{
    shared_dict = KNIL;
}

TValue try_shared_ref(TValue ref_token)
{
    /* TEMP: for now, only allow fixints in shared tokens */
    int32_t ref_num = ivalue(kcdr(ref_token));
    TValue tail = shared_dict;
    while (!ttisnil(tail)) {
	TValue head = kcar(tail);
	if (ref_num == ivalue(kcar(head)))
	    return kcdr(head);
	tail = kcdr(tail);
    }
    return kread_error("undefined shared ref found");
}

TValue try_shared_def(TValue def_token, TValue value)
{
    /* TEMP: for now, only allow fixints in shared tokens */
    int32_t ref_num = ivalue(kcdr(def_token));
    TValue tail = shared_dict;
    while (!ttisnil(tail)) {
	TValue head = kcar(tail);
	if (ref_num == ivalue(kcar(head)))
	    return kread_error("duplicate shared def found");
	tail = kcdr(tail);
    }
    
    /* XXX: what happens on out of mem? & gc?(inner cons is not rooted) */
    shared_dict = kcons(kcons(kcdr(def_token), value), 
			      shared_dict);
    return KINERT;
}

/* This overwrites a previouly made def, it is used in '() */
/* NOTE: the shared def is guaranteed to exist */
void change_shared_def(TValue def_token, TValue value)
{
    /* TEMP: for now, only allow fixints in shared tokens */
    int32_t ref_num = ivalue(kcdr(def_token));
    TValue tail = shared_dict;
    while (!ttisnil(tail)) {
	TValue head = kcar(tail);
	if (ref_num == ivalue(kcar(head))) {
	    kset_cdr(head, value);
	    return;
	}
	tail = kcdr(tail);
    }
    /* NOTE: can't really happen */
    return;
}

/*
** Reader FSM
*/
/* TEMP: For now we'll use just one big function */
TValue kread_fsm()
{
    push_state(ST_READ);

    /* read next token or process obj */
    bool read_next_token = true; 
    /* the obj just read/completed */
    TValue obj; 
    /* the source code information of that obj */
    TValue obj_si; 

    while (!(get_state() == ST_READ && !read_next_token)) {
	if (read_next_token) {
	    TValue tok = ktok_read_token();
	    if (ttispair(tok)) { /* special token */
		switch (chvalue(kcar(tok))) {
		case '(': {
		    if (get_state() == ST_PAST_LAST_ILIST)
			return kread_error("open paren found after "
					   "last element of improper list");
		    TValue np = kdummy_cons();
		    /* 
		    ** NOTE: the source info of the '(' is temporarily saved 
		    ** in np (later it will be replace by the source info
		    ** of the car of the list
		    */
		    kset_source_info(np, ktok_get_source_info());

		    /* update the shared def to point to the new list */
		    /* NOTE: this is necessary for self referrencing lists */
		    /* NOTE: the shared def was already checked for errors */
		    if (get_state() == ST_SHARED_DEF) 
			change_shared_def(kcar(get_data()), np);
		    
		    /* start reading elements of the new list */
		    push_state(ST_FIRST_LIST);
		    push_data(np);
		    read_next_token = true;
		    break;
		}
		case ')': {
		    switch(get_state()) {
		    case ST_FIRST_LIST: { /* empty list */
			/*
			** Discard the pair in sdata but
			** retain the source info
			** Return () for processing
			*/
			TValue fp_with_old_si = get_data();
			pop_data();
			pop_state();
			obj = KNIL;
			obj_si = kget_source_info(fp_with_old_si);
			read_next_token = false;
			break;
		    }
		    case ST_MIDDLE_LIST: /* end of list */
		    case ST_PAST_LAST_ILIST: { /* end of ilist */
			/* discard info on last pair */
			pop_data();
			pop_state();
			TValue fp_old_si = get_data();
			pop_data();
			pop_state();
			/* list read ok, process it in next iteration */
			obj = kcar(fp_old_si);
			obj_si = kcdr(fp_old_si);
			read_next_token = false;
			break;
		    }
		    case ST_LAST_ILIST:
			return kread_error("missing last element in "
					   "improper list");
		    case ST_SHARED_DEF:
			return kread_error("unmatched closing paren found "
					   "in shared def");
		    case ST_READ:
			return kread_error("unmatched closing paren found");
		    default:
			/* shouldn't happen */
			assert(0);
		    }
		    break;
		}
		case '.': {
		    switch(get_state()) {
		    case (ST_MIDDLE_LIST):
			/* tok ok, read next obj for cdr of ilist */
			pop_state();
			push_state(ST_LAST_ILIST);
			read_next_token = true;
			break;
		    case ST_FIRST_LIST:
			return kread_error("missing first element of "
					   "improper list");
		    case ST_LAST_ILIST:
		    case ST_PAST_LAST_ILIST:
			return kread_error("double dot in improper list");
		    case ST_SHARED_DEF:
			return kread_error("dot found in shared def");
		    case ST_READ:
			return kread_error("dot found outside list");
		    default:
			/* shouldn't happen */
			assert(0);
		    }
		    break;
		}
		case '=': { /* srfi-38 shared def */
		    switch (get_state()) {
		    case ST_SHARED_DEF:
			return kread_error("shared def found in "
					   "shared def");
		    case ST_PAST_LAST_ILIST:
			return kread_error("shared def found after "
					   "last element of improper list");
		    default: {
			TValue res = try_shared_def(tok, KNIL);
			/* TEMP: while error returns EOF */
			if (ttiseof(res)) {
			    return res;
			} else {
			    /* token ok, read defined object */
			    push_state(ST_SHARED_DEF);
			    /* NOTE: save the source info to return it 
			     after the defined object is read */
			    push_data(kcons(tok, ktok_get_source_info()));
			    read_next_token = true;
			}
		    }
		    }
		    break;
		}
		case '#': { /* srfi-38 shared ref */
		    switch(get_state()) {
		    case ST_SHARED_DEF:
			return kread_error("shared ref found in "
					   "shared def");
		    case ST_PAST_LAST_ILIST:
			return kread_error("shared ref found after "
					   "last element of improper list");
		    default: {
			TValue res = try_shared_ref(tok);
			/* TEMP: while error returns EOF */
			if (ttiseof(res)) {
			    return res;
			} else {
			    /* ref ok, process it in next iteration */
			    obj = res;
			    /* NOTE: use source info of ref token */
			    obj_si = ktok_get_source_info();
			    read_next_token = false;
			}
		    }
		    }
		    break;
		}
		default:
		    /* shouldn't happen */
		    assert(0);
		}
	    } else if (ttiseof(tok)) {
		switch (get_state()) {
		case ST_READ:
		    /* will exit in next loop */
		    obj = tok;
		    obj_si = ktok_get_source_info();
		    read_next_token = false;
		    break;
		case ST_FIRST_LIST:
		case ST_MIDDLE_LIST:
		    return kread_error("EOF found while reading list");
		case ST_LAST_ILIST:
		case ST_PAST_LAST_ILIST:
		    return kread_error("EOF found while reading "
				       "improper list");
		case ST_SHARED_DEF:
		    return kread_error("EOF found in shared def");
		default:
		    /* shouldn't happen */
		    assert(0);
		}
	    } else { /* this can only be a complete token */
		if (get_state() == ST_PAST_LAST_ILIST) {
		    return kread_error("Non paren found after last "
			"element of improper list");
		} else {
		    /* token ok, process it in next iteration */
		    obj = tok;
		    obj_si = ktok_get_source_info();
		    read_next_token = false;
		}
	    }
	} else { /* if(read_next_token) */
	    /* process the object just read */
	    switch(get_state()) {
	    case ST_FIRST_LIST: {
		TValue fp = get_data();
		/* replace source info in fp with the saved one */
		/* NOTE: the old one will be returned when list is complete */
		TValue fp_old_si = kget_source_info(fp);
		kset_source_info(fp, obj_si);
		kset_car(fp, obj);
		
		/* continue reading objects of list */
		push_state(ST_MIDDLE_LIST);
		pop_data();
		/* save first & last pair of the (still incomplete) list */
		push_data(kcons (fp, fp_old_si));
		push_data(fp);
		read_next_token = true;
		break;
	    }
	    case ST_MIDDLE_LIST: {
		TValue np = kcons(obj, KNIL);
		kset_source_info(np, obj_si);
		kset_cdr(get_data(), np);
		/* replace last pair of the (still incomplete) read next obj */
		pop_data();
		push_data(np);
		read_next_token = true;
		break;
	    }
	    case ST_LAST_ILIST:
		kset_cdr(get_data(), obj);
		/* only change the state, keep the pair in data to simplify 
		   the close paren code (same as for ST_MIDDLE_LIST) */
		pop_state();
		push_state(ST_PAST_LAST_ILIST);
		read_next_token = true;
		break;
	    case ST_SHARED_DEF: {
		/* shared def completed, continue processing obj */
		TValue def_si = get_data();
		pop_state();
		pop_data();

		change_shared_def(kcar(def_si), obj);
		
		/* obj = obj; */
		/* the source info returned is the one from the shared def */
		obj_si = kcdr(def_si);
		read_next_token = false;
		break;
	    }
	    case ST_READ:
		/* this shouldn't happen, should've exited the while */
		assert(0);
	    default:
		/* shouldn't happen */
		assert(0);
	    }
	}
    }

    return obj;
}

/*
** Reader Main Function
*/
TValue kread()
{
    TValue obj;

    /* TEMP: for now assume we are in the repl: reset source info */
    ktok_reset_source_info();

    obj = kread_fsm();

    /* NOTE: clear after function to allow earlier gc */
    clear_shared_dict();
    clear_state();
    clear_data();

    return obj;
}
