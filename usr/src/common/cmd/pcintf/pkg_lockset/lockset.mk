#ident	"@(#)pcintf:pkg_lockset/lockset.mk	1.1"
#! /bin/make -f
#
# Locus Bridge Products Group - LOCKSET Package Makefile
#
# This makefile is a version of pkg_lockset/Makefile to comply with the Unix 
# System V Makefile Guidelines from USL.
#

include		$(CMDRULES)

UNIX=		../../..
SRC=		$(UNIX)/pkg_lockset

LIB_NAME=	liblset.a
INCLUDES=	-I$(SRC) -I$(UNIX)/bridge -I$(UNIX)/pkg_lmf \
		-I$(INC)
LOCDEFLIST=	$(INCLUDES)

LIB_OBJ=	\
		lockset.o \
		lset_estr.o \
		lsetclr.o \
		lsetcount.o \
		lsetnew.o \
		lsetrmv.o \
		lsetset.o \
		lsetuse.o \
		lsetunuse.o

# don't let the library be deleted when make gets killed

.PRECIOUS:	$(LIB_NAME)

# default target: library

all:		$(LIB_NAME)

$(LIB_NAME):	$(LIB_OBJ)

lockset.o:	$(SRC)/lockset.c
	$(CC) $(CFLAGS) $(LOCDEFLIST) -c $?
	$(AR) $(ARFLAGS) $(LIB_NAME) $@

lset_estr.o:	$(SRC)/lset_estr.c
	$(CC) $(CFLAGS) $(LOCDEFLIST) -c $?
	$(AR) $(ARFLAGS) $(LIB_NAME) $@

lsetclr.o:	$(SRC)/lsetclr.c
	$(CC) $(CFLAGS) $(LOCDEFLIST) -c $?
	$(AR) $(ARFLAGS) $(LIB_NAME) $@

lsetcount.o:	$(SRC)/lsetcount.c
	$(CC) $(CFLAGS) $(LOCDEFLIST) -c $?
	$(AR) $(ARFLAGS) $(LIB_NAME) $@

lsetnew.o:	$(SRC)/lsetnew.c
	$(CC) $(CFLAGS) $(LOCDEFLIST) -c $?
	$(AR) $(ARFLAGS) $(LIB_NAME) $@

lsetrmv.o:	$(SRC)/lsetrmv.c
	$(CC) $(CFLAGS) $(LOCDEFLIST) -c $?
	$(AR) $(ARFLAGS) $(LIB_NAME) $@

lsetset.o:	$(SRC)/lsetset.c
	$(CC) $(CFLAGS) $(LOCDEFLIST) -c $?
	$(AR) $(ARFLAGS) $(LIB_NAME) $@

lsetuse.o:	$(SRC)/lsetuse.c
	$(CC) $(CFLAGS) $(LOCDEFLIST) -c $?
	$(AR) $(ARFLAGS) $(LIB_NAME) $@

lsetunuse.o:	$(SRC)/lsetunuse.c
	$(CC) $(CFLAGS) $(LOCDEFLIST) -c $?
	$(AR) $(ARFLAGS) $(LIB_NAME) $@

# clean out generated files.

clean:
	rm -f $(LIB_OBJ) $(LIB_LINTS)

clobber: clean
	rm -f $(LIB_NAME)

# lint files


lintit:	
	$(LINT) $(LINTFLAGS) $(LOCDEFLIST) $(SRC)/*.c
