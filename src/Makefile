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
	kencapsulation.o kpromise.o kport.o kinteger.o \
	kground.o kghelpers.o kgbooleans.o kgeqp.o kgequalp.o \
	kgsymbols.o kgcontrol.o kgpairs_lists.o kgpair_mut.o kgenvironments.o \
	kgenv_mut.o kgcombiners.o kgcontinuations.o kgencapsulations.o \
	kgpromises.o kgkd_vars.o kgks_vars.o kgports.o kgchars.o kgnumbers.o \
	kgstrings.o imath.o

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
	kerror.h klisp.h kinteger.h
kinteger.o: kinteger.c kinteger.h kobject.h kstate.h kmem.h klisp.h imath.h
kpair.o: kpair.c kpair.h kobject.h kstate.h kmem.h klisp.h
kstring.o: kstring.c kstring.h kobject.h kstate.h kmem.h klisp.h
# XXX: kpair.h because of use of list as symbol table
ksymbol.o: ksymbol.c ksymbol.h kobject.h kpair.h kstring.h kstate.h kmem.h \
	klisp.h
kread.o: kread.c kread.h kobject.h ktoken.h kpair.h kstate.h kerror.h klisp.h \
	kport.h
kwrite.o: kwrite.c kwrite.h kobject.h kpair.h kstring.h kstate.h kerror.h \
	klisp.h kport.h kinteger.h
kstate.o: kstate.c kstate.h klisp.h kobject.h kmem.h kstring.h klisp.h \
	kground.h kenvironment.h kpair.h keval.h koperative.h kground.h \
	krepl.h kcontinuation.h kapplicative.h kport.h ksymbol.h kport.h \
	kstring.h kinteger.h 
kmem.o: kmem.c kmem.h klisp.h kerror.h klisp.h kstate.h
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
kencapsulation.o: kencapsulation.c kencapsulation.h kmem.h kstate.h kobject.h \
	klisp.h kpair.h
kpromise.o: kpromise.c kpromise.h kmem.h kstate.h kobject.h \
	klisp.h kpair.h
kport.o: kport.c kport.h kmem.h kstate.h kobject.h klisp.h kerror.h kstring.h
keval.o: keval.c keval.h kcontinuation.h kenvironment.h kstate.h kobject.h \
	kpair.h kerror.h klisp.h
krepl.o: krepl.c krepl.h kcontinuation.h kstate.h kobject.h keval.h klisp.h \
	kread.h kwrite.h kenvironment.h ksymbol.h
kground.o: kground.c kground.h kstate.h kobject.h klisp.h kenvironment.h \
	kapplicative.h koperative.h ksymbol.h kerror.h kghelpers.h \
	kgbooleans.h kgeqp.h kgequalp.h kgsymbols.h kgpairs_lists.h \
	kgpair_mut.h kgenvironments.h kgenv_mut.h kgcombiners.h \
	kgcontinuations.h kgencapsulations.h kgpromises.h kgkd_vars.h \
	kgks_vars.h kgports.h kgchars.h kgnumbers.h kgstrings.o
kghelpers.o: kghelpers.c kghelpers.h kstate.h kstate.h klisp.h kpair.h \
	kapplicative.h koperative.h kerror.h kobject.h ksymbol.h \
	kcontinuation.h
kgbooleans.o: kgbooleans.c kgbooleans.c kghelpers.h kstate.h klisp.h \
	kobject.h kerror.h kpair.h kcontinuation.h ksymbol.h
kgeqp.o: kgeqp.c kgeqp.c kghelpers.h kstate.h klisp.h \
	kobject.h kerror.h kpair.h kcontinuation.h
kgequalp.o: kgequalp.c kgequalp.c kghelpers.h kstate.h klisp.h \
	kobject.h kerror.h kpair.h kcontinuation.h kgeqp.h kstring.h
