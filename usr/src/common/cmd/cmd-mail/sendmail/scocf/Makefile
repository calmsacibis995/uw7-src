#ident "@(#)Makefile	11.1"

include $(CMDRULES)

KSH =			/bin/ksh
TCL =			$(TOOLS)/usr/bin/tcl
MKCATDEFS =		$(TOOLS)/usr/bin/mkcatdefs
MKCATDECL =		$(TCL) $(TOOLS)/usr/bin/mkcatdecl
DONLS =			TCL=$(TCL) \
			INS=$(INS) \
			MKCATDECL="$(MKCATDECL)" \
			MKCATDEFS=$(MKCATDEFS) \
			USRLIB=$(USRLIB) ETC=$(ETC) \
			$(KSH) ../../bin/donls

LDLIBS =		-L$(USRLIB) -lsocket

build:			socket sendmailcf.factory sendmailrc nls

socket:			socket.c
			$(CC) -o socket $(CFLAGS) $(DEFLIST) socket.c $(LDFLAGS) $(LDLIBS)

sendmailcf.factory:	sendmail.cf
			cp  sendmail.cf sendmailcf.factory

sendmailrc:		sendmailrc_msg.sh sendmailrc.sh
			echo ":" > $@
			cat sendmailrc_msg.sh >> $@
			cat sendmailrc.sh >> $@

sendmailrc_msg.sh:	NLS/en/sendmailrc.gen
			$(MKCATDEFS) -a mail -s sendmailrc NLS/en/sendmailrc.gen > /dev/null

nls:
			$(DONLS) -d NLS build

clean:
			rm -f sendmailrc sendmailrc_msg.sh
			$(DONLS) -d NLS clean

clobber:		clean
			rm -f socket
			rm -f sendmailcf.factory
