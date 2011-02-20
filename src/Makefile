CC=gcc
CFLAGS=-O2 -g -std=c99 -Wall -m32 $(MYCFLAGS)
RM=rm -f
LIBS=-lm $(MYLIBS)

MYCFLAGS=
MYLDFLAGS=
MYLIBS=

CORE_O=	kobject.o ktoken.o kpair.o kstring.o ksymbol.o kread.o

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

klisp.o: klisp.c klisp.h kobject.h ktoken.h
kobject.o: kobject.c kobject.h
ktoken.o: ktoken.c ktoken.h kobject.h kpair.h kstring.h ksymbol.h
kpair.o: kpair.c kpair.h kobject.h
kstring.o: kstring.c kstring.h kobject.h
# XXX: kpair.h because of use of list as symbol table
ksymbol.o: ksymbol.c ksymbol.h kobject.h kpair.h
kread.o: kread.c kread.h kobject.h ktoken.h kpair.h