kgsymbols.o: kgsymbols.c kgsymbols.c kghelpers.h kstate.h klisp.h \
	kobject.h kerror.h kpair.h kcontinuation.h kstring.h
kgcontrol.o: kgcontrol.c kgcontrol.c kghelpers.h kstate.h klisp.h \
	kobject.h kerror.h kpair.h kcontinuation.h kgcombiners.h
kgpairs_lists.o: kgpairs_lists.c kgpairs_lists.h kghelpers.h kstate.h klisp.h \
	kobject.h kerror.h kpair.h ksymbol.h kcontinuation.h kgequalp.h \
	kenvironment.h kgnumbers.h kinteger.h
kgpair_mut.o: kgpair_mut.c kgpair_mut.h kghelpers.h kstate.h klisp.h \
	kobject.h kerror.h kpair.h ksymbol.h kcontinuation.h kgeqp.h
kgenvironments.o: kgenvironments.c kgenvironments.h kghelpers.h kstate.h \
	klisp.h kobject.h kerror.h kpair.h ksymbol.h kcontinuation.h \
	kenvironment.h kgenv_mut.h kgpair_mut.h kgcontrol.h
kgenv_mut.o: kgenv_mut.c kgenv_mut.h kghelpers.h kstate.h \
	klisp.h kobject.h kerror.h kpair.h ksymbol.h kcontinuation.h \
	kenvironment.h kgcontrol.h
kgcombiners.o: kgcombiners.c kgenvironments.h kghelpers.h kstate.h \
	klisp.h kobject.h kerror.h kpair.h ksymbol.h kcontinuation.h \
	kenvironment.h kapplicative.h koperative.h kgpair_mut.h kgnumbers.h
kgcontinuations.o: kgcontinuations.c kgcontinuations.h kghelpers.h kstate.h \
	klisp.h kobject.h kerror.h kpair.h ksymbol.h kcontinuation.h \
	kenvironment.h kapplicative.h koperative.h
kgencapsulations.o: kgencapsulations.c kgencapsulations.h kghelpers.h \
	kstate.h klisp.h kobject.h kerror.h kapplicative.h koperative.h \
	kencapsulation.h
kgpromises.o: kgpromises.c kgpromises.h kghelpers.h kstate.h klisp.h \
	kobject.h kerror.h kapplicative.h koperative.h kcontinuation.h \
	kpair.h kpromise.h
kgkd_vars.o: kgkd_vars.c kgkd_vars.h kghelpers.h kstate.h klisp.h \
	kobject.h kerror.h kapplicative.h koperative.h kcontinuation.h \
	kpair.h kenvironment.h kgcontinuations.h
kgks_vars.o: kgks_vars.c kgks_vars.h kghelpers.h kstate.h klisp.h \
	kobject.h kerror.h kapplicative.h koperative.h kcontinuation.h \
	kpair.h kenvironment.h
kgports.o: kgports.c kgports.h kghelpers.h kstate.h klisp.h \
	kobject.h kerror.h kapplicative.h koperative.h kcontinuation.h \
	kport.h ksymbol.h kread.h kwrite.h ktoken.h kgcontinuations.h \
	kpair.h kenvironment.h kgcontrol.h kgkd_vars.h
kgchars.o: kgchars.c kgchars.h kghelpers.h kstate.h klisp.h \
	kobject.h kerror.h kapplicative.h koperative.h kcontinuation.h
kgnumbers.o: kgnumbers.c kgnumbers.h kghelpers.h kstate.h klisp.h \
	kobject.h kerror.h kapplicative.h koperative.h kcontinuation.h \
	ksymbol.h kinteger.h 
kgstrings.o: kgstrings.c kgstrings.h kghelpers.h kstate.h klisp.h \
	kobject.h kerror.h kapplicative.h koperative.h kcontinuation.h \
	ksymbol.h kgnumbers.h
imath.o: kobject.h kstate.h kmem.h kerror.h