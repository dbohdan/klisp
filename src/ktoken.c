/*
** ktoken.c
** Tokenizer for the Kernel Programming Language
** See Copyright Notice in klisp.h
*/

/*
** TODO:
**
** - Support for complete number syntax (complex) (report)
** - Support for unicode (strings, char and symbols).
**
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>

#include "ktoken.h"
#include "kobject.h"
#include "kstate.h"
#include "kinteger.h"
#include "krational.h"
#include "kreal.h"
#include "kpair.h"
#include "kstring.h"
#include "kbytevector.h"
#include "ksymbol.h"
#include "kkeyword.h"
#include "kerror.h"
#include "kport.h"

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
kcharset ktok_delimiter, ktok_extended;
kcharset ktok_initial, ktok_subsequent;

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
** the cdr. 
** The sexp comment token with a ';' in the car.
** This way a special token can be easily tested for (with ttispair)
** and easily classified (with switch(chvalue(kcar(tok)))).
**
*/

void ktok_init(klisp_State *K)
{
    /* Character sets */
    kcharset_fill(ktok_alphabetic, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                  "abcdefghijklmnopqrstuvwxyz");
    kcharset_fill(ktok_numeric, "0123456789");
    /* keep synchronized with cases in main tokenizer switch */
    kcharset_fill(ktok_whitespace, " \t\v\r\n\f");

    kcharset_fill(ktok_delimiter, "()\";");
    kcharset_union(ktok_delimiter, ktok_whitespace);

    kcharset_fill(ktok_initial, "!$%&*./:<=>?@^_~");
    kcharset_union(ktok_initial, ktok_alphabetic);

    /* N.B. Unlike in scheme, kernel admits both '.' and 
       '@' as initial chars in identifiers, but doesn't allow
       '+' or '-'. There are 3 exceptions:
       both '+' and '-' alone are identifiers and '.' alone is
       not an identifier */
    kcharset_fill(ktok_extended, "+-");

    kcharset_empty(ktok_subsequent);
    kcharset_union(ktok_subsequent, ktok_initial);
    kcharset_union(ktok_subsequent, ktok_numeric);
    kcharset_union(ktok_subsequent, ktok_extended);
}

/*
** Error management
*/

void clear_shared_dict(klisp_State *K)
{
    K->shared_dict = KNIL;
}

#define ktok_error(K, str) ktok_error_g(K, str, false, KINERT)
#define ktok_error_extra(K, str, extra) ktok_error_g(K, str, true, extra)

void ktok_error_g(klisp_State *K, char *str, bool extra, TValue extra_value)
{
    /* all cleaning is done in throw 
       (stacks, shared_dict, rooted objs) */

    /* save the last source code info on the port */
    kport_update_source_info(K->curr_port, K->ktok_source_info.line,
                             K->ktok_source_info.col);

    /* include the source info (and extra value if present) in the error */
    TValue irritants;
    if (extra) {
        krooted_tvs_push(K, extra_value); /* will be popped by throw */
        TValue si = ktok_get_source_info(K);
        krooted_tvs_push(K, si); /* will be popped by throw */
        irritants = klist_g(K, false, 2, si, extra_value);
    } else {
        irritants = ktok_get_source_info(K);
    }
    krooted_tvs_push(K, irritants); /* will be popped by throw */
    klispE_throw_with_irritants(K, str, irritants);
}

/*
** Underlying stream interface & source code location tracking
*/

/* TODO/OPTIMIZE We should use buffering to shorten the 
   average code path to read each char */
/* this reads one character from curr_port */
int ktok_ggetc(klisp_State *K)
{
    /* XXX when full unicode is used (uint32_t) a different way should
       be use to signal EOF */
	          
    TValue port = K->curr_port;
    if (ttisfport(port)) {
        /* fport */
        FILE *file = kfport_file(port);

        /* LOCK: only a single lock should be acquired */
        klisp_unlock(K);
        int chi = getc(file);
        klisp_lock(K);

        if (chi == EOF) {
            /* NOTE: eof doesn't change source code location info */
            if (ferror(file) != 0) {
                /* clear error marker to allow retries later */
                clearerr(file);
                /* TODO put error info on the error obj */
                ktok_error(K, "reading error");
                return 0;
            } else { /* if (feof(file) != 0) */
                /* let the eof marker set */
                K->ktok_seen_eof = true;
                return EOF;
            }
        } else 
            return chi;
    } else {
        /* mport */
        if (kport_is_binary(port)) {
            /* bytevector port */
            if (kmport_off(port) >= kbytevector_size(kmport_buf(port))) {
                K->ktok_seen_eof = true;
                return EOF;
            }
            int chi = kbytevector_buf(kmport_buf(port))[kmport_off(port)];
            ++kmport_off(port);
            return chi;
        } else {
            /* string port */
            if (kmport_off(port) >= kstring_size(kmport_buf(port))) {
                K->ktok_seen_eof = true;
                return EOF;
            }
            int chi = kstring_buf(kmport_buf(port))[kmport_off(port)];
            ++kmport_off(port);
            return chi;
        }
    }
}

/* this returns one character to curr_port */
void ktok_gungetc(klisp_State *K, int chi)
{
    if (chi == EOF)
        return;

    TValue port = K->curr_port;
    if (ttisfport(port)) {
        /* fport */
        FILE *file = kfport_file(port);

        if (ungetc(chi, file) == EOF) {
            if (ferror(file) != 0) {
                /* clear error marker to allow retries later */
                clearerr(file);
            }
            /* TODO put error info on the error obj */
            ktok_error(K, "reading error");
            return;
        }
    } else {
        /* mport */
        if (kport_is_binary(port)) {
            /* bytevector port */
            --kmport_off(port);
        } else {
            /* string port */
            --kmport_off(port);
        }
    }
}

int ktok_peekc_getc(klisp_State *K, bool peekp)
{
    /* WORKAROUND: for stdin line buffering & reading of EOF, this flag
       is reset on every read */
    /* Otherwise, at least in linux, after reading or peeking an EOF from the 
       console, the next char isn't eof anymore */
    if (K->ktok_seen_eof)
        return EOF;

    int chi = ktok_ggetc(K);

    if (peekp) {
        ktok_gungetc(K, chi);
        return chi;
    }

    /* track source code location before returning the char */
    if (chi == '\t') {
        /* align column to next tab stop */
        K->ktok_source_info.col = 
            (K->ktok_source_info.col + K->ktok_source_info.tab_width) -
            (K->ktok_source_info.col % K->ktok_source_info.tab_width);
    } else if (chi == '\n') {
        K->ktok_source_info.line++;
        K->ktok_source_info.col = 0;
    } else {
        K->ktok_source_info.col++;
    }
    return chi;
}

void ktok_save_source_info(klisp_State *K)
{
    K->ktok_source_info.saved_line = K->ktok_source_info.line;
    K->ktok_source_info.saved_col = K->ktok_source_info.col;
}

TValue ktok_get_source_info(klisp_State *K)
{
    /* TEMP: for now, lines and column names are fixints */
    TValue pos = kcons(K, i2tv(K->ktok_source_info.saved_line),
                       i2tv(K->ktok_source_info.saved_col));
    krooted_tvs_push(K, pos);
    /* the filename is rooted in the port */
    TValue res = kcons(K, K->ktok_source_info.filename, pos);
    krooted_tvs_pop(K);
    return res;
}

void ktok_set_source_info(klisp_State *K, TValue filename, int32_t line,
                          int32_t col)
{
    K->ktok_source_info.filename = filename;
    K->ktok_source_info.line = line;
    K->ktok_source_info.col = col;
}


/*
** ktok_read_token() helpers
*/
void ktok_ignore_whitespace(klisp_State *K);
void ktok_ignore_single_line_comment(klisp_State *K);
void ktok_ignore_multi_line_comment(klisp_State *K);
bool ktok_check_delimiter(klisp_State *K);
char ktok_read_hex_escape(klisp_State *K);
TValue ktok_read_string(klisp_State *K);
TValue ktok_read_special(klisp_State *K);
TValue ktok_read_number(klisp_State *K, char *buf, int32_t len,
                        bool has_exactp, bool exactp, bool has_radixp, 
                        int32_t radix);
