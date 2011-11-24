/*
** klisp.c
** Kernel stand-alone interpreter
** See Copyright Notice in klisp.h
*/

/*
** TODO This needs a serious clean up, I hacked it together during
** an all nighter...
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <setjmp.h>

#include "klimits.h"

#include "klisp.h"
#include "kstate.h"
#include "kauxlib.h"

#include "kstring.h"
#include "kcontinuation.h"
#include "koperative.h"
#include "kapplicative.h"
#include "ksymbol.h"
#include "kenvironment.h"
#include "kport.h"
#include "kread.h"
#include "kwrite.h"
#include "kerror.h"
#include "kghelpers.h" /* for do_return_value */
#include "kgcontinuations.h" /* for do_pass_value */
#include "kgcontrol.h" /* for do_seq */
#include "kscript.h"
#include "krepl.h"

/* TODO update dependencies in makefile */

static const char *progname = KLISP_PROGNAME;

static void print_usage (void) 
{
    fprintf(stderr,
	    "usage: %s [options] [script [args]].\n"
	    "Available options are:\n"
	    "  -e exp  eval string " KLISP_QL("exp") "\n"
//	    "  -l name  require library " KLISP_QL("name") "\n"
	    "  -i       enter interactive mode after executing " 
	                KLISP_QL("script") "\n"
	    "  -v       show version information\n"
	    "  --       stop handling options\n"
	    "  -        execute stdin and stop handling options\n"
	    ,
	    progname);
    fflush(stderr);
}

static void k_message (const char *pname, const char *msg) 
{
    if (pname)
	fprintf(stderr, "%s: ", pname);
    fprintf(stderr, "%s\n", msg);
    fflush(stderr);
}

/* TODO move this to a common place to use it from elsewhere 
(like the repl) */
static void show_error(klisp_State *K, TValue obj) {
    /* FOR NOW used only for irritant list */
    TValue port = kcdr(K->kd_error_port_key);
    klisp_assert(ttisfport(port) && kfport_file(port) == stderr);

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
    fflush(stderr);
}

static int report (klisp_State *K, int status) 
{
    if (status != EXIT_SUCCESS) {
	const char *msg = "Error!";
	k_message(progname, msg);
	show_error(K, K->next_value);
    }
    return status;
}

static void print_version(void) 
{
    k_message(NULL, KLISP_RELEASE "  " KLISP_COPYRIGHT);
}

/* REFACTOR maybe these should be moved to a general place to be used
   from any program */
void do_str_eval(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /* 
    ** xparams[0]: dynamic environment
    */
    TValue denv = xparams[0];
    ktail_eval(K, obj, denv);
}

void do_str_read(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /* 
    ** xparams[0]: port
    */
    TValue port = xparams[0];
    UNUSED(obj);
    /* read just one value (as mutable data) */
    TValue obj1 = kread_from_port(K, port, true);

    /* obj may be eof, that's not a problem, it just won't do anything */

    krooted_tvs_push(K, obj1);
    TValue obj2 = kread_from_port(K, port, true);
    krooted_tvs_pop(K);
    
    if (!ttiseof(obj2)) {
	klispE_throw_simple_with_irritants(K, "More than one expression read", 
					   1, port);
	return;
    }

    /* all ok, just one exp read (or none and obj1 is eof) */
    kapply_cc(K, obj1);
}

void do_int_mark_error(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    /*
    ** xparams[0]: errorp pointer
    */
    UNUSED(denv);
    bool *errorp = (bool *) pvalue(xparams[0]);
    *errorp = true;
    /* ptree is (object divert) */
    TValue error_obj = kcar(ptree);
    /* pass the error along after setting the flag */
    kapply_cc(K, error_obj);
}

