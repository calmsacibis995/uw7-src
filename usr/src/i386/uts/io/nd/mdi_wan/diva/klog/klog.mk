#ident "@(#)klog.mk	29.1"
#
#   Name : klog.gemini.M
#
#   COPYRIGHT NOTICE:
#
#   Copyright (C) Eicon Technology Corporation, 1993.
#   This module contains Proprietary Information of
#   Eicon Technology Corporation, and should be treated
#   as confidential.
#
#   Description :
#   Gemini Makefile for lib.idi component

include $(UTSRULES)

# Directories of interest:

MAKEFILE = klog.mk
SCP_SRC = klog.c
KBASE = ../../../../..
# DEBUG = -DDEBUG
LOCALDEF = -DUNIX -DUNIXWARE -DNEW_DRIVER
ECKLOG = ../EtdK.cf/ecklog
ECKLOGD = ../EtdK.cf/ecklogd
EXE = $(ECKLOG) $(ECKLOGD) ktest
KLOG_OBJ = klog.o kloglib.o log.o
KLOG_LOBJS = klog.L kloglib.L log.L
KLOGD_OBJ = klogd.o kloglib.o
KLOGD_LOBJS = klogd.L kloglib.L

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

# not kernel code, don't use in-kernel defines in DEFLIST

.c.o:
	$(CC) $(CFLAGS) $(INCLIST) $(LOCALDEF) -D_KMEMUSER -D_KERNEL_HEADERS -c $<

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(EXE) $(MAKEARGS); \
	fi

$(ECKLOG): $(KLOG_OBJ)
	$(CC) -o $@ $(KLOG_OBJ)   

$(ECKLOGD): $(KLOGD_OBJ)
	$(CC) -o $@ $(KLOGD_OBJ)   

ktest: ktest.c
	$(CC) -o $@ ktest.c

lintit: $(KLOG_LOBJS) $(KLOGD_LOBJS)

install:

clean:
	rm -f $(KLOG_OBJ) $(KLOGD_OBJ) ktest

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(EXE); \
	fi


