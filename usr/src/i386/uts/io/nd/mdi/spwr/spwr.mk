########
#
#	SPWR Ethernet: Kernel-level driver
#
########
########
#
#		Copyright (c) 1988,1989,1990 AT&T
#			All Rights Reserved
#
#     THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#   The copyright notice above does not evidence any actual
#   or intended publication of such source code.
#
########

include $(UTSRULES)

MAKEFILE = spwr.mk
KBASE = ../../../..

#
# Local Extensions to make macros
#
SMCFLG=-DGEMINI -DEPIC -DLM9432 -DFREEBUFF -DSPWRDB -DSMC_DEBUG
LOCALDEF=-DUNIX -DCODE_386 -DUNIXWARE_2 -DETHERNET $(SMCFLG)
SCP_SRC = smcumac.c
SPWR = spwr.cf/Driver.o

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(SPWR) $(MAKEARGS); \
	fi

#
# Composition of the components
#
SPWR_OBJ=lm9432.o lm9432cf.o spwr_pci.o spwr_pci1.o smc9432.o smcumac.o

$(SPWR_OBJ): board_id.h eeprom2.h epic100.h epicreg.h lm9432.h lm9432_fn.h \
lm_macr.h lmstruct.h smc_landlp.h smc_snmp.h smcconst.h smcmacro.h smcprfix.h \
smctydef.h smcumac.h sme_space.h spwr.h spwr_space.h

#
# An explicit rule is necessary, to provide for proper specification of
# preprocesor variables, stripping of the target, and installation
#
$(SPWR):		$(SPWR_OBJ)
	$(LD) -r $(SPWR_OBJ) -o $(SPWR)

########
#
#	All dependencies and rules not explicitly stated
#	(including header and nested header dependencies)
#
########

./lm9432_fn.h: ./lm_macr.h
	sed -e "s/DRVNAME/spwr/g" ./lm_macr.h > ./lm9432_fn.h

########
#
#	Standard Targets
#
#	all		builds all the products specified by PRODUCTS
#	clean		removes all temporary files (ex. installable object)
#	clobber		"cleans", and then removes $(PRODUCTS)
#	install		installs products ; user defined in make.lo 
#
########

clean:
	rm -f *.o *.L

clobber:	clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(SPWR); \
	fi
	(cd spwr.cf; $(IDINSTALL) -R$(CONF) -d -e spwr)

install:	all
	(cd spwr.cf; $(IDINSTALL) -R$(CONF) -M spwr)

