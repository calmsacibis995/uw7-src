#ident	"@(#)bnu.mk	1.10"
#ident	"$Header$"
#	/*  11/45, 11/70, and VAX version ('-i' has no effect on VAX)	*/
#	/* for 11/23, 11/34 (without separate I/D), IFLAG= */
#
#	LOCALCEF:
#	-DSMALL can be used on small machines.
#	It reduces debugging statements in the object code.
#
#	${S5CFLAGS} is an environment variable set for the
#	SAFARI V build.  It MUST have a value of "-Ml", 
#	which specifies that the "large model" should be used
#	for compiling and linking.

include $(CMDRULES)

# All protocols are compiled in based on the definitions in parms.h
# To activate a protocol, the appropriate definition must be
# made in parms.h. This list must contain at least gio.[co]
# because it is automatically included in the source.
# i.e. D_PROTOCOL, E_PROTOCOL, X_PROTOCOL
PROTOCOLS = dio.o eio.o gio.o xio.o
PROTOCOLSRC = dio.c eio.c gio.c xio.c

# All connection related code has been removed from BNU. The dial()
# library routine now provides complete connection functionality.

# For TLI(S), define TLI(S) in parms.h (implies E_PROTOCOL) and
# use the following line on systems without shared libraries
USE_COFF=_s
# use the following line on systems with shared libraries
USE_COFF=

TLILIB=-liaf -lnsl$(USE_COFF)

# LOCALDEF include Safari V environment flags
# The second line will produce smaller a.outs by reducing degbu statements
LOCALDEF=$(S5CFLAGS)
# LOCALDEF = -O $(S5CFLAGS) -DSMALL

IFLAG =

LDLIBS= $(IFLAG) $(S5LDFLAGS) -lcrypt $(TLILIB) -lgen

# use this on systems that don't have strpbrk in libc
# STRPBRK = strpbrk.o
# STRPBRKSRC = strpbrk.c

# use this on systems that don't have getopt() in libc
# GETOPT = getopt.o
# GETOPTSRC = getopt.c

OWN=uucp
GRP=uucp

# save the last version in OLD<name> when installing new version
# OLD=-o

# If you want to maintain the old logical uucp file/directory structure,
# define SYMLINK to be "ln -s". This way files will appear to be in their
# old locations. This is intended to ease the conversion culture shock.
# The BNU code will always access the (new) physical pathnames for
# performance reasons.
# At this time, symbolic links are not created for directories which
# already exist, or are linked elsewhere. The administrator can
# create the links after saving the data in those directories.

# if you change these directories, change them in uucp.h as well

UUCPBIN=$(USRLIB)/uucp
UUCPDB=$(ETC)/uucp
EDEF=$(UUCPDB)/default
MDBASE=$(UUCPDB)/DBase

SPOOL=$(VAR)/spool
LOCKS=$(SPOOL)/locks
UUCPSPL=$(SPOOL)/uucp
UUCPVAR=$(VAR)/uucp
SUUCPVAR=/var/uucp
UUCPPUB=$(SPOOL)/uucppublic
#
ADMIN=$(UUCPVAR)/.Admin
SADMIN=$(SUUCPVAR)/.Admin
#		things are moved (linked) from UUCPSPL into XQTDIR and CORRUPT
CORRUPT=$(UUCPVAR)/.Corrupt
SCORRUPT=$(SUUCPVAR)/.Corrupt
#		this is optional
XQTDIR=	$(UUCPVAR)/.Xqtdir
SXQTDIR=$(SUUCPVAR)/.Xqtdir
#		for logfiles
LOGDIR=$(UUCPVAR)/.Log
SLOGDIR=$(SUUCPVAR)/.Log
LOGCICO=$(LOGDIR)/uucico
LOGUUCP=$(LOGDIR)/uucp
LOGUUX=$(LOGDIR)/uux
LOGUUXQT=$(LOGDIR)/uuxqt
#		for saving old log files
OLDLOG=$(UUCPVAR)/.Old
SOLDLOG=$(SUUCPVAR)/.Old
#		for sequence number files
SEQDIR=$(UUCPVAR)/.Sequence
SSEQDIR=$(SUUCPVAR)/.Sequence
#		for STST files
STATDIR=$(UUCPVAR)/.Status
SSTATDIR=$(SUUCPVAR)/.Status
#
WORKSPACE=$(UUCPVAR)/.Workspace
SWORKSPACE=$(SUUCPVAR)/.Workspace
#
MODEMDIR=Modems
DIALDEFS=$(MODEMDIR)/Dialers/
DETECT=$(MODEMDIR)/Detect
MDMDBASE=$(MODEMDIR)/DBase
ICSD=$(ETC)/ics
#
DIRS = $(UUCPBIN) $(UUCPDB) $(LOCKS) $(UUCPVAR) $(ADMIN) $(CORRUPT) \
	  $(LOGDIR) $(LOGCICO) $(LOGUUCP) $(LOGUUX) $(LOGUUXQT) $(OLDLOG) \
	  $(SEQDIR) $(STATDIR) $(WORKSPACE) $(XQTDIR)