static int dostring (klisp_State *K, const char *s, const char *name) 
{
    bool errorp = false; /* may be set to true in error handler */

    UNUSED(name); /* could use as filename?? */
    /* create a string input port */
    TValue str = kstring_new_b(K, s);
    krooted_tvs_push(K, str);
    TValue port = kmake_mport(K, str, false, false);
    krooted_tvs_pop(K);
    krooted_tvs_push(K, port);

    /* create the guard set error flag after errors */
    TValue exit_int = kmake_operative(K, do_int_mark_error, 
				      1, p2tv(&errorp));
    krooted_tvs_push(K, exit_int);
    TValue exit_guard = kcons(K, K->error_cont, exit_int);
    krooted_tvs_pop(K); /* already in guard */
    krooted_tvs_push(K, exit_guard);
    TValue exit_guards = kcons(K, exit_guard, KNIL);
    krooted_tvs_pop(K); /* already in guards */
    krooted_tvs_push(K, exit_guards);

    TValue entry_guards = KNIL;

    /* this is needed for interception code */
    TValue env = kmake_empty_environment(K);
    krooted_tvs_push(K, env);
    TValue outer_cont = kmake_continuation(K, K->root_cont, 
					   do_pass_value, 2, entry_guards, env);
    kset_outer_cont(outer_cont);
    krooted_tvs_push(K, outer_cont);
    TValue inner_cont = kmake_continuation(K, outer_cont, 
					   do_pass_value, 2, exit_guards, env);
    kset_inner_cont(inner_cont);
    krooted_tvs_pop(K); krooted_tvs_pop(K); krooted_tvs_pop(K);

    /* only port remains in the root stack */
    krooted_tvs_push(K, inner_cont);

    /* This continuation will discard the result of the evaluation
       and pass the root continuation #inert instead. This has to do with
       the way the exit value of the interpreter is calculated (see man page)
    */
    TValue discard_cont = kmake_continuation(K, inner_cont, do_return_value,
					     1, KINERT);

    krooted_tvs_pop(K); /* pop inner cont */
    krooted_tvs_push(K, discard_cont);

    /* XXX This should probably be an extra param to the function */
    env = K->next_env; /* this is the standard env that should be used for 
			  evaluation */
    TValue eval_cont = kmake_continuation(K, discard_cont, do_str_eval, 
					  1, env);
    krooted_tvs_pop(K); /* pop discard cont */
    krooted_tvs_push(K, eval_cont);
    TValue read_cont = kmake_continuation(K, eval_cont, do_str_read, 
					  1, port);
    krooted_tvs_pop(K); /* pop eval cont */
    krooted_tvs_pop(K); /* pop port */
    kset_cc(K, read_cont); /* this will protect all conts from gc */
    klispS_apply_cc(K, KINERT);

    klispS_run(K);

    int status = errorp? EXIT_FAILURE : EXIT_SUCCESS;
    /* get the standard environment again in K->next_env */
    K->next_env = env;
    return report(K, status);
}

void do_file_eval(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /* 
    ** xparams[0]: dynamic environment
    */
    TValue denv = xparams[0];
    TValue ls = obj;
    if (!ttisnil(ls)) {
	TValue new_cont = kmake_continuation(K, kget_cc(K), do_seq, 2, ls, denv);
	kset_cc(K, new_cont);
    } 
    kapply_cc(K, KINERT);
}

void do_file_read(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    UNUSED(obj);
    TValue port = xparams[0];
    /* read all file as a list (as immutable data) */
    TValue ls = kread_list_from_port(K, port, false);

    /* all ok, just one exp read (or none and obj1 is eof) */
    kapply_cc(K, ls);
}

