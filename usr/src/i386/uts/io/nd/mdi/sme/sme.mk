########
#
#	SME Ethernet: Kernel-level driver
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

MAKEFILE = sme.mk
KBASE = ../../../..

#
# Local Extensions to make macros
#
SMCFLG=-DGEMINI -DLM8000
LOCALDEF=-DUNIX -DCODE_386 -DUNIXWARE_2 -DETHERNET $(SMCFLG)
SCP_SRC = smcumac.c
SME = sme.cf/Driver.o

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(SME) $(MAKEARGS); \
	fi

#
# Composition of the components
#
SME_OBJ=lme.o getcnfg.o smc8000.o smcumac.o

#
# An explicit rule is necessary, to provide for proper specification of
# preprocesor variables, stripping of the target, and installation
#
$(SME):		$(SME_OBJ)
	$(LD) -r $(SME_OBJ) -o $(SME)

########
#
#	All dependencies and rules not explicitly stated
#	(including header and nested header dependencies)
#
########


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
		rm -f $(SME); \
	fi
	(cd sme.cf; $(IDINSTALL) -R$(CONF) -d -e sme)

install:	all
	(cd sme.cf; $(IDINSTALL) -R$(CONF) -M sme)

