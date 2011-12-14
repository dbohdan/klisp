# 
# use on all .k files
# i.e. sed -i -f tab_to_4spaces.sed tests/*.k
# Previously a combination of (4 spaces) tabs and spaces were used
# to attain a 4 (four) space indenting.
# From now on, no tabs will be used and indenting will,
# remain at 4 spaces
s/\     /    /g