/* name = NULL means use stdin */
static int dofile(klisp_State *K, const char *name) 
{
    bool errorp = false; /* may be set to true in error handler */

    /* create a file input port (unless it's stdin, then just use) */
    TValue port;

    /* XXX better do this in a continuation */
    if (name == NULL) {
	port = kcdr(K->kd_in_port_key);
    } else {
	FILE *file = fopen(name, "r");
	if (file == NULL) {
	    TValue mode_str = kstring_new_b(K, "r");
	    krooted_tvs_push(K, mode_str);
	    TValue name_str = kstring_new_b(K, name);
	    krooted_tvs_push(K, mode_str);
	    TValue error_obj = klispE_new_simple_with_errno_irritants
		(K, "fopen", 2, name_str, mode_str);
	    krooted_tvs_pop(K);
	    krooted_tvs_pop(K);
	    K->next_value = error_obj;
	    return report(K, EXIT_FAILURE);
	}
	    
	TValue name_str = kstring_new_b(K, name);
	krooted_tvs_push(K, name_str);
	port = kmake_std_fport(K, name_str, false, false, file);
	krooted_tvs_pop(K);
    }
    
    krooted_tvs_push(K, port);
    /* TODO this is exactly the same as in string, factor the code out */
    /* create the guard set error flag after errors */
    TValue exit_int = kmake_operative(K, do_int_mark_error, 
				      1, p2tv(&errorp));
    krooted_tvs_push(K, exit_int);
    TValue exit_guard = kcons(K, K->error_cont, exit_int);
    krooted_tvs_pop(K); /* already in guard */
    krooted_tvs_push(K, exit_guard);
    TValue exit_guards = kcons(K, exit_guard, KNIL);
    krooted_tvs_pop(K); /* already in guards */
    krooted_tvs_push(K, exit_guards);

    TValue entry_guards = KNIL;

    /* this is needed for interception code */
    TValue env = kmake_empty_environment(K);
    krooted_tvs_push(K, env);
    TValue outer_cont = kmake_continuation(K, K->root_cont, 
					   do_pass_value, 2, entry_guards, env);
    kset_outer_cont(outer_cont);
    krooted_tvs_push(K, outer_cont);
    TValue inner_cont = kmake_continuation(K, outer_cont, 
					   do_pass_value, 2, exit_guards, env);
    kset_inner_cont(inner_cont);
    krooted_tvs_pop(K); krooted_tvs_pop(K); krooted_tvs_pop(K);

    /* only port remains in the root stack */
    krooted_tvs_push(K, inner_cont);


    /* This continuation will discard the result of the evaluation
       and pass the root continuation #inert instead. This has to do with
       the way the exit value of the interpreter is calculated (see man page)
    */
    TValue discard_cont = kmake_continuation(K, inner_cont, do_return_value,
					     1, KINERT);

    krooted_tvs_pop(K); /* pop inner cont */
    krooted_tvs_push(K, discard_cont);

    /* XXX This should probably be an extra param to the function */
    env = K->next_env; /* this is the standard env that should be used for 
			  evaluation */
    TValue eval_cont = kmake_continuation(K, discard_cont, do_file_eval, 
					  1, env);
    krooted_tvs_pop(K); /* pop discard cont */
    krooted_tvs_push(K, eval_cont);
    TValue read_cont = kmake_continuation(K, eval_cont, do_file_read, 
					  1, port);
    krooted_tvs_pop(K); /* pop eval cont */
    krooted_tvs_pop(K); /* pop port */
    kset_cc(K, read_cont); /* this will protect all conts from gc */
    klispS_apply_cc(K, KINERT);

    klispS_run(K);

    int status = errorp? EXIT_FAILURE : EXIT_SUCCESS;

    /* get the standard environment again in K->next_env */
    K->next_env = env;
    return report(K, status);
}

static void dotty(klisp_State *K)
{
    TValue env = K->next_env;
    kinit_repl(K);
    klispS_run(K);
    /* get the standard environment again in K->next_env */
    K->next_env = env;
}

static int handle_script(klisp_State *K, char **argv, int n) 
{
    const char *fname;
    /* XXX/TODO save arguments to script */
//    int narg = getargs(L, argv, n);  /* collect arguments */
//    lua_setglobal(L, "arg");
    fname = argv[n];
    if (strcmp(fname, "-") == 0 && strcmp(argv[n-1], "--") != 0) 
	fname = NULL;  /* stdin */

    return dofile(K, fname);
}

/* check that argument has no extra characters at the end */
#define notail(x)	{if ((x)[2] != '\0') return -1;}

static int collectargs (char **argv, bool *pi, bool *pv, bool *pe) 
{
    int i;
    for (i = 1; argv[i] != NULL; i++) {
	if (argv[i][0] != '-')  /* not an option? */
	    return i;
	switch (argv[i][1]) {  /* option */
	case '-':
	    notail(argv[i]);
	    return (argv[i+1] != NULL ? i+1 : 0);
	case '\0':
	    return i;
	case 'i':
	    notail(argv[i]);
	    *pi = true;  /* go through */
	case 'v':
	    notail(argv[i]);
	    *pv = true;
	    break;
	case 'e':
	    *pe = true;  /* go through */
//	case 'l': /* No library for now */
	    if (argv[i][2] == '\0') {
		i++;
		if (argv[i] == NULL)
		    return -1;
	    }
	    break;
	default: 
	    return -1;  /* invalid option */
	}
    }
    return 0;
}

static int runargs (klisp_State *K, char **argv, int n) 
{
    /* There is a standard env in K->next_env, a common one is used for all 
       evaluations (init, expression args, script/repl) */
    TValue env = K->next_env; 
    UNUSED(env);

    /* TEMP All passes to root cont and all resulting values will be ignored,
     the only way to interrupt the running of arguments is to throw an error */
    for (int i = 1; i < n; i++) {
	if (argv[i] == NULL) 
	    continue;

	klisp_assert(argv[i][0] == '-');

	switch (argv[i][1]) {  /* option */
	case 'e': {
	    const char *chunk = argv[i] + 2;
	    if (*chunk == '\0') 
		chunk = argv[++i];
	    klisp_assert(chunk != NULL);

	    if (dostring(K, chunk, "=(command line)") != 0)
		return EXIT_FAILURE;
	    break;
	}
//	case 'l':  /* no libraries for now */
	default: 
	    break;
	}
    }
    return EXIT_SUCCESS;
}

