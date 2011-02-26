CC=gcc
CFLAGS=-O2 -g -std=c99 -Wall -m32 $(MYCFLAGS)
RM=rm -f
LIBS=-lm $(MYLIBS)

MYCFLAGS=
MYLDFLAGS=
MYLIBS=

CORE_O=	kobject.o ktoken.o kpair.o kstring.o ksymbol.o kread.o \
	kwrite.o kstate.o kmem.o kerror.o kauxlib.o

KRN_T=	klisp
KRN_O=	klisp.o

ALL_T= $(KRN_T)
ALL_O= $(CORE_O) $(KRN_O) 

default: 
	$(MAKE) all MYCFLAGS= MYLIBS=""

all:	$(ALL_T)

o:	$(ALL_O)

$(KRN_T): $(ALL_O)
	$(CC) -o $@ $(MYLDFLAGS) $(ALL_O) $(LIBS)

clean:
	$(RM) $(ALL_T) $(ALL_O)

# list targets that do not create files (but not all makes understand .PHONY)
.PHONY: all default o clean

klisp.o: klisp.c klisp.h kobject.h kread.h kwrite.h klimits.h kstate.h kmem.h \
	kerror.h kauxlib.h
kobject.o: kobject.c kobject.h
ktoken.o: ktoken.c ktoken.h kobject.h kstate.h kpair.h kstring.h ksymbol.h \
	kerror.h
kpair.o: kpair.c kpair.h kobject.h kstate.h kmem.h
kstring.o: kstring.c kstring.h kobject.h kstate.h kmem.h
# XXX: kpair.h because of use of list as symbol table
ksymbol.o: ksymbol.c ksymbol.h kobject.h kpair.h kstate.h kmem.h
kread.o: kread.c kread.h kobject.h ktoken.h kpair.h kstate.h kerror.h
kwrite.o: kwrite.c kwrite.h kobject.h kpair.h kstring.h kstate.h kerror.h
kstate.o: kstate.c kstate.h klisp.h kobject.h kmem.h kstring.h
kmem.o: kmem.c kmem.h klisp.h kerror.h
kerror.o: kerror.c kerror.h klisp.h kstate.h
kauxlib.o: kauxlib.c kauxlib.h klisp.h kstate.h