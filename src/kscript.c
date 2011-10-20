/*
** kscript.c
** klisp noninteractive script execution
** See Copyright Notice in klisp.h
*/
#include <stdio.h>
#include <setjmp.h>

#include "klisp.h"
#include "kstate.h"
#include "kobject.h"
#include "kcontinuation.h"
#include "kenvironment.h"
#include "kerror.h"
#include "kread.h"
#include "kwrite.h"
#include "kstring.h"
#include "krepl.h"
#include "kscript.h"
#include "ksymbol.h"
#include "kport.h"
#include "kpair.h"
#include "kgcontrol.h"
/* for names */
#include "ktable.h"

/* Push (v) in GC roots and return (v). */
static inline TValue krooted_tvs_pass(klisp_State *K, TValue v)
{
  krooted_tvs_push(K, v);
  return v;
}

#if KTRACK_SI
static inline TValue krooted_tvs_pass_si(klisp_State *K, TValue v, TValue si)
{
  krooted_tvs_push(K, v);
  kset_source_info(K, v, si);
  return v;
}
#endif

/* the exit continuation, it exits the loop */
void do_script_exit(klisp_State *K, TValue *xparams, TValue obj)
{
  UNUSED(xparams);

  /* save exit code */

  switch(ttype(obj)) {
    case K_TINERT:
      K->script_exit_code = 0;
      break;
    case K_TFIXINT:
      K->script_exit_code = (int) ivalue(obj);
      break;
    default:
      K->script_exit_code = KSCRIPT_DEFAULT_ERROR_EXIT_CODE;
      /* TODO: print error message here ? */
      break;
  }

  /* force the loop to terminate */
  K->next_func = NULL;
  return;
}


/* the underlying function of the error cont */
void do_script_error(klisp_State *K, TValue *xparams, TValue obj)
{
    /* 
    ** xparams[0]: dynamic environment
    */

    /* FOR NOW used only for irritant list */
    TValue port = kcdr(K->kd_error_port_key);
    klisp_assert(kport_file(port) == stderr);

    /* TEMP: obj should be an error obj */
    if (ttiserror(obj)) {
	Error *err_obj = tv2error(obj);
	TValue who = err_obj->who;
	char *who_str;
	/* TEMP? */
	if (ttiscontinuation(who))
	    who = tv2cont(who)->comb;

	if (ttisstring(who)) {
	    who_str = kstring_buf(who);
#if KTRACK_NAMES
	} else if (khas_name(who)) {
	    TValue name = kget_name(K, who);
	    who_str = ksymbol_buf(name);
#endif
	} else {
	    who_str = "?";
	}
	char *msg = kstring_buf(err_obj->msg);
	fprintf(stderr, "\n*ERROR*: \n");
	fprintf(stderr, "%s: %s", who_str, msg);

	krooted_tvs_push(K, obj);

	/* Msg + irritants */
	/* TODO move to a new function */
	if (!ttisnil(err_obj->irritants)) {
	    fprintf(stderr, ": ");
	    kwrite_display_to_port(K, port, err_obj->irritants, false);
	}
	kwrite_newline_to_port(K, port);

#if KTRACK_NAMES
#if KTRACK_SI
	/* Location */
	/* TODO move to a new function */
	/* MAYBE: remove */
	if (khas_name(who) || khas_si(who)) {
	    fprintf(stderr, "Location: ");
	    kwrite_display_to_port(K, port, who, false);
	    kwrite_newline_to_port(K, port);
	}

	/* Backtrace */
	/* TODO move to a new function */
	TValue tv_cont = err_obj->cont;
	fprintf(stderr, "Backtrace: \n");
	while(ttiscontinuation(tv_cont)) {
	    kwrite_display_to_port(K, port, tv_cont, false);
	    kwrite_newline_to_port(K, port);
	    Continuation *cont = tv2cont(tv_cont);
	    tv_cont = cont->parent;
	}
	/* add extra newline at the end */
	kwrite_newline_to_port(K, port);
#endif
#endif
	krooted_tvs_pop(K);
    } else {
	fprintf(stderr, "\n*ERROR*: not an error object passed to " 
		"error continuation");
    }

    /* Save the exit code to be returned from interpreter
       main(). Terminate the interpreter loop. */

    K->script_exit_code = KSCRIPT_DEFAULT_ERROR_EXIT_CODE;
    K->next_func = NULL;
}

