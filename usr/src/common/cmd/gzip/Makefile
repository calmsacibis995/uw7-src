#	Copyright (c) 1993 UNIVEL

#ident	"@(#)Makefile	1.2"
#ident "$Header$"

include $(CMDRULES)

CPP=/lib/cpp -DNO_UNDERLINE

# Things you might add to DEFS (configure usually figures out what to do):
# -DGNU_STANDARD        Behave as gzip even if invoked as gunzip (GNU standard)
# -DDIRENT              Use <dirent.h>  for recursion (-r)
# -DSYSDIR              Use <sys/dir.h> for recursion (-r)
# -DSYSNDIR             Use <sys/ndir.h> for recursion (-r)
# -DNDIR                Use <ndir.h> for recursion (-r)
# -DSTDC_HEADERS        Use <stdlib.h>
# -DHAVE_UNISTD_H	Use <unistd.h>
# -DNO_FCNTL_H          Don't use <fcntl.h>
# -DNO_UTIME_H		Don't use <utime.h>
# -DHAVE_SYSUTIME_H	Use <sys/utime.h>
# -DNO_MEMORY_H         Don't use <memory.h>. Not needed if STDC_HEADERS.
# -DNO_STRING_H         Use strings.h, not string.h. Not needed if STDC_HEADERS
# -DRETSIGTYPE=int      Define this if signal handlers must return an int.
# -DNO_SYMLINK          OS defines S_IFLNK but does not support symbolic links
# -DNO_MULTIPLE_DOTS    System does not allow file names with multiple dots
# -DNO_UTIME		System does not support setting file modification time
# -DNO_CHOWN		System does not support setting file owner
# -DNO_DIR		System does not support readdir()
# -DPROTO		Force function prototypes even if __STDC__ not defined
# -DASMV		Use asm version match.S
# -DDEBUG		Debug code
# -DDYN_ALLOC		Use dynamic allocation of large data structures
# -DRECORD_IO           read() and write() are rounded to record sizes.
# -DNO_STDIN_FSTAT      fstat() is not available on stdin
# -DNO_FSTAT            fstat() is not available
# -DNO_SIZE_CHECK       stat() does not give a reliable file size

DEFS =  -DASMV -DSTDC_HEADERS=1 -DHAVE_UNISTD_H=1 -DDIRENT=1
LIBS = 

# additional assembly sources for particular systems may be required.
OBJA = match.o

CFLAGS = 

# If you want debug on by default, use: CFLAGS="-g"

LDFLAGS = $(CFLAGS)

OBJS = gzip.o zip.o deflate.o trees.o bits.o unzip.o inflate.o util.o \
       crypt.o lzw.o unlzw.o unpack.o unlzh.o getopt.o $(OBJA)

.c.o:
	$(CC) -O -c $(DEFS) $(CFLAGS) $<

default:  gzip
all:	gzip
force:

#### Start of specific targets section. ####
#
# 'configure' works only on Unix systems. For other systems able to make
# sense of this makefile, you can define target specific entries here.
# For other systems such as MSDOS, separate Makefiles are
# provided in subdirectories.

clean:
	rm -f gzip gunzip $(OBJS)

gzip:	Makefile $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	rm -f gunzip
	ln gzip gunzip

gzip.o zip.o deflate.o trees.o bits.o unzip.o inflate.o: gzip.h tailor.h
util.o lzw.o unlzw.o unpack.o unlzh.o crypt.o: gzip.h tailor.h

gzip.o unlzw.o: revision.h lzw.h

bits.o unzip.o util.o zip.o: crypt.h

gzip.o getopt.o: getopt.h

match.o: match.S
	$(CPP) match.S > _match.s
	$(CC) -c _match.s
	mv _match.o match.o
	rm -f _match.s
