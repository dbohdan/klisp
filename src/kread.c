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
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "kread.h"
#include "kobject.h"
#include "kpair.h"
#include "ktoken.h"
#include "kstate.h"
#include "kerror.h"


/*
** Stack for the read FSM
**
** There is always one state in the stack while read is in process and
** selects the action to be performed on the next read token.
**
** The data saved in the stack is below the state and changes according to it:
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

typedef enum {
    ST_READ, ST_SHARED_DEF, ST_LAST_ILIST, ST_PAST_LAST_ILIST,
    ST_FIRST_LIST, ST_MIDDLE_LIST 
} state_t;

#define push_state(kst_, st_) (ks_spush(kst_, (i2tv((int32_t)(st_)))))
#define get_state(kst_) ((state_t) ivalue(ks_sget(kst_)))
#define pop_state(kst_) (ks_sdpop(kst_))

#define push_data(kst_, st_) (ks_spush(kst_, st_))
#define get_data(kst_) (ks_sget(kst_))
#define pop_data(kst_) (ks_sdpop(kst_))


/*
** Error management
*/
void kread_error(klisp_State *K, char *str)
{
    /* clear up before throwing */
    ks_tbclear(K);
    ks_sclear(K);
    clear_shared_dict(K);
    klispE_throw(K, str);
}

/*
** Shared Reference Management (srfi-38) 
*/

/* clear_shared_dict is defined in ktoken to allow cleaning up before errors */
/* It is called after kread to clear the shared alist */
TValue try_shared_ref(klisp_State *K, TValue ref_token)
{
    /* TEMP: for now, only allow fixints in shared tokens */
    int32_t ref_num = ivalue(kcdr(ref_token));
    TValue tail = K->shared_dict;
    while (!ttisnil(tail)) {
	TValue head = kcar(tail);
	if (ref_num == ivalue(kcar(head)))
	    return kcdr(head);
	tail = kcdr(tail);
    }
    
    kread_error(K, "undefined shared ref found");
    /* avoid warning */
    return KINERT;
}

TValue try_shared_def(klisp_State *K, TValue def_token, TValue value)
{
    /* TEMP: for now, only allow fixints in shared tokens */
    int32_t ref_num = ivalue(kcdr(def_token));
    TValue tail = K->shared_dict;
    while (!ttisnil(tail)) {
	TValue head = kcar(tail);
	if (ref_num == ivalue(kcar(head))) {
	    kread_error(K, "duplicate shared def found");
	    /* avoid warning */
	    return KINERT;
	}
	tail = kcdr(tail);
    }
    
    /* XXX: what happens on out of mem? & gc? (inner cons is not rooted) */
    K->shared_dict = kcons(K, kcons(K, kcdr(def_token), value), 
			   K->shared_dict);
    return KINERT;
}

