#ident "@(#)nd-test.mk  4.2"
#
# 	Copyright (C) The Santa Cruz Operation, 1996
# 	This module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as
#	Confidential
#
##########################################################################
#
#    NAME:            TET Top Level Makefile
#    PRODUCT:         TET (Test Environment Toolkit)
#    TARGETS:         all
#    MODIFICATIONS:
#
##########################################################################
 
# Before running this makefile, you should edit the following subsidiary
# makefiles:
#		tet_root/src/posix_c/makefile
#		tet_root/src/xpg3sh/api/makefile
#		tet_root/src/ksh/api/makefile
#		tet_root/src/perl/makefile
#

# Full pathname of the TET root directory to be passed down to API makefiles.
# This can be overriden at runtime using the TET_ROOT environment variable
# -> YOU PROBABLY WANT TO CHANGE THE VALUE BELOW.
#TET_ROOT =	/tmp/etet
#include $(CMDRULES)

# By default the "all" target includes
#
#     TET POSIX C API
#     TET XPG3 Shell API
#     TET Korn Shell API
#     TET Perl API
#
# To not build one of these targets just remove them from the all: line.
# To add the TET C++ binding to the default build add the target cplusplus
# to the following line.
#all:	posix_c xpg3sh ksh perl 
all:	posix_c xpg3sh 

posix_c:	nofile
	cd posix_c ; $(MAKE) -f posix_c.mk 

xpg3sh:	nofile
	cd xpg3sh/api ; $(MAKE) -f api.mk

ksh:	nofile
	cd ksh/api ; $(MAKE) -f api.mk TET_ROOT="$(TET_ROOT)"


perl:	nofile
	cd perl ; $(MAKE) -f perl.mk TET_ROOT="$(TET_ROOT)"

cplusplus: nofile
	cd posix_c ; $(MAKE) TET_ROOT="$TET_ROOT" cplusplus

clean:
	cd posix_c ; $(MAKE) clean
	cd xpg3sh/api ; $(MAKE) clean
	cd ksh/api; $(MAKE) clean
	cd perl; $(MAKE) clean

CLEAN:	clean

CLOBBER:
	cd posix_c ; $(MAKE) CLOBBER
	cd xpg3sh/api ; $(MAKE) CLOBBER
	cd ksh/api ; $(MAKE) CLOBBER
	cd perl ; $(MAKE) CLOBBER

nofile:

