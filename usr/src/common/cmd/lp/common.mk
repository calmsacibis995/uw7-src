#	copyright	"%c%"

#ident	"@(#)common.mk	1.3"
#ident "$Header$"
#
# Common makefile definitions used by all makefiles
#


#####
#
# Each makefile defines $(TOP) to be a reference to this top-level
# directory (e.g. TOP=..).
#####


RM	=	/bin/rm -f
CP	=	/bin/cp
LN	=	/bin/ln

USRLIBLP=	$(USRLIB)/lp
USRLIBHP=	$(USRLIB)/hpnp
USRSHARELIB=	$(USRSHARE)/lib

LPBINDIR=	$(USRLIBLP)/bin

VARSPOOL=	$(VAR)/spool

ETCLP   =       $(ETC)/lp


#####
#
# Typical owner and group for LP things. These can be overridden
# in the individual makefiles.
#####
OWNER	=	lp
GROUP	=	lp
LEVEL	=	SYS_PUBLIC
SUPER	=	root

#####
#
# $(EMODES): Modes for executables
# $(SMODES): Modes for setuid executables
# $(DMODES): Modes for directories
#####
EMODES	=	0555
SMODES	=	04555
DMODES	=	0775


INCSYS  =       $(INC)/sys

LPINC	=	$(TOP)/include
LPLIB	=	$(TOP)/lib

LIBACC	=	$(LPLIB)/access/liblpacc.a
LIBCLS	=	$(LPLIB)/class/liblpcls.a
LIBFLT	=	$(LPLIB)/filters/liblpflt.a
LIBFRM	=	$(LPLIB)/forms/liblpfrm.a
LIBLP	=	$(LPLIB)/lp/liblp.a
LIBMSG	=	$(LPLIB)/msgs/liblpmsg.a
LIBOAM	=	$(LPLIB)/oam/liblpoam.a
LIBPRT	=	$(LPLIB)/printers/liblpprt.a
LIBREQ	=	$(LPLIB)/requests/liblpreq.a
LIBSEC	=	$(LPLIB)/secure/liblpsec.a
LIBSYS	=	$(LPLIB)/systems/liblpsys.a
LIBUSR	=	$(LPLIB)/users/liblpusr.a
#LIBNET	=       $(LPLIB)/lpNet/liblpNet.a
LIBBSD  =       $(LPLIB)/bsd/liblpbsd.a

LINTACC	=	$(LPLIB)/access/llib-llpacc.ln
LINTBSD =	$(LPLIB)/bsd/llib-llpbsd.ln
LINTCLS	=	$(LPLIB)/class/llib-llpcls.ln
LINTFLT	=	$(LPLIB)/filters/llib-llpflt.ln
LINTFRM	=	$(LPLIB)/forms/llib-llpfrm.ln
LINTLP	=	$(LPLIB)/lp/llib-llp.ln
LINTNET =	$(LPLIB)/lpNet/llib-llpNet.ln
LINTMSG	=	$(LPLIB)/msgs/llib-llpmsg.ln
LINTOAM	=	$(LPLIB)/oam/llib-llpoam.ln
LINTPRT	=	$(LPLIB)/printers/llib-llpprt.ln
LINTREQ	=	$(LPLIB)/requests/llib-llpreq.ln
LINTSEC	=	$(LPLIB)/secure/llib-llpsec.ln
LINTSYS	=	$(LPLIB)/systems/llib-llpsys.ln
LINTUSR	=	$(LPLIB)/users/llib-llpusr.ln

LINTLN  =	$(LINTACC) \
		$(LINTCLS) \
		$(LINTFLT) \
		$(LINTFRM) \
		$(LINTLP) \
		$(LINTMSG) \
		$(LINTOAM) \
		$(LINTPRT) \
		$(LINTREQ) \
		$(LINTSEC) \
		$(LINTSYS) \
		$(LINTUSR)
