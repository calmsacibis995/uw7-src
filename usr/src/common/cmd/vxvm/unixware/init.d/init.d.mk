# @(#)src/cmd/vxvm/unixware/init.d/init.d.mk	1.1 10/16/96 02:17:34 - 
#ident	"@(#)cmd.vxvm:unixware/init.d/init.d.mk	1.2"

# Copyright(C)1996 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
# UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
# LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
# IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
# OR DISCLOSURE.
# 
# THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
# TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
# OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
# EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
# 
#               RESTRICTED RIGHTS LEGEND
# USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
# SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
# (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
# COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
#               VERITAS SOFTWARE
# 1600 PLYMOUTH STREET, MOUNTAIN VIEW, CA 94043

include $(CMDRULES)

# Force cmdrules include from $(TOOLS) to $(ROOT)/$(MACH)
INC = $(ROOT)/$(MACH)/usr/include

TARGETS = vxvm-sysboot vxvm-startup vxvm-reconfig vxvm-recover

.sh:
	cat $< > $@; chmod +x $@

all: $(TARGETS)

install: all
	for f in $(TARGETS) ; \
	do \
		rm -f $(ROOT)/$(MACH)//etc/init.d/$$f ; \
		$(INS) -f $(ROOT)/$(MACH)/etc/init.d -m 0444 -u root -g sys $$f ; \
	done

headinstall:

lint:
	@echo "Nothing to lint init.d"

lintclean:
	@echo "Nothing to lintclean init.d"

clean:

clobber: clean
	rm -f $(TARGETS)
