#ident	"@(#)pcintf:pkg_rlock/rlock.mk	1.1"
#! /bin/make -f
#
# Locus Bridge Products Group - RLOCK Package Makefile
#
# This makefile is a version of pkg_rlock/makefile to comply with the Unix 
# System V Makefile Guidelines from USL.
#

include		$(CMDRULES)

UNIX=		../../..
SRC=		$(UNIX)/pkg_rlock

PROGS=		rlockshm
LIB_NAME=	librlock.a

INCLUDES=	-I$(SRC) -I$(UNIX)/bridge -I$(UNIX)/pkg_lockset \
		-I$(UNIX)/pkg_lmf -I$(INC)
LOCDEFLIST=	$(INCLUDES) $(DFLAGS)

LIBFLAGS=	./librlock.a \
		../pkg_lockset/liblset.a \
		../pkg_lmf/liblmf.a \
		$(LIBF)

LIB_OBJ=	\
		multi_init.o \
		multi_lock.o \
		multi_open.o \
		rlock_estr.o \
		rlock_init.o \
		rlock_prim.o \
		set_cfg.o \
		shm_lock.o \
		shm_open.o \
		state.o

RLOCKSHM_OBJ=	\
		rlockshm.o

# don't let the library be deleted when make gets killed

.PRECIOUS:	$(LIB_NAME)
.SUFFIXES:	.ln

# default target: library

all:		$(LIB_NAME) $(PROGS)

$(LIB_NAME):	$(LIB_OBJ)

# RLOCK shared memory manipulation

rlockshm:	$(RLOCKSHM_OBJ) $(LIB_NAME)
	-$(CC) -o $@ $(RLOCKSHM_OBJ) $(LDFLAGS) $(LIBFLAGS)

rlockshm.o:	$(SRC)/rlockshm.c
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $?

# Library components

multi_init.o:	$(SRC)/multi_init.c
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $?
	$(AR) $(ARFLAGS) $(LIB_NAME) $@

multi_lock.o:	$(SRC)/multi_lock.c
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $?
	$(AR) $(ARFLAGS) $(LIB_NAME) $@

multi_open.o:	$(SRC)/multi_open.c
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $?
	$(AR) $(ARFLAGS) $(LIB_NAME) $@

rlock_estr.o:	$(SRC)/rlock_estr.c
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $?
	$(AR) $(ARFLAGS) $(LIB_NAME) $@

rlock_init.o:	$(SRC)/rlock_init.c
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $?
	$(AR) $(ARFLAGS) $(LIB_NAME) $@

rlock_prim.o:	$(SRC)/rlock_prim.c
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $?
	$(AR) $(ARFLAGS) $(LIB_NAME) $@

set_cfg.o:	$(SRC)/set_cfg.c
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $?
	$(AR) $(ARFLAGS) $(LIB_NAME) $@

shm_lock.o:	$(SRC)/shm_lock.c
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $?
	$(AR) $(ARFLAGS) $(LIB_NAME) $@

shm_open.o:	$(SRC)/shm_open.c
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $?
	$(AR) $(ARFLAGS) $(LIB_NAME) $@

state.o:	$(SRC)/state.c
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $?
	$(AR) $(ARFLAGS) $(LIB_NAME) $@

# clean out generated files.

clean:
	rm -f $(LIB_OBJ) $(RLOCKSHM_OBJ)
	rm -f $(LIB_LINTS) $(RLOCKSHM_LINTS)

clobber: clean
	rm -f $(LIB_NAME) $(PROGS)
	rm -f $(LINT_LIB_NAME)

# lint filesf

lintit:	
	$(LINT) $(LINTFLAGS) $(LOCDEFLIST) $(SRC)/*.c
