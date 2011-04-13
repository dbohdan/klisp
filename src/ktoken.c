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
** - Support other number types besides integers and exact infinities
** - Support for complete number syntax (inexacts, rationals, reals, complex)
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
#include "kinteger.h"
#include "kpair.h"
#include "kstring.h"
#include "ksymbol.h"
#include "kerror.h"

/*
** Char sets for fast ASCII char classification
*/

/*
** Char set function/macro interface
*/
void kcharset_empty(kcharset);
void kcharset_fill(kcharset, char *);
void kcharset_union(kcharset, kcharset);
/* contains in .h */
    
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
    /* Is this really necessary?? double check */
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
    /* Is this really necessary?? double check */
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
TValue ktok_read_number(klisp_State *K, char *buf, int32_t len,
			bool has_exactp, bool exactp, bool has_radixp, 
			int32_t radix);
TValue ktok_read_maybe_signed_numeric(klisp_State *K);
TValue ktok_read_identifier(klisp_State *K);
int ktok_read_until_delimiter(klisp_State *K);

/*
** Main tokenizer function
*/
TValue ktok_read_token(klisp_State *K)
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
/* TODO use read_until_delimiter in all these cases */
    case '#':
	return ktok_read_special(K);
    case '0': case '1': case '2': case '3': case '4': 
    case '5': case '6': case '7': case '8': case '9': {
        /* positive number, no exactness or radix indicator */
	int32_t buf_len = ktok_read_until_delimiter(K);
	char *buf = ks_tbget_buffer(K);
	return ktok_read_number(K, buf, buf_len, false, false, false, 10);
	}
    case '+': case '-':
        /* signed number, no exactness or radix indicator */
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
int32_t ktok_read_until_delimiter(klisp_State *K)
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
** TEMP: for now, only integers, ignore exactness
** The digits are in buf, that must be freed after use,
** len should be at least one 
*/
TValue ktok_read_number(klisp_State *K, char *buf, int32_t len,
			bool has_exactp, bool exactp, bool has_radixp, 
			int32_t radix)
{
    /* TODO use IMATH library to do this */
    uint32_t fixint_res = 0;
    bool is_fixint = true;
    TValue bigint_res;

    int32_t i = 0;
    bool is_pos = true;
    /* first check the sign */
    if (buf[i] == '+' || buf[i] == '-') {
	is_pos = (buf[i] == '+');
	++i;
	if (i == len) {
	    ktok_error(K, "No digit found in number");
	    /* avoid warning */
	    return KINERT;
	}
    }

    while(i < len) {
	char ch = buf[i++];

	if (!ktok_is_digit(ch, radix)) {
	    /* TODO show the char */
	    if (ktok_is_digit(ch, 16)) {
		ktok_error(K, "Invalid digit in this radix");
		return KINERT;
	    } else {
		ktok_error(K, "Invalid char found in number");
		return KINERT;
	    }
	}
	int32_t new_digit = ktok_digit_value(ch);

	if (is_fixint && can_add_digit(fixint_res, !is_pos, new_digit,
		radix)) {
	    fixint_res = fixint_res * radix + new_digit;
	} else {
	    if (is_fixint) {
		/* up to the last loop was fixint, but can't be anymore.
		 Create a bigint and mutate to add the new digits. This
		 avoids unnecessary consing and discarding values that would
		 occur if it used the regular bigint+ and bigint* */
		is_fixint = false;
		bigint_res = kbigint_new(K, false, fixint_res);
		/* GC: root bigint_res */
	    }
	    kbigint_add_digit(K, bigint_res, radix, new_digit);
	}
    }

    ks_tbclear(K);

    if (is_fixint) {
	int32_t fixint = (is_pos)? (int32_t) fixint_res : 
	    (int32_t) -((int64_t) fixint_res);
	return i2tv(fixint);
    } else {
	if (!is_pos)
	    kbigint_invert_sign(K, bigint_res);
	return bigint_res;
    }
}

