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

# detect single line function definition
# There are 44, all starting with do_
#/[(]klisp_State \*K, TValue \*xparams, TValue obj[)];/P

#detect functions names starting with do_
# There are 47, do_access, do_bind and do_vau are not continuation
#/void do_/P

# All function definition are one line, just replace them in the .h
# This is used to modify in place with sed -i -f <this-file> *.h
s/^void \(.*\)[(]klisp_State \*K, TValue \*xparams, TValue obj[)];/void \1(klisp_State *K);/
