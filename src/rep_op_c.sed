# This is a collection of sed commands to refactor operatives underlying 
# functions to just take a kernel state pointer (instead of also taking extra
# params, ptree and denv).

# All these tests are run one at a time with sed -n

# This is a collection of sed commands to refactor operatives underlying 
# functions to just take a kernel state pointer (instead of also taking extra
# params, ptree and denv).

# All these tests are run one at a time with sed -n

# detect single line function definition
# There are 0
#/^void \(.*\)[(]klisp_State \*K, TValue \*xparams, TValue ptree, TValue denv[)];/P

# All the single line definitions done

# try to detect multi line function definition
# There are 1, do_int_repl_error
#/^void \(.*\)[(]klisp_State \*K,/{
#N
#/^void \(.*\)[(]klisp_State \*K,[[:space:]]*TValue \*xparams,[[:space:]]*TValue ptree,[[:space:]]*TValue denv);/P
#}

# replace it
#/^void \(.*\)[(]klisp_State \*K,/{
#N
#s/^void \(.*\)[(]klisp_State \*K,[[:space:]]*TValue \*xparams,[[:space:]]*TValue ptree,[[:space:]]*#TValue denv);/void \1(klisp_State *K);/
#}

# done!

# Detect all with simple brace
# There are 101
#/^void \(.*\)[(]klisp_State \*K, TValue \*xparams, TValue ptree, TValue denv[)]/{
#N
#/^void \(.*\)[(]klisp_State \*K, TValue \*xparams, TValue ptree, TValue denv[)].*\n[{]/P
#}

# replace them
# This is used to modify in place with sed -i -f <this-file> *.c
#/^void \(.*\)[(]klisp_State \*K, TValue \*xparams, TValue ptree, TValue denv[)]/{
#N
#s/^void \(.*\)[(]klisp_State \*K, TValue \*xparams, TValue ptree, TValue denv[)].*\n[{]/void \1(klisp_State *K)\
#\{\
#    TValue *xparams = K->next_xparams;\
#    TValue ptree = K->next_value;\
#    TValue denv = K->next_env;\
#    klisp_assert(ttisenvironment(K->next_env));/
#}

# Detect the ones in two lines (with braces)
# There are 84
#/^void \(.*\)[(]klisp_State \*K,/{
#N
#N
#/^void \(.*\)[(]klisp_State \*K,[[:space:]]*TValue \*xparams,[[:space:]]*TValue ptree,[[:space:]]*TValue denv[)][[:space:]]*[{]/P
#}

# replace them
# This is used to modify in place with sed -i -f <this-file> *.c
/^void \(.*\)[(]klisp_State \*K,/{
N
N
s/^void \(.*\)[(]klisp_State \*K,[[:space:]]*TValue \*xparams,[[:space:]]*TValue ptree,[[:space:]]*TValue denv[)][[:space:]]*[{]/void \1(klisp_State *K)\
\{\
    TValue *xparams = K->next_xparams;\
    TValue ptree = K->next_value;\
    TValue denv = K->next_env;\
    klisp_assert(ttisenvironment(K->next_env));/
}

# keval_ofn was changed manually because the name of denv was env
# (denv was reserved for the den param in ptree)
# do_vau was changed manually because the name of ptree was obj
# (ptree was reserved for the ptree param)
# ffi_type_ref and ffi_type_ref were changed manually (were static)
