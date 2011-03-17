/*
** ktoken.c
** Tokenizer for the Kernel Programming Language
** See Copyright Notice in klisp.h
*/

/*
** Symbols should be converted to some standard case before interning
** (in this case downcase)
*/

/*
** TODO:
**
** From the Report:
** - Support other number types besides fixints and exact infinities
** - Support for complete number syntax (exactness, radix, etc)
** 
** NOT from the Report:
** - Support for unicode (strings, char and symbols).
** - srfi-30 stype #| ... |# nested comments and srfi-62 style #;
**    sexp comments.
** - more named chars (like #\tab and in strings "\t")
** - numeric escaped chars (like #\u0020)
**
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>

#include "ktoken.h"
#include "kobject.h"
#include "kstate.h"
#include "kpair.h"
#include "kstring.h"
#include "ksymbol.h"
#include "kerror.h"

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
    ({ unsigned char ch__ = (unsigned char) (ch_);	\
	kch_[KCHS_OCTANT(ch__)] & KCHS_BIT(ch__); })

    
/*
** Char set contains macro interface
*/
#define KCHS_OCTANT(ch) ((ch) >> 5)
#define KCHS_BIT(ch) (1 << ((ch) & 0x1f))

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
#define ktok_is_delimiter(chi_) ((chi_) == EOF ||			\
				 kcharset_contains(ktok_delimiter, chi_))
#define ktok_is_subsequent(chi_) kcharset_contains(ktok_subsequent, chi_)

/*
** Special Tokens 
**
** TEMP: defined in kstate.h
**
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

void ktok_init(klisp_State *K)
{
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
}

/*
** Underlying stream interface & source code location tracking
*/
int ktok_getc(klisp_State *K) {
    /* WORKAROUND: for stdin line buffering & reading of EOF */
    if (K->ktok_seen_eof) {
	return EOF;
    } else {
	int chi = getc(K->curr_in);
	if (chi == EOF) {
	    /* NOTE: eof doesn't change source code location info */
	    K->ktok_seen_eof = true;
	    return EOF;
	}
	
	/* track source code location before returning the char */
	if (chi == '\t') {
	    /* align column to next tab stop */
	    K->ktok_source_info.col = 
		(K->ktok_source_info.col + K->ktok_source_info.tab_width) -
		(K->ktok_source_info.col % K->ktok_source_info.tab_width);
	    return '\t';
	} else if (chi == '\n') {
	    K->ktok_source_info.line++;
	    K->ktok_source_info.col = 0;
	    return '\n';
	} else {
	    return chi;
	}
    }
}

int ktok_peekc(klisp_State *K) {
    /* WORKAROUND: for stdin line buffering & reading of EOF */
    if (K->ktok_seen_eof) {
	return EOF;
    } else {
	int chi = getc(K->curr_in);
	if (chi == EOF)
	    K->ktok_seen_eof = true;
	else
	    ungetc(chi, K->curr_in);
	return chi;
    }
}

void ktok_reset_source_info(klisp_State *K)
{
    /* line is 1-base and col is 0-based */
    K->ktok_source_info.line = 1;
    K->ktok_source_info.col = 0;
}

void ktok_save_source_info(klisp_State *K)
{
    K->ktok_source_info.saved_filename = K->ktok_source_info.filename;
    K->ktok_source_info.saved_line = K->ktok_source_info.line;
    K->ktok_source_info.saved_col = K->ktok_source_info.col;
}

TValue ktok_get_source_info(klisp_State *K)
{
    /* XXX: what happens on gc? (unrooted objects) */
    /* NOTE: the filename doesn't contains embedded '\0's */
    TValue filename_str = 
	kstring_new(K, K->ktok_source_info.saved_filename,
		    strlen(K->ktok_source_info.saved_filename));
    /* TEMP: for now, lines and column names are fixints */
    return kcons(K, filename_str, kcons(K, i2tv(K->ktok_source_info.saved_line),
					i2tv(K->ktok_source_info.saved_col)));
}

/*
** Error management
*/

void clear_shared_dict(klisp_State *K)
{
    K->shared_dict = KNIL;
}

void ktok_error(klisp_State *K, char *str)
{
    /* clear up before throwing */
    ks_tbclear(K);
    ks_sclear(K);
    clear_shared_dict(K);
    klispE_throw(K, str);
}


/*
** ktok_read_token() helpers
*/
void ktok_ignore_whitespace_and_comments(klisp_State *K);
bool ktok_check_delimiter(klisp_State *K);
TValue ktok_read_string(klisp_State *K);
TValue ktok_read_special(klisp_State *K);
TValue ktok_read_number(klisp_State *K, bool sign);
TValue ktok_read_maybe_signed_numeric(klisp_State *K);
TValue ktok_read_identifier(klisp_State *K);
int ktok_read_until_delimiter(klisp_State *K);

