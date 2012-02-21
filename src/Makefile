# makefile for building klisp
# SOURCE NOTE: this is mostly from lua

# == CHANGE THE SETTINGS BELOW TO SUIT YOUR ENVIRONMENT =======================

# Your platform. See PLATS for possible values.
PLAT= none

CC=gcc
# TEMP for now put in debug symbols
# TEMP for now only 32 bit binaries (see kobject.h)
CFLAGS=-arch i386 -O2 -g -std=c99 -Wall -m32 $(MYCFLAGS)
AR= ar rcu
RANLIB= ranlib
RM= rm -f
LIBS= -arch i386 -lm $(MYLIBS)

# Set USE_LIBFFI=1 (or other nonempty string) to enable libffi-dependent
# code.
USE_LIBFFI=
MINGW_LIBFFI_CFLAGS = -I/usr/local/lib/libffi-3.0.10/include
MINGW_LIBFFI_LDFLAGS = -L/usr/local/lib/

MYCFLAGS=
MYLDFLAGS=
MYLIBS=

# == END OF USER SETTINGS. NO NEED TO CHANGE ANYTHING BELOW THIS LINE =========

# TEMP only these two for now
PLATS= generic mingw posix

KRN_A= libklisp.a
CORE_O=	kobject.o ktoken.o kpair.o kstring.o ksymbol.o kread.o \
	kwrite.o kstate.o kmem.o kerror.o kauxlib.o kenvironment.o \
	kcontinuation.o koperative.o kapplicative.o keval.o krepl.o \
	kencapsulation.o kpromise.o kport.o kinteger.o krational.o ksystem.o \
	kreal.o ktable.o kgc.o imath.o imrat.o kbytevector.o kvector.o \
	kchar.o kkeyword.o kmodule.o \
	kground.o kghelpers.o kgbooleans.o kgeqp.o kgmodules.o \
	kgequalp.o kgsymbols.o kgcontrol.o kgpairs_lists.o kgpair_mut.o \
	kgenvironments.o kgenv_mut.o kgcombiners.o kgcontinuations.o \
	kgencapsulations.o kgpromises.o kgkd_vars.o kgks_vars.o kgports.o \
	kgchars.o kgnumbers.o kgstrings.o kgbytevectors.o kgvectors.o \
	kgsystem.o kgerrors.o kgkeywords.o \
	$(if $(USE_LIBFFI),kgffi.o)

# TEMP: in klisp there is no distinction between core & lib
LIB_O= 

KRN_T=	klisp
KRN_O=	klisp.o

ALL_O= $(CORE_O) $(LIB_O) $(KRN_O) 
ALL_T= $(KRN_A) $(KRN_T)
ALL_A= $(KRN_A)

default: $(PLAT)

all:	$(ALL_T)

o:	$(ALL_O)

a:	$(ALL_A)

$(KRN_A): $(CORE_O) $(LIB_O)
	$(AR) $@ $? $(MINGW_LDFLAGS)
	$(RANLIB) $@

$(KRN_T): $(KRN_O) $(KRN_A)
	$(CC) -o $@ $(MYLDFLAGS) $(KRN_O) $(KRN_A) $(LIBS)

clean:
	$(RM) $(ALL_T) $(ALL_O) kgffi.o TAGS

# XXX this fails if USE_LIBFFI is not defined
depend:
	@$(CC) $(CFLAGS) -DKUSE_LIBFFI=1 -MM k*.c imath.c imrat.c

echo:
	@echo "PLAT = $(PLAT)"
	@echo "CC = $(CC)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "AR = $(AR)"
	@echo "RANLIB = $(RANLIB)"
	@echo "RM = $(RM)"
	@echo "MYCFLAGS = $(MYCFLAGS)"
	@echo "MYLDFLAGS = $(MYLDFLAGS)"
	@echo "MYLIBS = $(MYLIBS)"

# convenience targets for popular platforms

none:
	@echo "Please choose a platform:"
	@echo "   $(PLATS)"