/* convert C style argc-argv pair to list of strings */
static TValue argv2value(klisp_State *K, int argc, char *argv[])
{
    TValue dummy = kcons_g(K, false, KINERT, KNIL);
    krooted_tvs_push(K, dummy);
    TValue tail = dummy;
    for (int i = 0; i < argc; i++) {
        TValue next_car = kstring_new_b_imm(K, argv[i]);
        krooted_tvs_push(K, next_car);
        TValue np = kcons_g(K, false, next_car, KNIL); 
        krooted_tvs_pop(K);
        kset_cdr_unsafe(K, tail, np);
        tail = np;
    }
    krooted_tvs_pop(K);
    return kcdr(dummy);
}

/* loader_body(K, ARGV, DENV) returns the value
 *
 *  ((load (car ARGV))
 *   ($if ($binds? DENV main) (main ARGV) #inert)
 *
 */
static TValue loader_body(klisp_State *K, TValue argv, TValue denv)
{
    int32_t rooted_tvs_mark = K->rooted_tvs_top;
#   define S(z) (krooted_tvs_pass(K, ksymbol_new(K, (z), KNIL)))
#   define C(car, cdr) (krooted_tvs_pass(K, kcons_g(K, false, (car), (cdr))))
#   define L(n, ...) (krooted_tvs_pass(K, klist_g(K, false, (n), __VA_ARGS__)))
    TValue main_sym = S("main");
    TValue script_name = krooted_tvs_pass(K, kcar(argv));
    TValue body =
        L(2, L(2, S("load"), script_name),
             L(4, S("$if"), L(3, S("$binds?"), denv, main_sym),
                            L(2, main_sym, C(S("list"), argv)),
                            KINERT));
#   undef S
#   undef L
    K->rooted_tvs_top = rooted_tvs_mark;
    return body;
}

/* call this to init the noninteractive mode */

void kinit_script(klisp_State *K, int argc, char *argv[])
{
#   define R(z) (krooted_tvs_pass(K, (z)))
#   define G(z, sym) \
      do { TValue symbol = ksymbol_new(K, (sym), KNIL); \
           krooted_tvs_push(K, symbol); \
           kadd_binding(K, K->ground_env, symbol, (z)); \
           krooted_tvs_pop(K); \
      } while (0)

#if KTRACK_SI
    TValue str = R(kstring_new_b_imm(K, __FILE__));
    TValue tail = R(kcons(K, i2tv(__LINE__), i2tv(0)));
    TValue si = kcons(K, str, tail);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_push(K, si);
#   define RSI(z) (krooted_tvs_pass_si(K, (z), si))
#else
#   define RSI(z) R(z)
#endif

    TValue std_env = RSI(kmake_environment(K, K->ground_env));
    TValue root_cont = RSI(kmake_continuation(K, KNIL, do_script_exit, 0));
    TValue error_cont = RSI(kmake_continuation(K, root_cont, do_script_error, 1, std_env));
    G(root_cont, "root-continuation");
    G(error_cont, "error-continuation");
    K->root_cont = root_cont;
    K->error_cont = error_cont;
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    TValue argv_value = RSI(argv2value(K, argc, argv));
    TValue loader = RSI(loader_body(K, argv_value, std_env));
    TValue loader_cont = RSI(kmake_continuation(K, root_cont, do_seq, 2, loader, std_env));
    kset_cc(K, loader_cont);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
#if KTRACK_SI
    krooted_tvs_pop(K);
#endif
    kapply_cc(K, KINERT);

#undef R
#undef RSI
#undef G
}

/* skips the unix script directive (#!), if present.
   returns number of lines skipped */
int kscript_eat_directive(FILE *fr)
{
  static const char pattern[] = "#! ";
  int c, n = 0;

  while (pattern[n] != '\0' && (c = getc(fr), c == pattern[n]))
    n++;

  if (pattern[n] == '\0') {
    while (c = getc(fr), c != EOF && c != '\n')
      ;
    return 1;
  } else {
    ungetc(c, fr);
    while (n > 0)
      ungetc(pattern[--n], fr);
    return 0;
  }
}
