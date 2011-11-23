/*
** klispconf.h
** This is a basic configuration file for klisp
** See Copyright Notice in klisp.h
*/

/*
** SOURCE NOTE: this is from lua (greatly reduced)
*/

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*
** ==================================================================
** Search for "@@" to find all configurable definitions.
** ===================================================================
*/

/*
@@ KLISP_ANSI controls the use of non-ansi features.
** CHANGE it (define it) if you want Klisp to avoid the use of any
** non-ansi feature or library.
*/
#if defined(__STRICT_ANSI__)
#define KLISP_ANSI
#endif


#if !defined(KLISP_ANSI) && defined(_WIN32)
#define KLISP_WIN
#endif

#if defined(KLISP_USE_LINUX)
#define KLISP_USE_POSIX
#define KLISP_USE_DLOPEN		/* needs an extra library: -ldl */
#define KLISP_USE_READLINE	/* needs some extra libraries */
#endif

#if defined(KLISP_USE_MACOSX)
#define KLISP_USE_POSIX
#define KLISP_DL_DYLD		/* does not need extra library */
#endif

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

/*
@@ KLISP_USE_POSIX includes all functionallity listed as X/Open System
@* Interfaces Extension (XSI).
** CHANGE it (define it) if your system is XSI compatible.
*/
#if defined(KLISP_USE_POSIX)
#define KLISP_USE_MKSTEMP
#define KLISP_USE_ISATTY
#define KLISP_USE_POPEN
#define KLISP_USE_ULONGJMP
#endif

/*
@@ LUA_PATH and LUA_CPATH are the names of the environment variables that
@* Lua check to set its paths.
@@ KLISP_INIT is the name of the environment variable that klisp
@* checks for initialization code.
** CHANGE them if you want different names.
*/
//#define LUA_PATH        "LUA_PATH"
//#define LUA_CPATH       "LUA_CPATH"
#define KLISP_INIT	"KLISP_INIT"

/*
@@ klisp_stdin_is_tty detects whether the standard input is a 'tty' (that
@* is, whether we're running klisp interactively).
** CHANGE it if you have a better definition for non-POSIX/non-Windows
** systems.
*/
#if defined(KLISP_USE_ISATTY)
#include <unistd.h>
#define klisp_stdin_is_tty()	isatty(0)
#elif defined(KLISP_WIN)
#include <io.h>
#include <stdio.h>
#define klisp_stdin_is_tty()	_isatty(_fileno(stdin))
#else
#define klisp_stdin_is_tty()	1  /* assume stdin is a tty */
#endif

/*
@@ KLISP_PROMPT is the default prompt used by stand-alone Klisp.
@@ KLISP_PROMPT2 is not currently used.
** CHANGE them if you want different prompts. 
*/
#define KLISP_PROMPT		"klisp> "
/* XXX not used for now */
#define KLISP_PROMPT2		">> "

/* temp defines till gc is stabilized */
#define KUSE_GC 1
/* Print msgs when starting and ending gc */
/* #define KDEBUG_GC 1 */

/*
#define KTRACK_MARKS true
*/

/* TODO use this defines */
#define KTRACK_NAMES true
#define KTRACK_SI true

/* These are unused for now, but will be once incremental collection is 
   activated */
/* TEMP: for now the threshold is set manually at the start and then
   manually adjusted after every collection to override the intenal
   calculation done with KLISPI_GCPAUSE */
/*
@@ KLISPI_GCPAUSE defines the default pause between garbage-collector cycles
@* as a percentage.
** CHANGE it if you want the GC to run faster or slower (higher values
** mean larger pauses which mean slower collection.) You can also change
** this value dynamically.
*/

/* In lua that has incremental gc this is setted to 200, in
   klisp as we don't yet have incremental gc, we set it to 400 */
#define KLISPI_GCPAUSE	400  /* 400% (wait memory to quadruple before next GC) */


/*
@@ KLISPI_GCMUL defines the default speed of garbage collection relative to
@* memory allocation as a percentage.
** CHANGE it if you want to change the granularity of the garbage
** collection. (Higher values mean coarser collections. 0 represents
** infinity, where each step performs a full collection.) You can also
** change this value dynamically.
*/
#define KLISPI_GCMUL	200 /* GC runs 'twice the speed' of memory allocation */

/*
@@ KLISP_API is a mark for all core API functions.
@@ KLISPLIB_API is a mark for all standard library functions.
** CHANGE them if you need to define those functions in some special way.
** For instance, if you want to create one Windows DLL with the core and
** the libraries, you may want to use the following definition (define
** KLISP_BUILD_AS_DLL to get it).
*/
#if defined(KLISP_BUILD_AS_DLL)

#if defined(KLISP_CORE) || defined(KLISP_LIB)
#define KLISP_API __declspec(dllexport)
#else
#define KLISP_API __declspec(dllimport)
#endif

#else

#define KLISP_API		extern

#endif

/* more often than not the libs go together with the core */
#define KLISPLIB_API	KLISP_API

/* TODO: add klisp_core/lib defines... see lua */
