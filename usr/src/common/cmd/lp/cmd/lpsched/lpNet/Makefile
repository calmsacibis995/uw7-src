#ident	"@(#)Makefile	1.3"
#ident  "$Header$"

include $(CMDRULES)

LPROOT	=	../../..
LPNETROOT=	.
LPLIB	=	$(LPROOT)/lib
LPINC	=	$(LPROOT)/include
INCSYS	=	$(INC)/sys

USRLIBLP=	$(USRLIB)/lp
ETCLP	=	$(ETC)/lp

OWNER	=	lp
GROUP	=	tty
MODES	=	0555

NETLIBS =	parent/parent.a	\
		svChild/svChild.a	\
		bsdChild/bsdChild.a	\
		nucChild/nucChild.a

LPNETLIB=	$(LPLIB)/lpNet/liblpNet.a
LIBBSD	=	$(LPLIB)/bsd/liblpbsd.a
LIBNW	=	$(LPLIB)/nuc/liblpnw.a

LPLIBS	=	$(LPNETLIB)			\
		$(LIBBSD)			\
		$(LIBNW)			\
		$(LPLIB)/requests/liblpreq.a	\
		$(LPLIB)/msgs/liblpmsg.a	\
		$(LPLIB)/secure/liblpsec.a	\
		$(LPLIB)/printers/liblpprt.a	\
		$(LPLIB)/systems/liblpsys.a	\
		$(LPLIB)/lp/liblp.a             \
		$(LPLIB)/oam/liblpoam.a

DEBUG=
#LOCALDEF=$(DEBUG)
GLIB	=
SYSLIBS	=	-lnls -lnsl -liaf -lgen -lcurses \
		-lsocket -ldl $(GLIB)
LIBS	=	$(NETLIBS) $(LPLIBS) $(SYSLIBS)

SRCDIRS	=	parent svChild bsdChild nucChild
TARGETS	=	lpNet

MAKECMD	=	$(MAKE) $(MAKEARGS)

PROBEFILE = parent/parent.c

.PRECIOUS:	$(TARGETS) $(NETLIBS)

all:
	@if [ -f $(PROBEFILE) ]; \
	then \
		cd parent; $(MAKECMD) all;\
		cd ..;\
		cd svChild; $(MAKECMD) all;\
		cd ..;\
		cd bsdChild; $(MAKECMD) all;\
		cd ..;\
		cd nucChild; $(MAKECMD) all;\
		cd ..;\
		$(CC) $(LDFLAGS) $(LIBS) $(SHLIBS) -o lpNet; \
	fi

install: all
		$(INS) -m $(MODES) -u $(OWNER) -g $(GROUP) \
			-f $(USRLIBLP) lpNet
		$(INS) -m 644 -u $(OWNER) -g $(GROUP) \
			-f $(ETCLP) Systems

strip:
		$(STRIP) lpNet

clean:
		@for d in $(SRCDIRS); \
		do \
			if [ -d $${d} ]; then\
				cd $${d}; \
				$(MAKECMD) $@; \
				cd ..; \
			fi;\
		done

clobber: clean
		@for d in $(SRCDIRS); \
		do \
			if [ -d $${d} ]; then \
				cd $${d}; \
				$(MAKECMD) $@; \
				cd ..; \
			fi;\
		done
		rm -f $(TARGETS)

product:
		@for t in $(TARGETS);	\
		do			\
			echo "$${t}";	\
		done

lintit:
		@for d in $(SRCDIRS); \
		do \
			if [ -d $$d ]; then\
				cd $${d}; \
				$(MAKECMD) $@; \
				cd ..; \
			fi;\
		done

lintlib:
		@for d in $(SRCDIRS); \
		do \
			if [ -d $$d ]; then\
				cd $${d}; \
				$(MAKECMD) $@; \
				cd ..; \
			fi;\
		done

lpNet:		$(NETLIBS)
		$(CC) $(LDFLAGS) $(LIBS) $(SHLIBS) -o lpNet

#parent/parent.a:	FRC
parent/parent.a:
	cd parent; $(MAKECMD) all

#svChild/svChild.a:	FRC
svChild/svChild.a:
	cd svChild; $(MAKECMD) all

#bsdChild/bsdChild.a:	FRC
bsdChild/bsdChild.a:
	cd bsdChild; $(MAKECMD) all

#nucChild/nucChild.a:	FRC
nucChild/nucChild.a:
	cd nucChild; $(MAKECMD) all

FRC:
