#		copyright	"%c"
#ident	"@(#)kern-i386at:io/cgmtr/cgmtr.mk	1.3"
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
DIR = io/cgmtr
LFILE = $(LINTDIR)/cgmtr.ln
MAKEFILE = cgmtr.mk
CGMTR = cgmtr.cf/Driver.o

FILES =  \
	cgmtr_piu.o \
	cgmtr_test.o \
	cgmtr_mcu.o \
	cgmtr.o 

CFILES =  \
	cgmtr_piu.c \
	cgmtr_test.c \
	cgmtr_mcu.c \
	cgmtr.c 

LFILES = \
	cgmtr_piu.ln \
	cgmtr_test.ln \
	cgmtr_mcu.ln \
	cgmtr.ln 

all:	$(CGMTR) 

$(CGMTR): $(FILES)
	$(LD) -r -o $@ $(FILES)

install: all
	cd cgmtr.cf; $(IDINSTALL) -R$(CONF) -M cgmtr

sysHeaders = \
	cgmtr.h

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
	-$(MODADMIN) -U cgmtr
	-$(IDINSTALL) -R$(CONF) -e -d cgmtr
	-rm -f /dev/cgmtr
	-rm -f /usr/include/sys/cgmtr.h

clean:
	-rm -f $(FILES) $(CGMTR) 

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e cgmtr

$(FILES): cgmtr.mk cgmtr.h 
