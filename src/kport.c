/*
** kport.c
** Kernel Ports
** See Copyright Notice in klisp.h
*/

#include <stdio.h>
#include <assert.h>

#include "kport.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"
#include "kerror.h"
#include "kstring.h"
#include "kgc.h"
#include "kpair.h"

/* XXX: per the c spec, this truncates the file if it exists! */
/* Ask John: what would be best? Probably should also include delete,
   file-exists? and a mechanism to truncate or append to a file, or 
   throw error if it exists.
   Should use open, but it is non standard (fcntl.h, POSIX only) */

/* GC: Assumes filename is rooted */
TValue kmake_port(klisp_State *K, TValue filename, bool writep)
{
    /* for now always use text mode */
    FILE *f = fopen(kstring_buf(filename), writep? "w": "r");
    if (f == NULL) {
        klispE_throw_errno_with_irritants(K, "fopen", 2, filename,
                                          kstring_new_b_imm(K, writep? "w": "r"));
	return KINERT;
    } else {
	return kmake_std_port(K, filename, writep, f);
    }
}

/* this is for creating ports for stdin/stdout/stderr &
 also a helper for the above */

/* GC: Assumes filename, name & si are rooted */
TValue kmake_std_port(klisp_State *K, TValue filename, bool writep, 
		      FILE *file)
{
    Port *new_port = klispM_new(K, Port);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_port, K_TPORT, 
		K_FLAG_CAN_HAVE_NAME | 
		(writep? K_FLAG_OUTPUT_PORT : K_FLAG_INPUT_PORT));

    /* port specific fields */
    new_port->filename = filename;
    new_port->file = file;
    TValue tv_port = gc2port(new_port);
    /* line is 1-based and col is 0-based */
    kport_line(tv_port) = 1;
    kport_col(tv_port) = 0;

    return tv_port;
}

/* if the port is already closed do nothing */
void kclose_port(klisp_State *K, TValue port)
{
    assert(ttisport(port));

    if (!kport_is_closed(port)) {
	FILE *f = tv2port(port)->file;
	if (f != stdin && f != stderr && f != stdout)
	    fclose(f); /* it isn't necessary to check the close ret val */

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
