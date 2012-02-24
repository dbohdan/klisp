# Makefile for the klisp manual
# some of this is from the elisp manual

srcs = klisp.texi index.texi \
	intro.texi interpreter.texi \
	booleans.texi equivalence.texi symbols.texi \
	control.texi pairs_lists.texi \
	environments.texi combiners.texi \
	continuations.texi encapsulations.texi \
	promises.texi keyed_vars.texi \
	numbers.texi strings.texi \
	characters.texi ports.texi \
	libraries.texi system.texi

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
