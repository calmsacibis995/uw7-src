#ident	"@(#)cplusbe:i386/Makefile	1.19"

#	C++ back end makefile
#
#	note: currently this only makes the "glue" part of the C++ back end
#	      it needs to have real dependencies upon acomp, cg, et al.

#patch it all together

include $(CMDRULES)

INS=$(SGSBASE)/sgs.install
INSDIR= $(CCSLIB)

CGDIR = ../../cg
AMDIR = ../../amigo
ACDIR = ../../acomp
FEDIR = ../../cplusfe
FPDIR = ../../fpemu
o=o

NODBG=	-DNODBG
EDGDEBUG=0

SCPOBJS = ac_cgs.o ac_err.o cg_be.o cg_eh.o cg_sm.o cg_sy.o \
	cg_tr.o cg_ty.o ehtables.o elfdebug.o

GLUEOBJS = intemu.$o

RESTOBJS = $(CGDIR)/$(CPU)/cg.o $(AMDIR)/$(CPU)/amigo.o \
	$(FEDIR)/$(CPU)/be.o $(FEDIR)/$(CPU)/usr_include.o \
	$(ACDIR)/$(CPU)/cgstuff.o $(ACDIR)/$(CPU)/init.o \
	$(ACDIR)/$(CPU)/file.o \
	$(ACDIR)/$(CPU)/expand.o $(ACDIR)/$(CPU)/p1allo.o \
	$(ACDIR)/$(CPU)/optim.o $(ACDIR)/$(CPU)/types.o

INCLIST = -I../$(CPU) -I../common \
	-I$(FEDIR)/$(CPU) -I$(FEDIR)/common \
	-I$(ACDIR)/$(CPU) -I$(ACDIR)/common \
	-I$(CGDIR)/$(CPU) -I$(CGDIR)/common \
	-I$(FPDIR)/$(CPU) -I../../inc/$(CPU) -I../../inc/common

PROBEFILE = $(FEDIR)/common/fe_common.h

PRODUCTS = c++be asfilt

# long long runtime needed for SCP build of XENV (not part of fe.o
#  delivered in SCP)
LONG_LONG_OBJECTS = $(FEDIR)/$(CPU)/llasgmul.o $(FEDIR)/$(CPU)/lldivrem.o

BINARIES = c++be.o

DEFLIST = -DEDGDEBUG=$(EDGDEBUG) $(NODBG) \
	-DCHECKING -DIL_SHOULD_BE_WRITTEN_TO_FILE \
	-DFP_EMULATE -DCOMPVERS='"01.01"' \
	-DOPTIM_SUPPORT -DGENERATING_DEBUG_INFO \
	-DGENERATE_EH_TABLES \
	-DDIRECTORY_PRAGMAS_ALLOWED

NUMSIZE	= -DNUMSIZE=64 -DNUM_LONG_LONG

CC_CMD = $(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST)

all:
	if [ -f $(PROBEFILE) ]; \
	then \
		$(MAKE) binaries $(MAKEARGS) EDGDEBUG=$(EDGDEBUG) NODBG=$(NODBG) ; \
	else \
		if test "$(NATIVE)" = "yes"; \
		then \
			dir=native; \
		else \
			dir=cross; \
			for file in $(LONG_LONG_OBJECTS); \
			do \
				if [ ! -r $$file ]; \
				then \
					echo "ERROR: $$file is missing" 1>&2; \
					false; \
					break; \
				fi \
			done \
		fi; \
		for file in $(BINARIES); \
		do \
			rm -f $$file; \
			if [ -r $$dir/$$file ]; \
			then \
				cp $$dir/$$file $$file; \
			else \
				echo "ERROR: $$dir/$$file is missing" 1>&2; \
				false; \
				break; \
			fi \
		done \
	fi

binaries:	c++be asfilt
	if test "$(NATIVE)" = "yes"; \
	then \
		rm -rf native; \
		mkdir native; \
		cp $(BINARIES) native; \
	else \
		rm -rf cross; \
		mkdir cross; \
		cp $(BINARIES) cross; \
	fi

.MUTEX:		all $(INSDIR)/$(SGS)c++be $(INSDIR)/$(SGS)asfilt

install:	all $(INSDIR)/$(SGS)c++be $(INSDIR)/$(SGS)asfilt

$(INSDIR)/$(SGS)c++be:	c++be.o $(GLUEOBJS) $(RESTOBJS)
	if test "$(NATIVE)" = "yes" ; then \
	    $(CC) $(CFLAGS) $(LDFLAGS) -o c++be c++be.o $(GLUEOBJS) $(RESTOBJS); \
	else \
	    $(CC) $(CFLAGS) $(LDFLAGS) -o c++be c++be.o $(LONG_LONG_OBJECTS) $(GLUEOBJS) $(RESTOBJS); \
	fi
	$(MV) c++be c++be.bak
	$(CP) c++be.bak c++be
	$(STRIP) c++be
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(CCSLIB)/$(SGS)c++be c++be
	$(MV) c++be.bak c++be

$(INSDIR)/$(SGS)asfilt:	asfilt
	$(MV) asfilt asfilt.bak
	$(CP) asfilt.bak asfilt
	$(STRIP) asfilt
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(CCSLIB)/$(SGS)asfilt asfilt
	$(MV) asfilt.bak asfilt


c++be: $(SCPOBJS) $(GLUEOBJS) $(RESTOBJS)
	$(LD) -r $(LDFLAGS) $(SCPOBJS) -o c++be.o
	if test "$(NATIVE)" = "yes" ; then \
	    $(CC) $(CFLAGS) $(LDFLAGS) -o c++be c++be.o $(GLUEOBJS) $(RESTOBJS); \
	else \
	    $(CC) $(CFLAGS) $(LDFLAGS) -o c++be c++be.o $(LONG_LONG_OBJECTS) $(GLUEOBJS) $(RESTOBJS); \
	fi

ac_cgs.o: ../common/ac_cgs.c
	$(CC_CMD) $?
ac_err.o: ../common/ac_err.c
	$(CC_CMD) $?
cg_be.o: ../common/cg_be.c
	$(CC_CMD) $?
cg_eh.o: ../common/cg_eh.c
	$(CC_CMD) $?
cg_sm.o: ../common/cg_sm.c
	$(CC_CMD) $?
cg_sy.o: ../common/cg_sy.c
	$(CC_CMD) $?
cg_tr.o: ../common/cg_tr.c
	$(CC_CMD) $?
cg_ty.o: ../common/cg_ty.c
	$(CC_CMD) $?
ehtables.o: ../common/ehtables.c
	$(CC_CMD) $?
elfdebug.o: ../common/elfdebug.c
	$(CC_CMD) $?

include ../../intemu/common/mkdeps

asfilt:	asfilt.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o asfilt asfilt.o

asfilt.o: ../common/asfilt.c
	$(CC) -c $(CFLAGS) $?

clean:
	rm -f $(SCPOBJS) $(GLUEOBJS) asfilt.o

# Note - native and cross subdirectories don't get clobbered! Needed
# for SCP build

clobber: clean
	rm -f $(PRODUCTS) $(BINARIES)
