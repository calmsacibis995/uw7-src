#ident	"@(#)sccs:help.d/help.mk	6.12"
#
#

include $(CMDRULES)

HELPLIB = $(CCSLIB)/help
UW_HELPLIB = $(UW_CCSLIB)/help
OSR5_HELPLIB = $(OSR5_CCSLIB)/help

FILES1 = ad bd cb cm cmds co de default
FILES2 = ge he prs rc un ut

all:

install: all
	[ -d $(HELPLIB) ] || mkdir -p $(HELPLIB)
	$(CH)-chmod 775 $(HELPLIB)
	-cd $(HELPLIB); rm -f $(FILES2) $(FILES2)
	cp $(FILES1) $(FILES2) $(HELPLIB)
	-cd $(HELPLIB); $(CH)chmod 664 $(FILES1)	$(FILES2)
	-@cd $(HELPLIB); $(CH)chgrp $(GRP) $(FILES1) $(FILES2) .
	-@cd $(HELPLIB); $(CH)chown $(OWN) $(FILES1) $(FILES2) .
	[ -d $(UW_HELPLIB) ] || mkdir -p $(UW_HELPLIB)
	$(CH)-chmod 775 $(UW_HELPLIB)
	-cd $(UW_HELPLIB); rm -f $(FILES2) $(FILES2)
	cp $(FILES1) $(FILES2) $(UW_HELPLIB)
	-cd $(UW_HELPLIB); $(CH)chmod 664 $(FILES1)	$(FILES2)
	-@cd $(UW_HELPLIB); $(CH)chgrp $(GRP) $(FILES1) $(FILES2) .
	-@cd $(UW_HELPLIB); $(CH)chown $(OWN) $(FILES1) $(FILES2) .
	[ -d $(OSR5_HELPLIB) ] || mkdir -p $(OSR5_HELPLIB)
	$(CH)-chmod 775 $(OSR5_HELPLIB)
	-cd $(OSR5_HELPLIB); rm -f $(FILES2) $(FILES2)
	cp $(FILES1) $(FILES2) $(OSR5_HELPLIB)
	-cd $(OSR5_HELPLIB); $(CH)chmod 664 $(FILES1)	$(FILES2)
	-@cd $(OSR5_HELPLIB); $(CH)chgrp $(GRP) $(FILES1) $(FILES2) .
	-@cd $(OSR5_HELPLIB); $(CH)chown $(OWN) $(FILES1) $(FILES2) .

clean:

clobber:
