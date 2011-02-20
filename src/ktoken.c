/*
** ktoken.c
** Tokenizer for the Kernel Programming Language
** See Copyright Notice in klisp.h
*/


/*
** TODO:
**
** - Support other number types besides fixints and exact infinities
** - Support for complete number syntax (exactness, radix, etc)
** - Support for unicode (strings, char and symbols).
** - Error handling
**
*/
#include <stdio.h>
/* XXX for malloc */
#include <stdlib.h>
/* TODO: use a generalized alloc function */

/* TEMP: for out of mem errors */
#include <assert.h>

#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>

#include "ktoken.h"
#include "kobject.h"
#include "kpair.h"
#include "kstring.h"
#include "ksymbol.h"

/*
** Char sets for fast ASCII char classification
*/

/* Each bit correspond to a char in the 0-255 range */
typedef uint32_t kcharset[8];

/*
** Char set function/macro interface
*/
void kcharset_empty(kcharset);
void kcharset_fill(kcharset, char *);
void kcharset_union(kcharset, kcharset);
#define kcharset_contains(kch_, ch_) \
    ({ unsigned char ch__ = (unsigned char) ch_;	\
	kch_[KCHS_OCTANT(ch__)] & KCHS_BIT(ch__); })

    
/*
** Char set contains macro interface
*/
#define KCHS_OCTANT(ch) (ch >> 5)
#define KCHS_BIT(ch) (1 << (ch & 0x1f))

void kcharset_empty(kcharset chs)
{
    for (int i = 0; i < 8; i++) {
	chs[i] = 0;
    }
}

void kcharset_fill(kcharset chs, char *chars_)
{
    unsigned char *chars = (unsigned char *) chars_;
    unsigned char ch;

    kcharset_empty(chs);

    while ((ch = *chars++)) {
	chs[KCHS_OCTANT(ch)] |= KCHS_BIT(ch);
    }
}

void kcharset_union(kcharset chs, kcharset chs2)
{
    for (int i = 0; i < 8; i++) {
	chs[i] |= chs2[i];
    }
}

/*
** Character sets for classification
*/
kcharset ktok_alphabetic, ktok_numeric, ktok_whitespace;
kcharset ktok_delimiter, ktok_extended, ktok_subsequent;

#define ktok_is_alphabetic(chi_) kcharset_contains(ktok_alphabetic, chi_)
/* TODO: add is_digit, that takes the base as parameter */
#define ktok_is_numeric(chi_) kcharset_contains(ktok_numeric, chi_)
/* TODO: add hex digits */
#define ktok_digit_value(ch_) (ch_ - '0')
#define ktok_is_whitespace(chi_) kcharset_contains(ktok_whitespace, chi_)
#define ktok_is_delimiter(chi_) (chi == EOF || \
				 kcharset_contains(ktok_delimiter, chi_))
#define ktok_is_subsequent(chi_) kcharset_contains(ktok_subsequent, chi_)

/*
** Special Tokens
*/

/*
** RATIONALE:
**
** Because a pair is not a token, they can be used to represent special tokens
** instead of creating an otherwise useless special token type
** lparen, rparen and dot are represented as a pair with the corresponding 
** char in the car and nil in the cdr.
** srfi-38 tokens are also represented with a char in the car indicating if 
** it's a defining token ('=') or a referring token ('#') and the number in 
** the cdr. This way a special token can be easily tested for (with ttispair)
** and easily classified (with switch(chvalue(kcar(tok)))).
**
*/
TValue ktok_lparen, ktok_rparen, ktok_dot;

/* TODO: move this to the global state */
char *ktok_buffer;
uint32_t ktok_buffer_size;
#define KTOK_BUFFER_INITIAL_SIZE 1024
/* WORKAROUND: for stdin line buffering & reading of EOF */
bool ktok_seen_eof;

