/*
** klisp.c
** Kernel stand-alone interpreter
** See Copyright Notice in klisp.h
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <setjmp.h>

#include "klimits.h"

#include "klisp.h"
#include "kstate.h"
#include "kauxlib.h"
#include "kscript.h"

/* TODO this should be moved to a file named like klispconf.h (see lua) */
/*
@@ KLISP_PROGNAME is the default name for the stand-alone klisp program.
** CHANGE it if your stand-alone interpreter has a different name and
** your system is not able to detect that name automatically.
*/
#define KLISP_PROGNAME		"klisp"

/*
@@ KLISP_QL describes how error messages quote program elements.
** CHANGE it if you want a different appearance.
*/
#define KLISP_QL(x)	"'" x "'"
#define KLISP_QS	KLISP_QL("%s")
/* /TODO */

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

static void print_version (void) 
{
    k_message(NULL, KLISP_RELEASE "  " KLISP_COPYRIGHT);
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

	    /* TODO do string */
	    UNUSED(K);
//	    if (dostring(L, chunk, "=(command line)") != 0)
//		return 1;
	    break;
	}
//	case 'l':  /* no libraries for now */
	default: 
	    break;
	}
    }
    return 0;
}

/* This is weird but was done to follow lua scheme */
struct Smain {
    int argc;
    char **argv;
    int status;
};

static int pmain (klisp_State *K) 
{
    /* This is weird but was done to follow lua scheme */
    struct Smain *s = (struct Smain *) pvalue(K->next_obj);
    char **argv = s->argv;

    if (argv[0] && argv[0][0])
	progname = argv[0];

    /* TODO Here we should load libraries, however we don't have any
       non native bindings in the ground environment yet */
    /* RATIONALE I wanted to write all bindings in c, so that I can later on
       profile them against non native versions and see how they fare.
       Also by writing all in c it's easy to be consistent, especially with
       error messages */

    /* TODO do init */
    bool has_i = false, has_v = false, has_e = false;
    int script = collectargs(argv, &has_i, &has_v, &has_e);

    if (script < 0) { /* invalid args? */
	print_usage();
	s->status = 1;
	return 0;
    }

    if (has_v)
	print_version();

    s->status = runargs(K, argv, (script > 0) ? script : s->argc);

    if (s->status != 0)
	return 0;

    if (script > 0) /* XXX FIX script */
	s->status = 0;

    if (s->status != 0)
	return 0;

    if (has_i) /* TODO FIX REPL */
	s->status = 0;
    else if (script == 0 && !has_e && !has_v) {
	print_version();
	s->status = 0; /* TODO FIX REPL */
    } else 
	s->status = 0; /* TODO do FILE */

    return 0;
}

int main(int argc, char *argv[]) 
{
    int status;
    struct Smain s;
    klisp_State *K = klispL_newstate();

    if (K == NULL) {
	k_message(argv[0], "cannot create state: not enough memory");
	return EXIT_FAILURE;
    }

    /* This is weird but was done to follow lua scheme */
    s.argc = argc;
    s.argv = argv;
    K->next_obj = p2tv(&s);
    status = pmain(K);

    klisp_close(K);

    return (status || s.status)? EXIT_FAILURE : EXIT_SUCCESS;
}