CLEAN=
#	lint needs to find local header file(s)
LINTFLAGS=-I $(INC)
#
USERCMDS = ct cu uuglist uucp uuname uustat uux uudecode uuencode infparse atdialer siofifo ati isdndialer
UUCPCMDS = bnuconvert permld remote.unknown uucheck uucleanup uusched uucico uuxcmd uuxqt
#
OFILES=utility.o cpmv.o expfile.o gename.o getpwinfo.o \
	ulockf.o xqt.o logent.o versys.o gnamef.o systat.o \
	sysfiles.o $(GETOPT)
LFILES=utility.c cpmv.c expfile.c gename.c getpwinfo.c \
	ulockf.c xqt.c logent.c gnamef.c systat.c \
	sysfiles.c $(GETOPTSRC)
OPERMLD=permld.o uucpdefs.o gnamef.o
LPERMLD=permld.c uucpdefs.c gnamef.c
OUUCP=uucpdefs.o uucp.o gwd.o permission.o getargs.o getprm.o uucpname.o\
	versys.o gtcfile.o grades.o $(STRPBRK) chremdir.o mailst.o
LUUCP=uucpdefs.c uucp.c gwd.c permission.c getargs.c getprm.c uucpname.c\
	versys.c gtcfile.c grades.c $(STRPBRKSRC) chremdir.c mailst.c
OUUX=uucpdefs.o uux.o permission.o getprm.o uucpname.o getargs.o
LUUX=uucpdefs.c uux.c permission.c getprm.c uucpname.c getargs.c
OUUXCMD=uucpdefs.o uuxcmd.o mailst.o gwd.o permission.o getargs.o getprm.o\
	uucpname.o versys.o gtcfile.o grades.o chremdir.o $(STRPBRK) \
	sum.o
LUUXCMD=uucpdefs.c uuxcmd.c mailst.c gwd.c permission.c getargs.c getprm.c\
	uucpname.c versys.c gtcfile.c grades.c chremdir.c $(STRPBRKSRC) \
	sum.c
OUUXQT=uucpdefs.o uuxqt.o mailst.o getprm.o uucpname.o \
	permission.o getargs.o gtcfile.o grades.o $(STRPBRK) \
	shio.o chremdir.o account.o perfstat.o statlog.o security.o \
	limits.o sum.o
LUUXQT=uucpdefs.c uuxqt.c mailst.c getprm.c uucpname.c \
	permission.c getargs.c gtcfile.c grades.c $(STRPBRKSRC) \
	shio.c chremdir.c account.c perfstat.c statlog.c security.c \
	limits.c sum.c
OUUCICO=uucpdefs.o uucico.o cntrl.o pk0.o pk1.o \
	anlwrk.o permission.o getargs.o \
	gnxseq.o pkdefs.o imsg.o gtcfile.o grades.o \
	mailst.o uucpname.o line.o chremdir.o \
	statlog.o perfstat.o account.o rwioctl.o \
	security.o limits.o dialerr.o $(STRPBRK) $(PROTOCOLS)
LUUCICO=uucpdefs.c uucico.c cntrl.c pk0.c pk1.c \
	anlwrk.c permission.c getargs.c \
	gnxseq.c pkdefs.c imsg.c gtcfile.c grades.c \
	mailst.c uucpname.c line.c chremdir.c \
	statlog.c perfstat.c account.c\
	security.c limits.c dialerr.c $(STRPBRKSRC) $(PROTOCOLSRC)
OUUNAME=uuname.o uucpname.o uucpdefs.o getpwinfo.o sysfiles.o 
LUUNAME=uuname.c uucpname.c uucpdefs.c getpwinfo.c sysfiles.c 
OUUSTAT=uustat.o gnamef.o expfile.o uucpdefs.o getpwinfo.o ulockf.o getargs.o \
	utility.o uucpname.o versys.o sysfiles.o cpmv.o \
	mailst.o permission.o $(STRPBRK) $(GETOPT)
LUUSTAT=uustat.c gnamef.c expfile.c uucpdefs.c getpwinfo.c ulockf.c getargs.c \
	utility.c uucpname.c versys.c sysfiles.c cpmv.c \
	mailst.c permission.c $(STRPBRKSRC) $(GETOPTSRC)
OUUSCHED=uusched.o gnamef.o expfile.o uucpdefs.o getpwinfo.o ulockf.o \
	systat.o getargs.o utility.o limits.o permission.o uucpname.o \
	$(GETOPT)
LUUSCHED=uusched.c gnamef.c expfile.c uucpdefs.c getpwinfo.c ulockf.c \
	systat.c getargs.c utility.c limits.c permission.c uucpname.c \
	$(GETOPTSRC)
OUUCLEANUP=uucleanup.o gnamef.o expfile.o uucpdefs.o getpwinfo.o \
	uucpname.o ulockf.o getargs.o cpmv.o utility.o $(GETOPT)
LUUCLEANUP=uucleanup.c gnamef.c expfile.c uucpdefs.c getpwinfo.c \
	uucpname.c ulockf.c getargs.c cpmv.c utility.c $(GETOPTSRC)
OUUGLIST=grades.o cpmv.o getargs.o getpwinfo.o \
	uuglist.o uucpdefs.o expfile.o uucpname.o $(GETOPT)
LUUGLIST=grades.c cpmv.c getargs.c getpwinfo.c \
	uuglist.c uucpdefs.c expfile.c uucpname.c $(GETOPTSRC)
OBNUCONVERT=bnuconvert.o uucpdefs.o grades.o \
	getpwinfo.o getargs.o cpmv.o chremdir.o expfile.o gename.o \
	gnamef.o gtcfile.o logent.o systat.o ulockf.o utility.o \
	uucpname.o $(GETOPT)
LBNUCONVERT=bnuconvert.c uucpdefs.c grades.c \
	getpwinfo.c getargs.c cpmv.c chremdir.c expfile.c gename.c \
	gnamef.c gtcfile.c logent.c systat.c ulockf.c utility.c \
	uucpname.c $(GETOPTSRC)
OUNKNOWN=unknown.o uucpdefs.o expfile.o getpwinfo.o
LUNKNOWN=unknown.c uucpdefs.c expfile.c getpwinfo.c
OCU =  cu.o getargs.o line.o uucpdefs.o rwioctl.o dialerr.o
LCU =  cu.c getargs.c line.c uucpdefs.c rwioctl.c dialerr.c
OCT =  ct.o getargs.o line.o uucpdefs.o rwioctl.o
LCT =  ct.c getargs.c line.c uucpdefs.c rwioctl.c
OATDIALER=	atdialer.o atfields.o deflt.o 
OISDNDIALER=	isdndialer.o
OATI=ati.o ulockf.o
OUUDECODE=uudecode.o
LUUDECODE=uudecode.c
OUUENCODE=uuencode.o
LUUENCODE=uuencode.c
OUUCHECK=uucheck.o uucpname.o $(GETOPT)
LUUCHECK=uucheck.c uucpname.c $(GETOPTSRC)
OSIOFIFO=siofifo.o
OINFPARSE=infparse.o strcasecmp.o

MSGS	= Uutry.str cu.str

.MUTEX: copyright $(DIRS) spdirs all

all:	init $(USERCMDS) $(UUCPCMDS)

install: copyright $(DIRS) spdirs all shells cp check $(MSGS)
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 Uutry.str
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 cu.str

shells:
	#	SetUp appropriate BNU database files
	ROOT="$(ROOT)" ; MACH="$(MACH)" ; export ROOT MACH ; sh SetUp  "$(SYMLINK)"
	$(INS) $(OLD) -f $(UUCPBIN) -m 0555 -u $(OWN) -g $(GRP) SetUp
	$(INS) $(OLD) -f $(UUCPBIN) -m 0555 -u $(OWN) -g $(GRP) Teardown
	$(INS) $(OLD) -f $(UUCPBIN) -m 0555 -u $(OWN) -g $(GRP) Uutry
	$(INS) $(OLD) -f $(UUCPBIN) -m 0555 -u $(OWN) -g $(GRP) uudemon.admin
	$(INS) $(OLD) -f $(UUCPBIN) -m 0555 -u $(OWN) -g $(GRP) uudemon.clean
	$(INS) $(OLD) -f $(UUCPBIN) -m 0555 -u $(OWN) -g $(GRP) uudemon.hour
	$(INS) $(OLD) -f $(UUCPBIN) -m 0555 -u $(OWN) -g $(GRP) uudemon.poll
	$(INS) $(OLD) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) uulog
	$(INS) $(OLD) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) uupick
	$(INS) $(OLD) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) uuto
	$(INS) $(OLD) -f $(UUCPBIN) -m 0555 -u $(OWN) -g $(GRP) modem