static void populate_argument_lists(klisp_State *K, char **argv, int argc, 
				    int script)
{
    /* first create the script list */
    TValue tail = KNIL;
    TValue obj = KINERT;
    krooted_tvs_push(K, tail);
    krooted_tvs_push(K, obj);
    while(argc > script) {
	char *arg = argv[--argc];
	obj = kstring_new_b_imm(K, arg);
	tail = kimm_cons(K, obj, tail);
    }
    /* Store the script argument list */
    obj = ksymbol_new(K, "get-script-arguments", KNIL);
    klisp_assert(kbinds(K, K->ground_env, obj));
    obj = kunwrap(kget_binding(K, K->ground_env, obj));
    tv2op(obj)->extra[0] = tail;

    while(argc > 0) {
	char *arg = argv[--argc];
	obj = kstring_new_b_imm(K, arg);
	tail = kimm_cons(K, obj, tail);
    }
    /* Store the interpreter argument list */
    obj = ksymbol_new(K, "get-interpreter-arguments", KNIL);
    klisp_assert(kbinds(K, K->ground_env, obj));
    obj = kunwrap(kget_binding(K, K->ground_env, obj));
    tv2op(obj)->extra[0] = tail;

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
}

static int handle_klispinit(klisp_State *K) 
{
  const char *init = getenv(KLISP_INIT);
  if (init == NULL) 
      return EXIT_SUCCESS;
  else 
      return dostring(K, init, "=" KLISP_INIT);
}

/* This is weird but was done to follow lua scheme */
struct Smain {
    int argc;
    char **argv;
    int status;
};

static void pmain(klisp_State *K) 
{
    /* This is weird but was done to follow lua scheme */
    struct Smain *s = (struct Smain *) pvalue(K->next_value);
    char **argv = s->argv;
    s->status = EXIT_SUCCESS;

    /* There is a standard env in K->next_env, a common one is used for all 
       evaluations (init, expression args, script/repl) */
    //TValue env = K->next_env; 

    if (argv[0] && argv[0][0])
	progname = argv[0];

    /* TODO Here we should load libraries, however we don't have any
       non native bindings in the ground environment yet */

    /* RATIONALE I wanted to write all bindings in c, so that I can later on
       profile them against non native versions and see how they fare.
       Also by writing all in c it's easy to be consistent, especially with
       error messages */

    /* init (eval KLISP_INIT env variable contents) */
    s->status = handle_klispinit(K);
    if (s->status != EXIT_SUCCESS)
	return;

    bool has_i = false, has_v = false, has_e = false;
    int script = collectargs(argv, &has_i, &has_v, &has_e);

    if (script < 0) { /* invalid args? */
	print_usage();
	s->status = EXIT_FAILURE;
	return;
    }

    if (has_v)
	print_version();

    /* TEMP this could be either set before or after running the arguments,
       we'll do it before for now */
    populate_argument_lists(K, argv, s->argc, (script > 0) ? script : s->argc);
    
    s->status = runargs(K, argv, (script > 0) ? script : s->argc);

    if (s->status != EXIT_SUCCESS)
	return;

    if (script > 0) {
	s->status = handle_script(K, argv, script);
    }

    if (s->status != EXIT_SUCCESS)
	return;

    if (has_i) { 
	dotty(K);
    } else if (script == 0 && !has_e && !has_v) {
	if (true) {
	    print_version();
	    dotty(K);
	} else {
	    s->status = dofile(K, NULL);
	}
    }
}

int main(int argc, char *argv[]) 
{
    struct Smain s;
    klisp_State *K = klispL_newstate();

    if (K == NULL) {
	k_message(argv[0], "cannot create state: not enough memory");
	return EXIT_FAILURE;
    }

    /* This is weird but was done to follow lua scheme */
    s.argc = argc;
    s.argv = argv;
    K->next_value = p2tv(&s);

    pmain(K);

    if (s.status == EXIT_SUCCESS) {
	/* must check value passed to the root continuation to
	   return proper exit status */
	if (ttisinert(K->next_value)) {
	    s.status = EXIT_SUCCESS;
	} else if (ttisboolean(K->next_value)) {
	    s.status = kis_true(K->next_value)? EXIT_SUCCESS : EXIT_FAILURE;
	} else if (ttisfixint(K->next_value)) {
	    s.status = ivalue(K->next_value);
	} else {
	    s.status = EXIT_FAILURE;
	}
    }

    klisp_close(K);

    return s.status;
}
