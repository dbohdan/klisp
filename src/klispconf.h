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
#define KTRACK_MARKS (true)
*/

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
#define KLISPI_GCPAUSE	200  /* 200% (wait memory to double before next GC) */


/*
@@ KLISPI_GCMUL defines the default speed of garbage collection relative to
@* memory allocation as a percentage.
** CHANGE it if you want to change the granularity of the garbage
** collection. (Higher values mean coarser collections. 0 represents
** infinity, where each step performs a full collection.) You can also
** change this value dynamically.
*/
#define KLISPI_GCMUL	200 /* GC runs 'twice the speed' of memory allocation */
