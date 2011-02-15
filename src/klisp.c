/*
** klisp.c
** Kernel stand-alone interpreter
** See Copyright Notice in klisp.h
*/

#include <stdio.h>

#include "kobject.h"

int main(int argc, char *argv[]) 
{
    printf("Tests\n");

    printf("\nVariables: \n");
    printf("nil: %d\n", ttisnil(knil));
    printf("ignore: %d\n", ttisignore(kignore));
    printf("inert: %d\n", ttisinert(kinert));
    printf("eof: %d\n", ttiseof(keof));
    printf("true: %d\n", ttisboolean(ktrue));
    printf("false: %d\n", ttisboolean(kfalse));


    printf("\nConstants: \n");

    printf("nil: %d\n", ttisnil(KNIL));
    printf("ignore: %d\n", ttisignore(KIGNORE));
    printf("inert: %d\n", ttisinert(KINERT));
    printf("eof: %d\n", ttiseof(KEOF));
    printf("true: %d\n", ttisboolean(KTRUE));
    printf("false: %d\n", ttisboolean(KFALSE));

    printf("int: %d\n", 
	   ttisfixint(((TValue) {.tv = {.t = K_TAG_FIXINT, .v = { .i = 3}}})));
    printf("double: %d\n", ttisdouble((TValue){.d = 1.0}));

    printf("\nSwitch: \n");

    printf("nil: %d\n", ttype(KNIL));
    printf("ignore: %d\n", ttype(KIGNORE));
    printf("inert: %d\n", ttype(KINERT));
    printf("eof: %d\n", ttype(KEOF));
    printf("true: %d\n", ttype(KTRUE));
    printf("false: %d\n", ttype(KFALSE));
    printf("int: %d\n", 
	   ttype(((TValue) {.tv = {.t = K_TAG_FIXINT, .v = { .i = 3}}})));
    printf("double: %d\n", ttype((TValue){.d = 1.0}));

    return 0;
}
