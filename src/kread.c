/*
** kread.c
** Reader for the Kernel Programming Language
** See Copyright Notice in klisp.h
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "kread.h"
#include "kobject.h"
#include "kpair.h"
#include "ktoken.h"
#include "kstate.h"
#include "kerror.h"
#include "ktable.h"
#include "kport.h"
#include "kstring.h"


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
**        corrected to car of list) and cdr: source info of the '(' token that 
**        started the [i]list.
**   - another pair, that is the last pair of the list so far. 
** ST_PAST_LAST_ILIST: a pair with car: first pair and cdr: source
**   info as above (but no pair with last pair).
** ST_SHARED_DEF: a pair with car: shared def token and cdr: source
**   info of the shared def token.
** ST_SEXP_COMMENT: the source info of the comment token
** ST_FIRST_EOF_LIST: first pair of the list (with source info, start of file)
** ST_MIDDLE_EOF_LIST: two elements, first below, second on top:
**   - a pair with car: first pair of the list (with source info corrected 
**        to car of list) and cdr: source info of the start of file.
**   - last pair of the list so far. 
*/

typedef enum {
    ST_READ, ST_SHARED_DEF, ST_LAST_ILIST, ST_PAST_LAST_ILIST,
    ST_FIRST_LIST, ST_MIDDLE_LIST, ST_SEXP_COMMENT, ST_FIRST_EOF_LIST,
    ST_MIDDLE_EOF_LIST
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
#define kread_error(K, str)                     \
    kread_error_g(K, str, false, KINERT)
#define kread_error_extra(K, str, extra)        \
    kread_error_g(K, str, true, extra)

void kread_error_g(klisp_State *K, char *str, bool extra, TValue extra_value)
{
    /* all cleaning is done in throw 
       (stacks, shared_dict, rooted objs) */

    /* save the source code info on the port */
    kport_update_source_info(K->curr_port, K->ktok_source_info.line,
                             K->ktok_source_info.col);

    /* include the source info (and extra value if present) in the error */
    TValue irritants;
    if (extra) {
        krooted_tvs_push(K, extra_value); /* will be popped by throw */
        TValue si = ktok_get_source_info(K);
        krooted_tvs_push(K, si); /* will be popped by throw */
        irritants = klist_g(K, false, 2, si, extra_value);
    } else {
        irritants = ktok_get_source_info(K);
    }
    krooted_tvs_push(K, irritants); /* will be popped by throw */
    klispE_throw_with_irritants(K, str, irritants);
}

/*
** Shared Reference Management (srfi-38) 
*/

/* clear_shared_dict is defined in ktoken to allow cleaning up before errors */
/* It is called after kread to clear the shared alist */
TValue try_shared_ref(klisp_State *K, TValue ref_token)
{
    /* IMPLEMENTATION RESTRICTION: only allow fixints in shared tokens */
    int32_t ref_num = ivalue(kcdr(ref_token));
    TValue tail = K->shared_dict;
    while (!ttisnil(tail)) {
        TValue head = kcar(tail);
        if (ref_num == ivalue(kcar(head)))
            return kcdr(head);
        tail = kcdr(tail);
    }
    
    kread_error_extra(K, "undefined shared ref found", i2tv(ref_num));
    /* avoid warning */
    return KINERT;
}

/* GC: def token is rooted */
void try_shared_def(klisp_State *K, TValue def_token, TValue value)
{
    /* IMPLEMENTATION RESTRICTION: only allow fixints in shared tokens */
    int32_t ref_num = ivalue(kcdr(def_token));
    TValue tail = K->shared_dict;
    while (!ttisnil(tail)) {
        TValue head = kcar(tail);
        if (ref_num == ivalue(kcar(head))) {
            kread_error_extra(K, "duplicate shared def found", i2tv(ref_num));
            /* avoid warning */
            return;
        }
        tail = kcdr(tail);
    }
    
    TValue new_tok = kcons(K, kcdr(def_token), value);
    krooted_tvs_push(K, new_tok);
    K->shared_dict = kcons(K, new_tok, K->shared_dict);
    krooted_tvs_pop(K);
    return;
}

/* This overwrites a previouly made def, it is used in '() */
/* NOTE: the shared def is guaranteed to exist */
void change_shared_def(klisp_State *K, TValue def_token, TValue value)
{
    /* IMPLEMENTATION RESTRICTION: only allow fixints in shared tokens */
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
    klisp_assert(0); /* shouldn't happen */
    return;
}

/* NOTE: the shared def is guaranteed to exist */
void remove_shared_def(klisp_State *K, TValue def_token)
{
    /* IMPLEMENTATION RESTRICTION: only allow fixints in shared tokens */
    int32_t ref_num = ivalue(kcdr(def_token));
    TValue tail = K->shared_dict;
    TValue last_pair = KNIL;
    while (!ttisnil(tail)) {
        TValue head = kcar(tail);
        if (ref_num == ivalue(kcar(head))) {
            if (ttisnil(last_pair)) {
                /* this is the first value */
                K->shared_dict = kcdr(tail);
            } else {
                kset_cdr(last_pair, kcdr(tail));
            }
            return;
        }
        last_pair = tail;
        tail = kcdr(tail);
    }
    klisp_assert(0); /* shouldn't happen */
    return;
}

/*
** Reader FSM
*/

/* 
** listp: 
** false: read one value
** true: read all values as a list
*/

/* TEMP: For now we'll use just one big function */
TValue kread_fsm(klisp_State *K, bool listp)
{
    /* TODO add more specific sexp comment error msgs */
    /* TODO replace some read errors with asserts where appropriate */
    klisp_assert(ks_sisempty(K));
    klisp_assert(ttisnil(K->shared_dict));

    push_state(K, ST_READ);

    if (listp) { /* read a list of values */
        /* create the first pair */
        TValue np = kcons_g(K, K->read_mconsp, KINERT, KNIL);
        krooted_tvs_push(K, np);
        /* 
        ** NOTE: the source info of the start of file is temporarily 
        ** saved in np (later it will be replace by the source info
        ** of the car of the list)
        */
        TValue si = ktok_get_source_info(K);
        krooted_tvs_push(K, si);
#if KTRACK_SI
        kset_source_info(K, np, si);
#endif
        krooted_tvs_pop(K);
        push_data(K, np);
        krooted_tvs_pop(K);
        push_state(K, ST_FIRST_EOF_LIST);
    }

    /* read next token or process obj */
    bool read_next_token = true; 
    /* the obj just read/completed */
    TValue obj = KINERT; /* put some value for gc */ 
    /* the source code information of that obj */
    TValue obj_si = KNIL; /* put some value for gc */ 
    int32_t sexp_comments = 0;
    TValue last_sexp_comment_si = KNIL; /* put some value for gc */
    /* list of shared list, each element represent a nested sexp comment,
       each is a list of shared defs in that particular level, to be
       undefined after the sexp comment ends */
    TValue sexp_comment_shared = KNIL;

    krooted_vars_push(K, &obj);
    krooted_vars_push(K, &obj_si);
    krooted_vars_push(K, &last_sexp_comment_si);
    krooted_vars_push(K, &sexp_comment_shared);

    while (!(get_state(K) == ST_READ && !read_next_token)) {
        if (read_next_token) {
            TValue tok = ktok_read_token(K); /* only root it when necessary */

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
                    TValue np = kcons_g(K, K->read_mconsp, KINERT, KNIL);
                    krooted_tvs_push(K, np);
                    /* 
                    ** NOTE: the source info of the '(' is temporarily saved 
                    ** in np (later it will be replace by the source info
                    ** of the car of the list
                    */
                    TValue si = ktok_get_source_info(K);
                    krooted_tvs_push(K, si);
#if KTRACK_SI
                    kset_source_info(K, np, si);
#endif
                    krooted_tvs_pop(K);
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

                    krooted_tvs_pop(K);
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
#if KTRACK_SI
                        obj_si = kget_source_info(K, fp_with_old_si);
#else
                        UNUSED(fp_with_old_si);
                        obj_si = KNIL;
#endif
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
                    case ST_SEXP_COMMENT:
                        kread_error_extra(K, "unmatched closing paren found in "
                                          "sexp comment", last_sexp_comment_si);
                        /* avoid warning */
                        return KINERT;
                    case ST_READ:
                    case ST_FIRST_EOF_LIST:
                    case ST_MIDDLE_EOF_LIST:
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
                    case ST_SEXP_COMMENT:
                        kread_error_extra(K, "dot found outside list in sexp "
                                          "comment", last_sexp_comment_si);
                        /* avoid warning */
                        return KINERT;
                    case ST_READ:
                    case ST_FIRST_EOF_LIST:
                    case ST_MIDDLE_EOF_LIST:
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
                        krooted_tvs_push(K, tok);
                        try_shared_def(K, tok, KNIL);
                        /* token ok */
                        /* save the token for later undefining */
                        if (sexp_comments > 0) {
                            kset_car(sexp_comment_shared, 
                                     kcons(K, tok, kcar(sexp_comment_shared)));
                        }
                        /* read defined object */
                        /* NOTE: save the source info to return it 
                           after the defined object is read */
                        TValue si = ktok_get_source_info(K);
                        krooted_tvs_push(K, si);
                        push_data(K, kcons(K, tok, si));
                        krooted_tvs_pop(K);
                        krooted_tvs_pop(K);
                        push_state(K, ST_SHARED_DEF);
                        read_next_token = true;
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
                        /* ref ok, process it in next iteration */
                        obj = res;
                        /* NOTE: use source info of ref token */
                        obj_si = ktok_get_source_info(K);
                        read_next_token = false;
                    }
                    }
                    break;
                }
                case ';': { /* sexp comment */
                    klisp_assert(sexp_comments < 1000);
                    ++sexp_comments;
                    sexp_comment_shared = 
                        kcons(K, KNIL, sexp_comment_shared);
                    push_data(K, last_sexp_comment_si);
                    push_state(K, ST_SEXP_COMMENT);
                    last_sexp_comment_si = ktok_get_source_info(K);
                    read_next_token = true;
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
                case ST_SEXP_COMMENT:
                    kread_error_extra(K, "EOF found while reading sexp "
                                      " comment", last_sexp_comment_si);
                    /* avoid warning */
                    return KINERT;		    
                case ST_FIRST_EOF_LIST: {
                    pop_state(K);
                    TValue fp_with_old_si = get_data(K);
                    pop_data(K);
                    obj = KNIL;
#if KTRACK_SI
                    obj_si = kget_source_info(K, fp_with_old_si);
#else
                    UNUSED(fp_with_old_si);
                    obj_si = KNIL;
#endif
                    read_next_token = false;
                    break;
                }
                case ST_MIDDLE_EOF_LIST: {
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
                case ST_READ:
                    obj = tok;
                    obj_si = ktok_get_source_info(K);
                    /* will exit in next loop */
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
        } else { /* read_next_token == false */
            /* process the object just read */
            switch(get_state(K)) {
            case ST_FIRST_EOF_LIST: 
            case ST_FIRST_LIST: {
                state_t state = get_state(K);
                /* get the state out of the way */
                pop_state(K);
                TValue fp = get_data(K);
                /* replace source info in fp with the saved one */
                /* NOTE: the old one will be returned when list is complete */
                /* GC: the way things are done here fp is rooted at all
                   times */
#if KTRACK_SI
                TValue fp_old_si = kget_source_info(K, fp);
#else
                TValue fp_old_si = KNIL;
#endif
                krooted_tvs_push(K, fp);
                krooted_tvs_push(K, fp_old_si);
#if KTRACK_SI
                kset_source_info(K, fp, obj_si);
#endif
                kset_car_unsafe(K, fp, obj);
		
                /* continue reading objects of list */
                /* save first & last pair of the (still incomplete) list */
                pop_data(K);
                push_data(K, kcons (K, fp, fp_old_si));
                krooted_tvs_pop(K);
                krooted_tvs_pop(K);
                push_state(K, state);
                push_data(K, fp);
                if (state == ST_FIRST_LIST) {
                    push_state(K, ST_MIDDLE_LIST);
                } else {
                    push_state(K, ST_MIDDLE_EOF_LIST);
                    /* shared dict must be cleared after every element
                       of an eof list */
                    clear_shared_dict(K);
                }
                read_next_token = true;
                break;
            }
            case ST_MIDDLE_LIST:
            case ST_MIDDLE_EOF_LIST: {
                state_t state = get_state(K);
                /* get the state out of the way */
                pop_state(K);
                /* construct the list with the correct type of pair */
                /* GC: np is rooted by push_data */
                TValue np = kcons_g(K, K->read_mconsp, obj, KNIL);
                krooted_tvs_push(K, np);
#if KTRACK_SI
                kset_source_info(K, np, obj_si);
#endif
                kset_cdr_unsafe(K, get_data(K), np);
                /* replace last pair of the (still incomplete) read next obj */
                pop_data(K);
                push_data(K, np);
                push_state(K, state);
                if (state == ST_MIDDLE_EOF_LIST) {
                    /* shared dict must be cleared after every element
                       of an eof list */
                    clear_shared_dict(K);
                }
                krooted_tvs_pop(K);
                read_next_token = true;
                break;
            }
            case ST_LAST_ILIST:
                /* only change the state, keep the pair data to simplify 
                   the close paren code (same as for ST_MIDDLE_LIST) */
                pop_state(K);
                kset_cdr_unsafe(K, get_data(K), obj);
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
            case ST_SEXP_COMMENT:
                klisp_assert(sexp_comments > 0);
                --sexp_comments;
                /* undefine all shared obj defined in the context
                   of this sexp comment */
                while(!ttisnil(kcar(sexp_comment_shared))) {
                    TValue first = kcaar(sexp_comment_shared);
                    remove_shared_def(K, first);
                    kset_car(sexp_comment_shared, kcdar(sexp_comment_shared));
                }
                sexp_comment_shared = kcdr(sexp_comment_shared);
                pop_state(K);
                last_sexp_comment_si = get_data(K);
                pop_data(K);
                read_next_token = true;
                break;
            default:
                /* shouldn't happen */
                kread_error(K, "unknown read state in process obj");
                /* avoid warning */
                return KINERT;
            }
        }
    }

    krooted_vars_pop(K);
    krooted_vars_pop(K);
    krooted_vars_pop(K);
    krooted_vars_pop(K);

    pop_state(K);
    klisp_assert(ks_sisempty(K));
    return obj;
}