generic:
	$(MAKE) all MYCFLAGS=

mingw:
	$(MAKE) "KRN_A=klisp01.dll" "KRN_T=klisp.exe" \
	"AR=$(CC) -shared -o" \
	"RANLIB=strip --strip-unneeded" \
	"MYCFLAGS=-DKLISP_BUILD_AS_DLL $(if $(USE_LIBFFI),-DKUSE_LIBFFI=1 $(MINGW_LIBFFI_CFLAGS))" \
	"MYLIBS=$(if $(USE_LIBFFI), $(MINGW_LIBFFI_LDFLAGS) -lffi.dll)" \
	"MINGW_LDFLAGS=$(if $(USE_LIBFFI), $(MINGW_LIBFFI_LDFLAGS) -lffi.dll)" \
	"MYLDFLAGS=-s" klisp.exe
#lisp_use_posix isn't used right now...
posix:
	$(MAKE) all \
		"MYCFLAGS=-DKLISP_USE_POSIX -D_POSIX_SOURCE $(if $(USE_LIBFFI),-DKUSE_LIBFFI=1)" \
		"MYLIBS=$(if $(USE_LIBFFI), -rdynamic -ldl -lffi)"

# for use in emacs
tags:
	rm -f TAGS; etags --declarations -o TAGS *.[ch]
# list targets that do not create files (but not all makes understand .PHONY)
.PHONY: all default o clean

kapplicative.o: kapplicative.c kobject.h klimits.h klisp.h klispconf.h \
 kstate.h ktoken.h kmem.h kapplicative.h koperative.h kgc.h
kauxlib.o: kauxlib.c klisp.h kobject.h klimits.h klispconf.h kstate.h \
 ktoken.h kmem.h
kbytevector.o: kbytevector.c kbytevector.h kobject.h klimits.h klisp.h \
 klispconf.h kstate.h ktoken.h kmem.h kgc.h kstring.h
kchar.o: kchar.c kobject.h klimits.h klisp.h klispconf.h
kcontinuation.o: kcontinuation.c kcontinuation.h kobject.h klimits.h \
 klisp.h klispconf.h kstate.h ktoken.h kmem.h kgc.h
kencapsulation.o: kencapsulation.c kobject.h klimits.h klisp.h \
 klispconf.h kmem.h kstate.h ktoken.h kencapsulation.h kpair.h kgc.h
kenvironment.o: kenvironment.c kenvironment.h kobject.h klimits.h klisp.h \
 klispconf.h kstate.h ktoken.h kmem.h kpair.h kgc.h ksymbol.h kstring.h \
 kerror.h ktable.h kapplicative.h koperative.h
kerror.o: kerror.c klisp.h kobject.h klimits.h klispconf.h kpair.h \
 kstate.h ktoken.h kmem.h kgc.h kstring.h kerror.h
keval.o: keval.c klisp.h kobject.h klimits.h klispconf.h kstate.h \
 ktoken.h kmem.h kpair.h kgc.h kenvironment.h kcontinuation.h kerror.h \
 kghelpers.h kapplicative.h koperative.h ksymbol.h kstring.h ktable.h
kgbooleans.o: kgbooleans.c kobject.h klimits.h klisp.h klispconf.h \
 kstate.h ktoken.h kmem.h kpair.h kgc.h ksymbol.h kstring.h \
 kcontinuation.h kerror.h kghelpers.h kapplicative.h koperative.h \
 kenvironment.h ktable.h kgbooleans.h
kgbytevectors.o: kgbytevectors.c kstate.h klimits.h klisp.h kobject.h \
 klispconf.h ktoken.h kmem.h kapplicative.h koperative.h kcontinuation.h \
 kerror.h kpair.h kgc.h kbytevector.h kghelpers.h kenvironment.h \
 ksymbol.h kstring.h ktable.h kgbytevectors.h
kgc.o: kgc.c kgc.h kobject.h klimits.h klisp.h klispconf.h kstate.h \
 ktoken.h kmem.h kport.h imath.h imrat.h ktable.h kstring.h kbytevector.h \
 kvector.h kerror.h kpair.h
kgchars.o: kgchars.c kstate.h klimits.h klisp.h kobject.h klispconf.h \
 ktoken.h kmem.h kapplicative.h koperative.h kcontinuation.h kerror.h \
 kpair.h kgc.h kchar.h kghelpers.h kenvironment.h ksymbol.h kstring.h \
 ktable.h kgchars.h
kgcombiners.o: kgcombiners.c kstate.h klimits.h klisp.h kobject.h \
 klispconf.h ktoken.h kmem.h kpair.h kgc.h kenvironment.h kcontinuation.h \
 ksymbol.h kstring.h koperative.h kapplicative.h kerror.h kghelpers.h \
 ktable.h kgcombiners.h
kgcontinuations.o: kgcontinuations.c kstate.h klimits.h klisp.h kobject.h \
 klispconf.h ktoken.h kmem.h kpair.h kgc.h kenvironment.h kcontinuation.h \
 kapplicative.h koperative.h ksymbol.h kstring.h kerror.h kghelpers.h \
 ktable.h kgcontinuations.h
kgcontrol.o: kgcontrol.c kstate.h klimits.h klisp.h kobject.h klispconf.h \
 ktoken.h kmem.h kpair.h kgc.h kcontinuation.h kerror.h kghelpers.h \
 kapplicative.h koperative.h kenvironment.h ksymbol.h kstring.h ktable.h \
 kgcontrol.h
kgencapsulations.o: kgencapsulations.c kstate.h klimits.h klisp.h \
 kobject.h klispconf.h ktoken.h kmem.h kencapsulation.h kapplicative.h \
 koperative.h kerror.h kpair.h kgc.h kghelpers.h kcontinuation.h \
 kenvironment.h ksymbol.h kstring.h ktable.h kgencapsulations.h
kgenv_mut.o: kgenv_mut.c kstate.h klimits.h klisp.h kobject.h klispconf.h \
 ktoken.h kmem.h kpair.h kgc.h kenvironment.h kcontinuation.h ksymbol.h \
 kstring.h kerror.h kghelpers.h kapplicative.h koperative.h ktable.h \
 kgenv_mut.h
kgenvironments.o: kgenvironments.c kstate.h klimits.h klisp.h kobject.h \
 klispconf.h ktoken.h kmem.h kpair.h kgc.h kenvironment.h kcontinuation.h \
 ksymbol.h kstring.h kerror.h kghelpers.h kapplicative.h koperative.h \
 ktable.h kport.h kread.h kgenvironments.h
kgeqp.o: kgeqp.c kstate.h klimits.h klisp.h kobject.h klispconf.h \
 ktoken.h kmem.h kpair.h kgc.h kcontinuation.h kerror.h kghelpers.h \
 kapplicative.h koperative.h kenvironment.h ksymbol.h kstring.h ktable.h \
 kgeqp.h
kgequalp.o: kgequalp.c kstate.h klimits.h klisp.h kobject.h klispconf.h \
 ktoken.h kmem.h kpair.h kgc.h kvector.h kstring.h kbytevector.h \
 kcontinuation.h kerror.h kghelpers.h kapplicative.h koperative.h \
 kenvironment.h ksymbol.h ktable.h kgequalp.h
kgerrors.o: kgerrors.c kstate.h klimits.h klisp.h kobject.h klispconf.h \
 ktoken.h kmem.h kstring.h kpair.h kgc.h kerror.h kghelpers.h \
 kapplicative.h koperative.h kcontinuation.h kenvironment.h ksymbol.h \
 ktable.h kgerrors.h
kgffi.o: kgffi.c imath.h kobject.h klimits.h klisp.h klispconf.h kstate.h \
 ktoken.h kmem.h kinteger.h kpair.h kgc.h kerror.h kbytevector.h \
 kencapsulation.h ktable.h kghelpers.h kapplicative.h koperative.h \
 kcontinuation.h kenvironment.h ksymbol.h kstring.h kgffi.h
