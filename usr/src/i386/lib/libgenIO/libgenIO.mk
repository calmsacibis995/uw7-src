#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libgenIO:i386/lib/libgenIO/libgenIO.mk	1.3.4.2"
#ident	"$Header$"

include $(LIBRULES)

# libgenIO.a has *not* (yet) been made re-entrant.  The defining of
# _REENTRANT below is only to get the per-thread handling on errno, so
# that a thread other than the initial thread of a process will get
# the right errno back from the libgenIO routines.  But only one
# thread per process can access the libgenIO routines at once.

DEFLIST = -D_REENTRANT

MAKEFILE = libgenIO.mk
LIBRARY = libgenIO.a
INCSYS=$(INC)/sys

OBJECTS =  g_init.o g_read.o g_write.o

SOURCES =  g_init.c g_read.c g_write.c

all:		$(LIBRARY)

$(LIBRARY):	$(LIBRARY)(g_write.o) $(LIBRARY)(g_read.o) \
		$(LIBRARY)(g_init.o) 


$(LIBRARY)(g_init.o):	 $(INC)/errno.h \
		 $(INCSYS)/errno.h $(INCSYS)/types.h \
		 $(INCSYS)/stat.h $(INC)/fcntl.h \
		 $(INCSYS)/fcntl.h $(INCSYS)/sysi86.h \
		 $(INC)/stdio.h $(INC)/string.h \
		 $(INC)/ftw.h $(INC)/libgenIO.h $(INCSYS)/mkdev.h

$(LIBRARY)(g_read.o):	 $(INC)/errno.h \
		 $(INCSYS)/errno.h $(INC)/libgenIO.h 

$(LIBRARY)(g_write.o):	 $(INC)/errno.h \
		 $(INCSYS)/errno.h $(INC)/libgenIO.h 

GLOBALINCS = $(INC)/errno.h $(INC)/fcntl.h $(INC)/ftw.h \
	$(INC)/stdio.h $(INC)/string.h $(INCSYS)/errno.h \
	$(INCSYS)/fcntl.h $(INCSYS)/stat.h \
	$(INCSYS)/sysi86.h $(INCSYS)/types.h 

.c.a:
	$(CC) $(CFLAGS) $(DEFLIST) -c $<
	$(AR) $(ARFLAGS) $@ $(<F:.c=.o)
	-rm -f $(<F:.c=.o)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(LIBRARY)

install: all
	$(INS) -f $(USRLIB) -m 644 $(LIBRARY)
