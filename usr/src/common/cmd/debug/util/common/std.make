#ident	"@(#)debugger:util/common/std.make	1.6"

include ../../util/$(CPU)/$(MACHDEFS)
include ../../util/common/defs.make
include	../../util/common/CC.rules

SOURCES = $(CSOURCES) $(CCSOURCES)

clean:
	-rm -f *.o *.O y.* lex.yy.c

clobber:	clean
	rm -f $(TARGET) $(OSR5_TARGET)

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
	echo '#ident	"@(#)debugger:util/common/std.make	1.5"\n' > DEPEND
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