check:
	if [ \( -z "$(ROOT)" -o "$(ROOT)" = "/" \) -a "$(CH)" != "#" ]; then ./uucheck; fi

cp:	all 
	$(INS) $(OLD) -f $(USRBIN) -m 6111 -u root     -g $(GRP) ct
	$(INS) $(OLD) -f $(USRBIN) -m 2111 -u $(OWN) -g $(GRP) cu
	$(INS) $(OLD) -f $(USRBIN) -m 6111 -u $(OWN) -g $(GRP) uucp
	$(INS) $(OLD) -f $(USRBIN) -m 0111 -u $(OWN) -g $(GRP) uuglist
	$(INS) $(OLD) -f $(USRBIN) -m 2111 -u $(OWN) -g $(GRP) uuname
	$(INS) $(OLD) -f $(USRBIN) -m 2111 -u $(OWN) -g $(GRP) uustat
	$(INS) $(OLD) -f $(USRBIN) -m 2111 -u $(OWN) -g $(GRP) uux
	$(INS) $(OLD) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) uudecode
	$(INS) $(OLD) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) uuencode

	$(INS) $(OLD) -f $(UUCPBIN) -m 2111 -u $(OWN) -g $(GRP) remote.unknown
	$(INS) $(OLD) -f $(UUCPBIN) -m 6111 -u $(OWN) -g $(GRP) uucico
	$(INS) $(OLD) -f $(UUCPBIN) -m 2111 -u $(OWN) -g $(GRP) uusched
	$(INS) $(OLD) -f $(UUCPBIN) -m 6111 -u $(OWN) -g $(GRP) uuxcmd
	$(INS) $(OLD) -f $(UUCPBIN) -m 6111 -u root     -g $(GRP) uuxqt

# uucheck should only be run by root or uucp administrator
# uucleanup should only be run by root or uucp administrator
# bnuconvert should only be run by root or uucp administrator
# atdialer should only be run by root or uucp administrator
# isdndialer should only be run by root or uucp administrator
# siofifo should only be run by root or uucp administrator
	$(INS) $(OLD) -f $(UUCPBIN) -m 0110 -u $(OWN) -g $(GRP) bnuconvert
	$(INS) $(OLD) -f $(UUCPBIN) -m 0110 -u $(OWN) -g $(GRP) permld
	$(INS) $(OLD) -f $(UUCPBIN) -m 0110 -u $(OWN) -g $(GRP) uucheck
	$(INS) $(OLD) -f $(UUCPBIN) -m 0110 -u $(OWN) -g $(GRP) uucleanup
	$(INS) $(OLD) -f $(UUCPBIN) -m 0110 -u $(OWN) -g $(GRP) atdialer 
	$(INS) $(OLD) -f $(UUCPBIN) -m 0110 -u $(OWN) -g $(GRP) isdndialer 
	$(INS) $(OLD) -f $(UUCPBIN) -m 0110 -u $(OWN) -g $(GRP) siofifo 
	$(INS) $(OLD) -f $(UUCPBIN) -m 0110 -u $(OWN) -g $(GRP) ati 
	sh ./mkinf
	sh ./mkmdm
	[ -d $(EDEF) ] || mkdir -p $(EDEF)
	find $(DIALDEFS) -type f -print |xargs -i -t cp -f {} $(EDEF)
	cp -f $(DETECT) $(UUCPDB)
	[ -d $(MDBASE) ] || mkdir -p $(MDBASE)
	find $(MDMDBASE) -type f -print |xargs -i -t cp -f {} $(MDBASE)
	[ -d $(ICSD) ] || mkdir -p $(ICSD)
	cp -f Callfilter $(ICSD)
	cp -f Callservices $(ICSD)
	cp -f Autoacts $(ICSD)

