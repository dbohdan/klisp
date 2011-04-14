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

/* XXX: per the c spec, this truncates the file if it extists! */
/* Ask John: what would be best? Probably should also include delete,
   file-exists? and a mechanism to truncate or append to a file, or 
   throw error if it exists.
   Should use open, but it is non standard (fcntl.h, POSIX only) */
TValue kmake_port(klisp_State *K, TValue filename, bool writep, TValue name, 
			  TValue si)
{
    /* for now always use text mode */
    FILE *f = fopen(kstring_buf(filename), writep? "w": "r");
    if (f == NULL) {
	klispE_throw(K, "Create port: could't open file");
	return KINERT;
    } else {
	return kmake_std_port(K, filename, writep, name, si, f);
    }
}

/* this is for creating ports for stdin/stdout/stderr &
 also a helper for the above */
TValue kmake_std_port(klisp_State *K, TValue filename, bool writep, 
		      TValue name, TValue si, FILE *file)
{
    Port *new_port = klispM_new(K, Port);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_port, K_TPORT, 
	writep? K_FLAG_OUTPUT_PORT : K_FLAG_INPUT_PORT);

    /* port specific fields */
    new_port->name = name;
    new_port->si = si;
    new_port->filename = filename;
    new_port->file = file;

    return gc2port(new_port);
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
