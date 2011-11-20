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

static void print_usage (void) {
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

static void k_message (const char *pname, const char *msg) {
    if (pname)
	fprintf(stderr, "%s: ", pname);
    fprintf(stderr, "%s\n", msg);
    fflush(stderr);
}

struct Smain {
  int argc;
  char **argv;
  int status;
};

int main(int argc, char *argv[]) 
{
    if (argv[0] && argv[0][0])
	progname = argv[0];

    klisp_State *K = klispL_newstate();

    if (K == NULL) {
	k_message(argv[0], "cannot create state: not enough memory");
	return EXIT_FAILURE;
    }

    /* TODO Here we should load libraries, however we don't have any
       non native bindings in the ground environment yet */
    /* RATIONALE I wanted to write all bindings in c, so that I can later on
       profile them against non native versions and see how they fare.
       Also by writing all in c it's easy to be consistent, especially with
       error messages */

    /* XXX Fix REPL, Fix Script */

    // klispS_run(K); /* XXX Now this does nothing */
    int exit_code = EXIT_FAILURE; // K->script_exit_code;
    klisp_close(K);

    /* TEMP */
    print_usage();

    return exit_code;
}