restore:
	$(CH)mv -f $(USRBIN)/OLDct $(USRBIN)/ct
	$(CH)chown root $(USRBIN)/ct
	$(CH)chgrp $(GRP) $(USRBIN)/ct
	$(CH)chmod 6111 $(USRBIN)/ct
	$(CH)mv -f $(USRBIN)/OLDcu $(USRBIN)/cu
	$(CH)chown $(OWN) $(USRBIN)/cu
	$(CH)chgrp $(GRP) $(USRBIN)/cu
	$(CH)chmod 2111 $(USRBIN)/cu
	$(CH)mv -f $(USRBIN)/OLDuucp $(USRBIN)/uucp
	$(CH)chown $(OWN) $(USRBIN)/uucp
	$(CH)chgrp $(GRP) $(USRBIN)/uucp
	$(CH)chmod 6111 $(USRBIN)/uucp
	$(CH)mv -f $(USRBIN)/OLDuuglist $(USRBIN)/uuglist
	$(CH)chown $(OWN) $(USRBIN)/uuglist
	$(CH)chgrp $(GRP) $(USRBIN)/uuglist
	$(CH)chmod 0111 $(USRBIN)/uuglist
	$(CH)mv -f $(USRBIN)/OLDuulog $(USRBIN)/uulog
	$(CH)chown $(OWN) $(USRBIN)/uulog
	$(CH)chgrp $(GRP) $(USRBIN)/uulog
	$(CH)chmod 555 $(USRBIN)/uulog
	$(CH)mv -f $(USRBIN)/OLDuuname $(USRBIN)/uuname
	$(CH)chown $(OWN) $(USRBIN)/uuname
	$(CH)chgrp $(GRP) $(USRBIN)/uuname
	$(CH)chmod 2111 $(USRBIN)/uuname
	$(CH)mv -f $(USRBIN)/OLDuustat $(USRBIN)/uustat
	$(CH)chown $(OWN) $(USRBIN)/uustat
	$(CH)chgrp $(GRP) $(USRBIN)/uustat
	$(CH)chmod 2111 $(USRBIN)/uustat
	$(CH)mv -f $(USRBIN)/OLDuux $(USRBIN)/uux
	$(CH)chown $(OWN) $(USRBIN)/uux
	$(CH)chgrp $(GRP) $(USRBIN)/uux
	$(CH)chmod 2111 $(USRBIN)/uux
	$(CH)mv -f $(USRBIN)/OLDuudecode $(USRBIN)/uudecode
	$(CH)chown $(OWN) $(USRBIN)/uudecode
	$(CH)chgrp $(GRP) $(USRBIN)/uudecode
	$(CH)chmod 0555 $(USRBIN)/uudecode
	$(CH)mv -f $(USRBIN)/OLDuuencode $(USRBIN)/uuencode
	$(CH)chown $(OWN) $(USRBIN)/uuencode
	$(CH)chgrp $(GRP) $(USRBIN)/uuencode
	$(CH)chmod 0555 $(USRBIN)/uuencode

	$(CH)mv -f $(UUCPBIN)/OLDbnuconvert $(UUCPBIN)/bnuconvert
	$(CH)chown $(OWN) $(UUCPBIN)/bnuconvert
	$(CH)chgrp $(GRP) $(UUCPBIN)/bnuconvert
	$(CH)chmod 0110 $(UUCPBIN)/bnuconvert
	$(CH)mv -f $(UUCPBIN)/OLDpermld $(UUCPBIN)/permld
	$(CH)chown $(OWN) $(UUCPBIN)/permld
	$(CH)chgrp $(GRP) $(UUCPBIN)/permld
	$(CH)chmod 0110 $(UUCPBIN)/permld
	$(CH)mv -f $(UUCPBIN)/OLDremote.unknown $(UUCPBIN)/remote.unknown
	$(CH)chown $(OWN) $(UUCPBIN)/remote.unknown
	$(CH)chgrp $(GRP) $(UUCPBIN)/remote.unknown
	$(CH)chmod 2111 $(UUCPBIN)/remote.unknown
	$(CH)mv -f $(UUCPBIN)/OLDuucico $(UUCPBIN)/uucico
	$(CH)chown $(OWN) $(UUCPBIN)/uucico
	$(CH)chgrp $(GRP) $(UUCPBIN)/uucico
	$(CH)chmod 6111 $(UUCPBIN)/uucico
	$(CH)mv -f $(UUCPBIN)/OLDuucheck $(UUCPBIN)/uucheck
	$(CH)chown $(OWN) $(UUCPBIN)/uucheck
	$(CH)chgrp $(GRP) $(UUCPBIN)/uucheck
	$(CH)chmod 0110 $(UUCPBIN)/uucheck
	$(CH)mv -f $(UUCPBIN)/OLDuucleanup $(UUCPBIN)/uucleanup
	$(CH)chown $(OWN) $(UUCPBIN)/uucleanup
	$(CH)chgrp $(GRP) $(UUCPBIN)/uucleanup
	$(CH)chmod 0110 $(UUCPBIN)/uucleanup
	$(CH)mv -f $(UUCPBIN)/OLDuusched $(UUCPBIN)/uusched
	$(CH)chown $(OWN) $(UUCPBIN)/uusched
	$(CH)chgrp $(GRP) $(UUCPBIN)/uusched
	$(CH)chmod 2111 $(UUCPBIN)/uusched
	$(CH)mv -f $(UUCPBIN)/OLDuuxcmd $(UUCPBIN)/uuxcmd
	$(CH)chown root $(UUCPBIN)/uuxcmd
	$(CH)chgrp $(GRP) $(UUCPBIN)/uuxcmd
	$(CH)chmod 6111 $(UUCPBIN)/uuxcmd
	$(CH)mv -f $(UUCPBIN)/OLDuuxqt $(UUCPBIN)/uuxqt
	$(CH)chown root $(UUCPBIN)/uuxqt
	$(CH)chgrp $(GRP) $(UUCPBIN)/uuxqt
	$(CH)chmod 6111 $(UUCPBIN)/uuxqt

