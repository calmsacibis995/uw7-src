#ident	"@(#)Makefile	11.3"

# Mon Dec 16 11:11:37 PST 1996
# Added lines in support of IMAP C-CLIENT interface:
#	-DCCLIENT to LOCALDEF line
#	-lc-client to LIBS line
#	changed the VERSION line
#	added the file callbacks.o to OBJS

SHELL	= /bin/sh

LOCALDEF=	-Ihdr -DCCLIENT -DSVR4ES -I.
LIBS	=	../c-client/libc-client.a libmailx/libmailx.a -lgen -lsocket

include $(CMDRULES)

LDLIBS	=	-L$(USRLIB)

VERSION="4.2mp \(using c-client\)"
HELP=	help
DESTDIR= $(USRBIN)

build:	mailx mailx.help mailx.help.~

OBJS =	aux.o \
	callbacks.o \
	ioctl.o \
	cmd1.o \
	cmd2.o \
	cmd3.o \
	cmd4.o \
	cmdtab.o \
	codes.o \
	collect.o \
	edit.o \
	init.o \
	is.o \
	fio.o \
	getname.o\
	head.o \
	hostname.o \
	lex.o \
	list.o \
	lock.o \
	lpaths.o \
	main.o \
	myfopen.o \
	names.o \
	optim.o \
	popen.o \
	quit.o \
	receipt.o \
	send.o \
	shared.o \
	sigretro.o \
	stralloc.o \
	temp.o \
	translate.o \
	tty.o \
	usg.local.o \
	vars.o \
	version.o

mailx:	$(OBJS) libmailx/libmailx.a ../c-client/libc-client.a
	$(CC) $(LDFLAGS) -o mailx $(OBJS) $(LDLIBS) $(LIBS)

libmailx/libmailx.a:
	(cd libmailx; $(MAKE) -$(MAKEFLAGS))

mailx.help: $(HELP)/mailx.help
	grep -v '^#.*@(' $(HELP)/mailx.help > mailx.help

mailx.help.~: help/mailx.help.tilde
	grep -v '^#.*@(' $(HELP)/mailx.help.tilde > mailx.help.~

install: ckdirs mailx symlinks
	$(INS) -f $(BIN) mailx
	$(INS) -f $(USRLIB)/mailx mailx.help
	$(INS) -f $(USRLIB)/mailx mailx.help.~
	$(INS) -f $(ETC)/mail mailx.rc

version.c:
	echo "char *version=\"$(VERSION)\";" > version.c

clean:
	-rm -f $(OBJS)
	-rm -f version.c a.out core makeout* nohup.out
	(cd libmailx; $(MAKE) -$(MAKEFLAGS) clean)

clobber:	clean
	-rm -f mailx mailx.help mailx.help.~
	(cd libmailx; $(MAKE) -$(MAKEFLAGS) clobber)


ckdirs:
	DIRS="$(USRLIB)/mailx $(ETC)/mail $(BIN)" ; \
	for i in $$DIRS; \
	do \
		if [ ! -d $$i ] ; then mkdir -p $$i ; fi ; \
	done

symlinks:
	rm -f	$(USRLIB)/mailx/mailx.help \
		$(USRLIB)/mailx/mailx.help.~ \
		$(USRLIB)/mailx/mailx.rc ; \
	$(CH)$(SYMLINK) $(LDESTLIB)/mailx.help \
		$(CH)$(USRLIB)/mailx/mailx.help ; \
	$(CH)$(SYMLINK) $(LDESTLIB)/mailx.help.~ \
		$(CH)$(USRLIB)/mailx/mailx.help.~ ; \
	$(CH)$(SYMLINK) $(LRCDIR)/mailx.rc \
		$(CH)$(USRLIB)/mailx/mailx.rc

local-install:
	if [ ! -d /etc/mail ] ; then mkdir -p /etc/mail ; fi
	if [ ! -d /usr/lib/mailx ] ; then mkdir -p /usr/lib/mailx ; fi
	cp mailx.rc /etc/mail
	cp mailx /usr/bin
	rm -f /usr/bin/mail
	ln /usr/bin/mailx /usr/bin/mail
	rm -f /usr/lib/mailx/mailx.rc
	ln -s /etc/mail/mailx.rc /usr/lib/mailx/mailx.rc
	cp mailx.help /usr/lib/mailx
	cp mailx.help.~ /usr/lib/mailx

local-remove:
	rm -fr /etc/mail/mailx.rc
	rm -fr /usr/lib/mailx
	rm -f /usr/bin/mailx
	rm -f /usr/bin/mail
