#ident	"@(#)pcintf:util/util.mk	1.1"
#! /bin/make -f
#
#
# Warning: The Merge version does not use this makefile.
#
# This makefile is a version of util/Makefile to comply with the Unix System 
# V Makefile Guidelines from USL.
# The assumption is made that "CFLAGS = -O" in the CMDRULES file.  It is also
# assumed that INC, LDFLAGS, LINT, and LINTFLAGS are set in this file.
#
# Defines for lmf source directory

include $(CMDRULES)

SRC=../../../util
LMF_SRC=../../../pkg_lmf
LCS_SRC=../../../pkg_lcs
INCLUDES=-I$(LMF_SRC) -I$(LCS_SRC) -I$(INC)
LOCDEFLIST=$(DFLAGS) $(INCLUDES)
LIBS=../pkg_lmf/liblmf.a ../pkg_lcs/liblcs.a
LIBFLAGS=$(LIBFLAGS)
OBJS=convert.o getopt.o

default: all
all: convert

../pkg_lcs/liblcs.a:
	cd $(LCS_SRC); $(MAKE) -f lcs.mk

../pkg_lmf/liblmf.a:
	cd $(LMF_SRC); $(MAKE) -f lmf.mk

getopt.o: ${SRC}/getopt.c 
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) ${SRC}/getopt.c

convert.o: ${SRC}/convert.c 
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) ${SRC}/convert.c

convert: $(OBJS) $(LIBS)
	$(CC) -o $@ $(CFLAGS) $(LOCDEFLIST) $(LDFLAGS) $(OBJS) $(LIBS) $(LIBFLAGS)
	rm -f dos2unix unix2dos aix2dos dos2aix charconv
	ln convert dos2unix
	ln convert unix2dos
	ln convert aix2dos
	ln convert dos2aix
	ln convert charconv

clean:
	rm -f convert.o getopt.o

clobber: clean
	rm -f convert dos2unix unix2dos aix2dos dos2aix charconv

# lint the files
lintit:
	$(LINT) $(LINTFLAGS) $(LOCDEFLIST) $(SRC)/*.c
