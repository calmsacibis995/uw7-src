#	@(#) vidparse.mk 12.1 95/05/09 SCOINC
#
#	Copyright (C) The Santa Cruz Operation, 1984-1993.
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.

# vidparse.mk
#

include ../../make.inc

#CC 		= cc
LIBDIR		= $(DEVSYS)/usr/lib
LIBRARIES 	= -lmalloc -ll 
DEBUG 		= 
#CFLAGS 		= $(DEBUG)
DEST		= $(ROOT)/usr/lib/vidconf

OBJS 		= main.o lex.yy.o parse.o data.o error.o
SRC 		= ($OBJ:.o=.c)


.SUFFIXES: .c .o

.c.o : 
	$(CC) -c $(CFLAGS) $<


#
# target
#
vidparse : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBRARIES)

debug : clean
	make -f vidparse.mk DEBUG=-DDEBUG


#
# dependencies
#
main.o : main.c vidparse.h
lex.yy.o : lex.yy.c
lex.yy.c : lex.l vptokens.h
	lex lex.l
parse.o : parse.c vidparse.h vptokens.h
data.o : data.c vidparse.h
error.o : error.c 

#
# cleanup
#
clean :
	rm -f *.o vidparse core

clobber : clean
	rm -f lex.yy.c


#
# distribution
#
dist install : $(DEST) $(DEST)/vidparse $(DEST)/mkcfiles $(DEST)/mkcfiles.h $(DEST)/update.sh

$(DEST) :
	[ -d $(DEST) ] || mkdir -p $(DEST)

$(DEST)/vidparse : vidparse
	cp vidparse $(DEST)/vidparse
	chmod +x $(DEST)/vidparse

$(DEST)/mkcfiles : mkcfiles
	cp mkcfiles $(DEST)/mkcfiles
	chmod +x $(DEST)/mkcfiles

$(DEST)/mkcfiles.h : mkcfiles.h
	cp mkcfiles.h $(DEST)/mkcfiles.h

$(DEST)/update.sh : update.sh 
	cp update.sh $(DEST)/update.sh
	chmod +x $(DEST)/update.sh
