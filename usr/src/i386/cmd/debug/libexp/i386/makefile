#ident	"@(#)debugger:libexp/i386/makefile	1.18"

# this makefile includes modified contents of ../../util/common/std.make
# and ../../util/common/lib.make.  The modifications were needed to
# handle building without the cplusfe headers for source code customers

include $(CMDRULES)

TARGET = $(PRODLIB)/libexp.a

OSR5_TARGET = $(OSR5_LIB)/libexp.a

include ../../util/$(CPU)/$(MACHDEFS)
include ../../util/common/defs.make
include	../../util/common/CC.rules

CCSOURCES = \
	$(COMMON)/Call.C \
	CCcallMach.C \
	$(COMMON)/CCconvert.C \
	$(COMMON)/CCeval.C \
	$(COMMON)/CCexpr.C \
	$(COMMON)/CCnames.C \
	$(COMMON)/CCprint.C \
	$(COMMON)/CCresolver.C \
	$(COMMON)/CCtree.C \
	$(COMMON)/CCtype.C \
	$(COMMON)/CCutil.C \
	$(COMMON)/CCvalue.C \
	$(COMMON)/CCglue.C \
	$(COMMON)/CCtranslate.C \
	$(COMMON)/Debug_var.C \
	$(COMMON)/Dbgvarsupp.C \
	$(COMMON)/Expr.C \
	$(COMMON)/Print.C \
	$(COMMON)/Resolver.C \
	$(COMMON)/Rvalue.C \
	$(COMMON)/Value.C \
	ValueMach.C \
	cvt_util.C \
	$(COMMON)/Spaces.C

SOURCES = $(CSOURCES) $(CCSOURCES)

CFE_DIR=$(SGSBASE)/cplusfe
CFE_DEFS=-DCHECKING -D__ANSIC__ -DSHOBJ_SUPPORT -DUSL_DEBUGGER_SUPPORT \
	-DBACK_END_IS_C_GEN_BE=0
DFLAGS=-I$(CFE_DIR)/common -I$(CFE_DIR)/i386 -I$(LIBCINC) $(CFE_DEFS) -Tused
include DEPEND

PROBEFILE = $(CFE_DIR)/common/il_def.h

$(TARGET):	$(OBJECTS)
	rm -f $(TARGET)
	$(AR) -qc $(TARGET) $(OBJECTS)
	chmod 664 $(TARGET)

$(OSR5_TARGET):	$(OSR5_OBJECTS)
	rm -f $(OSR5_TARGET)
	$(AR) -qc $(OSR5_TARGET) $(OSR5_OBJECTS)
	chmod 664 $(OSR5_TARGET)

install:	all

all:
	if [ -f $(PROBEFILE) ]; \
	then \
		$(MAKE) $(TARGET) $(MAKEARGS) $(DEBUGARGS) ; \
	else \
		if [ ! -r $(TARGET) ]; \
		then \
			echo "ERROR: $(TARGET) is missing" 1>&2; \
			false; \
			break; \
		fi \
	fi

osr5:
	if [ -f $(PROBEFILE) ]; \
	then \
		$(MAKE) $(OSR5_TARGET) $(MAKEARGS) $(DEBUGARGS) ; \
	else \
		if [ ! -r $(OSR5_TARGET) ]; \
		then \
			echo "ERROR: $(OSR5_TARGET) is missing" 1>&2; \
			false; \
			break; \
		fi \
	fi

clobber:	clean
	-if [ -f $(PROBEFILE) ]; \
	then \
		rm -f $(TARGET) $(OSR5_TARGET); \
	fi

clean:
	-rm -f *.o y.* lex.yy.c

basedepend:
	rm -f BASEDEPEND OBJECT.list COBJECT.list
	@if [ "$(CCSOURCES)" ] ;\
		then echo "sh	../../util/common/depend $(CPLUS_CMD_FLAGS) $(CCSOURCES) >> BASEDEPEND" ; \
		CC=$(CC) sh ../../util/common/depend $(CPLUS_CMD_FLAGS) $(CCSOURCES) >> BASEDEPEND ; \
	fi
	@if [ "$(CSOURCES)" ] ;\
		then echo "sh	../../util/common/depend $(CC_CMD_FLAGS) $(CSOURCES) >> BASEDEPEND" ; \
		CC=$(CC) sh ../../util/common/depend $(CC_CMD_FLAGS) $(CSOURCES) >> BASEDEPEND ; \
	fi
	chmod 666 BASEDEPEND

depend:	local_depend
	rm -f DEPEND
	echo '#ident	"@(#)debugger:util/common/std.make	1.4"\n' > DEPEND
	chmod 644 DEPEND
	cat BASEDEPEND | \
		sh ../../util/common/substdir $(PRODINC) '$$(PRODINC)' | \
		sh ../../util/common/substdir $(SGSBASE) '$$(SGSBASE)' | \
		sh ../../util/common/substdir $(INCC) '$$(INCC)' | \
		sh ../../util/common/substdir $(INC) '$$(INC)' >> DEPEND
	sh ../../util/common/mkdefine OBJECTS < OBJECT.list >> DEPEND
	sh ../../util/common/mkdefine OSR5_OBJECTS < COBJECT.list >> DEPEND
	rm -f BASEDEPEND OBJECT.list COBJECT.list

local_depend:	basedepend

rebuild:	clobber depend all