/*
** Main tokenizer function
*/
TValue ktok_read_token (klisp_State *K)
{
    assert(ks_tbisempty(K));

    ktok_ignore_whitespace_and_comments(K);
    /*
    ** NOTE: We jumped over all whitespace
    ** so either the next token starts here or eof was reached, 
    ** in any case we save the location of the port
    */

    /* save the source info of the start of the next token */
    ktok_save_source_info(K);

    int chi = ktok_peekc(K);

    switch(chi) {
    case EOF:
	ktok_getc(K);
	return KEOF;
    case '(':
	ktok_getc(K);
	return K->ktok_lparen;
    case ')':
	ktok_getc(K);
	return K->ktok_rparen;
    case '.':
	ktok_getc(K);
	if (ktok_check_delimiter(K))
	    return K->ktok_dot;
	else {
	    ktok_error(K, "no delimiter found after dot");
	    /* avoid warning */
	    return KINERT;
	}
    case '"':
	return ktok_read_string(K);
    case '#':
	return ktok_read_special(K);
    case '0': case '1': case '2': case '3': case '4': 
    case '5': case '6': case '7': case '8': case '9':
	return ktok_read_number(K, true); /* positive number */
    case '+': case '-':
	return ktok_read_maybe_signed_numeric(K);
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
	return ktok_read_identifier(K);
    default:
	ktok_getc(K);
	ktok_error(K, "unrecognized token starting char");
	/* avoid warning */
	return KINERT;
    }
}

/*
** Comments and Whitespace
*/
void ktok_ignore_comment(klisp_State *K)
{
    int chi;
    do {
	chi = ktok_getc(K);
    } while (chi != EOF && chi != '\n');
}

void ktok_ignore_whitespace_and_comments(klisp_State *K)
{
    /* NOTE: if it's not a whitespace or comment do nothing (even on eof) */
    bool end = false;
    while(!end) {
	int chi = ktok_peekc(K);

	if (chi == EOF) {
	    end = true; 
	} else {
	    char ch = (char) chi;
	    if (ktok_is_whitespace(ch)) {
		ktok_getc(K);
	    } else if (ch == ';') {
		ktok_ignore_comment(K); /* NOTE: this also reads again the ';' */
	    } else {
		end = true;
	    }
	}
    }
}

/*
** Delimiter checking
*/
bool ktok_check_delimiter(klisp_State *K)
{
    int chi = ktok_peekc(K);
    return (ktok_is_delimiter(chi));
}

/*
** Returns the number of bytes read
*/
int ktok_read_until_delimiter(klisp_State *K)
{
    int i = 0;

    while (!ktok_check_delimiter(K)) {
	/* NOTE: can't be eof, because eof is a delimiter */
	char ch = (char) ktok_getc(K);
	ks_tbadd(K, ch);
	i++;
    }
    ks_tbadd(K, '\0');
    return i;
}

/*
** Numbers
** TEMP: for now, only fixints in base 10
*/
TValue ktok_read_number(klisp_State *K, bool is_pos) 
{
    int32_t res = 0;

    while(!ktok_check_delimiter(K)) {
	/* NOTE: can't be eof because it's a delimiter */
	char ch = (char) ktok_getc(K);
	if (!ktok_is_numeric(ch)) {
	    ktok_error(K, "Not a digit found in number");
	    /* avoid warning */
	    return KINERT;
	}
	res = res * 10 + ktok_digit_value(ch);
    }

    if (!is_pos) 
	res = -res;
    return i2tv(res);
}

TValue ktok_read_maybe_signed_numeric(klisp_State *K) 
{
    /* NOTE: can't be eof, it's either '+' or '-' */
    char ch = (char) ktok_getc(K);
    if (ktok_check_delimiter(K)) {
	ks_tbadd(K, ch);
	ks_tbadd(K, '\0');
	TValue new_sym = ksymbol_new(K, ks_tbget_buffer(K));
	ks_tbclear(K);
	return new_sym;
    } else {
	return ktok_read_number(K, ch == '+');
    }
}

/*
** Strings
*/
TValue ktok_read_string(klisp_State *K)
{
    /* discard opening quote */
    ktok_getc(K);

    bool done = false;
    int i = 0;

    while(!done) {
	int chi = ktok_getc(K);
	char ch = (char) chi;

	if (chi == EOF) {
	    ktok_error(K, "EOF found while reading a string");
	    /* avoid warning */
	    return KINERT;
	}
	if (ch == '"') {
	    ks_tbadd(K, '\0');
	    done = true;
	} else {
	    if (ch == '\\') {
		chi = ktok_getc(K);
	
		if (chi == EOF) {
		    ktok_error(K, "EOF found while reading a string");
		    /* avoid warning */
		    return KINERT;
		}

		ch = (char) chi;

		if (ch != '\\' && ch != '"') {
		    ktok_error(K, "Invalid char after '\\' " 
			       "while reading a string");
		    /* avoid warning */
		    return KINERT;
		}
	    } 
	    ks_tbadd(K, ch);
	    i++;
	}
    }
    TValue new_str = kstring_new(K, ks_tbget_buffer(K), i);
    ks_tbclear(K);
    return new_str;
}

