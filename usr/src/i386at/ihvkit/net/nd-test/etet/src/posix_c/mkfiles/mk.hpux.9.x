# Sample makefile for HPUX 9.x
#
#  Copyright 1990 Open Software Foundation (OSF)
#  Copyright 1990 Unix International (UI)
#  Copyright 1990 X/Open Company Limited (X/Open)
# 
#  Permission to use, copy, modify, and distribute this software and its
#  documentation for any purpose and without fee is hereby granted, provided
#  that the above copyright notice appear in all copies and that both that
#  copyright notice and this permission notice appear in supporting
#  documentation, and that the name of OSF, UI or X/Open not be used in 
#  advertising or publicity pertaining to distribution of the software 
#  without specific, written prior permission.  OSF, UI and X/Open make 
#  no representations about the suitability of this software for any purpose.  
#  It is provided "as is" without express or implied warranty.
# 
#  OSF, UI and X/Open DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
#  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO 
#  EVENT SHALL OSF, UI or X/Open BE LIABLE FOR ANY SPECIAL, INDIRECT OR 
#  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
#  USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
#  OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
#  PERFORMANCE OF THIS SOFTWARE.
# 
##########################################################################
#
#    SCCS:            @(#)makefile	1.8 11/01/91
#    NAME:            TET Top Level Makefile
#    PRODUCT:         TET (Test Environment Toolkit)
#    AUTHOR:          OSF Validation & SQA
#    DATE CREATED:    14 May 1991
#    TARGETS:         tcc & api
#    MODIFICATIONS:
#                     "TET Rework"
#                     UniSoft Ltd, August 1991.
#
#		      Additions for SunSoft API extensions in sun_lib
#		      SunSoft ETET update. May 1993
#
#		      Additions for C++ API binding
#		      UNIX System Labs, Inc, ETET Update, May 1993.
#
#		      Additions for ETET1.10.1
#		      UNIX System Labs, Inc, June 1993.
#
#		      Additions for ETET1.10.2. Add SIG_LEAVE defaults
#		      to handle Job Control signals.
#		      UNIX System Labs, Inc, October 1993.
##########################################################################
 

# Full pathname of the TET root directory, to be compiled into the TCC
# This can be overriden at runtime using the TET_ROOT environment variable
# -> YOU PROBABLY WANT TO CHANGE THE VALUE BELOW.
TET_ROOT =	/tmp/etet

# C compiler command
# If you use a compiler other than cc you'll need to change this, some
# examples are given below:
CC =		cc

# For AIX use c89
# CC =		c89
#
# GNU gcc
# CC =		gcc

# Compiler options: these need to be changed to enable NSIG to be defined.
# Either add another feature test macro which will make it visible in
# <signal.h>, or add -DNSIG=<value>.  NSIG should be 1 greater than
# the highest supported signal number on the system.

# For SVR4, SunOS, Ultrix 4.2 : NSIG=32
#NSIG= 32

# For SGI Irix NSIG=33
#NSIG= 33

# For AIX 3.1  NSIG=64
#NSIG= 64

#COPTS =
# For SVR4.0 COPTS=-Xc, NSIG=32
#COPTS =	 -Xc 

# For HP-UX 9.x, COPTS=-Aa,  NSIG=31
NSIG= 31
COPTS = -Aa

DEFINES =	-D_POSIX_SOURCE -DNSIG=$(NSIG) 

# For SunOS 4.1 define SunOS41 and set  RANLIB=ranlib
# and optionally -DSUNTEST will also activate alternate temporary results file
# handling - without SUNTEST this can be activated at runtime
# by setting TET_EXTENDED=true
#DEFINES =	-D_POSIX_SOURCE -DNSIG=$(NSIG) -DSunOS41 \
# -DEXIT_SUCCESS=0 -DEXIT_FAILURE=1 -DNULL=0

# For ICL DRS/NX on SPARC add -Dsparc 
#COPTS = -Xc
#NSIG = 32
#DEFINES =	-D_POSIX_SOURCE -DNSIG=$(NSIG) -Dsparc

# For Ultrix 4.2, NSIG=32, RANLIB=ranlib, add -YPOSIX, 
#DEFINES =	-D_POSIX_SOURCE -DNSIG=$(NSIG) -YPOSIX

# Comma-separated list of signal numbers to be ignored by the TCC
#SIG_IGNORE =

#For HP-UX 9.x, window size change signal SIGWINCH=23
SIG_IGNORE = 23 

# Comma-separated list of signal numbers to be left alone by the TCC.
# Usually includes number for SIGCLD, if supported separately from SIGCHLD.
# Include job control signals if you want to use job control on the TCC.
# The job control signals to include are: SIGTSTP and SIGCONT, the 
# example below is for SVR4
#SIG_LEAVE =24,25

# For HP-UX 9.x  SIGSTP=25, SIGCONT=26
SIG_LEAVE =25,26

# Library building commands: if $(AR) adds symbol tables itself, or the
# system has ranlib, set LORDER=echo and TSORT=cat.  If ranlib is not
# required, set RANLIB=echo.
AR =		ar cr
RANLIB =	echo
LORDER =	echo
TSORT =		cat

# For SunOS4 and Ultrix 4.2, RANLIB=ranlib
#RANLIB =	ranlib

all:	tcc api sun_lib

tcc:	nofile
	cd tcc ; $(MAKE) TET_ROOT="$(TET_ROOT)" CC="$(CC)" COPTS="$(COPTS)" \
		DEFINES="$(DEFINES)" SIG_IGNORE="$(SIG_IGNORE)" \
		SIG_LEAVE="$(SIG_LEAVE)"

api:	nofile
	cd api ; $(MAKE) CC="$(CC)" COPTS="$(COPTS)" DEFINES="$(DEFINES)" \
		SIG_IGNORE="$(SIG_IGNORE)" SIG_LEAVE="$(SIG_LEAVE)" \
		AR="$(AR)" RANLIB="$(RANLIB)" LORDER="$(LORDER)" TSORT="$(TSORT)"
sun_lib: nofile
	cd sun_lib ;$(MAKE) CC="$(CC)" COPTS="$(COPTS)" DEFINES="$(DEFINES)" \
        AR="$(AR)" RANLIB="$(RANLIB)" LORDER="$(LORDER)" \
        TSORT="$(TSORT)"

cplusplus: nofile
	cd cplusplus ;$(MAKE)  \
        AR="$(AR)" RANLIB="$(RANLIB)" LORDER="$(LORDER)" \
        TSORT="$(TSORT)"

clean:
	cd tcc ; $(MAKE) clean
	cd api ; $(MAKE) clean
	cd sun_lib; $(MAKE) clean

nofile:

