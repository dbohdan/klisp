/*
** klisp.c
** Kernel stand-alone interpreter
** See Copyright Notice in klisp.h
*/

#include <stdio.h>

#include "kobject.h"
#include "ktoken.h"

int main(int argc, char *argv[]) 
{
    /*
    ** Simple tokenizer loop
    */
    printf("Tokenizer Type Test\n");
    
    ktok_file = stdin;
    ktok_init();

    TValue tok = KNIL;

    while(!ttiseof(tok)) {
	tok = ktok_read_token();
	if (ttisnil(tok)) {
	    /* there was an error */
	    break;
	}
	printf("\nToken Type: %s\n", ttname(tok));
    }

    return 0;
}
