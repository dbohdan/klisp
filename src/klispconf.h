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
