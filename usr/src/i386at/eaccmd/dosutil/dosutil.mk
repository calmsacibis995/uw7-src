#	copyright	"%c%"

#ident	"@(#)eac:i386at/eaccmd/dosutil/dosutil.mk	1.1"
#ident  "$Header$"

include $(CMDRULES)

#	@(#) dosutil.mk 22.1 89/11/14 
#
#	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987.
#	Copyright (C) Microsoft Corporation, 1984, 1985, 1986, 1987.
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, Microsoft Corporation
#	and AT&T, and should be treated as Confidential.

install:
	-echo "$(MAKE) -f doscmd_i386.mk $(MAKEARGS) install " 
	$(MAKE) -f doscmd_i386.mk $(MAKEARGS) install
	-echo "cd dosslice && $(MAKE) -f dosslice.mk $(MAKEARGS) install " 
	cd dosslice && $(MAKE) -f dosslice.mk $(MAKEARGS) install

clean: 
	-echo "$(MAKE) -f doscmd_i386.mk $(MAKEARGS) clean "
	$(MAKE) -f doscmd_i386.mk $(MAKEARGS) clean
	-echo "cd dosslice && $(MAKE) -f dosslice.mk $(MAKEARGS) clean " 
	cd dosslice && $(MAKE) -f dosslice.mk $(MAKEARGS) clean

clobber: clean
	-echo "$(MAKE) -f doscmd_i386.mk $(MAKEARGS) clobber "
	$(MAKE) -f doscmd_i386.mk $(MAKEARGS) clobber
	-echo "cd dosslice && $(MAKE) -f dosslice.mk $(MAKEARGS) clobber " 
	cd dosslice && $(MAKE) -f dosslice.mk $(MAKEARGS) clobber

lintit:
	-echo "$(MAKE) -f doscmd_i386.mk $(MAKEARGS) lintit "
	$(MAKE) -f doscmd_i386.mk $(MAKEARGS) lintit
	-echo "cd dosslice && $(MAKE) -f dosslice.mk $(MAKEARGS) lintit " 
	cd dosslice && $(MAKE) -f dosslice.mk $(MAKEARGS) lintit