/* This overwrites a previouly made def, it is used in '() */
/* NOTE: the shared def is guaranteed to exist */
void change_shared_def(klisp_State *K, TValue def_token, TValue value)
{
    /* TEMP: for now, only allow fixints in shared tokens */
    int32_t ref_num = ivalue(kcdr(def_token));
    TValue tail = K->shared_dict;
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
TValue kread_fsm(klisp_State *K)
{
    assert(ks_sisempty(K));
    assert(ttisnil(K->shared_dict));
    push_state(K, ST_READ);

    /* read next token or process obj */
    bool read_next_token = true; 
    /* the obj just read/completed */
    TValue obj; 
    /* the source code information of that obj */
    TValue obj_si; 

    while (!(get_state(K) == ST_READ && !read_next_token)) {
	if (read_next_token) {
	    TValue tok = ktok_read_token(K);
	    if (ttispair(tok)) { /* special token */
		switch (chvalue(kcar(tok))) {
		case '(': {
		    if (get_state(K) == ST_PAST_LAST_ILIST) {
			kread_error(K, "open paren found after "
				    "last element of improper list");
			/* avoid warning */
			return KINERT;
		    }
		    /* construct the list with the correct type of pair */
		    TValue np = kcons_g(K, K->read_cons_flag, KINERT, KNIL);
		    /* 
		    ** NOTE: the source info of the '(' is temporarily saved 
		    ** in np (later it will be replace by the source info
		    ** of the car of the list
		    */
		    kset_source_info(np, ktok_get_source_info(K));

		    /* update the shared def to point to the new list */
		    /* NOTE: this is necessary for self referencing lists */
		    /* NOTE: the shared def was already checked for errors */
		    if (get_state(K) == ST_SHARED_DEF) { 
			/* take the state out of the way */
			pop_state(K);
			change_shared_def(K, kcar(get_data(K)), np);
			push_state(K, ST_SHARED_DEF);
		    }
		    
		    /* start reading elements of the new list */
		    push_data(K, np);
		    push_state(K, ST_FIRST_LIST);
		    read_next_token = true;
		    break;
		}
		case ')': {
		    switch(get_state(K)) {
		    case ST_FIRST_LIST: { /* empty list */
			/*
			** Discard the pair in sdata but
			** retain the source info
			** Return () for processing
			*/
			pop_state(K);
			TValue fp_with_old_si = get_data(K);
			pop_data(K);

			obj = KNIL;
			obj_si = kget_source_info(fp_with_old_si);
			read_next_token = false;
			break;
		    }
		    case ST_MIDDLE_LIST: /* end of list */
		    case ST_PAST_LAST_ILIST: { /* end of ilist */
			pop_state(K);
			/* discard info on last pair */
			pop_data(K);
			pop_state(K);
			TValue fp_old_si = get_data(K);
			pop_data(K);
			/* list read ok, process it in next iteration */
			obj = kcar(fp_old_si);
			obj_si = kcdr(fp_old_si);
			read_next_token = false;
			break;
		    }
		    case ST_LAST_ILIST:
			kread_error(K, "missing last element in "
				    "improper list");
			/* avoid warning */
			return KINERT;
		    case ST_SHARED_DEF:
			kread_error(K, "unmatched closing paren found "
				    "in shared def");
			/* avoid warning */
			return KINERT;
		    case ST_READ:
			kread_error(K, "unmatched closing paren found");
			/* avoid warning */
			return KINERT;
		    default:
			/* shouldn't happen */
			kread_error(K, "Unknown read state in )");
			/* avoid warning */
			return KINERT;
		    }
		    break;
		}
		case '.': {
		    switch(get_state(K)) {
		    case (ST_MIDDLE_LIST):
			/* tok ok, read next obj for cdr of ilist */
			pop_state(K);
			push_state(K, ST_LAST_ILIST);
			read_next_token = true;
			break;
		    case ST_FIRST_LIST:
			kread_error(K, "missing first element of "
				    "improper list");
			/* avoid warning */
			return KINERT;
		    case ST_LAST_ILIST:
		    case ST_PAST_LAST_ILIST:
			kread_error(K, "double dot in improper list");
			/* avoid warning */
			return KINERT;
		    case ST_SHARED_DEF:
			kread_error(K, "dot found in shared def");
			/* avoid warning */
			return KINERT;
		    case ST_READ:
			kread_error(K, "dot found outside list");
			/* avoid warning */
			return KINERT;
		    default:
			/* shouldn't happen */
			kread_error(K, "Unknown read state in .");
			/* avoid warning */
			return KINERT;
		    }
		    break;
		}
		case '=': { /* srfi-38 shared def */
		    switch (get_state(K)) {
		    case ST_SHARED_DEF:
			kread_error(K, "shared def found in "
				    "shared def");
			/* avoid warning */
			return KINERT;
		    case ST_PAST_LAST_ILIST:
			kread_error(K, "shared def found after "
				    "last element of improper list");
			/* avoid warning */
			return KINERT;
		    default: {
			TValue res = try_shared_def(K, tok, KNIL);
			/* TEMP: while error returns EOF */
			if (ttiseof(res)) {
			    return res;
			} else {
			    /* token ok, read defined object */
			    /* NOTE: save the source info to return it 
			     after the defined object is read */
			    push_data(K, kcons(K, tok, ktok_get_source_info(K)));
			    push_state(K, ST_SHARED_DEF);
			    read_next_token = true;
			}
		    }
		    }
		    break;
		}
		case '#': { /* srfi-38 shared ref */
		    switch(get_state(K)) {
		    case ST_SHARED_DEF:
			kread_error(K, "shared ref found in "
				    "shared def");
			/* avoid warning */
			return KINERT;
		    case ST_PAST_LAST_ILIST:
			kread_error(K, "shared ref found after "
				    "last element of improper list");
			/* avoid warning */
			return KINERT;
		    default: {
			TValue res = try_shared_ref(K, tok);
			/* TEMP: while error returns EOF */
			if (ttiseof(res)) {
			    return res;
			} else {
			    /* ref ok, process it in next iteration */
			    obj = res;
			    /* NOTE: use source info of ref token */
			    obj_si = ktok_get_source_info(K);
			    read_next_token = false;
			}
		    }
		    }
		    break;
		}
		default:
		    /* shouldn't happen */
		    kread_error(K, "unknown special token");
		    /* avoid warning */
		    return KINERT;
		}
	    } else if (ttiseof(tok)) {
		switch (get_state(K)) {
		case ST_READ:
		    /* will exit in next loop */
		    obj = tok;
		    obj_si = ktok_get_source_info(K);
		    read_next_token = false;
		    break;
		case ST_FIRST_LIST:
		case ST_MIDDLE_LIST:
		    kread_error(K, "EOF found while reading list");
		    /* avoid warning */
		    return KINERT;
		case ST_LAST_ILIST:
		case ST_PAST_LAST_ILIST:
		    kread_error(K, "EOF found while reading "
				       "improper list");
		    /* avoid warning */
		    return KINERT;
		case ST_SHARED_DEF:
		    kread_error(K, "EOF found in shared def");
		    /* avoid warning */
		    return KINERT;
		default:
		    /* shouldn't happen */
		    kread_error(K, "unknown read state in EOF");
		    /* avoid warning */
		    return KINERT;
		}
	    } else { /* this can only be a complete token */
		if (get_state(K) == ST_PAST_LAST_ILIST) {
		    kread_error(K, "Non paren found after last "
				"element of improper list");
		    /* avoid warning */
		    return KINERT;
		} else {
		    /* token ok, process it in next iteration */
		    obj = tok;
		    obj_si = ktok_get_source_info(K);
		    read_next_token = false;
		}
	    }
	} else { /* if(read_next_token) */
	    /* process the object just read */
	    switch(get_state(K)) {
	    case ST_FIRST_LIST: {
		/* get the state out of the way */
		pop_state(K);
		TValue fp = get_data(K);
		/* replace source info in fp with the saved one */
		/* NOTE: the old one will be returned when list is complete */
		TValue fp_old_si = kget_source_info(fp);
		kset_source_info(fp, obj_si);
		kset_car(fp, obj);
		
		/* continue reading objects of list */
		/* save first & last pair of the (still incomplete) list */
		pop_data(K);
		push_data(K, kcons (K, fp, fp_old_si));
		push_state(K, ST_FIRST_LIST);
		push_data(K, fp);
		push_state(K, ST_MIDDLE_LIST);
		read_next_token = true;
		break;
	    }
	    case ST_MIDDLE_LIST: {
		/* get the state out of the way */
		pop_state(K);
		/* construct the list with the correct type of pair */
		TValue np = kcons_g(K, K->read_cons_flag, obj, KNIL);
		kset_source_info(np, obj_si);
		kset_cdr(get_data(K), np);
		/* replace last pair of the (still incomplete) read next obj */
		pop_data(K);
		push_data(K, np);
		push_state(K, ST_MIDDLE_LIST);
		read_next_token = true;
		break;
	    }
	    case ST_LAST_ILIST:
		/* only change the state, keep the pair data to simplify 
		   the close paren code (same as for ST_MIDDLE_LIST) */
		pop_state(K);
		kset_cdr(get_data(K), obj);
		push_state(K, ST_PAST_LAST_ILIST);
		read_next_token = true;
		break;
	    case ST_SHARED_DEF: {
		/* shared def completed, continue processing obj */
		pop_state(K);
		TValue def_si = get_data(K);
		pop_data(K);

		change_shared_def(K, kcar(def_si), obj);
		
		/* obj = obj; */
		/* the source info returned is the one from the shared def */
		obj_si = kcdr(def_si);
		read_next_token = false;
		break;
	    }
	    case ST_READ:
		/* this shouldn't happen, should've exited the while */
		kread_error(K, "invalid read state (read in while)");
		/* avoid warning */
		return KINERT;
	    default:
		/* shouldn't happen */
		kread_error(K, "unknown read state in process obj");
		/* avoid warning */
		return KINERT;
	    }
	}
    }

    pop_state(K);
    assert(ks_sisempty(K));
    return obj;
}

/*
** Reader Main Function
*/
TValue kread(klisp_State *K)
{
    TValue obj;

    assert(ttisnil(K->shared_dict));
    /* TEMP: workaround repl problem with eofs */
    K->ktok_seen_eof = false;

    obj = kread_fsm(K);

    /* NOTE: clear after function to allow earlier gc */
    clear_shared_dict(K);

    return obj;
}
