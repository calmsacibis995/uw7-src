#ident	"@(#)telnet.mk	1.5"

include $(CMDRULES)

INSDIR=		$(USRBIN)
OWN=		bin
GRP=		bin

LOCALDEF=	-DINTL -DUW -DFILIO_H -DUSE_TERMIO -DKLUDGELINEMODE -DENV_HACK\
		-DOLD_ENVIRON -Dsignal=sigset

LDLIBS=		-L../../usr.lib/libtelnet -ltelnet -lsocket -lnsl -lcurses

OBJS=		commands.o main.o network.o ring.o sys_bsd.o telnet.o\
		terminal.o utilities.o

all:		telnet

telnet_msg.h:	NLS/en/telnet.gen
		$(MKCATDEFS) telnet $? >/dev/null

commands.o main.o sys_bsd.o telnet.o utilities.o: telnet_msg.h

telnet:		$(OBJS)
		$(CC) -o telnet $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

install:	all
		[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) telnet
		$(DOCATS) -d NLS $@

clean:
		rm -f $(OBJS) telnet_msg.h

clobber:	clean
		rm -f telnet
		$(DOCATS) -d NLS $@

lintit:
		$(LINT) $(LINTFLAGS) *.c
