#ident	"@(#)pcintf:pkg_lmf/lmf.mk	1.1"
#
#  Makefile for LMF Message File Library
#
# This makefile is a version of pkg_lmf/Makefile to comply with the Unix System 
# V Makefile Guidelines from USL.
#

include 	$(CMDRULES)

SRC=../../../pkg_lmf
INCLUDES=	-I$(SRC) -I$(INC)
LOCDEFLIST=	$(INCLUDES) $(DFLAGS)
LIBFLAGS=

all: lmfgen.bld lmfgen lmfdump lmfmsg

lmfgen: lmfgen.o
	$(CC) $(CFLAGS) $(LOCDEFLIST) -o $@ lmfgen.o ${LIBFLAGS}

lmfgen.bld: $(SRC)/lmfgen.c $(SRC)/lmf.h $(SRC)/lmf_int.h
	rm -f lmfgen.o
	$(HCC) -I$(SRC) -o $@ $(SRC)/lmfgen.c 
	rm -f lmfgen.o

lmfdump: lmfdump.o
	$(CC) $(CFLAGS) $(LOCDEFLIST) -o $@ lmfdump.o ${LIBFLAGS}

lmfmsg: lmfmsg.o liblmf.a
	$(CC) $(CFLAGS) $(LOCDEFLIST) -o $@ $(LDFLAGS) lmfmsg.o liblmf.a ${LIBFLAGS}

liblmf.a: lmf_lib.o lmf_fmt.o
	$(AR) ruv $@ $?

lmfgen.o: $(SRC)/lmfgen.c $(SRC)/lmf.h $(SRC)/lmf_int.h
	$(CC) $(CFLAGS) $(LOCDEFLIST) -c $(SRC)/lmfgen.c

lmfdump.o: $(SRC)/lmfdump.c $(SRC)/lmf.h $(SRC)/lmf_int.h
	$(CC) $(CFLAGS) $(LOCDEFLIST) -c $(SRC)/lmfdump.c

lmfmsg.o: $(SRC)/lmfmsg.c $(SRC)/lmf.h
	$(CC) $(CFLAGS) $(LOCDEFLIST) -c $(SRC)/lmfmsg.c

lmf_lib.o: $(SRC)/lmf_lib.c $(SRC)/lmf.h $(SRC)/lmf_int.h
	$(CC) $(CFLAGS) $(LOCDEFLIST) -c $(SRC)/lmf_lib.c

lmf_fmt.o: $(SRC)/lmf_fmt.c $(SRC)/lmf.h
	$(CC) $(CFLAGS) $(LOCDEFLIST) -c $(SRC)/lmf_fmt.c

# "clean" removes intermediate files that were made.
clean:
	rm -f *.o

# "clobber" removes all files that were made.
clobber: clean
	rm -f lmfgen.bld
	rm -f lmfgen lmfdump lmfmsg
	rm -f liblmf.a

# lint the files
lintit:
	$(LINT) $(LINTFLAGS) $(LOCDEFLIST) $(SRC)/*.c