kghelpers.o: kghelpers.c kghelpers.h kstate.h klimits.h klisp.h kobject.h \
 klispconf.h ktoken.h kmem.h kerror.h kpair.h kgc.h kapplicative.h \
 koperative.h kcontinuation.h kenvironment.h ksymbol.h kstring.h ktable.h \
 kinteger.h imath.h krational.h imrat.h kbytevector.h kvector.h \
 kencapsulation.h kpromise.h
kgkd_vars.o: kgkd_vars.c kstate.h klimits.h klisp.h kobject.h klispconf.h \
 ktoken.h kmem.h kpair.h kgc.h kcontinuation.h koperative.h \
 kapplicative.h kenvironment.h kerror.h kghelpers.h ksymbol.h kstring.h \
 ktable.h kgkd_vars.h
kgks_vars.o: kgks_vars.c kstate.h klimits.h klisp.h kobject.h klispconf.h \
 ktoken.h kmem.h kpair.h kgc.h kcontinuation.h koperative.h \
 kapplicative.h kenvironment.h kerror.h kghelpers.h ksymbol.h kstring.h \
 ktable.h kgks_vars.h
kgnumbers.o: kgnumbers.c kstate.h klimits.h klisp.h kobject.h klispconf.h \
 ktoken.h kmem.h kapplicative.h koperative.h kcontinuation.h kerror.h \
 kpair.h kgc.h ksymbol.h kstring.h kinteger.h imath.h krational.h imrat.h \
 kreal.h kghelpers.h kenvironment.h ktable.h kgnumbers.h
kgpair_mut.o: kgpair_mut.c kstate.h klimits.h klisp.h kobject.h \
 klispconf.h ktoken.h kmem.h kpair.h kgc.h kcontinuation.h ksymbol.h \
 kstring.h kerror.h kghelpers.h kapplicative.h koperative.h \
 kenvironment.h ktable.h kgpair_mut.h
kgpairs_lists.o: kgpairs_lists.c kstate.h klimits.h klisp.h kobject.h \
 klispconf.h ktoken.h kmem.h kpair.h kgc.h kstring.h kcontinuation.h \
 kenvironment.h ksymbol.h kerror.h kghelpers.h kapplicative.h \
 koperative.h ktable.h kgpairs_lists.h
kgports.o: kgports.c kstate.h klimits.h klisp.h kobject.h klispconf.h \
 ktoken.h kmem.h kport.h kstring.h kbytevector.h kenvironment.h \
 kapplicative.h koperative.h kcontinuation.h kpair.h kgc.h kerror.h \
 ksymbol.h kread.h kwrite.h kghelpers.h ktable.h kgports.h ktable.h
kgpromises.o: kgpromises.c kstate.h klimits.h klisp.h kobject.h \
 klispconf.h ktoken.h kmem.h kpromise.h kpair.h kgc.h kapplicative.h \
 koperative.h kcontinuation.h kerror.h kghelpers.h kenvironment.h \
 ksymbol.h kstring.h ktable.h kgpromises.h
kgmodules.o: kgmodules.c kstate.h klimits.h klisp.h kobject.h \
 klispconf.h ktoken.h kmem.h kmodule.h kpair.h kgc.h kapplicative.h \
 koperative.h kcontinuation.h kerror.h kghelpers.h kenvironment.h \
 ksymbol.h kstring.h ktable.h kgmodules.h kpair.h kkeyword.h
kground.o: kground.c kstate.h klimits.h klisp.h kobject.h klispconf.h \
 ktoken.h kmem.h kground.h kghelpers.h kerror.h kpair.h kgc.h \
 kapplicative.h koperative.h kcontinuation.h kenvironment.h ksymbol.h \
 kstring.h ktable.h kgbooleans.h kgeqp.h kgequalp.h kgsymbols.h \
 kgcontrol.h kgpairs_lists.h kgpair_mut.h kgenvironments.h kgenv_mut.h \
 kgcombiners.h kgcontinuations.h kgencapsulations.h kgpromises.h \
 kgkd_vars.h kgks_vars.h kgnumbers.h kgstrings.h kgchars.h kgports.h \
 kgbytevectors.h kgvectors.h kgsystem.h kgerrors.h kgmodules.h \
 kgffi.h keval.h krepl.h
