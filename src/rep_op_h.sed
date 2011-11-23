# This is a collection of sed commands to refactor operatives underlying 
# functions to just take a kernel state pointer (instead of also taking extra
# params, ptree and denv).

# All these tests are run one at a time with sed -n

# detect single line function definition
# There are 97
/^void \(.*\)[(]klisp_State \*K, TValue \*xparams, TValue ptree, TValue denv[)];/P

# Replace them in place with sed -i -f <this-file> *.h
#s/^void \(.*\)[(]klisp_State \*K, TValue \*xparams, TValue ptree, TValue denv[)];/void \1(klisp_State *K);/

# All the single line definitions done

# try to detect multi line function definition
# There are 62
#/^void \(.*\)[(]klisp_State \*K,/{
#N
#/^void \(.*\)[(]klisp_State \*K,[[:space:]]*TValue \*xparams,[[:space:]]*TValue ptree,[[:space:]]*TValue denv);/P
#}

# replace them 
# equalp had a type (was xparas instead of xparams), correct first
s/xparas/xparams/
/^void \(.*\)[(]klisp_State \*K,/{
N
s/^void \(.*\)[(]klisp_State \*K,[[:space:]]*TValue \*xparams,[[:space:]]*TValue ptree,[[:space:]]*TValue denv);/void \1(klisp_State *K);/
}

# Done!