void ktok_init()
{
    assert(ktok_file != NULL);
    assert(ktok_source_info.filename != NULL);

    /* WORKAROUND: for stdin line buffering & reading of EOF */
    ktok_seen_eof = false;
    /* string buffer */
    /* TEMP: for now use a fixed size */
    ktok_buffer_size = KTOK_BUFFER_INITIAL_SIZE;
    ktok_buffer = malloc(KTOK_BUFFER_INITIAL_SIZE);
    /* TEMP: while there is no error handling code */
    assert(ktok_buffer != NULL);

    /* Character sets */
    kcharset_fill(ktok_alphabetic, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		  "abcdefghijklmnopqrstuvwxyz");
    kcharset_fill(ktok_numeric, "0123456789");
    kcharset_fill(ktok_whitespace, " \t\v\r\n\f");

    kcharset_fill(ktok_delimiter, "()\";");
    kcharset_union(ktok_delimiter, ktok_whitespace);

    kcharset_fill(ktok_extended, "!$%&*+-./:<=>?@^_~");

    kcharset_empty(ktok_subsequent);
    kcharset_union(ktok_subsequent, ktok_alphabetic);
    kcharset_union(ktok_subsequent, ktok_numeric);
    kcharset_union(ktok_subsequent, ktok_extended);

    /* Special Tokens */
    ktok_lparen = kcons(ch2tv('('), KNIL);
    ktok_rparen = kcons(ch2tv(')'), KNIL);
    ktok_dot = kcons(ch2tv('.'), KNIL);
}

/*
** Underlying stream interface & source code location tracking
*/
int ktok_getc() {
    /* WORKAROUND: for stdin line buffering & reading of EOF */
    if (ktok_seen_eof) {
	return EOF;
    } else {
	int chi = getc(ktok_file);
	if (chi == EOF) {
	    /* NOTE: eof doesn't change source code location info */
	    ktok_seen_eof = true;
	    return EOF;
	}
	
	/* track source code location before returning the char */
	if (chi == '\t') {
	    /* align column to next tab stop */
	    ktok_source_info.col = 
		(ktok_source_info.col + ktok_source_info.tab_width) -
		(ktok_source_info.col % ktok_source_info.tab_width);
	    return '\t';
	} else if (chi == '\n') {
	    ktok_source_info.line++;
	    ktok_source_info.col = 0;
	    return '\n';
	} else {
	    return chi;
	}
    }
}

int ktok_peekc() {
    /* WORKAROUND: for stdin line buffering & reading of EOF */
    if (ktok_seen_eof) {
	return EOF;
    } else {
	int chi = getc(ktok_file);
	if (chi == EOF)
	    ktok_seen_eof = true;
	else
	    ungetc(chi, ktok_file);
	return chi;
    }
}

void ktok_reset_source_info()
{
    /* line is 1-base and col is 0-based */
    ktok_source_info.line = 1;
    ktok_source_info.col = 0;
}

void ktok_save_source_info()
{
    ktok_source_info.saved_filename = ktok_source_info.filename;
    ktok_source_info.saved_line = ktok_source_info.line;
    ktok_source_info.saved_col = ktok_source_info.col;
}

TValue ktok_get_source_info()
{
    /* XXX: what happens on gc? (unrooted objects) */
    /* NOTE: the filename doesn't contains embedded '\0's */
    TValue filename_str = kstring_new(ktok_source_info.saved_filename,
				      strlen(ktok_source_info.saved_filename));
    /* TEMP: for now, lines and column names are fixints */
    return kcons(filename_str, kcons(i2tv(ktok_source_info.saved_line),
				     i2tv(ktok_source_info.saved_col)));
}

/*
** Error management
*/
TValue ktok_error(char *str)
{
    /* TODO: Decide on error handling mechanism for reader (& tokenizer) */
    /* TEMP: Use eof object */
    printf("TOK ERROR: %s\n", str);
    return KEOF;
}


/*
** ktok_read_token() helpers
*/
void ktok_ignore_whitespace_and_comments();
bool ktok_check_delimiter();
TValue ktok_read_string();
TValue ktok_read_special();
TValue ktok_read_number(bool);
TValue ktok_read_maybe_signed_numeric();
TValue ktok_read_identifier();
int ktok_read_until_delimiter();

