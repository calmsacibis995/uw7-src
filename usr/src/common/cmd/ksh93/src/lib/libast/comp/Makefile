#ident	"@(#)ksh93:src/lib/libast/comp/Makefile	1.1"
#  ast library make

include $(LIBRULES)

CCFLAGS = -O $(CC.PIC)

INSTALLROOT = ../../../..

LOCALINC = -I$(INSTALLROOT)/src/lib/libast/include \
	-I$(INSTALLROOT)/src/lib/libast/obsolete \
	-I$(INSTALLROOT)/src/lib/libast/sfio \
	-I$(INSTALLROOT)/src/lib/libast/misc \
	-I$(INSTALLROOT)/src/lib/libast/dir \
	-I$(INSTALLROOT)/src/lib/libast/re \
	-I$(INSTALLROOT)/src/lib/libast/hash \
	-I$(INSTALLROOT)/src/lib/libast/string \
	-I$(INSTALLROOT)/src/lib/libast/man \
	-I$(INSTALLROOT)/src/lib/libast

HEADERSRC = 

SOURCES = atexit.c dup2.c errno.c execve.c fcntl.c \
	getgroups.c getpgrp.c getwd.c killpg.c link.c \
	lstat.c memccpy.c memchr.c memcmp.c memcpy.c \
	memmove.c memset.c mkdir.c mkfifo.c mknod.c \
	mktemp.c mount.c open.c readlink.c \
	remove.c rename.c rmdir.c setlocale.c setpgid.c \
	setsid.c sigunblock.c spawnve.c spawnveg.c statvfs.c strchr.c \
	strrchr.c strtod.c strtol.c strtoul.c symlink.c \
	system.c tmpnam.c unlink.c vfork.c waitpid.c

OBJECTS = $(SOURCES:.c=.o)

all:	$(OBJECTS)

clean:
	rm -f $(OBJECTS)

clobber:	clean

.c.o:
	$(CC) $(CCFLAGS) $(LOCALINC) $(GLOBALINC) -c $< && ln -f $@ ..