clean:
	-rm -f *.o

clobber:	clean
	-rm -f $(USERCMDS) $(UUCPCMDS)

burn:
	-rm -f $(USRBIN)/OLDct
	-rm -f $(USRBIN)/OLDcu
	-rm -f $(USRBIN)/OLDuucp
	-rm -f $(USRBIN)/OLDuuglist
	-rm -f $(USRBIN)/OLDuuname
	-rm -f $(USRBIN)/OLDuustat
	-rm -f $(USRBIN)/OLDuux
	-rm -f $(USRBIN)/OLDuudecode
	-rm -f $(USRBIN)/OLDuuencode
	-rm -f $(UUCPBIN)/OLDpermld
	-rm -f $(UUCPBIN)/OLDuucleanup
	-rm -f $(UUCPBIN)/OLDuucheck
	-rm -f $(UUCPBIN)/OLDuucico
	-rm -f $(UUCPBIN)/OLDuusched
	-rm -f $(UUCPBIN)/OLDuuxcmd
	-rm -f $(UUCPBIN)/OLDuuxqt

cmp:	all
	cmp ct $(USRBIN)/ct
	rm ct
	cmp cu $(USRBIN)/cu
	rm cu
	cmp uucp $(USRBIN)/uucp
	rm uucp
	cmp uuglist $(USRBIN)/uuglist
	rm uuglist
	cmp uuname $(USRBIN)/uuname
	rm uuname
	cmp uustat $(USRBIN)/uustat
	rm uustat
	cmp uux $(USRBIN)/uux
	rm uux
	cmp uudecode $(USRBIN)/uudecode
	rm uudecode
	cmp uuencode $(USRBIN)/uuencode
	rm uuencode
	cmp permld $(UUCPBIN)/permld
	rm permld
	cmp uucleanup $(UUCPBIN)/uucleanup
	rm uucleanup
	cmp uucheck $(UUCPBIN)/uucheck
	rm uucheck
	cmp uucico $(UUCPBIN)/uucico
	rm uucico
	cmp uusched $(UUCPBIN)/uusched
	rm uusched
	cmp uuxcmd $(UUCPBIN)/uuxcmd
	rm uuxcmd
	cmp uuxqt $(UUCPBIN)/uuxqt
	rm uuxqt
	rm *.o

copyright:
	@/bin/echo "\n\n**********************************************"
	@/bin/echo "* Copyright (c) 1991 USL. Inc.		 *"
	@/bin/echo "*           All Rights Reserved              *"
	@/bin/echo "* THIS IS UNPUBLISHED PROPRIETARY SOURCE     *"
	@/bin/echo "* CODE OF UNIX SYSTEM LABORATORIES, INC.     *"
	@/bin/echo "* The copyright notice above does not        *"
	@/bin/echo "* evidence any actual or intended            *"
	@/bin/echo "* publication of such source code.           *"
	@/bin/echo "**********************************************\n\n"


