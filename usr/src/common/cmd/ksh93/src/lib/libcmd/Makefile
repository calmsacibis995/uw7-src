#ident	"@(#)ksh93:src/lib/libcmd/Makefile	1.1.1.3"
# command library for commands small enough to be builtins

# :PACKAGE: ast
# VERSION = 1.1

include $(LIBRULES)

INSTALLROOT = ../../..

CCFLAGS = -O $(CC.PIC)

SOURCES = cmdinit.c cmdrecurse.c \
	chgrp.c chown.c comm.c \
	cut.c dirname.c fold.c getconf.c head.c id.c join.c logname.c \
	mkfifo.c paste.c pathchk.c rev.c rmdir.c tail.c tee.c \
	tty.c uniq.c \
	revlib.c

OBJECTS = $(SOURCES:.c=.o)

INCLIBS = 

LOCALINC = -I$(INSTALLROOT)/src/lib/libcmd -I$(INSTALLROOT)/src/lib/libast \
	-I$(INSTALLROOT)/src/lib/libast/include

all :	libcmd.a

clean :	
	rm -f $(OBJECTS)

clobber : clean
	rm -f libcmd.a

libcmd.a : $(OBJECTS)
	$(AR) $(ARFLAGS) libcmd.a $(OBJECTS) $(INCLIBS)

.c.o:
	$(CC) $(CCFLAGS) $(LOCALINC) $(GLOBALINC) -c $<
