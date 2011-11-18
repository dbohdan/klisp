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
TValue kmake_fport(klisp_State *K, TValue filename, bool writep, bool binaryp);

/* this is for creating ports for stdin/stdout/stderr &
 helper for the one above */
/* GC: Assumes filename, name & si are rooted */
TValue kmake_std_fport(klisp_State *K, TValue filename, bool writep, 
		       bool binaryp, FILE *file);

/* This closes the underlying FILE * (unless it is a std port) and also
   set the closed flag to true, this shouldn't throw errors because it
   is also called when deallocating all objs. If errors need to be thrown
   fork this function instead of simply modifying */
void kclose_port(klisp_State *K, TValue port);

#define kport_filename(p_) (tv2port(p_)->filename)
#define kport_line(p_) (tv2port(p_)->row)
#define kport_col(p_) (tv2port(p_)->col)

#define kport_file(p_) (tv2fport(p_)->file)

void kport_reset_source_info(TValue port);
void kport_update_source_info(TValue port, int32_t line, int32_t col);

#endif