/*
** Reader Main Function
*/
TValue kread(klisp_State *K, bool listp)
{
    klisp_assert(ttisnil(K->shared_dict));

    TValue obj = kread_fsm(K, listp); 
    clear_shared_dict(K); /* clear after function to allow earlier gc */
    return obj;
}

/* port is protected from GC in curr_port */
TValue kread_from_port_g(klisp_State *K, TValue port, bool mut, bool listp)
{
    if (!tv_equal(port, K->curr_port)) {
        K->ktok_seen_eof = false; /* WORKAROUND: for repl problem with eofs */
        K->curr_port = port;
    }
    K->read_mconsp = mut;

    ktok_set_source_info(K, kport_filename(port), 
                         kport_line(port), kport_col(port));

    TValue obj = kread(K, listp);

    kport_update_source_info(port, K->ktok_source_info.line, 
                             K->ktok_source_info.col);
    return obj;
}

/*
** Reader Interface
*/

TValue kread_from_port(klisp_State *K, TValue port, bool mut)
{
    klisp_assert(ttisport(port));
    klisp_assert(kport_is_input(port));
    klisp_assert(kport_is_open(port));
    klisp_assert(kport_is_textual(port));
    return kread_from_port_g(K, port, mut, false);
}

TValue kread_list_from_port(klisp_State *K, TValue port, bool mut)
{
    klisp_assert(ttisport(port));
    klisp_assert(kport_is_input(port));
    klisp_assert(kport_is_open(port));
    klisp_assert(kport_is_textual(port));
    return kread_from_port_g(K, port, mut, true);
}

