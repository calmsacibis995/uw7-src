#
#  Copyright 1993 UNIX System Laboratories, Inc. (USL)
# 
#  Permission to use, copy, modify, and distribute this software and its
#  documentation for any purpose and without fee is hereby granted, provided
#  that the above copyright notice appear in all copies and that both that
#  copyright notice and this permission notice appear in supporting
#  documentation, and that the name of USL not be used in 
#  advertising or publicity pertaining to distribution of the software 
#  without specific, written prior permission.  USL makes
#  no representations about the suitability of this software for any purpose.  
#  It is provided "as is" without express or implied warranty.
# 
#  USL DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
#  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO 
#  EVENT SHALL USL BE LIABLE FOR ANY SPECIAL, INDIRECT OR 
#  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
#  USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
#  OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
#  PERFORMANCE OF THIS SOFTWARE.
# 
##########################################################################
#
#    SCCS:            @(#)etet/src/posix_c/cplusplus/makefile	1.2
#    NAME:            C++ Simple Binding Top Level Makefile
#    PRODUCT:         TET (Test Environment Toolkit)
#    AUTHOR:          USL
#    DATE CREATED:    10 May 1993
#    TARGETS:         libCtcm.a libCtcmc.a
#    MODIFICATIONS:
#
##########################################################################
 
# Default directory locations
TET_ROOT =	../../..
INSTLIB =	$(TET_ROOT)/lib/posix_c
C_PLUSTCM = libCtcm.a
C_PLUSTCMC = libCtcmc.a
LIBFILES = $(C_PLUSTCM) $(C_PLUSTCMC)
#inlcude $(CMDRULES)

# NOTE That this makefile builds both C and C++ objects and hence
# requires both compilers to be named.
# CC is the C compiler
# C_PLUS is the C++ compiler
# C_SUFFIX is the suffix recognised by the C++ compiler for C++ files
#  (for those compilers that don't recognise .C)
#
# So far we have tested this with the following:
#  USL C++ 3.0.2
#  GNU g++ 2.3.3

# C compiler
CC =	cc

# GNU C compiler
#CC =	gcc


# C++ compiler
C_PLUS = CC

# GNU C++ compiler (g++)
#C_PLUS = g++

# C++ suffix
# The default is .C which USL C++ and g++ recognise
C_SUFFIX =	C
# If your C++ compiler recognises some other format then
# you'll have to rename the .C files in this directory
#C_SUFFIX=c++
#C_SUFFIX= cc

RM =	rm -f

# Library building commands: if $(AR) adds symbol tables itself, or the
# system has ranlib, set LORDER=echo and TSORT=cat.  If ranlib is not
# required, set RANLIB=echo.

AR =	ar cr
RANLIB =	echo
LORDER =	echo
TSORT =	cat

# For SunOS41, RANLIB=ranlib
#RANLIB =	ranlib

COPTS =	-c
DEFINES =	$(OS)
# For SunOS41 with g++
#DEFINES =       $(OS) -D_WCHAR_T

INCS =          -I../inc -I$(TET_ROOT)/inc/posix_c
CFLOCAL =
CFLAGS =	$(CFLOCAL) $(INCS) $(DEFINES) $(COPTS)

# For SunOS41 CFLAGS
#CFLAGS = -I$(INCDIR) -D_POSIX_SOURCE -DNSIG=32 -DEXIT_SUCCESS=0 -DEXIT_FAILURE=1 \
# -DNULL=0 $(CFLOCAL) $(INCS) $(DEFINES) $(COPTS)


C_FILES =	tcm.$(C_SUFFIX) tcmchild.$(C_SUFFIX)
O_FILES =	tcm.o tcmchild.o

OFILES =	c_tcm.o c_tcmchild.o
#

all:		$(OFILES)  $(O_FILES) $(C_PLUSTCM) $(C_PLUSTCMC)
		cp $(C_PLUSTCM) $(INSTLIB)
		cp $(C_PLUSTCMC) $(INSTLIB)
		
tcm.o:		tcm.$(C_SUFFIX)
		$(C_PLUS) $(CFLAGS) tcm.$(C_SUFFIX)

c_tcm.o:
		$(CC) $(CFLAGS) c_tcm.c

$(C_PLUSTCM):	tcm.o c_tcm.o
		$(AR) $@ `$(LORDER) tcm.o c_tcm.o | $(TSORT)`
		$(RANLIB) $@

tcmchild.o:	tcmchild.$(C_SUFFIX)
		$(C_PLUS) $(CFLAGS) tcmchild.$(C_SUFFIX)

c_tcmchild.o:
		$(CC) $(CFLAGS) c_tcmchild.c

$(C_PLUSTCMC):	tcmchild.o c_tcmchild.o $(TET_ROOT)/inc/posix_c/tet_api.h
		$(AR) $@ `$(LORDER) tcmchild.o c_tcmchild.o | $(TSORT)`
		$(RANLIB) $@


FORCE:		CLOBBER all


CLEAN:
		$(RM) $(OFILES) $(O_FILES) Makefile.bak $(LIBFILES)

CLOBBER:	CLEAN
		$(RM) $(INSTLIB)/$(C_PLUSTCM)  $(INSTLIB)/$(C_PLUSTCMC)

clean:		CLEAN


clobber:	CLOBBER
