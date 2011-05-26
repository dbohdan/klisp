# Makefile for the klisp manual
# some of this is from the elisp manual

srcs = klisp.texi intro.texi \
  index.texi

#TODO add dvi/pdf output
#TODO check what happens with xrefs

.PHONY: all clean

all:	clean info html

info: $(srcs)
	makeinfo -o ../klisp.info klisp.texi

html: $(srcs)
	makeinfo --html -o ../html klisp.texi

clean:
	rm -r -f ../klisp.info ../html
