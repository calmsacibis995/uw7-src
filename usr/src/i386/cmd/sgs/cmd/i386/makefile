#ident	"@(#)sgs-cmd:i386/makefile	1.1.1.26"

include $(CMDRULES)

INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

HFILES = $(CPUDIR)/machdep.h $(COMDIR)/cc.h $(CPUINC)/paths.h $(CPUINC)/sgs.h

SOURCES = $(COMDIR)/cc.c $(CPUDIR)/machdep.c

CC_OBJECTS = CC.d/cc.o CC.d/machdep.o
cc_OBJECTS = cc.d/cc.o cc.d/machdep.o

OBJECTS = $(CC_OBJECTS) $(cc_OBJECTS)

PRODUCTS = cc CC 

CC_DEFLIST = -D$(ARCH) -D$(CPU) -DCCflag=1 \
	     -DIL_SHOULD_BE_WRITTEN_TO_FILE=1 \
	     -DASFILT=1 $(SGS_COMPAT)\
             -DAUTOMATIC_TEMPLATE_INSTANTIATION=1 \
             -DDEFAULT_AUTOMATIC_INSTANTIATION_MODE=1 \
	     -DINSTANTIATION_BY_IMPLICIT_INCLUSION=1 \
	     -DDEFAULT_IMPLICIT_TEMPLATE_INCLUSION_MODE=1
#		the above four defines *must* agree with the settings
#		in the C++ front end (cplusfe:common/lang_feat.h)!

cc_DEFLIST = -D$(ARCH) -D$(CPU) -DCCflag=0 $(SGS_COMPAT)

INCLIST=-I. -I$(COMDIR) -I$(CPUINC) -I$(INC) -I$(COMINC)

CC_CMD = $(CC) $(CFLAGS) $(CC_DEFLIST) $(INCLIST)
cc_CMD = $(CC) $(CFLAGS) $(cc_DEFLIST) $(INCLIST)

.MUTEX: all $(INSDIR)/$(SGS)cc $(INSDIR)/$(SGS)CC $(INSDIR)/$(SGS)c89

.MUTEX: cc.d CC.d $(PRODUCTS)

all: cc.d CC.d  $(PRODUCTS)
	
cc : $(cc_OBJECTS)
	$(CC) $(cc_OBJECTS) $(LINK_MODE) $(LIBSGS) -o $(@)
	echo "-lnsl\n-lsocket" > libxnet.a

CC : $(CC_OBJECTS)
	$(CC) $(CC_OBJECTS) $(LINK_MODE) $(LIBSGS) -o $(@)

cc.d:
	mkdir $(@)

CC.d:
	mkdir $(@)


.MUTEX:	all do_install_cc do_install_CC

install : all do_install_cc do_install_CC 

do_install_cc : $(INSDIR)/$(SGS)cc

do_install_CC : $(INSDIR)/$(SGS)CC


$(INSDIR)/$(SGS)cc : cc
	$(MV) cc cc.bak
	$(CP) cc.bak cc
	$(STRIP) cc
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(@) cc
	$(MV) cc.bak cc
	ln $(INSDIR)/$(SGS)cc $(INSDIR)/$(SGS)c89
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(CCSLIB)/libxnet.a libxnet.a


$(INSDIR)/$(SGS)CC : CC
	$(MV) CC CC.bak
	$(CP) CC.bak CC
	$(STRIP) CC
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(@) CC
	$(MV) CC.bak CC

lintit:	$(HFILES) $(SOURCES)
	$(LINT) $(LINTFLAGS) $(CC_DEFLIST) $(INCLIST) $(SOURCES)


clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(PRODUCTS)

cc.d/cc.o: $(HFILES) $(COMDIR)/cc.c
	$(cc_CMD) -c $(COMDIR)/cc.c && $(MV) $(@F) $(@)

cc.d/machdep.o : $(HFILES) $(CPUDIR)/machdep.c
	$(cc_CMD) -c $(CPUDIR)/machdep.c && $(MV) $(@F) $(@)

CC.d/cc.o : $(HFILES) $(COMDIR)/cc.c
	$(CC_CMD) -c $(COMDIR)/cc.c && $(MV) $(@F) $(@)

CC.d/machdep.o : $(HFILES) $(CPUDIR)/machdep.c
	$(CC_CMD) -c $(CPUDIR)/machdep.c && $(MV) $(@F) $(@)

