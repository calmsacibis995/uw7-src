#ident	"@(#)Makefile	11.1"

SHELL =		/bin/sh

LOCALDEF =	-Di386 -DSVR4 -DSVR4_1 -DSVr4 -DSVr4_1

# This lib is only used by mailx, so it's as if it were part of
# the command modules anyways. No need to change CMDRULES to 
# LIBRULES for this.
include	$(CMDRULES)

LIB =		libmailx.a

OBJS =		Strerror.o \
		bcollapse.o \
		cascmp.o \
		casncmp.o \
		copystream.o \
		delempty.o \
		getdomain.o \
		mailsystem.o \
		mgetcharset.o \
		mgetenv.o \
		skip2space.o \
		skipspace.o \
		strmove.o \
		trimnl.o \
		xgetenv.o

$(LIB):		$(OBJS)
		ar ruv $(LIB) $?

clean:
		rm -f $(OBJS)

clobber:	clean
		rm -f $(LIB)