init:	copyright anlwrk.o permission.o cpmv.o expfile.o gename.o \
	getargs.o getprm.o getpwinfo.o gnamef.o \
	gnxseq.o gwd.o imsg.o logent.o \
	mailst.o shio.o \
	systat.o ulockf.o uucpname.o versys.o xqt.o

bnuconvert:	$(OBNUCONVERT)
	$(CC) $(LDFLAGS) $(OBNUCONVERT) -o $@ $(LDLIBS) $(SHLIBS)
 
ct:	$(OCT)
	$(CC) $(OCT) $(LDFLAGS) -o $@ $(LDLIBS) $(SHLIBS)

cu:	$(OCU)
	$(CC) $(OCU) $(LDFLAGS) -o $@ $(LDLIBS) $(SHLIBS)

atdialer:	$(OATDIALER)
	$(CC) $(OATDIALER) $(LDFLAGS) -o $@ $(LDLIBS) -lx $(SHLIBS)

isdndialer:	$(OISDNDIALER)
	$(CC) $(OISDNDIALER) $(LDFLAGS) -o $@ -lnsl

ati:	$(OATI)
	$(CC) $(OATI) $(LDFLAGS) -o $@ $(LDLIBS) -lx $(SHLIBS)

permld: $(OPERMLD)
	$(CC) $(LDFLAGS) $(OPERMLD) -o $@ $(LDLIBS) $(SHLIBS)
		 
remote.unknown: $(OUNKNOWN)
	$(CC) $(LDFLAGS) $(OUNKNOWN) -o $@ $(LDLIBS) $(SHLIBS)
		 
uucheck:	$(OUUCHECK)
	$(CC) $(LDFLAGS) $(OUUCHECK) -o $@ $(LDLIBS) $(SHLIBS)
 
uucico:	$(OUUCICO) $(OFILES)
	$(CC) $(LDFLAGS) $(OUUCICO) $(OFILES) -o $@ $(LDLIBS) $(SHLIBS)

uucleanup:	$(OUUCLEANUP)
	$(CC) $(LDFLAGS) $(OUUCLEANUP) -o $@ $(LDLIBS) $(SHLIBS)
 
uucp:	$(OUUCP) $(OFILES)
	$(CC) $(LDFLAGS) $(OUUCP) $(OFILES) -o $@ $(LDLIBS) $(SHLIBS)

uuglist: $(OUUGLIST)
	 $(CC) $(LDFLAGS) $(OUUGLIST) -o $@ $(LDLIBS) $(SHLIBS)

uuname:	$(OUUNAME)
	$(CC) $(LDFLAGS) $(OUUNAME) -o $@ $(LDLIBS) $(SHLIBS)

uusched:	$(OUUSCHED)
	$(CC) $(LDFLAGS) $(OUUSCHED) -o $@ $(LDLIBS) $(SHLIBS)
 
uustat:	$(OUUSTAT)
	$(CC) $(LDFLAGS) $(OUUSTAT) -o $@ $(LDLIBS) $(SHLIBS)
 
uux:	$(OUUX) $(OFILES)
	$(CC) $(LDFLAGS) $(OUUX) $(OFILES) -o $@ $(LDLIBS) $(SHLIBS)

uuxcmd:	$(OUUXCMD) $(OFILES)
	$(CC) $(LDFLAGS) $(OUUXCMD) $(OFILES) -o $@ $(LDLIBS) $(SHLIBS)

uuxqt:	$(OUUXQT) $(OFILES)
	$(CC) $(LDFLAGS) $(OUUXQT) $(OFILES) -o $@ $(LDLIBS) $(SHLIBS)

uudecode:	$(OUUDECODE)
	$(CC) $(LDFLAGS) $(OUUDECODE) -o $@ $(LDLIBS) $(SHLIBS)

uuencode:	$(OUUENCODE)
	$(CC) $(LDFLAGS) $(OUUENCODE) -o $@ $(LDLIBS) $(SHLIBS)

infparse:	$(OINFPARSE)
	$(CC) $(LDFLAGS) $(OINFPARSE) -o $@ $(IFLAG) $(S5LDFLAGS) $(TLILIB) -lgen $(SHLIBS) 

