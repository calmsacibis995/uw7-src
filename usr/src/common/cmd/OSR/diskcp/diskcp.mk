#ident	"@(#)OSRcmds:diskcp/diskcp.mk	1.1"
#	@(#) compress.mk 25.5 95/03/06 
#
#	Copyright (C) The Santa Cruz Operation, 1984-1995.
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, Microsoft Corporation
#	and AT&T, and should be treated as Confidential.
#
#	Modification History
#	L001	9-Nov-92	scol!johnfa
#	- Added Message Catalogue dependencies
#	- Added $(XPGUTIL) to CFLAGS and LDFLAGS
#	- Added INTL definition
#	L002	3-Dec-92	scol!harveyt
#	- Created small floppy versions for N2 disk.
#	L003	20-Jan-94	scol!ianw
#	- Removed all references to the retired lzh-crc.c.
#	L004	15 Feb 95	scol!donaldp
#	- Added support for multiple message catalogues.
#	L005	 6 Mar 95	scol!ianw
#	- Removed support for the N2 floppy version (compress.fl),
#	  it is no longer required.
#

include $(CMDRULES)
include ../make.inc
DESTDIR = $(OSRDIR)

DBIN	= $(DESTDIR)

SRCS=test.sh

INTL	= -DINTL

all:	diskcp.sh
	cp diskcp.sh diskcp

install:	all
	$(INS) -f $(DBIN) diskcp

cmp:	all
	cmp	diskcp	$(DBIN)/diskcp

clean:
	rm -f	diskcp

clobber: clean

