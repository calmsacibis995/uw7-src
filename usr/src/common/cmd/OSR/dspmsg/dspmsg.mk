#ident	"@(#)OSRcmds:dspmsg/dspmsg.mk	1.1"
#	@(#)dspmsg.mk 25.2 95/02/16 

include $(CMDRULES)
include ../make.inc

INSDIR=$(USRBIN)

#LDFLAGS = -s $(XPGUTIL) $(LDLIBS)
#CFLAGS = -O $(XPGUTIL) -I$(INC)
CFLAGS = -O -I$(INC)

SOURCES = dspmsg.c

OBJECTS = $(SOURCES:.c=.o)

LIBOSOBJS = ../lib/libos/catgets_sa.o \
		../lib/libos/errorl.o \
		../lib/libos/psyserrorl.o \
		../lib/libos/errorv.o \
		../lib/libos/psyserrorv.o \
		../lib/libos/errmsg_fp.o \
		../lib/libos/libos_intl.o \
		../lib/libos/sysmsgstr.o
all: dspmsg

dspmsg:	$(OBJECTS)
	$(CC) $(CFLAGS) -o dspmsg $(OBJECTS) $(LIBOSOBJS) $(LDFLAGS)

dspmsg.o:	dspmsg.c dspmsg_msg.h \
		$(INC)/stdio.h \
		$(INC)/stdlib.h \
		$(INC)/sys/types.h \
		$(INC)/sys/stat.h \
		$(INC)/errno.h $(INC)/sys/errno.h \
		$(INC)/string.h \
		$(INC)/nl_types.h \
		$(INC)/locale.h

dspmsg_msg.h:	NLS/en/dspmsg.gen
	$(MKCATDEFS) dspmsg $? >/dev/null

install: all
	$(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) dspmsg
	$(DOCATS) -d NLS $@

clean:
	rm -f $(OBJECTS) dspmsg_msg.h

clobber: clean
	rm -f dspmsg
	$(DOCATS) -d NLS $@

strip:
	$(STRIP) dspmsg

lintit:
	$(LINT) $(LINTFLAGS) dspmsg.c
