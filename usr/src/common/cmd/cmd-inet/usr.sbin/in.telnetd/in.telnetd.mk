#ident	"@(#)in.telnetd.mk	1.6"

include $(CMDRULES)

INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		bin

LOCALDEF=	-DINTL -DFILIO_H -DUSE_TERMIO -DKLUDGELINEMODE -DSTREAMSPTY\
		-DUW -DDIAGNOSTICS -DENV_HACK -DUTMPX -Dsignal=sigset -DKEEPCFG\
		-DDEFAULT_IM='"\r\n\r\nSCO UnixWare %v   (support level 7.0.0t) (%h) (%t)\r\n\r\r\n\r"'

LDLIBS=		-L../../usr.lib/libtelnet -ltelnet -lsocket -lnsl -liaf -lcurses

OBJS=		telnetd.o state.o termstat.o slc.o sys_term.o utility.o\
		global.o authenc.o

all:		in.telnetd

telnetd_msg.h:	NLS/en/telnetd.gen
		$(MKCATDEFS) telnetd $? >/dev/null

state.o sys_term.o telnetd.o: telnetd_msg.h

in.telnetd:	$(OBJS)
		$(CC) -o in.telnetd $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

install:	all
		[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) in.telnetd
		$(DOCATS) -d NLS $@

clean:
		rm -f $(OBJS) telnetd_msg.h

clobber:	clean
		rm -f in.telnetd
		$(DOCATS) -d NLS $@

lintit:
		$(LINT) $(LINTFLAGS) *.c
