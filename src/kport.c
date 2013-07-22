/*
** kport.c
** Kernel Ports
** See Copyright Notice in klisp.h
*/

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "kport.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"
#include "kerror.h"
#include "kstring.h"
#include "kbytevector.h"
#include "kgc.h"
#include "kpair.h"

bool kportp(TValue o)
{
    return ttisport(o);
}

bool kinput_portp(TValue o)
{
    return ttisport(o) && kport_is_input(o);
}

bool koutput_portp(TValue o)
{
    return ttisport(o) && kport_is_output(o);
}

bool kbinary_portp(TValue o)
{
    return ttisport(o) && kport_is_binary(o);
}

bool ktextual_portp(TValue o)
{
    return ttisport(o) && kport_is_textual(o);
}

bool kfile_portp(TValue o)
{
    return ttisfport(o);
}

bool kstring_portp(TValue o)
{
    return ttismport(o) && kport_is_textual(o);
}

bool kbytevector_portp(TValue o)
{
    return ttismport(o) && kport_is_binary(o);
}

bool kport_openp(TValue o) 
{ 
    klisp_assert(ttisport(o));
    return kport_is_open(o); 
}

bool kport_closedp(TValue o) 
{ 
    klisp_assert(ttisport(o));
    return kport_is_closed(o); 
}

/* XXX: per the c spec, this truncates the file if it exists! */
/* Ask John: what would be best? Probably should also include delete,
   file-exists? and a mechanism to truncate or append to a file, or 
   throw error if it exists.
   Should use open, but it is non standard (fcntl.h, POSIX only) */

/* GC: Assumes filename is rooted */
TValue kmake_fport(klisp_State *K, TValue filename, bool writep, bool binaryp)
{
    /* for now always use text mode */
    char *mode;
    if (binaryp)
        mode = writep? "wb": "rb";
    else
        mode = writep? "w": "r";
	    
    FILE *f = fopen(kstring_buf(filename), mode);
    if (f == NULL) {
        TValue mode_str = kstring_new_b(K, mode);
        krooted_tvs_push(K, mode_str);
        klispE_throw_errno_with_irritants(K, "fopen", 2, filename, mode_str);
        return KINERT;
    } else {
        return kmake_std_fport(K, filename, writep, binaryp, f);
    }
}

/* this is for creating ports for stdin/stdout/stderr &
   also a helper for the above */

/* GC: Assumes filename, name & si are rooted */
TValue kmake_std_fport(klisp_State *K, TValue filename, bool writep, 
                       bool binaryp, FILE *file)
{
    FPort *new_port = klispM_new(K, FPort);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_port, K_TFPORT, 
                K_FLAG_CAN_HAVE_NAME | 
                (writep? K_FLAG_OUTPUT_PORT : K_FLAG_INPUT_PORT) |
                (binaryp? K_FLAG_BINARY_PORT : 0));

    /* port specific fields */
    new_port->filename = filename;
    new_port->file = file;
    TValue tv_port = gc2fport(new_port);
    /* line is 1-based and col is 0-based */
    kport_line(tv_port) = 1;
    kport_col(tv_port) = 0;

    return tv_port;
}

TValue kmake_mport(klisp_State *K, TValue buffer, bool writep, bool binaryp)
{
    klisp_assert(!writep || ttisinert(buffer));
    klisp_assert(writep || (ttisbytevector(buffer) && binaryp) ||
                 (ttisstring(buffer) && !binaryp));

    if (writep) {
        buffer = binaryp? kbytevector_new_s(K, MINBYTEVECTORPORTBUFFER) :
            kstring_new_s(K, MINSTRINGPORTBUFFER);
    }

    krooted_tvs_push(K, buffer);
    
    MPort *new_port = klispM_new(K, MPort);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_port, K_TMPORT, 
                K_FLAG_CAN_HAVE_NAME | 
                (writep? K_FLAG_OUTPUT_PORT : K_FLAG_INPUT_PORT) |
                (binaryp? K_FLAG_BINARY_PORT : 0));

    /* port specific fields */
    TValue tv_port = gc2mport(new_port);
    kport_filename(tv_port) = G(K)->empty_string; /* XXX for now no filename */
    /* line is 1-based and col is 0-based */
    kport_line(tv_port) = 1;
    kport_col(tv_port) = 0;
    kmport_buf(tv_port) = buffer;
    kmport_off(tv_port) = 0; /* no bytes read/written */
    krooted_tvs_pop(K); 
    return tv_port;
}

/* if the port is already closed do nothing */
/* This is also called from GC, so it shouldn't throw any error */
void kclose_port(klisp_State *K, TValue port)
{
    assert(ttisport(port));

    if (!kport_is_closed(port)) {
        if (ttisfport(port)) {
            FILE *f = tv2fport(port)->file;
            if (f != stdin && f != stderr && f != stdout)
                fclose(f); /* it isn't necessary to check the close ret val */
        }
        kport_set_closed(port);
    }

    return;
}

void kport_reset_source_info(TValue port)
{
    /* line is 1-based and col is 0-based */
    kport_line(port) = 1;
    kport_col(port) = 0;
}

void kport_update_source_info(TValue port, int32_t line, int32_t col)
{
    kport_line(port) = line;
    kport_col(port) = col;
}

/* Always grow by doubling the size (until min_size is reached) */
/* GC: port should be rooted */
void kmport_resize_buffer(klisp_State *K, TValue port, size_t min_size)
{
    klisp_assert(ttismport(port));
    klisp_assert(kport_is_output(port));

    uint32_t old_size = (kport_is_binary(port))?
        kbytevector_size(kmport_buf(port)) :
        kstring_size(kmport_buf(port));
    uint64_t new_size = old_size;
    
    while (new_size < min_size) {
        new_size *= 2;
        if (new_size > SIZE_MAX)
            klispM_toobig(K);
    }
    
    if (new_size == old_size)
        return;

    if (kport_is_binary(port)) {
        TValue new_bb = kbytevector_new_s(K, new_size);
        uint32_t off = kmport_off(port);
        if (off != 0) {
            memcpy(kbytevector_buf(new_bb), 
                   kbytevector_buf(kmport_buf(port)), 
                   off);
        }
        kmport_buf(port) = new_bb; 	
    } else {
        TValue new_str = kstring_new_s(K, new_size);
        uint32_t off = kmport_off(port);
        if (off != 0) {
            memcpy(kstring_buf(new_str), 
                   kstring_buf(kmport_buf(port)), 
                   off);
        }
        kmport_buf(port) = new_str; 	
    }
}
