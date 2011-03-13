CC=gcc
CFLAGS=-O2 -g -std=c99 -Wall -m32 $(MYCFLAGS)
RM=rm -f
LIBS=-lm $(MYLIBS)

MYCFLAGS=
MYLDFLAGS=
MYLIBS=

CORE_O=	kobject.o ktoken.o kpair.o kstring.o ksymbol.o kread.o \
	kwrite.o kstate.o kmem.o kerror.o kauxlib.o kenvironment.o \
	kcontinuation.o koperative.o kapplicative.o keval.o krepl.o \
	kground.o kghelpers.o kgbooleans.o kgeqp.o kgequalp.o \
	kgsymbols.o kgcontrol.o

KRN_T=	klisp
KRN_O=	klisp.o

ALL_T= $(KRN_T)
ALL_O= $(CORE_O) $(KRN_O) 

default: 
	$(MAKE) all MYCFLAGS= MYLIBS=""

all:	$(ALL_T)

o:	$(ALL_O)

$(KRN_T): $(ALL_O)
	$(CC) $(CFLAGS) -o $@ $(MYLDFLAGS) $(ALL_O) $(LIBS)

clean:
	$(RM) $(ALL_T) $(ALL_O)

# list targets that do not create files (but not all makes understand .PHONY)
.PHONY: all default o clean

klisp.o: klisp.c klisp.h kobject.h kread.h kwrite.h klimits.h kstate.h kmem.h \
	kerror.h kauxlib.h koperative.h kenvironment.h kcontinuation.h \
	kapplicative.h koperative.h keval.h krepl.h
kobject.o: kobject.c kobject.h klisp.h
ktoken.o: ktoken.c ktoken.h kobject.h kstate.h kpair.h kstring.h ksymbol.h \
	kerror.h klisp.h
kpair.o: kpair.c kpair.h kobject.h kstate.h kmem.h klisp.h
kstring.o: kstring.c kstring.h kobject.h kstate.h kmem.h klisp.h
# XXX: kpair.h because of use of list as symbol table
ksymbol.o: ksymbol.c ksymbol.h kobject.h kpair.h kstate.h kmem.h klisp.h
kread.o: kread.c kread.h kobject.h ktoken.h kpair.h kstate.h kerror.h klisp.h
kwrite.o: kwrite.c kwrite.h kobject.h kpair.h kstring.h kstate.h kerror.h \
	klisp.h
kstate.o: kstate.c kstate.h klisp.h kobject.h kmem.h kstring.h klisp.h \
	kground.h kenvironment.h kpair.h keval.h koperative.h kground.h \
	krepl.h
kmem.o: kmem.c kmem.h klisp.h kerror.h klisp.h
kerror.o: kerror.c kerror.h klisp.h kstate.h klisp.h kmem.h kstring.h
kauxlib.o: kauxlib.c kauxlib.h klisp.h kstate.h klisp.h
kenvironment.o: kenvironment.c kenvironment.h kpair.h kobject.h kerror.h \
	kmem.h kstate.h klisp.h
kcontinuation.o: kcontinuation.c kcontinuation.h kmem.h kstate.h kobject.h \
	klisp.h
koperative.o: koperative.c koperative.h kmem.h kstate.h kobject.h \
	klisp.h
kapplicative.o: kapplicative.c kapplicative.h kmem.h kstate.h kobject.h \
	klisp.h
keval.o: keval.c keval.h kcontinuation.h kenvironment.h kstate.h kobject.h \
	kpair.h kerror.h klisp.h
krepl.o: krepl.c krepl.h kcontinuation.h kstate.h kobject.h keval.h klisp.h \
	kread.h kwrite.h kenvironment.h
kground.o: kground.c kground.h kstate.h kobject.h klisp.h kenvironment.h \
	kpair.h kapplicative.h koperative.h ksymbol.h kerror.h kghelpers.h \
	kgbooleans.h kgeqp.h kgequalp.h kgsymbols.h
kghelpers.o: kghelpers.c kghelpers.h kstate.h kstate.h klisp.h kpair.h \
	kapplicative.h koperative.h kerror.h kobject.h ksymbol.h
kgbooleans.o: kgbooleans.c kgbooleans.c kghelpers.h kstate.h klisp.h \
	kobject.h kerror.h kpair.h kcontinuation.h
kgeqp.o: kgeqp.c kgeqp.c kghelpers.h kstate.h klisp.h \
	kobject.h kerror.h kpair.h kcontinuation.h
kgequalp.o: kgequalp.c kgequalp.c kghelpers.h kstate.h klisp.h \
	kobject.h kerror.h kpair.h kcontinuation.h kgeqp.h kstring.h
kgsymbols.o: kgsymbols.c kgsymbols.c kghelpers.h kstate.h klisp.h \
	kobject.h kerror.h kpair.h kcontinuation.h kstring.h
kgcontrol.o: kgcontrol.c kgcontrol.c kghelpers.h kstate.h klisp.h \
	kobject.h kerror.h kpair.h kcontinuation.h