TValue kread_peek_char_from_port(klisp_State *K, TValue port, bool peek)
{
    klisp_assert(ttisport(port));
    klisp_assert(kport_is_input(port));
    klisp_assert(kport_is_open(port));
    klisp_assert(kport_is_textual(port));

    if (!tv_equal(port, K->curr_port)) {
        K->ktok_seen_eof = false; /* WORKAROUND: for repl problem with eofs */
        K->curr_port = port;
    }
    int ch;
    if (peek) {
        ch = ktok_peekc(K);
    } else {
        ktok_set_source_info(K, kport_filename(port), 
                             kport_line(port), kport_col(port));
        ch = ktok_getc(K);
        kport_update_source_info(port, K->ktok_source_info.line, 
                                 K->ktok_source_info.col);    
    }
    return ch == EOF? KEOF : ch2tv((char)ch);
}

TValue kread_peek_u8_from_port(klisp_State *K, TValue port, bool peek)
{
    klisp_assert(ttisport(port));
    klisp_assert(kport_is_input(port));
    klisp_assert(kport_is_open(port));
    klisp_assert(kport_is_binary(port));

    if (!tv_equal(port, K->curr_port)) {
        K->ktok_seen_eof = false; /* WORKAROUND: for repl problem with eofs */
        K->curr_port = port;
    }
    int32_t u8;
    if (peek) {
        u8 = ktok_peekc(K);
    } else {
        ktok_set_source_info(K, kport_filename(port), 
                             kport_line(port), kport_col(port));
        u8 = ktok_getc(K);
        kport_update_source_info(port, K->ktok_source_info.line, 
                                 K->ktok_source_info.col);    
    }
    return u8 == EOF? KEOF : i2tv(u8 & 0xff);
}