/*
** Special constants (starting with "#")
** (Special number syntax, char constants, #ignore, #inert, srfi-38 tokens) 
*/
TValue ktok_read_special(klisp_State *K)
{
    /* discard the '#' */
    ktok_getc(K);
    
    int chi = ktok_getc(K);
    char ch = (char) chi;

    if (chi == EOF) {
	ktok_error(K, "EOF found while reading a '#' constant");
	/* avoid warning */
	return KINERT;
    }

    switch(ch) {
    case 'i': {
	/* ignore or inert */
	/* XXX: could also be an inexact number */
	ktok_read_until_delimiter(K);
	/* NOTE: can use strcmp even in the presence of '\0's */
	TValue ret_val;
	char *buf = ks_tbget_buffer(K);
	if (strcmp(buf, "gnore") == 0)
	    ret_val = KIGNORE;
	else if (strcmp(buf, "nert") == 0)
	    ret_val =  KINERT;
	else {
	    ktok_error(K, "unexpected char in # constant");
	    /* avoid warning */
	    return KINERT;
	}
	ks_tbclear(K);
	return ret_val;
    }
    case 'e': {
	/* an exact infinity */
	/* XXX: could also be an exact number */
	ktok_read_until_delimiter(K);
	TValue ret_val;
	/* NOTE: can use strcmp even in the presence of '\0's */
	char *buf = ks_tbget_buffer(K);
	if (strcmp(buf, "+infinity") == 0) {
	    ret_val = KEPINF;
	} else if (strcmp(buf, "-infinity") == 0) {
	    ret_val =  KEMINF;
	} else {
	    ktok_error(K, "unexpected char in # constant");
	    /* avoid warning */
	    return KINERT;
	}
	ks_tbclear(K);
	return ret_val;
    }
    case 't':
    case 'f':
	/* boolean constant */
	if (ktok_check_delimiter(K))
	    return b2tv(ch == 't');
	else {
	    ktok_error(K, "unexpected char in # constant");
	    /* avoid warning */
	    return KINERT;
	}
    case '\\': {
	/* char constant */
	/* 
	** RATIONALE: in the scheme spec (R5RS) it says that only alphabetic 
	** char constants need a delimiter to disambiguate the cases with 
	** character names. It would be more consistent if all characters
	** needed a delimiter (and is probably implied by the yet incomplete
	** Kernel report (R-1RK))
	** For now we follow the scheme report 
	*/
	chi = ktok_getc(K);
	ch = (char) chi;

	if (chi == EOF) {
	    ktok_error(K, "EOF found while reading a char constant");
	    /* avoid warning */
	    return KINERT;
	}

	if (!ktok_is_alphabetic(ch) || ktok_check_delimiter(K))
	    return ch2tv(ch);

	ktok_read_until_delimiter(K);
	char *p = ks_tbget_buffer(K);
	while (*p) {
	    *p = tolower(*p);
	    p++;
	}
	ch = tolower(ch);
	/* NOTE: can use strcmp even in the presence of '\0's */
	char *buf = ks_tbget_buffer(K);
	if (ch == 's' && strcmp(buf, "pace") == 0)
	    ch = ' ';
	else if (ch == 'n' && strcmp(buf, "ewline") == 0)
	    ch = ('\n');
	else {
	    ktok_error(K, "Unrecognized character name");
	    /* avoid warning */
	    return KINERT;
	}
	ks_tbclear(K);
	return ch2tv(ch);
    }
    case '0': case '1': case '2': case '3': case '4': 
    case '5': case '6': case '7': case '8': case '9': {
	/* srfi-38 type token (can be either a def or ref) */
	/* TODO: allow bigints */
	    int32_t res = 0;
	    while(ch != '#' && ch != '=') {
		if (!ktok_is_numeric(ch)) {
		    ktok_error(K, "Invalid char found in srfi-38 token");
		    /* avoid warning */
		    return KINERT;
		}

		res = res * 10 + ktok_digit_value(ch);

		chi = ktok_getc(K);
		ch = (char) chi;
	
		if (chi == EOF) {
		    ktok_error(K, "EOF found while reading a srfi-38 token");
		    /* avoid warning */
		    return KINERT;
		}
	    }
	    return kcons(K, ch2tv(ch), i2tv(res));
	}
    /* TODO: add real with no primary value and undefined */
    default:
	ktok_error(K, "unexpected char in # constant");
	/* avoid warning */
	return KINERT;
    }
}

/*
** Identifiers
*/
TValue ktok_read_identifier(klisp_State *K)
{
    while (!ktok_check_delimiter(K)) {
	/* NOTE: can't be eof, because eof is a delimiter */
	char ch = (char) ktok_getc(K);

	/* NOTE: is_subsequent of '\0' is false, so no embedded '\0' */
	if (ktok_is_subsequent(ch))
	    ks_tbadd(K, ch);
	else
	    ktok_error(K, "Invalid char in identifier");	    
    }
    ks_tbadd(K, '\0');
    TValue new_sym = ksymbol_new(K, ks_tbget_buffer(K));
    ks_tbclear(K);
    return new_sym;
}


