# 
# use on all .c, and .h (but NOT on the Makefile)
# i.e. sed -i -f tab_to_8spaces.sed *.[ch] 
# Previously a combination of (8 spaces) tabs and spaces were used
# to attain a 4 (four) space indenting.
# From now on, no tabs will be used and indenting will,
# remain at 4 spaces
s/\     /        /g