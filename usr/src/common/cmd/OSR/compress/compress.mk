#ident	"@(#)OSRcmds:compress/compress.mk	1.2"
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
DBITS	= -DBITS=16

SRCS=compress.c     huf-coding.c   huf-table.c    huf-tree.c \
     lzh-decode.c   lzh-encode.c   lzh-io.c       lzh-util.c
OBJS=$(SRCS:.c=.o)
OSRSTUFF=	../lib/libos/catgets_sa.o \
		../lib/libos/errorl.o \
		../lib/libos/errorv.o \
		../lib/libos/libos_intl.o \
		../lib/libos/nl_confirm.o \
		../lib/libos/psyserrorl.o \
		../lib/libos/psyserrorv.o \
		../lib/libos/sysmsgstr.o

INTL	= -DINTL
# CFLAGS	= -O $(XPGUTIL)
CFLAGS	= -O -s

MSG_SRC = compress.gen
MSG_INC = compress_msg.h


all:	compress

install:	all
	$(DOCATS) -d NLS $@
	$(INS) -f $(DBIN) compress

cmp:	all
	cmp	compress	$(DBIN)/compress

clean:
	rm -f	$(OBJS) $(MSG_INC)

clobber: clean
	rm -f	compress
	$(DOCATS) -d NLS $@

compress: $(OBJS) $(MSG_BIN)
	$(CC) $(CFLAGS) $(OBJS) $(OSRSTUFF) -o $@

compress.o: compress.c $(MSG_INC) ../include/osr.h
	$(CC) $(DBITS) $(CFLAGS) -c compress.c $(LDFLAGS)

$(MSG_INC):	NLS/en/$(MSG_SRC)
	$(MKCATDEFS) compress $? >/dev/null
	cat compress_msg.h | sed 's/"compress.cat@Unix"/"OSRcompress.cat@Unix"/g' > compress_msgt.h
	mv compress_msgt.h compress_msg.h

huf-coding.o: huf-coding.c lzh-defs.h
huf-table.o:  huf-table.c  lzh-defs.h
huf-tree.o:   huf-tree.c   lzh-defs.h
lzh-decode.o: lzh-decode.c lzh-defs.h
lzh-encode.o: lzh-encode.c lzh-defs.h
lzh-io.o:     lzh-io.c     lzh-defs.h
lzh-util.o:   lzh-util.c   lzh-defs.h

lint:
	lint $(DBITS) compress.c