/*
** Main tokenizer function
*/
TValue ktok_read_token ()
{
    ktok_ignore_whitespace_and_comments();
    /*
    ** NOTE: We jumped over all whitespace
    ** so either the next token starts here or eof was reached, 
    ** in any case we save the location of the port
    */

    /* save the source info of the start of the next token */
    ktok_save_source_info();

    int chi = ktok_peekc();

    switch(chi) {
    case EOF:
	ktok_getc();
	return KEOF;
    case '(':
	ktok_getc();
	return ktok_lparen;
    case ')':
	ktok_getc();
	return ktok_rparen;
    case '.':
	ktok_getc();
	if (ktok_check_delimiter())
	    return ktok_dot;
	else
	    return ktok_error("no delimiter found after dot");
    case '"':
	return ktok_read_string();
    case '#':
	return ktok_read_special();
    case '0': case '1': case '2': case '3': case '4': 
    case '5': case '6': case '7': case '8': case '9':
	return ktok_read_number(true); /* positive number */
    case '+': case '-':
	return ktok_read_maybe_signed_numeric();
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': 
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': 
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': 
    case 'V': case 'W': case 'X': case 'Y': case 'Z': 
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': 
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': 
    case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': 
    case 'v': case 'w': case 'x': case 'y': case 'z': 
    case '!': case '$': case '%': case '&': case '*': case '/': case ':': 
    case '<': case '=': case '>': case '?': case '@': case '^': case '_': 
    case '~': 
	/*
	** NOTE: the cases for '+', '-', '.' and numbers were already 
	** considered so identifier-subsequent is used instead of 
	** identifier-first-char (in the cases above)
	*/
	return ktok_read_identifier();
    default:
	ktok_getc();
	return ktok_error("unrecognized token starting char");
    }
}

/*
** Comments and Whitespace
*/
void ktok_ignore_comment()
{
    int chi;
    do {
	chi = ktok_getc();
    } while (chi != EOF && chi != '\n');
}

void ktok_ignore_whitespace_and_comments()
{
    /* NOTE: if it's not a whitespace or comment do nothing (even on eof) */
    bool end = false;
    while(!end) {
	int chi = ktok_peekc();

	if (chi == EOF) {
	    end = true; 
	} else {
	    char ch = (char) chi;
	    if (ktok_is_whitespace(ch)) {
		ktok_getc();
	    } else if (ch == ';') {
		ktok_ignore_comment(); /* NOTE: this also reads again the ';' */
	    } else {
		end = true;
	    }
	}
    }
}

/*
** Delimiter checking
*/
bool ktok_check_delimiter()
{
    int chi = ktok_peekc();
    return (ktok_is_delimiter(chi));
}

/*
** Returns the number of bytes read
*/
int ktok_read_until_delimiter()
{
    int i = 0;

    while (!ktok_check_delimiter()) {
	/* TODO: allow buffer to grow */
	assert(i + 1 < ktok_buffer_size);

	/* NOTE: can't be eof, because eof is a delimiter */
	char ch = (char) ktok_getc();
	ktok_buffer[i++] = ch;
    }
    ktok_buffer[i] = '\0';
    return i;
}

/*
** Numbers
** TEMP: for now, only fixints in base 10
*/
TValue ktok_read_number(bool is_pos) 
{
    int32_t res = 0;

    while(!ktok_check_delimiter()) {
	/* NOTE: can't be eof because it's a delimiter */
	char ch = (char) ktok_getc();
	if (!ktok_is_numeric(ch))
	    return ktok_error("Not a digit found in number");
	res = res * 10 + ktok_digit_value(ch);
    }

    if (!is_pos) 
	res = -res;
    return i2tv(res);
}

TValue ktok_read_maybe_signed_numeric() 
{
    /* NOTE: can't be eof, it's either '+' or '-' */
    char ch = (char) ktok_getc();
    if (ktok_check_delimiter()) {
	ktok_buffer[0] = ch;
	ktok_buffer[1] = '\0';
	return ksymbol_new(ktok_buffer);
    } else {
	return ktok_read_number(ch == '+');
    }
}

/*
** Strings
*/
TValue ktok_read_string()
{
    /* discard opening quote */
    ktok_getc();

    bool done = false;
    int i = 0;

    while(!done) {
	int chi = ktok_getc();
	char ch = (char) chi;

	if (chi == EOF)
	    return ktok_error("EOF found while reading a string");
	if (ch == '"') {
	    ktok_buffer[i] = '\0';
	    done = true;
	} else {
	    if (ch == '\\') {
		chi = ktok_getc();
	
		if (chi == EOF)
		    return ktok_error("EOF found while reading a string");

		ch = (char) chi;

		if (ch != '\\' && ch != '"') {
		    return ktok_error("Invalid char after '\\' " 
				      "while reading a string");
		}
	    } 
	    /* TODO: allow buffer to grow */
	    assert(i+1 < ktok_buffer_size);

	    ktok_buffer[i++] = ch;
	}
    }
    return kstring_new(ktok_buffer, i);
}

/*
** Special constants (starting with "#")
** (Special number syntax, char constants, #ignore, #inert, srfi-38 tokens) 
*/
TValue ktok_read_special()
{
    /* discard the '#' */
    ktok_getc();
    
    int chi = ktok_getc();
    char ch = (char) chi;

    if (chi == EOF)
	return ktok_error("EOF found while reading a '#' constant");

    switch(ch) {
    case 'i':
	/* ignore or inert */
	/* XXX: could also be an inexact number */
	ktok_read_until_delimiter();
	/* NOTE: can use strcmp even in the presence of '\0's */
	if (strcmp(ktok_buffer, "gnore") == 0)
	    return KIGNORE;
	else if (strcmp(ktok_buffer, "nert") == 0)
	    return KINERT;
	else
	    return ktok_error("unexpected char in # constant");
    case 'e':
	/* an exact infinity */
	/* XXX: could also be an exact number */
	if (ktok_read_until_delimiter()) {
	    /* NOTE: can use strcmp even in the presence of '\0's */
	    if (strcmp(ktok_buffer, "+infinity") == 0)
		return KEPINF;
	    else if (strcmp(ktok_buffer, "-infinity") == 0)
		return KEMINF;
	    else
		return ktok_error("unexpected char in # constant");
	} else
	    return ktok_error("unexpected error in # constant");
    case 't':
    case 'f':
	/* boolean constant */
	if (ktok_check_delimiter())
	    return b2tv(ch == 't');
	else
	    return ktok_error("unexpected char in # constant");
    case '\\':
	/* char constant */
	/* 
	** RATIONALE: in the scheme spec (R5RS) it says that only alphabetic 
	** char constants need a delimiter to disambiguate the cases with 
	** character names. It would be more consistent if all characters
	** needed a delimiter (and is probably implied by the yet incomplete
	** Kernel report (R-1RK))
	** For now we follow the scheme report 
	*/
	chi = ktok_getc();
	ch = (char) chi;

	if (chi == EOF)
	    return ktok_error("EOF found while reading a char constant");

	if (!ktok_is_alphabetic(ch) || ktok_check_delimiter())
	    return ch2tv(ch);

	ktok_read_until_delimiter();
	char *p = ktok_buffer;
	while (*p) {
	    *p = tolower(*p);
	    p++;
	}
	ch = tolower(ch);
	/* NOTE: can use strcmp even in the presence of '\0's */
	if (ch == 's' && strcmp(ktok_buffer, "pace") == 0)
	    return ch2tv(' ');
	else if (ch == 'n' && strcmp(ktok_buffer, "ewline") == 0)
	    return ch2tv('\n');
	else 
	    return ktok_error("Unrecognized character name");
    case '0': case '1': case '2': case '3': case '4': 
    case '5': case '6': case '7': case '8': case '9': {
	/* srfi-38 type token (can be either a def or ref) */
	/* TODO: allow bigints */
	    int32_t res = 0;
	    while(ch != '#' && ch != '=') {
		if (!ktok_is_numeric(ch))
		    return ktok_error("Invalid char found in srfi-38 token");

		res = res * 10 + ktok_digit_value(ch);

		chi = ktok_getc();
		ch = (char) chi;
	
		if (chi == EOF)
		    return ktok_error("EOF found while reading a srfi-38 token");
	    }
	    return kcons(ch2tv(ch), i2tv(res));
	}
    /* TODO: add real with no primary value and undefined */
    default:
	return ktok_error("unexpected char in # constant");
    }
}

/*
** Identifiers
*/
TValue ktok_read_identifier()
{
    int i = 0;

    while (!ktok_check_delimiter()) {
	/* TODO: allow buffer to grow */
	assert(i+1 < ktok_buffer_size);

	/* NOTE: can't be eof, because eof is a delimiter */
	char ch = (char) ktok_getc();

	/* NOTE: is_subsequent of '\0' is false, so no embedded '\0' */
	if (ktok_is_subsequent(ch))
	    ktok_buffer[i++] = ch;
	else
	    return ktok_error("Invalid char in identifier");	    
    }
    ktok_buffer[i] = '\0';
    return ksymbol_new(ktok_buffer);
}

