#ident	"@(#)sccs:lib/cassi/cassi.mk	1.15"
#
#

include $(CMDRULES)


INCBASE=../../hdr

LIBRARY = ../cassi.a

PRODUCTS = $(LIBRARY)

LLIBRARY = llib-lcassi.ln

SGSBASE=../../..

INCLIST=-I$(SGSBASE)/sgs/inc/common

FILES = gf.o	\
	cmrcheck.o	\
	deltack.o	\
	error.o	\
	filehand.o

LFILES = gf.ln	\
	cmrcheck.ln	\
	deltack.ln	\
	error.ln	\
	filehand.ln

all: $(PRODUCTS)
	@echo "Library $(PRODUCTS) is up to date\n"

install:	$(PRODUCTS)

clean:
	-rm -f *.o
	-rm -f lint.out
	-rm -f *.ln

clobber:	clean
	-rm -f $(PRODUCTS)

.SUFFIXES : .o .c .e .r .f .y .yr .ye .l .s .ln

.c.ln:
	echo "$<:" >> lint.out
	$(LINT) $(CFLAGS) $(LINTFLAGS) $(INCLIST) $< -c >> lint.out

lintit: $(LLIBRARY)
	@echo "Library $(LLIBRARY) is up to date\n"

.c.o:
	$(CC) $(CFLAGS) $(INCLIST) -c $< 

$(LIBRARY): $(FILES)
	$(AR) cr $(LIBRARY) `$(LORDER) *.o | tsort`
	$(CH) chmod 664 $(LIBRARY)

$(LLIBRARY): $(LFILES)
	rm -f $(LLIBRARY)
	$(LINT) $(CFLAGS) $(LINTFLAGS) *.ln -o cassi >> lint.out
	ln -f $(LLIBRARY) ../$(LLIBRARY)

gf.ln:	gf.c $(INCBASE)/filehand.h
cmrcheck.ln:	cmrcheck.c $(INCBASE)/filehand.h
deltack.ln:	deltack.c $(INCBASE)/filehand.h $(INCBASE)/had.h $(INCBASE)/defines.h
filehand.ln:	filehand.c $(INCBASE)/filehand.h
error.ln:	error.c

gf.o:	gf.c	\
	 ../../hdr/filehand.h
cmrcheck.o:	cmrcheck.c	\
	 ../../hdr/filehand.h
deltack.o:	deltack.c	\
	 ../../hdr/filehand.h	\
	 ../../hdr/had.h	\
	 ../../hdr/defines.h
filehand.o:	filehand.c ../../hdr/filehand.h

