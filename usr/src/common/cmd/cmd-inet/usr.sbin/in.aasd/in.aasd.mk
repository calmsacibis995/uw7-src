#	@(#)in.aasd.mk	1.4
#
# Copyrighted as an unpublished work.
# (c) Copyright 1987-1996 Computer Associates International, Inc.
# All rights reserved.
#

#
# Makefile for the Address Allocation Server
#

include $(CMDRULES)

MKDEPEND= /usr/bin/X11/makedepend

LOCALDEF= -D_IN_AASD

LDFLAGS = -s
LDLIBS	= -laas -ldb -lsocket -lnsl -lgen

INSDIR=$(USRSBIN)
INSFLAGS= -f

CMD= in.aasd

OBJS	= \
	protocol.o \
	receive.o \
	af_inet.o \
	af_unix.o \
	addr_db.o \
	atype_inet.o \
	util.o \
	main.o

SRCS= $(OBJS:.o=.c)

all: $(CMD) 

$(CMD): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

install: all
	$(INS) $(INSFLAGS) $(INSDIR) -m 0755 -u $(OWN) -g $(GRP) $(CMD)

clean:
	rm -f *.o

clobber: clean
	rm -f $(CMD)

depend:
	${MKDEPEND} -f *.mk -- $(DEFLIST) $(INCLIST) $(CFLAGS) $(SRCS)