TValue ktok_read_maybe_signed_numeric(klisp_State *K);
TValue ktok_read_identifier_or_dot(klisp_State *K, bool keywordp);
TValue ktok_read_bar_identifier(klisp_State *K, bool keywordp);
int ktok_read_until_delimiter(klisp_State *K);

/*
** Main tokenizer function
*/
TValue ktok_read_token(klisp_State *K)
{
    klisp_assert(ks_tbisempty(K));

    while(true) {
        /* save the source info in case a token starts here */
        ktok_save_source_info(K);

        int chi = ktok_peekc(K);

        switch(chi) {
        case EOF:
            ktok_getc(K);
            return KEOF;
        case ' ':
        case '\n':
        case '\r':
        case '\t':
        case '\v': 
        case '\f': /* Keep synchronized with whitespace chars */
            ktok_ignore_whitespace(K);
            continue;
        case ';':
            ktok_ignore_single_line_comment(K);
            continue;
        case '(':
            ktok_getc(K);
            return G(K)->ktok_lparen;
        case ')':
            ktok_getc(K);
            return G(K)->ktok_rparen;
        case '"':
            return ktok_read_string(K);
        case '|':
            return ktok_read_bar_identifier(K, false);
/* TODO use read_until_delimiter in all these cases */
        case '#': {
            ktok_getc(K);
            chi = ktok_peekc(K);
            switch(chi) {
            case EOF:
                ktok_error(K, "# constant is too short");
                return KINERT; /* avoid warning */
            case '!': /* single line comment (alternative syntax) */
                /* this handles the #! style script header too! */
                ktok_ignore_single_line_comment(K);
                continue;
            case '|': /* nested/multiline comment */
                ktok_getc(K); /* discard the '|' */
                klisp_assert(K->ktok_nested_comments == 0);
                K->ktok_nested_comments = 1;
                ktok_ignore_multi_line_comment(K);
                continue;
            case ';': /* sexp comment */
                ktok_getc(K); /* discard the ';' */
                return G(K)->ktok_sexp_comment;
            case ':': /* keyword */
                ktok_getc(K); /* discard the ':' */
                chi = ktok_peekc(K);
                if (chi == EOF) {
                    ktok_error(K, "# constant is too short");
                    return KINERT; /* avoid warning */
                } else if (chi == '|') {
                    return ktok_read_bar_identifier(K, true);
                } else if (chi == '\\' || ktok_is_initial(chi)) {
                    return ktok_read_identifier_or_dot(K, true);
                } else if (chi == '+' || chi == '-') {
                    char ch = (char) chi;
                    ktok_getc(K); /* discard the '+' or '-' */
                    if (ktok_check_delimiter(K)) {
                        return kkeyword_new_bs(K, &ch, 1);
                    } else {
                        ktok_error_extra(K, "invalid start in keyword", 
                                         ch2tv(ch));
                        return KINERT; /* avoid warning */
                    }
                } else {
                    ktok_error_extra(K, "invalid char starting keyword",
                                     ch2tv((char) chi));
                    return KINERT; /* avoid warning */
                }
            default:
                return ktok_read_special(K);
            }
        }
        case '0': case '1': case '2': case '3': case '4': 
        case '5': case '6': case '7': case '8': case '9': {
            /* positive number, no exactness or radix indicator */
            int32_t buf_len = ktok_read_until_delimiter(K);
            char *buf = ks_tbget_buffer(K);
            /* read number should free the tbbuffer */
            return ktok_read_number(K, buf, buf_len, false, false, false, 10);
        }
        case '+': case '-':
            /* signed number, no exactness or radix indicator */
            return ktok_read_maybe_signed_numeric(K);
        case '\\': /* this is a symbol that starts with an hex escape */
            /* These should be kept synchronized with initial */
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
        case '.': /* this is either a symbol or a dot token */
            /*
            ** N.B.: the cases for '+', and '-', were already 
            ** considered 
            */
            return ktok_read_identifier_or_dot(K, false);
        default:
            chi = ktok_getc(K);
            ktok_error_extra(K, "unrecognized token starting char", 
                             ch2tv((char) chi));
            /* avoid warning */
            return KINERT;
        }
    }
}

