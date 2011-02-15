CC=gcc
CFLAGS=-O2 -std=c99 -Wall -m32 $(MYCFLAGS)
RM=rm -f
LIBS=-lm $(MYLIBS)

MYCFLAGS=
MYLDFLAGS=
MYLIBS=

CORE_O=	

KRN_T=	klisp
KRN_O=	klisp.o

ALL_T= $(KRN_T)
ALL_O= $(CORE_O) $(KRN_O) 

default: 
	$(MAKE) all MYCFLAGS= MYLIBS=""

all:	$(ALL_T)

o:	$(ALL_O)

$(KRN_T): $(ALL_O)
	$(CC) -o $@ $(MYLDFLAGS) $(KRN_O) $(LIBS)

clean:
	$(RM) $(ALL_T) $(ALL_O)

# list targets that do not create files (but not all makes understand .PHONY)
.PHONY: all default o clean

klisp.o: klisp.c klisp.h kobject.h