TValue ktok_read_maybe_signed_numeric(klisp_State *K)
{
    /* NOTE: can't be eof, it's either '+' or '-' */
    char ch = (char) ktok_getc(K);
    if (ktok_check_delimiter(K)) {
	ks_tbadd(K, ch);
	ks_tbadd(K, '\0');
	TValue new_sym = ksymbol_new_i(K, ks_tbget_buffer(K), 1);
	ks_tbclear(K);
	return new_sym;
    } else {
	ks_tbadd(K, ch);
	int32_t buf_len = ktok_read_until_delimiter(K);
	char *buf = ks_tbget_buffer(K);
	/* no exactness or radix prefix, default radix: 10 */
	return ktok_read_number(K, buf, buf_len, false, false, false, 10);
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
	if (ch < 0 || ch > 127) {
	    ktok_error(K, "Non ASCII char found while reading a string");
	    /* avoid warning */
	    return KINERT;
	} else if (ch == '"') {
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

/* this include the named chars as a subcase */
struct kspecial_token {
    const char *ext_rep; /* downcase external representation */
    TValue obj;
} kspecial_tokens[] = { { "#t", KTRUE_ },
			{ "#f", KFALSE_ },
			{ "#ignore", KIGNORE_ },
			{ "#inert", KINERT_ },
			{ "#e+infinity", KEPINF_ },
			{ "#e-infinity", KEMINF_ },
			/* TODO add undefined, real with on primary value,
			   and inexact infinities */
			{ "#\\space", KSPACE_ },
			{ "#\\newline", KNEWLINE_ }
 }; 

#define MAX_EXT_REP_SIZE 64  /* all special tokens have much than 64 chars */

TValue ktok_read_special(klisp_State *K)
{
    /* the # is still pending (was only peeked) */
    int32_t buf_len = ktok_read_until_delimiter(K);
    char *buf = ks_tbget_buffer(K);

    if (buf_len < 2) {
	/* we need at least one char in addition to the '#' */
	ktok_error(K, "# constant is too short");
	/* avoid warning */
	return KINERT;
    }

    /* first check that is not an output only representation, 
       they begin with '#[' and end with ']', but we know
       that buf[0] == '#' */
    if (buf_len > 2 && buf[1] == '[' && buf[buf_len-1] == ']') {
	ktok_error(K, "output only representation found");
	/* avoid warning */
	return KINERT;
    }

    /* Then check for simple chars, this is the only thing
       that is case dependant, so after this we downcase buf */
    /* REFACTOR: move this to a new function */
    /* char constant, needs at least 3 chars  */
    if (buf_len > 2 && buf[1] == '\\') {
	/* 
	** RATIONALE: in the scheme spec (R5RS) it says that only alphabetic 
	** char constants need a delimiter to disambiguate the cases with 
	** character names. It would be more consistent if all characters
	** needed a delimiter (and is probably implied by the yet incomplete
	** Kernel report (R-1RK))
	** For now we follow the scheme report 
	*/

	char ch = buf[2];

	if (ch < 0 || ch > 127) {
	    ktok_error(K, "Non ASCII char found as character constant");
	    /* avoid warning */
	    return KINERT;
	} 

	if (!ktok_is_alphabetic(ch) || buf_len == 3) { /* simple char */
	    ks_tbclear(K);
	    return ch2tv(ch);
	}

	/* char names are a subcase of special tokens so this case
	   will be handled later */
	/* fall through */
    }

    /* we ignore case in all remaining comparisons */
    for(char *str2 = buf; *str2 != '\0'; str2++)
	*str2 = tolower(*str2);

    /* REFACTOR: move this to a new function */
    /* then check the known constants */
    size_t stok_size = sizeof(kspecial_tokens) / 
	sizeof(struct kspecial_token);
    size_t i;
    for (i = 0; i < stok_size; i++) {
	struct kspecial_token token = kspecial_tokens[i];
	/* NOTE: must check type because buf may contain embedded '\0's */
	if (buf_len == strlen(token.ext_rep) &&
	       strcmp(token.ext_rep, buf) == 0) {
	    ks_tbclear(K);
	    return token.obj; 
	}
    }

    if (buf[1] == '\\') { /* this is to have a meaningful error msg */
	ktok_error(K, "Unrecognized character name");
	/* avoid warning */
	return KINERT;
    }

    /* REFACTOR: move this to a new function */
    /* It was not a special token so it must be either a srfi-38 style
       token, or a char constant or a number. srfi-38 tokens are a '#' a 
       decimal number and end with a '=' or a '#' */
    if (buf_len > 2 && ktok_is_numeric(buf[1])) {
	/* srfi-38 type token (can be either a def or ref) */
	/* IMPLEMENTATION RESTRICTION: only allow fixints in shared tokens */
	int32_t res = 0;
	int32_t i = 1;
	char ch = buf[i];
	while(i < buf_len && ch != '#' && ch != '=') {
	    if (!ktok_is_numeric(ch)) {
		ktok_error(K, "Invalid char found in srfi-38 token");
		/* avoid warning */
		return KINERT;
	    }

	    int new_digit = ktok_digit_value(ch);
	    if (can_add_digit(res, false, new_digit, 10)) {
		res = res * 10 + new_digit;
	    } else {
		ktok_error(K, "IMP. RESTRICTION: shared token too big");
		/* avoid warning */
		return KINERT;
	    }
	    ch = buf[i++];
	}
	if (i == buf_len) {
	    ktok_error(K, "Missing last char in srfi-38 token");
	    return KINERT;
	} /* else buf[i] == '#' or '=' */
	ks_tbclear(K);
	return kcons(K, ch2tv(ch), i2tv(res));
    }
    
    /* REFACTOR: move to new function */

    /* the only possibility left is that it is a number with
       an exactness or radix refix */
    bool has_exactp = false;
    bool exactp = false;  /* the default exactness will depend on the format */
    bool has_radixp = false;
    int32_t radix = 10;
	
    int32_t idx = 1;
    while (idx < buf_len) {
	char ch = buf[idx];
	switch(ch) {
	case 'i':
	case 'e':
	    if (has_exactp) {
		ktok_error(K, "two exactness prefixes in number");
		return KINERT;
	    }
	    has_exactp = true;
	    exactp = (ch == 'e');
	    break;
	case 'b': radix = 2; goto RADIX;
	case 'o': radix = 8; goto RADIX;
	case 'd': radix = 10; goto RADIX;
	case 'x': radix = 16; goto RADIX;
	RADIX: 
	    if (has_radixp) {
		ktok_error(K, "two radix prefixes in number");
		return KINERT;
	    }
	    has_radixp = true;
	    break;
	default:
	    ktok_error(K, "unexpected char in number after #");
	    /* avoid warning */
	    return KINERT;
	}
	++idx;
	if (idx == buf_len)
	    break;
	ch = buf[idx];

	switch(ch) {
	case '#': {
	    ++idx; /* get next exacness or radix prefix */
	    break;
	}
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9': 
	case 'a': case 'b': case 'c': case 'd': case 'e':
	case 'f': case '+': case '-': { /* read the number */
		if (has_exactp && !exactp) {
		    ktok_error(K, "inexact numbers not supported");
		    return KINERT;
		} else if (idx == buf_len) {
		    ktok_error(K, "no digits found in number");
		} else {
		    return ktok_read_number(K, buf+idx, buf_len - idx,
					    has_exactp, exactp, 
					    has_radixp, radix);
		}
	}
	default:
	    ktok_error(K, "unexpected char in number");
	    /* avoid warning */
	    return KINERT;
	}
    }
    /* this means that the number wasn't found after the prefixes */
    ktok_error(K, "no digits found in number");
    /* avoid warning */
    return KINERT;
}

/*
** Identifiers
*/
TValue ktok_read_identifier(klisp_State *K)
{
    int32_t i = 1;
    while (!ktok_check_delimiter(K)) {
	/* NOTE: can't be eof, because eof is a delimiter */
	char ch = (char) ktok_getc(K);

	/* NOTE: is_subsequent of '\0' is false, so no embedded '\0' */
	if (ktok_is_subsequent(ch)) {
	    ks_tbadd(K, ch);
	    i++;
	} else
	    ktok_error(K, "Invalid char in identifier");	    
    }
    ks_tbadd(K, '\0');
    TValue new_sym = ksymbol_new_i(K, ks_tbget_buffer(K), i-1);
    ks_tbclear(K);
    return new_sym;
}


