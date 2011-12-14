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

/* can't be inline because we also use pointers to them,
   (at least gcc doesn't bother to create them and the linker fails) */
bool kportp(TValue o);
bool kinput_portp(TValue o);
bool koutput_portp(TValue o);
bool kbinary_portp(TValue o);
bool ktextual_portp(TValue o);
bool kfile_portp(TValue o);
bool kstring_portp(TValue o);
bool kbytevector_portp(TValue o);
bool kport_openp(TValue o);
bool kport_closedp(TValue o);

/* GC: Assumes filename is rooted */
TValue kmake_fport(klisp_State *K, TValue filename, bool writep, bool binaryp);

/* this is for creating ports for stdin/stdout/stderr &
   helper for the one above */
/* GC: Assumes filename, name & si are rooted */
TValue kmake_std_fport(klisp_State *K, TValue filename, bool writep, 
                       bool binaryp, FILE *file);

/* GC: buffer doesn't need to be rooted, but should probably do it anyways */
TValue kmake_mport(klisp_State *K, TValue buffer, bool writep, bool binaryp);

/* This closes the underlying FILE * (unless it is a std port or memory port) 
   and also set the closed flag to true, this shouldn't throw errors because 
   it is also called when deallocating all objs. If errors need to be thrown
   fork this function instead of simply modifying */
void kclose_port(klisp_State *K, TValue port);

#define kport_filename(p_) (tv2port(p_)->filename)
#define kport_line(p_) (tv2port(p_)->row)
#define kport_col(p_) (tv2port(p_)->col)

#define kfport_file(p_) (tv2fport(p_)->file)

#define kmport_off(p_) (tv2mport(p_)->off)
#define kmport_buf(p_) (tv2mport(p_)->buf)

void kport_reset_source_info(TValue port);
void kport_update_source_info(TValue port, int32_t line, int32_t col);
/* GC: port should be rooted */
void kmport_resize_buffer(klisp_State *K, TValue port, uint32_t min_size);

#endif
