#ident	"@(#)etc.mk	1.4"
#****************************************************************************
#*                                                                          *
#*   Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991, 1992,    *
#*                 1993, 1994  Novell, Inc. All Rights Reserved.            *
#*                                                                          *
#****************************************************************************
#*      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	    *
#*      The copyright notice above does not evidence any   	            *
#*      actual or intended publication of such source code.                 *
#****************************************************************************

include $(CMDRULES)

TOP = ..

include $(TOP)/local.def

MOSY = $(USRSBIN)/mosy

ETC_NM = $(ROOT)/$(MACH)/etc/netmgt

STD_DEFS = smi.defs mibII.defs nm.defs

MAIN_DEFS = unixwared.defs \
		snmpd.defs

UNIXWARE_DEFS = hr.defs

XMODE = 444
OWN = root
GRP = sys

all: $(MAIN_DEFS)

install: all
	$(INS) -f $(ETC_NM) -m $(XMODE) -u $(OWN) -g $(GRP) unixwared.defs
	$(INS) -f $(ETC_NM) -m $(XMODE) -u $(OWN) -g $(GRP) snmpd.defs
	$(INS) -f $(ETC_NM) -m $(XMODE) -u $(OWN) -g $(GRP) hr.mib
	$(INS) -f $(ETC_NM) -m $(XMODE) -u $(OWN) -g $(GRP) rmon.mib
	$(INS) -f $(ETC_NM) -m $(XMODE) -u $(OWN) -g $(GRP) nm.mib
	$(INS) -f $(ETC_NM) -m $(XMODE) -u $(OWN) -g $(GRP) mibII.my
	$(INS) -f $(ETC_NM) -m $(XMODE) -u $(OWN) -g $(GRP) smi.my

# Standard Definitions

smi.defs: smi.my
	$(MOSY) -o smi.defs -s smi.my

mibII.defs: mibII.my
	$(MOSY) -o mibII.defs -s mibII.my

nm.defs: nm.mib
	$(MOSY) -o nm.defs -s nm.mib

# SNMPD Definitions

snmpd.defs: $(NWUMPS_DEFS) $(STD_DEFS)
	cat smi.defs > snmpd.defs; \
	cat mibII.defs >> snmpd.defs; \
	cat nm.defs >> snmpd.defs; \
	cat hr.defs >> snmpd.defs;

# UnixWare Definitions

hr.defs: hr.mib
	$(MOSY) -o hr.defs -s hr.mib

unixwared.defs: $(UNIXWARE_DEFS) $(STD_DEFS)
	cat smi.defs > unixwared.defs; \
	cat mibII.defs >> unixwared.defs; \
	cat hr.defs >> unixwared.defs;

clean:
	rm -f $(STD_DEFS)
	rm -f $(UNIXWARE_DEFS)

clobber: clean
	rm -f $(MAIN_DEFS)
