#ident	"@(#)in.bootpd.mk	1.3"

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

#
# Copyrighted as an unpublished work.
# (c) Copyright 1987-1994 Lachman Technology, Inc.
# All rights reserved.
#

#
# Makefile for the BOOTP programs:
#   bootpd	- BOOTP server daemon
#   bootpef	- BOOTP extension file builder
#   bootpgw	- BOOTP gateway daemon
#   bootptest	- BOOTP tester (client)
#

# OPTion DEFinitions:
# Remove the -DVEND_CMU if you don't wish to support the "CMU vendor format"
# in addition to the RFC1048 format.  Leaving out DEBUG saves little.
include $(CMDRULES)
#CFLAGS= -DVEND_CMU -DDEBUG
#OPTDEFS= -DVEND_CMU -DDEBUG

# SYStem DEFinitions:
# Either uncomment one of the following, or do:
#	"make sunos4"	(or "make sunos5")
# SYSDEFS= -DSUNOS -DETC_ETHERS
#SYSDEFS= -DSVR4
SYSLIBS= -lsocket -lnsl
#Note: do not define -DBSD_COMP
LOCALDEF=-DVEND_CMU -DDEBUG -DSVR4 -DSYSV -DSTRNET -DUSE_TLI

# Uncomment this if your system does not provide streror(3)
# STRERROR=strerror.o

# FILE DEFinitions:
# The next few lines may be uncommented and changed to alter the default
# filenames bootpd uses for its configuration and dump files.
#CONFFILE= -DCONFIG_FILE=\"/usr/etc/bootptab\"
#DUMPFILE= -DDUMP_FILE=\"/usr/etc/bootpd.dump\"
#FILEDEFS= $(CONFFILE) $(DUMPFILE)

# MORE DEFinitions (whatever you might want add)
# One might define NDEBUG (to remove "assert()" checks).
# MOREDEFS=

INSTALL=$(INS)
INSDIR=$(USRSBIN)
OWN=            bin
GRP=            bin
ETCINET=${ETC}/inet
CFLAGS= $(OPTDEFS) $(SYSDEFS) $(FILEDEFS) $(MOREDEFS)
PROGS= in.bootpd in.bootpef in.bootpgw
#PROGS= in.bootpd bootpef bootpgw bootptest
TESTS= trylook trygetif trygetea

all: $(PROGS)

tests: $(TESTS)

system: install

bootptab.samp:
	cp bootptab $@

install: $(PROGS) bootptab.samp
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) in.bootpd
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) in.bootpef
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) in.bootpgw
		$(INS) -f $(ETCINET) -m 0444 -u root -g sys bootptab.samp

clean:
	-rm -f core *.o

clobber: clean
	-rm -f $(PROGS) $(TESTS)


#
# Handy targets for individual systems:
#

# SunOS 4.X
sunos4:
	$(MAKE) SYSDEFS="-DSUNOS -DETC_ETHERS" \
		STRERROR=strerror.o

# Solaris 2.X (i.e. SunOS 5.X)
sunos5:
	$(MAKE) SYSDEFS="-DSVR4 -DETC_ETHERS" \
		SYSLIBS="-lsocket -lnsl"

# UNIX System V Rel. 4 (also: IRIX 5.X, others)
svr4:
	$(MAKE) SYSDEFS="-DSVR4" SYSLIBS="-lsocket -lnsl"

# Control Data EP/IX 1.4.3 system, BSD 4.3 mode
epix143:
	$(MAKE) CC="cc -systype bsd43" \
	  SYSDEFS="-Dconst= -D_SIZE_T -DNO_UNISTD"

# Control Data EP/IX 2.1.1 system, SVR4 mode
epix211:
	$(MAKE) CC="cc -systype svr4" \
	  SYSDEFS="-DSVR4" \
	  SYSLIBS="-lsocket -lnsl"

#
# How to build each program:
#

OBJ_D=	bootpd.o dovend.o readfile.o hash.o dumptab.o \
	 lookup.o getif.o hwaddr.o tzone.o report.o endpt.o $(STRERROR)
in.bootpd: $(OBJ_D)
	$(CC) -o $@ $(OBJ_D) $(SYSLIBS)

OBJ_EF=	bootpef.o dovend.o readfile.o hash.o dumptab.o \
	 lookup.o hwaddr.o tzone.o report.o $(STRERROR)
in.bootpef: $(OBJ_EF)
	$(CC) -o $@ $(OBJ_EF) $(SYSLIBS)

OBJ_GW= bootpgw.o getif.o hwaddr.o report.o endpt.o $(STRERROR)
in.bootpgw: $(OBJ_GW)
	$(CC) -o $@ $(OBJ_GW) $(SYSLIBS)

OBJ_TEST= bootptest.o print-bootp.o getif.o getether.o \
	 report.o $(STRERROR)
bootptest: $(OBJ_TEST)
	$(CC) -o $@ $(OBJ_TEST) $(SYSLIBS)

print-bootp.o : print-bootp.c
	$(CC) $(CFLAGS) -DBOOTPTEST -c $<

# This is just for testing the lookup functions.
TRYLOOK= trylook.o lookup.o report.o $(STRERROR)
trylook : $(TRYLOOK)
	$(CC) -o $@ $(TRYLOOK) $(SYSLIBS)

# This is just for testing getif.
TRYGETIF= trygetif.o getif.o report.o $(STRERROR)
trygetif : $(TRYGETIF)
	$(CC) -o $@ $(TRYGETIF) $(SYSLIBS)

# This is just for testing getether.
TRYGETEA= trygetea.o getether.o report.o $(STRERROR)
trygetea : $(TRYGETEA)
	$(CC) -o $@ $(TRYGETEA) $(SYSLIBS)

# This rule keeps the define only where it is needed.
report.o : report.c
	$(CC) $(CFLAGS) -DLOG_BOOTP=LOG_LOCAL2 -c $<

# Punt SunOS -target noise

# These are out of date

#
# Header file dependencies:
#

bootpd.o : bootp.h hash.h bootpd.h dovend.h getif.h hwaddr.h
bootpd.o : readfile.h report.h tzone.h patchlevel.h

bootpef.o: bootp.h hash.h bootpd.h dovend.h hwaddr.h
bootpef.o: readfile.h report.h tzone.h patchlevel.h

bootpgw.o: bootp.h getif.h hwaddr.h report.h patchlevel.h

bootptest.o : bootp.h bootptest.h patchlevel.h
dovend.o: bootp.h bootpd.h report.h dovend.h
dumptab.o: bootp.h hash.h bootpd.h report.h patchlevel.h
getif.o: getif.h
hash.o: hash.h
hwaddr.o: hwaddr.h report.h
lookup.o: lookup.h report.h
print-bootp.o : bootp.h bootptest.h
readfile.o: bootp.h hash.h bootpd.h lookup.h readfile.h
report.o: report.h
tzone.o: tzone.h
