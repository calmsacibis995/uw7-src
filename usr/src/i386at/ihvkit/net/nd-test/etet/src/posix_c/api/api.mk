#
# Copyright 1990 Open Software Foundation (OSF)
# Copyright 1990 Unix International (UI)
# Copyright 1990 X/Open Company Limited (X/Open)
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation, and that the name of OSF, UI or X/Open not be used in 
# advertising or publicity pertaining to distribution of the software 
# without specific, written prior permission.  OSF, UI and X/Open make 
# no representations about the suitability of this software for any purpose.  
# It is provided "as is" without express or implied warranty.
#
# OSF, UI and X/Open DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO 
# EVENT SHALL OSF, UI or X/Open BE LIABLE FOR ANY SPECIAL, INDIRECT OR 
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
# USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
# PERFORMANCE OF THIS SOFTWARE.

##########################################################################
#
#	SCCS:		@(#)makefile	1.8 11/01/91
#	NAME:		'C' API
#	PRODUCT:	TET (Test Environment Toolkit)
#	AUTHOR:		Geoff Clare, UniSoft Ltd.
#	DATE CREATED:	23 June 1990
#	TARGETS:	tcm.o tcmchild.o libapi.a
#	MODIFICATIONS:
#
#		Geoff Clare, 10 Sept 1990
#			Use -I$(TET_ROOT)/inc
#			Add install target
#
#		Geoff Clare, 10 Oct 1990
#			Change file names and install directories
#
#		Geoff Clare, 22 Oct 1990
#			Default target is INSTALL
#			Distribute tet_api.h in $(TET_ROOT)/inc/posix_c
#				instead of "." & modify references to it
#
#		Geoff Clare, 30 Nov 1990
#			Change -I$(TET_ROOT)/inc to -I../inc
#
#		SunSoft ETET Extension, May 1993
#		  	Add -D_REENTRANT for multi threading
#
#		Andrew Josey, 12 May 1993
#			Add libapi.a to CLEAN action
#
##########################################################################

# PLEASE NOTE THAT THIS FILE SHOULD NOT NEED EDITING. IT SHOULD
# SUFFICE JUST TO EDIT THE MAKEFILE IN THE DIRECTORY ABOVE THIS ONE.

# Default directory locations
TET_ROOT =	../../..
INSTLIB =	$(TET_ROOT)/lib/posix_c

CC =	cc
RM =	rm -f
LINT =	lint

# Library building commands: if $(AR) adds symbol tables itself, or the
# system has ranlib, set LORDER=echo and TSORT=cat.  If ranlib is not
# required, set RANLIB=echo.
AR =	ar cr
RANLIB =	echo
LORDER =	echo
TSORT =	cat

# Compiler options: these need to be changed to enable NSIG to be defined.
# Either add another feature test macro which will make it visible in
# <signal.h>, or add -DNSIG=<value>.  NSIG should be 1 greater than
# the highest supported signal number on the system.
COPTS =	 -g
#DEFINES =	-D_POSIX_SOURCE
INCS =		-I../inc -I$(TET_ROOT)/inc/posix_c
CFLOCAL =

# the _REENTRANT define is so the api will work for multi-threaded test
# cases -- it should have no effect on non-threaded tests
# this is only for SunSoft
#CFLAGS =	$(CFLOCAL) $(INCS) $(DEFINES) $(COPTS) -DTET_SIG_IGN=$(SIG_IGNORE) -DTET_SIG_LEAVE=$(SIG_LEAVE) -D_REENTRANT
CFLAGS =	$(CFLOCAL) $(INCS) $(DEFINES) $(COPTS) -DTET_SIG_IGN=$(SIG_IGNORE) -DTET_SIG_LEAVE=$(SIG_LEAVE) 

LINTFLAGS =	$(INCS) $(DEFINES) -n
LINTLIBS =	-lposix

CFILES_TCM =	tcm.c
OFILES_TCM =	tcm.o
CFILES_TCMCH =	tcmchild.c
OFILES_TCMCH =	tcmchild.o
CFILES_API =	cancel.c config.c resfile.c tet_exec.c tet_fork.c
OFILES_API =	cancel.o config.o resfile.o tet_exec.o tet_fork.o

CFILES =	$(CFILES_TCM) $(CFILES_TCMCH) $(CFILES_API)
OFILES =	$(OFILES_TCM) $(OFILES_TCMCH) $(OFILES_API)


##############################################################################
#include $(CMDRULES)

$(INSTLIB)/INSTALLED:	$(OFILES_TCM) $(OFILES_TCMCH) libapi.a
		[ -d $(INSTLIB) ] || mkdir -p $(INSTLIB)
		cp $(OFILES_TCM) $(OFILES_TCMCH) libapi.a $(INSTLIB)
		touch $(INSTLIB)/INSTALLED

all:		$(OFILES_TCM) $(OFILES_TCMCH) libapi.a

libapi.a:	$(OFILES_API)
		$(AR) $@ `$(LORDER) $(OFILES_API) | $(TSORT)`
		$(RANLIB) $@

$(OFILES):	$(TET_ROOT)/inc/posix_c/tet_api.h

FORCE:		CLOBBER all

LINT:
		$(LINT) $(LINTFLAGS) $(CFILES) $(LINTLIBS)

LINTLIB:	llib-ltcm.ln llib-ltcmc.ln

llib-ltcm.ln:	llib-ltcm.c
		$(LINT) -o tcm $(LINTFLAGS) -u $?

llib-ltcmc.ln:	llib-ltcmc.c
		$(LINT) -o tcmc $(LINTFLAGS) -u $?

CLEAN:
		$(RM) $(OFILES) makefile.bak llib-ltcm.ln llib-ltcmc.ln libapi.a

CLOBBER:	CLEAN
		$(RM) $(INSTLIB)/tcm.o $(INSTLIB)/tcmchild.o \
			$(INSTLIB)/libapi.a 

clean:		CLEAN
