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

/*
** Char set contains macro interface
*/
#define KCHS_OCTANT(ch) ((ch) >> 5)
#define KCHS_BIT(ch) (1 << ((ch) & 0x1f))

/* Each bit correspond to a char in the 0-255 range */
typedef uint32_t kcharset[8];

extern kcharset ktok_alphabetic, ktok_numeric, ktok_whitespace;
extern kcharset ktok_delimiter, ktok_extended, ktok_subsequent;

#define ktok_is_alphabetic(chi_) kcharset_contains(ktok_alphabetic, chi_)
#define ktok_is_numeric(chi_) kcharset_contains(ktok_numeric, chi_)

#define ktok_is_whitespace(chi_) kcharset_contains(ktok_whitespace, chi_)
#define ktok_is_delimiter(chi_) ((chi_) == EOF ||			\
				 kcharset_contains(ktok_delimiter, chi_))
#define ktok_is_subsequent(chi_) kcharset_contains(ktok_subsequent, chi_)

#define kcharset_contains(kch_, ch_) \
    ({ unsigned char ch__ = (unsigned char) (ch_);	\
	kch_[KCHS_OCTANT(ch__)] & KCHS_BIT(ch__); })


/* NOTE: only lowercase chars for hexa */
inline bool ktok_is_digit(char ch, int32_t radix)
{
    return (ktok_is_numeric(ch) && (ch - '0') < radix) ||
	(ktok_is_alphabetic(ch) && (10 + (ch - 'a')) < radix);
}

inline int32_t ktok_digit_value(char ch)
{
    return (ch <= '9')? ch - '0' : 10 + (ch - 'a');
}

/* This takes the args in sign magnitude form (sign & res),
   but must work for any representation of negative numbers */
inline bool can_add_digit(uint32_t res, bool sign, uint32_t new_digit, 
			  int32_t radix)
{
    return (sign)? res <= -(INT32_MIN + new_digit) / radix :
	res <= (INT32_MAX - new_digit) / radix;
}

#endif
