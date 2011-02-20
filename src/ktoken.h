/*
** ktoken.h
** Tokenizer for the Kernel Programming Language
** See Copyright Notice in klisp.h
*/

#ifndef ktoken_h
#define ktoken_h

#include "kobject.h"

#include <stdio.h>

/*
** Tokenizer interface
*/
void ktok_init();
TValue ktok_read_token();
void ktok_reset_source_info();
TValue ktok_get_source_info();

/* TODO: move this to the global state */
FILE *ktok_file;

/* XXX: for now, lines and column names are fixints */
typedef struct {
    char *filename;
    int32_t tab_width;
    int32_t line;
    int32_t col;
    
    char *saved_filename;
    int32_t saved_line;
    int32_t saved_col;
} ksource_info_t;

ksource_info_t ktok_source_info;

#endif
