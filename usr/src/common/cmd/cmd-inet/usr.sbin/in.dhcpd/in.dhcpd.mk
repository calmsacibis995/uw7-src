#	"@(#)in.dhcpd.mk	1.6"
#
# Makefile for the DHCP server deamon (in.dhcpd) 
#

include $(CMDRULES)

MKDEPEND= /usr/bin/X11/makedepend

#LOCALDEF= -DUSE_TLI

LDFLAGS= -s
LDLIBS= -laas -lsocket -lnsl -lgen

INSDIR=$(USRSBIN)
INSFLAGS= -f

CMD= in.dhcpd 

OBJS	= \
	database.o \
	endpt.o \
	hash.o \
	main.o \
	options.o \
	parse.o \
	ping.o \
	protocol.o \
	setarp.o \
	util.o

SRCS= $(OBJS:.o=.c)

all: $(CMD)

$(CMD): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

install: all 
	$(INS) $(INSFLAGS) $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(CMD) 

clean:
	-rm -f *.o

clobber: clean
	-rm -f $(CMD)

depend:
	${MKDEPEND} -f *.mk -- ${CFLAGS} *.c