/*
** Comments and Whitespace
*/
void ktok_ignore_single_line_comment(klisp_State *K)
{
    int chi;
    do {
        chi = ktok_getc(K);
    } while (chi != EOF && chi != '\n');
}

void ktok_ignore_multi_line_comment(klisp_State *K)
{
    /* the first "#|' was already read */
    klisp_assert(K->ktok_nested_comments == 1);
    int chi;
    TValue last_nested_comment_si = ktok_get_source_info(K);
    krooted_vars_push(K, &last_nested_comment_si);
    ks_spush(K, KNIL);

    while(K->ktok_nested_comments > 0) {
        chi = ktok_peekc(K);
        while (chi != EOF && chi != '|' && chi != '#') {
            UNUSED(ktok_getc(K));
            chi = ktok_peekc(K);
        }
        if (chi == EOF)
            goto eof_error;

        char first_char = (char) chi;

        /* this first char will actually be the same just peeked, that's no
           problem, it will save the source info the first time around the 
           loop */
        chi = ktok_peekc(K);
        while (chi != EOF && chi == first_char) {
            ktok_save_source_info(K);
            UNUSED(ktok_getc(K));
            chi = ktok_peekc(K);
        }
        if (chi == EOF)
            goto eof_error;

        UNUSED(ktok_getc(K));

        if (chi == '#') {
            /* close comment (first char was '|', so the seq is "|#") */
            --K->ktok_nested_comments;
            last_nested_comment_si = ks_spop(K);
        } else if (chi == '|') {
            /* open comment (first char was '#', so the seq is "#|") */
            klisp_assert(K->ktok_nested_comments < 1000);
            ++K->ktok_nested_comments;
            ks_spush(K, last_nested_comment_si);
            last_nested_comment_si = ktok_get_source_info(K);
        } 
        /* else lone '#' or '|', just continue */
    }
    krooted_vars_pop(K);
    return;
eof_error:
    K->ktok_nested_comments = 0;
    ktok_save_source_info(K);
    UNUSED(ktok_getc(K));
    krooted_vars_pop(K);
    ktok_error_extra(K, "unterminated multi line comment", last_nested_comment_si);
}