siofifo:	$(OSIOFIFO)
	$(CC) $(LDFLAGS) $(OSIOFIFO) -o $@ $(LDLIBS) $(SHLIBS)

uucheck.o:	permission.c

dio.o: dk.h

account.o cntrl.o perfstat.o security.o uucico.o uuxqt.o: log.h

gio.o pk0.o pk1.o pkdefs.o:	pk.h

ct.o sysfiles.o uucheck.o:	sysfiles.h

account.o anlwrk.o bnuconvert.o chremdir.o cntrl.o \
	cpmv.o ct.o cu.o dio.o eio.o expfile.o gename.o \
	getargs.o getopt.o getprm.o getpwinfo.o gio.o \
	gnamef.o gnxseq.o grades.o gtcfile.o gwd.o imsg.o \
	limits.o line.o logent.o mailst.o perfstat.o \
	permission.o permld.o pk0.o pk1.o pkdefs.o rwioctl.o \
	security.o shio.o statlog.o strpbrk.o sum.o \
	sysfiles.o systat.o ulockf.o unknown.o utility.o uucheck.o \
	uucico.o uucleanup.o uucp.o uucpdefs.o uucpname.o \
	uuglist.o uuname.o uusched.o uustat.o uux.o uuxcmd.o \
	uuxqt.o versys.o xio.o xqt.o: parms.h uucp.h

$(DIRS):
	[ -d $@ ] || mkdir -p $@ ;\
	$(CH)chmod 775 $@ ;\
	$(CH)chown $(OWN) $@ ;\
	$(CH)chgrp $(GRP) $@

spdirs:
	[ -d $(UUCPSPL) ] || mkdir -p $(UUCPSPL) ;\
	$(CH)chmod 770 $(UUCPSPL) ;\
	$(CH)chown $(OWN) $(UUCPSPL) ;\
	$(CH)chgrp $(GRP) $(UUCPSPL)
	[ -d $(UUCPPUB) ] || mkdir -p $(UUCPPUB) ;\
	$(CH)chmod 1777 $(UUCPPUB) ;\
	$(CH)chown $(OWN) $(UUCPPUB) ;\
	$(CH)chgrp $(GRP) $(UUCPPUB)
	-rm -f $(UUCPSPL)/.Admin
	$(SYMLINK) $(SADMIN) $(UUCPSPL)
	-rm -f $(UUCPSPL)/.Corrupt
	$(SYMLINK) $(SCORRUPT) $(UUCPSPL)
	-rm -f $(UUCPSPL)/.Log
	$(SYMLINK) $(SLOGDIR) $(UUCPSPL)
	-rm -f $(UUCPSPL)/.Old
	$(SYMLINK) $(SOLDLOG) $(UUCPSPL)
	-rm -f $(UUCPSPL)/.Sequence
	$(SYMLINK) $(SSEQDIR) $(UUCPSPL)
	-rm -f $(UUCPSPL)/.Status
	$(SYMLINK) $(SSTATDIR) $(UUCPSPL)
	-rm -f $(UUCPSPL)/.Workspace
	$(SYMLINK) $(SWORKSPACE) $(UUCPSPL)
	-rm -f $(UUCPSPL)/.Xqtdir
	$(SYMLINK) $(SXQTDIR) $(UUCPSPL)

#  lint procedures

lintit:	lintct lintcu lintuucico lintuucp lintuudecode \
	lintuuencode lintuuname lintuux lintuuxcmd lintuuxqt

lintct:
	$(LINT) $(LINTFLAGS) $(LCT)

lintcu:
	$(LINT) $(LINTFLAGS) $(LCU)

lintpermld:
	$(LINT) $(LINTFLAGS) $(LPERMLD)

lintuucico:
	$(LINT) $(LINTFLAGS) $(LUUCICO) $(LFILES)

lintuucp:
	$(LINT) $(LINTFLAGS) $(LUUCP) $(LFILES)

lintuudecode:
	$(LINT) $(LINTFLAGS) $(LUUDECODE)

lintuuencode:
	$(LINT) $(LINTFLAGS) $(LUUENCODE)

lintuuname:
	$(LINT) $(LINTFLAGS) $(LUUNAME)

lintuux:
	$(LINT) $(LINTFLAGS) $(LUUX)

lintuuxcmd:
	$(LINT) $(LINTFLAGS) $(LUUXCMD) $(LFILES)

lintuuxqt:
	$(LINT) $(LINTFLAGS) $(LUUXQT) $(LFILES)
