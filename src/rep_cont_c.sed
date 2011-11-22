# This is a collection of sed commands to refactor continuation underlying 
# functions to just take a kernel state pointer (instead of also taking extra
# params and value object.

# All these tests are run one at a time with sed -n

# detect lonely parens
# /[(] /P
# none remaining

# detect klisp_State pointer without open parens on the same line
# /[^(]klisp_State \*K, /P
# none remaining

# detect single line function definition (trailing ;)
# There are 3
#/[(]klisp_State \*K, TValue \*xparams, TValue obj[)];/P
# use the rep_cont_h.sed script to replace them
# There are 0 now

# detect single line function definition (no trailing ;)
#/[(]klisp_State \*K, TValue \*xparams, TValue obj[)]/P
# There are 48, that is one for each of the 3 we just did, 44 for
# the ones defined in .h and probably 1 with no definition

# All are single line, detect them with the opening brace
#/[(]klisp_State \*K, TValue \*xparams, TValue obj[)]/{
#N
#/[(]klisp_State \*K, TValue \*xparams, TValue obj[)].*\n[{]/P
#}

# All function definition are one line, just replace them in the .c
# This is used to modify in place with sed -i -f <this-file> *.c
# The only problem was do_ffi_callback_decode_arguments (was two lines)
/[(]klisp_State \*K, TValue \*xparams, TValue obj[)]/{
N
s/^void \(.*\)[(]klisp_State \*K, TValue \*xparams, TValue obj[)].*\n[{]/void \1(klisp_State *K)\
\{\
    TValue *xparams = K->next_xparams;\
    TValue obj = K->next_value;\
    klisp_assert(ttisnil(K->next_env));/
}