kgstrings.o: kgstrings.c kstate.h klimits.h klisp.h kobject.h klispconf.h \
 ktoken.h kmem.h kapplicative.h koperative.h kcontinuation.h kerror.h \
 kpair.h kgc.h ksymbol.h kstring.h kchar.h kvector.h kbytevector.h \
 kghelpers.h kenvironment.h ktable.h kgstrings.h
kgsymbols.o: kgsymbols.c kstate.h klimits.h klisp.h kobject.h klispconf.h \
 ktoken.h kmem.h kcontinuation.h kpair.h kgc.h kstring.h ksymbol.h \
 kerror.h kghelpers.h kapplicative.h koperative.h kenvironment.h ktable.h \
 kgsymbols.h
kgkeywords.o: kgkeywords.c kgkeywords.h kstate.h klimits.h klisp.h kobject.h \
 klispconf.h ktoken.h kmem.h kcontinuation.h kpair.h kgc.h kstring.h \
 ksymbol.h kerror.h kghelpers.h kapplicative.h koperative.h kenvironment.h \
 ktable.h kkeyword.h
kgsystem.o: kgsystem.c kstate.h klimits.h klisp.h kobject.h klispconf.h \
 ktoken.h kmem.h kpair.h kgc.h kerror.h ksystem.h kghelpers.h \
 kapplicative.h koperative.h kcontinuation.h kenvironment.h ksymbol.h \
 kstring.h ktable.h kgsystem.h kinteger.h kmem.h imath.h kgc.h
kgvectors.o: kgvectors.c kstate.h klimits.h klisp.h kobject.h klispconf.h \
 ktoken.h kmem.h kapplicative.h koperative.h kcontinuation.h kerror.h \
 kpair.h kgc.h kvector.h kbytevector.h kghelpers.h kenvironment.h \
 ksymbol.h kstring.h ktable.h kgvectors.h
kinteger.o: kinteger.c kinteger.h kobject.h klimits.h klisp.h klispconf.h \
 kstate.h ktoken.h kmem.h imath.h kgc.h
klisp.o: klisp.c klimits.h klisp.h kobject.h klispconf.h kstate.h \
 ktoken.h kmem.h kauxlib.h kstring.h kcontinuation.h koperative.h \
 kapplicative.h ksymbol.h kenvironment.h kport.h kread.h kwrite.h \
 kerror.h kpair.h kgc.h krepl.h kghelpers.h ktable.h
kmem.o: kmem.c klisp.h kobject.h klimits.h klispconf.h kstate.h ktoken.h \
 kmem.h kerror.h kpair.h kgc.h
kobject.o: kobject.c kobject.h klimits.h klisp.h klispconf.h
koperative.o: koperative.c koperative.h kobject.h klimits.h klisp.h \
 klispconf.h kstate.h ktoken.h kmem.h kgc.h
kpair.o: kpair.c kpair.h kobject.h klimits.h klisp.h klispconf.h kstate.h \
 ktoken.h kmem.h kgc.h
kport.o: kport.c kport.h kobject.h klimits.h klisp.h klispconf.h kstate.h \
 ktoken.h kmem.h kerror.h kpair.h kgc.h kstring.h kbytevector.h
kpromise.o: kpromise.c kobject.h klimits.h klisp.h klispconf.h kstate.h \
 ktoken.h kmem.h kpromise.h kpair.h kgc.h
kmodule.o: kmodule.c kobject.h klimits.h klisp.h klispconf.h kstate.h \
 ktoken.h kmem.h kmodule.h kgc.h