TValue kread_line_from_port(klisp_State *K, TValue port)
{
    klisp_assert(ttisport(port));
    klisp_assert(kport_is_input(port));
    klisp_assert(kport_is_open(port));
    klisp_assert(kport_is_textual(port));

    if (!tv_equal(port, K->curr_port)) {
        K->ktok_seen_eof = false; /* WORKAROUND: for repl problem with eofs */
        K->curr_port = port;
    }

    uint32_t size = MINREADLINEBUFFER; 
    uint32_t i = 0;
    int ch;
    TValue new_str = kstring_new_s(K, size);
    krooted_vars_push(K, &new_str);

    char *buf = kstring_buf(new_str);
    ktok_set_source_info(K, kport_filename(port), 
                         kport_line(port), kport_col(port));
    bool found_newline = false;
    while(true) {
        ch = ktok_getc(K);
        if (ch == EOF) {
            break;
        } else if (ch == '\n') {
            /* adjust string to the right size if necessary */
            if (i < size) {
                new_str = kstring_new_bs(K, kstring_buf(new_str), i);
            }
            found_newline = true;
            break;
        } else {
            if (i == size) {
                size *= 2;
                char *old_buf = kstring_buf(new_str);
                new_str = kstring_new_s(K, size);
                buf = kstring_buf(new_str);
                /* copy the data we have */
                memcpy(buf, old_buf, i);
                buf += i;
            }
            *buf++ = (char) ch;
            ++i;
        }
    }
    kport_update_source_info(port, K->ktok_source_info.line, 
                             K->ktok_source_info.col);    
    krooted_vars_pop(K);
    return found_newline? new_str : KEOF;
}

/* This is needed by the repl to ignore trailing spaces (especially newlines)
   that could affect the (freshly reset) source info */
void kread_clear_leading_whitespace_from_port(klisp_State *K, TValue port)
{
    if (!tv_equal(port, K->curr_port)) {
        K->ktok_seen_eof = false; /* WORKAROUND: for repl problem with eofs */
        K->curr_port = port;
    }
    /* source code info isn't important because it will be reset later */
    ktok_ignore_whitespace(K);
}
