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

int main(int argc, char *argv[]) 
{
    /*
    ** Simple read loop
    */
    printf("Read Type Test\n");

    kread_file = stdin;
    kread_filename = "*STDIN*";
    kread_init();

    TValue obj = KNIL;

    while(!ttiseof(obj)) {
	obj = kread();
	printf("\nRead Object Type: %s\n", ttname(obj));
    }

    return 0;
}
