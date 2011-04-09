/*
** ktoken.h
** Tokenizer for the Kernel Programming Language
** See Copyright Notice in klisp.h
*/

#ifndef ktoken_h
#define ktoken_h

#include "kobject.h"
#include "kstate.h"

#include <stdio.h>

/*
** Tokenizer interface
*/
void ktok_init(klisp_State *K);
TValue ktok_read_token(klisp_State *K);
void ktok_reset_source_info(klisp_State *K);
TValue ktok_get_source_info(klisp_State *K);

/* This is needed here to allow cleanup of shared dict from tokenizer */
void clear_shared_dict(klisp_State *K);

/* This is needed for string->symbol to check if a symbol has external
   representation as an identifier */
/* REFACTOR: think out a better interface to all this */

/* Each bit correspond to a char in the 0-255 range */
typedef uint32_t kcharset[8];

extern kcharset ktok_alphabetic, ktok_numeric, ktok_whitespace;
extern kcharset ktok_delimiter, ktok_extended, ktok_subsequent;

#define ktok_is_alphabetic(chi_) kcharset_contains(ktok_alphabetic, chi_)
/* TODO: add is_digit, that takes the base as parameter */
#define ktok_is_numeric(chi_) kcharset_contains(ktok_numeric, chi_)
/* TODO: add hex digits */
#define ktok_digit_value(ch_) (ch_ - '0')
#define ktok_is_whitespace(chi_) kcharset_contains(ktok_whitespace, chi_)
#define ktok_is_delimiter(chi_) ((chi_) == EOF ||			\
				 kcharset_contains(ktok_delimiter, chi_))
#define ktok_is_subsequent(chi_) kcharset_contains(ktok_subsequent, chi_)

#define kcharset_contains(kch_, ch_) \
    ({ unsigned char ch__ = (unsigned char) (ch_);	\
	kch_[KCHS_OCTANT(ch__)] & KCHS_BIT(ch__); })

/* TODO: add other bases */
#define CAN_ADD_DIGIT(res, new_digit) \
    ((res) <= (INT32_MAX - new_digit) / 10)

/*
** Char set contains macro interface
*/
#define KCHS_OCTANT(ch) ((ch) >> 5)
#define KCHS_BIT(ch) (1 << ((ch) & 0x1f))

#endif
