#		copyright	"%c"
#ident	"@(#)kern-i386at:io/cpumtr/cpumtr.mk	1.3"
#ident	"$Header$"


# Copyright (c) 1996 International Computers Ltd
# Copyright (c) 1996 HAL Computer Systems Inc.
#
# make all        make the driver
# make install    install driver and .h file
# make uninstall  uninstall driver and .h file
# make clean      remove executables
#

include $(UTSRULES)
KBASE = ../..
LINTDIR = $(KBASE)/lindir
DIR = io/cpumtr
LFILE = $(LINTDIR)/cpumtr.ln
MAKEFILE = cpumtr.mk
CPUMTR = cpumtr.cf/Driver.o

FILES =  \
	cpumtr_p6.o \
	cpumtr.o 

CFILES =  \
	cpumtr_p6.c \
	cpumtr.c 

LFILES = \
	cpumtr_p6.ln \
	cpumtr.ln 

all:	$(CPUMTR) 

$(CPUMTR): $(FILES)
	$(LD) -r -o $@ $(FILES)

install: all
	cd cpumtr.cf; $(IDINSTALL) -R$(CONF) -M cpumtr

sysHeaders = \
	cpumtr.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	done

#
# Configuration Section
#
uninstall:
	-$(MODADMIN) -U cpumtr
	-$(IDINSTALL) -R$(CONF) -e -d cpumtr
	-rm -f /dev/cpumtr
	-rm -f /usr/include/sys/cpumtr.h

clean:
	-rm -f $(FILES) $(CPUMTR) 

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e cpumtr

$(FILES): cpumtr.mk cpumtr.h 
