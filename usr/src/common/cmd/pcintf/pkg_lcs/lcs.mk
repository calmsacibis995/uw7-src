#ident	"@(#)pcintf:pkg_lcs/lcs.mk	1.1"
#
#  Makefile for LMF Library
#
# This makefile is a version of pkg_lcs/Makefile to comply with the Unix System 
# V Makefile Guidelines from USL.
#

include	$(CMDRULES)

SRC=../../../pkg_lcs
LOCAL_CC=$(HCC)
LOC_CCDEFS= -I$(SRC) -I/usr/include $(DFLAGS)
CCDEFLIST= -I$(SRC) -I$(INC) $(DFLAGS)

HEADERS=	$(SRC)/lcs.h $(SRC)/lcs_int.h

all: lcsgen.bld lcsgen lcsdump testtrc testmap liblcs.a \
	8859.lcs  pc437.lcs pc850.lcs \
	pc860.lcs pc863.lcs pc865.lcs

lcsgen.bld: $(SRC)/lcsgen.c $(SRC)/stricmp.c
	$(LOCAL_CC) $(CFLAGS) $(LOC_CCDEFS) -o lcsgen.bld $(SRC)/lcsgen.c $(SRC)/stricmp.c 
	rm -f lcsgen.o stricmp.o

lcsgen: lcsgen.o stricmp.o
	$(CC) $(CFLAGS) $(CCDEFLIST) -o lcsgen lcsgen.o stricmp.o ${LIBFLAGS}

testmap: testmap.o liblcs.a
	$(CC) $(CFLAGS) $(CCDEFLIST) -o testmap testmap.o liblcs.a ${LIBFLAGS}

testtrc: testtrc.o liblcs.a
	$(CC) $(CFLAGS) $(CCDEFLIST) -o testtrc testtrc.o liblcs.a ${LIBFLAGS}

lcsdump: lcsdump.o liblcs.a
	$(CC) $(CFLAGS) $(CCDEFLIST) -o lcsdump lcsdump.o liblcs.a ${LIBFLAGS}

liblcs.a: get_tab.o set_tab.o prim.o t_string.o t_block.o caseconv.o

testmap.o:	$(SRC)/testmap.c $(HEADERS)
	$(CC) $(CFLAGS) $(CCDEFLIST) -c $(SRC)/$(@:.o=.c)

testtrc.o:	$(SRC)/testtrc.c $(HEADERS)
	$(CC) $(CFLAGS) $(CCDEFLIST) -c $(SRC)/$(@:.o=.c)

lcsgen.o:	$(SRC)/lcsgen.c $(HEADERS)
	$(CC) $(CFLAGS) $(CCDEFLIST) -c $(SRC)/$(@:.o=.c)

stricmp.o:	$(SRC)/stricmp.c $(HEADERS)
	$(CC) $(CFLAGS) $(CCDEFLIST) -c $(SRC)/$(@:.o=.c)

lcsdump.o:	$(SRC)/lcsdump.c $(HEADERS)
	$(CC) $(CFLAGS) $(CCDEFLIST) -c $(SRC)/$(@:.o=.c)

get_tab.o:	$(SRC)/get_tab.c $(HEADERS)
	$(CC) $(CFLAGS) $(CCDEFLIST) -c $(SRC)/$(@:.o=.c)
	$(AR) $(ARFLAGS) liblcs.a $@

set_tab.o:	$(SRC)/set_tab.c $(HEADERS)
	$(CC) $(CFLAGS) $(CCDEFLIST) -c $(SRC)/$(@:.o=.c)
	$(AR) $(ARFLAGS) liblcs.a $@

prim.o:		$(SRC)/prim.c $(HEADERS)
	$(CC) $(CFLAGS) $(CCDEFLIST) -c $(SRC)/$(@:.o=.c)
	$(AR) $(ARFLAGS) liblcs.a $@

t_string.o:	$(SRC)/t_string.c $(HEADERS)
	$(CC) $(CFLAGS) $(CCDEFLIST) -c $(SRC)/$(@:.o=.c)
	$(AR) $(ARFLAGS) liblcs.a $@

t_block.o:	$(SRC)/t_block.c $(HEADERS)
	$(CC) $(CFLAGS) $(CCDEFLIST) -c $(SRC)/$(@:.o=.c)
	$(AR) $(ARFLAGS) liblcs.a $@

caseconv.o:	$(SRC)/caseconv.c $(HEADERS)
	$(CC) $(CFLAGS) $(CCDEFLIST) -c $(SRC)/$(@:.o=.c)
	$(AR) $(ARFLAGS) liblcs.a $@

10646.lcs: $(SRC)/10646.cs lcsgen.bld
	./lcsgen.bld $(SRC)/10646.cs $@
6937.lcs: $(SRC)/6937.cs lcsgen.bld
	./lcsgen.bld $(SRC)/6937.cs $@
8859.lcs: $(SRC)/8859.cs lcsgen.bld
	./lcsgen.bld $(SRC)/8859.cs $@
euc-jis.lcs: $(SRC)/euc-jis.cs lcsgen.bld
	./lcsgen.bld $(SRC)/euc-jis.cs $@
pc437.lcs: $(SRC)/pc437.cs lcsgen.bld
	./lcsgen.bld $(SRC)/pc437.cs $@
pc850.lcs: $(SRC)/pc850.cs lcsgen.bld
	./lcsgen.bld $(SRC)/pc850.cs $@
pc860.lcs: $(SRC)/pc860.cs lcsgen.bld
	./lcsgen.bld $(SRC)/pc860.cs $@
pc863.lcs: $(SRC)/pc863.cs lcsgen.bld
	./lcsgen.bld $(SRC)/pc863.cs $@
pc865.lcs: $(SRC)/pc865.cs lcsgen.bld
	./lcsgen.bld $(SRC)/pc865.cs $@
sjis.lcs: $(SRC)/sjis.cs lcsgen.bld
	./lcsgen.bld $(SRC)/sjis.cs $@

# "clean" removes intermediate files that were made.
clean:
	rm -f caseconv.o get_tab.o lcsdump.o lcsgen.o
	rm -f prim.o set_tab.o stricmp.o t_block.o t_string.o
	rm -f testmap.o testtrc.o

# "clobber" removes all files that were made.
clobber: clean
	rm -f lcsgen.bld testtrc testmap
	rm -f lcsgen lcsdump liblcs.a
	rm -f *.lcs

# lint the files
lintit:
	$(LINT) $(LINTFLAGS) $(CCDEFLIST) $(SRC)/*.c