krational.o: krational.c krational.h kobject.h klimits.h klisp.h \
 klispconf.h kstate.h ktoken.h kmem.h kinteger.h imath.h imrat.h kgc.h
kread.o: kread.c kread.h kobject.h klimits.h klisp.h klispconf.h kstate.h \
 ktoken.h kmem.h kpair.h kgc.h kerror.h ktable.h kport.h kstring.h
kreal.o: kreal.c kreal.h kobject.h klimits.h klisp.h klispconf.h kstate.h \
 ktoken.h kmem.h kinteger.h imath.h krational.h imrat.h kgc.h kpair.h \
 kerror.h
krepl.o: krepl.c klisp.h kobject.h klimits.h klispconf.h kstate.h \
 ktoken.h kmem.h kcontinuation.h kenvironment.h kerror.h kpair.h kgc.h \
 kread.h kwrite.h kstring.h krepl.h ksymbol.h kport.h ktable.h \
 kghelpers.h kapplicative.h koperative.h
kstate.o: kstate.c klisp.h kobject.h klimits.h klispconf.h kstate.h \
 ktoken.h kmem.h kstring.h kpair.h kgc.h keval.h koperative.h \
 kapplicative.h kcontinuation.h kenvironment.h kground.h krepl.h \
 ksymbol.h kport.h ktable.h kbytevector.h kvector.h kghelpers.h kerror.h \
 kgerrors.h
kstring.o: kstring.c kstring.h kobject.h klimits.h klisp.h klispconf.h \
 kstate.h ktoken.h kmem.h kgc.h
ksymbol.o: ksymbol.c ksymbol.h kobject.h klimits.h klisp.h klispconf.h \
 kstate.h kmem.h kstring.h kgc.h
kkeyword.o: kkeyword.c kkeyword.h kobject.h klimits.h klisp.h klispconf.h \
 kstate.h kmem.h kstring.h kgc.h
ksystem.o: ksystem.c kobject.h klimits.h klisp.h klispconf.h kstate.h \
 ktoken.h kmem.h kerror.h kpair.h kgc.h ksystem.h
ksystem.posix.o: ksystem.posix.c kobject.h klimits.h klisp.h klispconf.h \
 kstate.h ktoken.h kmem.h kinteger.h imath.h ksystem.h
ksystem.win32.o: ksystem.win32.c kobject.h klimits.h klisp.h klispconf.h \
 kstate.h ktoken.h kmem.h kinteger.h imath.h ksystem.h
ktable.o: ktable.c klisp.h kobject.h klimits.h klispconf.h kgc.h kstate.h \
 ktoken.h kmem.h ktable.h kapplicative.h koperative.h kghelpers.h \
 kerror.h kpair.h kcontinuation.h kenvironment.h ksymbol.h kstring.h
ktoken.o: ktoken.c ktoken.h kobject.h klimits.h klisp.h klispconf.h \
 kstate.h kmem.h kinteger.h imath.h krational.h imrat.h kreal.h kpair.h \
 kgc.h kstring.h kbytevector.h ksymbol.h kerror.h kport.h kkeyword.h
kvector.o: kvector.c kvector.h kobject.h klimits.h klisp.h klispconf.h \
 kstate.h ktoken.h kmem.h kgc.h
kwrite.o: kwrite.c kwrite.h kobject.h klimits.h klisp.h klispconf.h \
 kstate.h ktoken.h kmem.h kinteger.h imath.h krational.h imrat.h kreal.h \
 kpair.h kgc.h kstring.h ksymbol.h kkeyword.h kerror.h ktable.h kport.h \
 kenvironment.h kbytevector.h kvector.h ktoken.h
imath.o: imath.c imath.h kobject.h klimits.h klisp.h klispconf.h kstate.h \
 ktoken.h kmem.h kerror.h kpair.h kgc.h
imrat.o: imrat.c imrat.h imath.h kobject.h klimits.h klisp.h klispconf.h \
 kstate.h ktoken.h kmem.h kerror.h kpair.h kgc.h