void ktok_ignore_whitespace(klisp_State *K)
{
    /* NOTE: if it's not whitespace do nothing (even on eof) */
    while(true) {
        int chi = ktok_peekc(K);

        if (chi == EOF) {
            return;
        } else {
            char ch = (char) chi;
            if (ktok_is_whitespace(ch)) {
                ktok_getc(K);
            } else {
                return;
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
** TEMP: for now, only integers & rationals
** The digits are in buf, that must be freed after use,
** len should be at least one 
*/
TValue ktok_read_number(klisp_State *K, char *buf, int32_t len, 
                        bool has_exactp, bool exactp, bool has_radixp, 
                        int32_t radix)
{
    UNUSED(len); /* not needed really, buf ends with '\0' */
    TValue n;
    if (radix == 10) {
        /* only allow decimals with radix 10 */
        bool decimalp = false;
        if (!krational_read_decimal(K, buf, radix, &n, NULL, &decimalp)) {
            /* TODO throw meaningful error msgs, use last param */
            ktok_error(K, "Bad format in number");
            return KINERT;
        }
        if (decimalp && !has_exactp) {
            /* handle decimal format as an explicit #i */
            has_exactp = true;
            exactp = false;
        }
    } else {
        if (!krational_read(K, buf, radix, &n, NULL)) {
            /* TODO throw meaningful error msgs, use last param */
            ktok_error(K, "Bad format in number");
            return KINERT;
        }
    }
    ks_tbclear(K);
    
    if (has_exactp && !exactp) {
        krooted_tvs_push(K, n);
        n = kexact_to_inexact(K, n);
        krooted_tvs_pop(K);
    }
    return n;
}

TValue ktok_read_maybe_signed_numeric(klisp_State *K)
{
    /* NOTE: can't be eof, it's either '+' or '-' */
    char ch = (char) ktok_getc(K);
    if (ktok_check_delimiter(K)) {
        ks_tbadd(K, ch);
        ks_tbadd(K, '\0');
        /* save the source info in the symbol */
        TValue si = ktok_get_source_info(K);
        krooted_tvs_push(K, si); /* will be popped by throw */
        TValue new_sym = ksymbol_new_bs(K, ks_tbget_buffer(K), 1, si);
        krooted_tvs_pop(K); /* already in symbol */
        krooted_tvs_push(K, new_sym);
        ks_tbclear(K); /* this shouldn't cause gc, but just in case */
        krooted_tvs_pop(K);
        return new_sym;
    } else {
        ks_tbadd(K, ch);
        int32_t buf_len = ktok_read_until_delimiter(K)+1;
        char *buf = ks_tbget_buffer(K);
        /* no exactness or radix prefix, default radix: 10 */
        return ktok_read_number(K, buf, buf_len, false, false, false, 10);
    }
}

/*
** Hex escapes for strings and symbols
** "#\xXXXXXX;"
** "#\x" already read
*/
char ktok_read_hex_escape(klisp_State *K)
{
    /* enough space for any unicode char + 2 */
    int ch;
    char buf[10];
    int c = 0;
    bool at_least_onep = false;
    for(ch = ktok_getc(K); ch != EOF && ch != ';'; 
        ch = ktok_getc(K)) {
        if (!ktok_is_digit(ch, 16)) {
            ktok_error_extra(K, "Invalid char found in hex escape", 
                             ch2tv(ch));
            return '\0'; /* avoid warning */
        } 
        /* 
        ** This will allow one space for '\0' and one extra
        ** char in case the value is too big, and so will 
        ** naturally result in a value outside the unicode
        ** range without the need to record any extra 
        ** characters other than the first 8 (without 
        ** leading zeroes).
        */
        at_least_onep = true;
        if (c < sizeof(buf) - 1 && (c > 0 || ch != '0')) 
            buf[c++] = ch;
    }
    if (ch == EOF) {
        ktok_error(K, "EOF found while reading hex escape");
        return '\0'; /* avoid warning */
    } else if (!at_least_onep) {
        ktok_error(K, "Empty hex escape found");
        return '\0'; /* avoid warning */
    } else if (c == 0) { /* this is the case of a NULL char */
        buf[c++] = '0'; 
    }
    buf[c++] = '\0';
    /* buf now contains the hex value of the char */
    TValue n;
    int res = kinteger_read(K, buf, 16, &n, NULL);
    /* can't fail, all digits were checked already */
    klisp_assert(res == true);
    if (!ttisfixint(n) || ivalue(n) > 127) {
        krooted_tvs_push(K, n);
        ktok_error_extra(K, "hex escaped char out of ASCII range", n);
        return '\0'; /* avoid warning */
    }
    /* all ok, we pass the char */
    return (char) ivalue(n);
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
        int ch = ktok_getc(K);
    just_read: /* this comes from escaped newline */
        if (ch == EOF) {
            ktok_error(K, "EOF found while reading a string");
            return KINERT; /* avoid warning */
        } else 	if (ch < 0 || ch > 127) {
            ktok_error(K, "Non ASCII char found while reading a string");
            return KINERT; /* avoid warning */
        }


        if (ch == '"') {
            ks_tbadd(K, '\0');
            done = true;
        } else if (ch == '\\') {
            ch = ktok_getc(K);
	
            if (ch == EOF) {
                ktok_error(K, "EOF found while reading a string");
                return KINERT; /* avoid warning */
            }

            switch(ch) {
                /* These two will self insert */
            case '"':
            case '\\':
                break;
                /* These are naming chars (like in c, mostly) */
            case '0':
                ch = '\0';
                break;
            case 'a':
                ch = '\a';
                break;
            case 'b':
                ch = '\b';
                break;
            case 't':
                ch = '\t';
                break;
            case 'n':
                ch = '\n';
                break;
            case 'r':
                ch = '\r';
                break;
            case 'v':
                ch = '\v';
                break;
            case 'f':
                ch = '\f';
                break;
                /* 
                ** These signal an escaped newline (not included in string)
                */
            case ' ':
            case '\t':
                /* eat up all intraline spacing */
                while((ch = ktok_getc(K)) != EOF &&
                      (ch == ' ' || ch == '\t'))
                    ;
                if (ch == EOF) {
                    ktok_error(K, "EOF found while reading a string");
                    return KINERT; /* avoid warning */
                } else if (ch != '\n' && ch != '\r') {
                    ktok_error(K, "Invalid char found after \\ while "
                               "reading a string");
                    return KINERT; /* avoid warning */
                }
                /* fall through */
            case '\n': 
            case '\r':
                /* use the r6rs definition for line end */
                if (ch == 'r') {
                    ch = ktok_peekc(K);
                    if (ch != EOF && ch == '\n')
                        ktok_getc(K);
                }
                /* eat up all intraline spacing */
                while((ch = ktok_getc(K)) != EOF &&
                      (ch == ' ' || ch == '\t'))
                    ;
                /* this will check for EOF and continue reading the 
                   string at the top of the loop */
                goto just_read;
                /* This is an hex escaped char */
            case 'x': 
                ch = ktok_read_hex_escape(K);
                break;
            default:
                ktok_error_extra(K, "Invalid char after '\\' " 
                                 "while reading a string", ch2tv(ch));
                return KINERT; /* avoid warning */
            }
            ks_tbadd(K, ch);
            ++i;
        } else { 
            ks_tbadd(K, ch);
            ++i;
        }
    }
    /* TEMP: for now strings "read" are mutable but strings "loaded" are
       not */
    TValue new_str = kstring_new_bs_g(K, K->read_mconsp, 
                                      ks_tbget_buffer(K), i); 
    krooted_tvs_push(K, new_str);
    ks_tbclear(K); /* shouldn't cause gc, but still */
    krooted_tvs_pop(K);
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
                        { "#i+infinity", KIPINF_ },
                        { "#i-infinity", KIMINF_ },
                        { "#real", KRWNPV_ },
                        { "#undefined", KUNDEF_ },
                        /* 
                        ** Character names 
                        ** (r7rs + vtab from r6rs) 
                        */
                        { "#\\null", KNULL_ },
                        { "#\\alarm", KALARM_ },
                        { "#\\backspace", KBACKSPACE_ },
                        { "#\\tab", KTAB_ },
                        { "#\\newline", KNEWLINE_ }, /* kernel */
                        { "#\\return", KRETURN_ },
                        { "#\\escape", KESCAPE_ },
                        { "#\\space", KSPACE_ }, /* kernel */
                        { "#\\delete", KDELETE_ },
                        { "#\\vtab", KVTAB_ }, /* r6rs, only */
                        { "#\\formfeed", KFORMFEED_ } /* r6rs in strings */
}; 

#define MAX_EXT_REP_SIZE 64  /* all special tokens have less than 64 chars */

TValue ktok_read_special(klisp_State *K)
{
    /* the # is already consumed, add it manually */
    ks_tbadd(K, '#');
    int32_t buf_len = ktok_read_until_delimiter(K) + 1;
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
       that is case dependant, so after this we downcase buf
       (except that an escaped char needs a small 'x' */
    /* REFACTOR: move this to a new function */
    /* char constant, needs at least 3 chars unless it's a delimiter
     * char! */
    if (buf_len == 2 && buf[1] == '\\') {
        /* was a delimiter char... read it */
        int ch_i = ktok_getc(K);
        if (ch_i == EOF) {
            ktok_error(K, "EOF found while reading character name");
            return KINERT; /* avoid warning */
        }
        ks_tbclear(K);
        return ch2tv((char)ch_i);
    } else if (buf[1] == '\\') {
        /* 
        ** RATIONALE: in the scheme spec (R5RS) it says that only alphabetic 
        ** char constants need a delimiter to disambiguate the cases with 
        ** character names. It would be more consistent if all characters
        ** needed a delimiter (and is probably implied by the yet incomplete
        ** Kernel report (R-1RK))
        ** For now we follow the scheme report 
        */
        char ch = buf[2]; /* we know buf_len > 2 */

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

    /* first save the third char, in case it's an hex escaped char
       (that should be a lowercase x)  */
    char saved_third = buf[2]; /* there's at least 2 chars, so in the worst
                                  case buf[2] is just '\0' */

    /* now, we ignore case in all remaining comparisons */
    size_t i = 0;
    for(char *str2 = buf; i < buf_len; ++str2, ++i)
        *str2 = tolower(*str2);

    /* REFACTOR: move this to a new function */
    /* then check the known constants (including named characters) */
    size_t stok_size = sizeof(kspecial_tokens) / 
        sizeof(struct kspecial_token);
    for (i = 0; i < stok_size; i++) {
        struct kspecial_token token = kspecial_tokens[i];
        /* NOTE: must check type because buf may contain embedded '\0's */
        if (buf_len == strlen(token.ext_rep) &&
            strcmp(token.ext_rep, buf) == 0) {
            ks_tbclear(K);
            return token.obj; 
        }
    }

    /* It wasn't a special token or named char, but it can still be a srfi-38
       token or a character escape */

    if (buf[1] == '\\') { /* this is to have a meaningful error msg */
        if (saved_third != 'x') { /* case is significant here, so
                                     we use the saved char */
            ktok_error(K, "Unrecognized character name");
            return KINERT;
        }
        /* We already checked that length != 3 (x is alphabetic), 
           so there's at least on more char */
        TValue n;
        char *end;

        /* test for - and + explicitly, becayse kinteger read would parse them
           without complaining (it will also parse spaces, but we read until 
           delimiter so... */
        if (buf[3] == '-' || buf[3] == '+' ||
            !kinteger_read(K, buf+3, 16, &n, &end) || 
            end - buf != buf_len) {
            ktok_error(K, "Bad char in hex escaped character constant");
            return KINERT;
        } else if (!ttisfixint(n) || ivalue(n) > 127) {
            ktok_error(K, "Non ASCII char found in hex escaped character constant");
            /* avoid warning */
            return KINERT;
        } else {
            /* all ok, we just clean up and return the char */
            ks_tbclear(K);
            return ch2tv(ivalue(n));
        }
    }

    /* REFACTOR: move this to a new function */
    /* It was not a special token so it must be either a srfi-38 style
       token, or a number. srfi-38 tokens are a '#' a 
       decimal number and end with a '=' or a '#' */
    if (buf_len > 2 && ktok_is_numeric(buf[1])) {
        /* NOTE: it's important to check is_numeric to avoid problems with 
           sign in kinteger_read */
        /* srfi-38 type token (can be either a def or ref) */
        /* TODO: lift this implementation restriction */
        /* IMPLEMENTATION RESTRICTION: only allow fixints in shared tokens */
        char ch = buf[buf_len-1]; /* remember last char */
        buf[buf_len-1] = '\0'; /* replace last char with 0 to read number */

        if (ch != '#' && ch != '=') {
            ktok_error(K, "Missing last char in srfi-38 token");
            return KINERT;
        } /* else buf[i] == '#' or '=' */
        TValue n;
        char *end;
        /* 10 is the radix for srfi-38 tokens, buf+1 to jump over the '#',
           end+1 to count the last char */
        /* N.B. buf+1 can't be + or -, we already tested numeric before */
        if (!kinteger_read(K, buf+1, 10, &n, &end) || end+1 - buf != buf_len) {
            ktok_error(K, "Bad char in srfi-38 token");
            return KINERT;
        } else if (!ttisfixint(n)) {
            ktok_error(K, "IMP. RESTRICTION: shared token too big");
            /* avoid warning */
            return KINERT;
        }
        ks_tbclear(K);
        /* GC: no need to root n, for now it's a fixint */
        return kcons(K, ch2tv(ch), n);
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
            ktok_error(K, "unknown # constant or "
                       "unexpected char in number after #");
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
            if (idx == buf_len) {
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
** Identifiers & Keywords (and dot token)
*/
TValue ktok_read_identifier_or_dot(klisp_State *K, bool keywordp)
{
    bool seen_dot = false;
    int32_t i = 0;
    while (!ktok_check_delimiter(K)) {
        /* NOTE: can't be eof, because eof is a delimiter */
        char ch = (char) ktok_getc(K);
        /* this is needed to differentiate a dot from an equivalent escape */
        seen_dot |= ch == '.';
        /* NOTE: is_subsequent of '\0' is false, so no embedded '\0' */
        if (ktok_is_subsequent(ch)) {
            /* downcase all non-escaped chars */
            ks_tbadd(K, tolower(ch));
            ++i;
        } else if (ch == '\\') {
            /* should be inline hex escape */
            ch = ktok_getc(K);
            if (ch == EOF) {
                ktok_error(K, "EOF found while reading character escape");
            } else if (ch != 'x') {
                ktok_error_extra(K, keywordp? 
                                 "Invalid char after \\ in keyword" :
                                 "Invalid char after \\ in identifier", 
                                 ch2tv((char)ch));
            }
            ch = ktok_read_hex_escape(K);
            /* don't downcase escaped chars */
            ks_tbadd(K, ch);
            ++i;
        } else {
            ktok_error_extra(K, keywordp? "Invalid char in keyword" :
                             "Invalid char in identifier", ch2tv((char)ch));
        }
    }

    if (i == 1 && seen_dot) {
        if (keywordp) {
            ktok_error(K, "Invalid syntax in keyword");
            return KINERT; /* avoid warning */
        } else {
            ks_tbclear(K);
            return G(K)->ktok_dot;
        }
    }

    ks_tbadd(K, '\0');
    TValue new_obj;
    if (keywordp) {
        new_obj = kkeyword_new_bs(K, ks_tbget_buffer(K), i);
    } else {
        TValue si = ktok_get_source_info(K);
        krooted_tvs_push(K, si); /* will be popped by throw */
        new_obj = ksymbol_new_bs(K, ks_tbget_buffer(K), i, si);
        krooted_tvs_pop(K); /* already in symbol */
    }
    krooted_tvs_push(K, new_obj);
    ks_tbclear(K); /* this shouldn't cause gc, but just in case */
    krooted_tvs_pop(K);
    return new_obj;
}

TValue ktok_read_bar_identifier(klisp_State *K, bool keywordp)
{
    /* discard opening bar */
    ktok_getc(K);

    bool done = false;
    int i = 0;

    /* Never downcase chars in |...| escaped symbols */
    while(!done) {
        int ch = ktok_getc(K);
        if (ch == EOF) {
            ktok_error(K, keywordp? 
                       "EOF found while reading a #:|keyword|" :
                       "EOF found while reading an |identifier|");
            return KINERT; /* avoid warning */
        } else 	if (ch < 0 || ch > 127) {
            ktok_error(K, keywordp? 
                       "Non ASCII char found while reading a #:|keyword|" :
                       "Non ASCII char found while reading an |identifier|");
            return KINERT; /* avoid warning */
        }

        if (ch == '|') {
            ks_tbadd(K, '\0');
            done = true;
        } else if (ch == '\\') {
            ch = ktok_getc(K);
	
            if (ch == EOF) {
                ktok_error(K, keywordp? 
                           "EOF found while reading a #:|keyword|" :
                           "EOF found while reading an |identifier|");
                return KINERT; /* avoid warning */
            }

            switch(ch) {
                /* These two will self insert */
            case '|':
            case '\\':
                break;
            case 'x':
                ch = ktok_read_hex_escape(K);
                break;
            default:
                ktok_error_extra(K,  keywordp? 
                                 "Invalid char after '\\' while reading a "
                                 "#:|keyword|" :
                                 "Invalid char after '\\' while reading an "
                                 "|identifier|", ch2tv(ch));
                return KINERT; /* avoid warning */
            }
            ks_tbadd(K, ch);
            ++i;
        } else { 
            ks_tbadd(K, ch);
            ++i;
        }
    }
    TValue new_obj;
    if (keywordp) {
        new_obj = kkeyword_new_bs(K, ks_tbget_buffer(K), i);
    } else {
        TValue si = ktok_get_source_info(K);
        krooted_tvs_push(K, si); /* will be popped by throw */
        new_obj = ksymbol_new_bs(K, ks_tbget_buffer(K), i, si);
        krooted_tvs_pop(K); /* already in symbol */
    }
    krooted_tvs_push(K, new_obj);
    ks_tbclear(K); /* this shouldn't cause gc, but just in case */
    krooted_tvs_pop(K);
    return new_obj;
}

