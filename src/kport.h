/*
** kport.h
** Kernel Ports
** See Copyright Notice in klisp.h
*/

#ifndef kport_h
#define kport_h

#include <stdio.h>

#include "kobject.h"
#include "kstate.h"

/* GC: Assumes filename is rooted */
TValue kmake_port(klisp_State *K, TValue filename, bool writep);

/* this is for creating ports for stdin/stdout/stderr &
 helper for the one above */
/* GC: Assumes filename, name & si are rooted */
TValue kmake_std_port(klisp_State *K, TValue filename, bool writep, 
		      TValue name, TValue si, FILE *file);

/* This closes the underlying FILE * (unless it is a std port) and also
   set the closed flag to true, this shouldn't throw errors because it
   is also called when deallocating all objs. If errors need to be thrown
   fork this function instead of simply modifying */
void kclose_port(klisp_State *K, TValue port);

#define kport_file(p_) (tv2port(p_)->file)
#endif
