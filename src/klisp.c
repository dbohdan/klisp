/*
** klisp.c
** Kernel stand-alone interpreter
** See Copyright Notice in klisp.h
*/

#include <stdio.h>

#include <inttypes.h>
#include <math.h>

#include "kobject.h"
#include "kread.h"
#include "kwrite.h"

int main(int argc, char *argv[]) 
{
    /*
    ** Simple read/write loop
    */
    printf("Read/Write Test\n");

    kread_file = stdin;
    kread_filename = "*STDIN*";
    kwrite_file = stdout;
    kread_init();
    kwrite_init();

    TValue obj = KNIL;

    while(!ttiseof(obj)) {
	obj = kread();
	kwrite(obj);
	knewline();
    }

    return 0;
}
