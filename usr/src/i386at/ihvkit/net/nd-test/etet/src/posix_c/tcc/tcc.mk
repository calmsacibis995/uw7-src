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
#    SCCS:            @(#)makefile	1.10 06/23/92
#    NAME:            TET Tools Makefile
#    PRODUCT:         TET (Test Environment Toolkit)
#    AUTHOR:          OSF Validation & SQA
#    DATE CREATED:    14 May 1991
#    TARGETS:         tcc
#    MODIFICATIONS:
#                     "TET Rework"
#                     UniSoft Ltd, August 1991.
#
##########################################################################

# these are normally inherited from the top level makefile.
TET_ROOT   = ../../..
COPTS      =
DEFINES    = -D_POSIX_SOURCE -DNSIG=32
SIG_IGNORE =
SIG_LEAVE  = 20

RM         = rm -f
BIN        = $(TET_ROOT)/bin
INCDIR     = $(TET_ROOT)/src/posix_c/inc
INC        = -I$(INCDIR) 

CFLAGS     = $(INC) -DTET_ROOT='"$(TET_ROOT)"' $(COPTS) $(DEFINES) \
		-DTET_SIG_IGNORE="$(SIG_IGNORE)" -DTET_SIG_LEAVE="$(SIG_LEAVE)"

LINT       = lint
LINTFLAGS  = $(INC) $(DEFINES) -n
LINTLIBS   = -lposix

TCCOBJS	   = again.o config.o exec.o journal.o scenario.o startit.o tcc.o \
             tool.o newscen.o compress_write.o
TCCFILES   = again.c config.c exec.c journal.c scenario.c startit.c tcc.c \
             tool.c newscen.c compress_write.c
TCCHDRS    = $(INCDIR)/tcc_env.h $(INCDIR)/tcc_mac.h $(INCDIR)/tet_jrnl.h \
            $(INCDIR)/tcc_prot.h

#include $(CMDRULES)


INSTALL: $(BIN)/tcc


$(BIN)/tcc: tcc
	cp -f tcc $(BIN)/tcc
	chmod a+rx $(BIN)/tcc

all:	tcc 


tcc:	$(TCCOBJS)	
	$(CC) -o tcc $(TCCOBJS)

$(TCCOBJS): $(TCCHDRS)

clean:	CLEAN

CLEAN:
	-$(RM) $(TCCOBJS) tcc 

CLOBBER: CLEAN
	-$(RM) $(BIN)/tcc


FORCE:	CLOBBER all


LINT:	LINT_TCC

LINT_TCC:
	$(LINT) $(LINTFLAGS) $(TCCFILES) $(LINTLIBS)

