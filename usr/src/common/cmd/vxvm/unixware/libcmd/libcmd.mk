# @(#)cmd.vxvm:unixware/libcmd/libcmd.mk	1.7 10/10/97 15:55:38 - cmd.vxvm:unixware/libcmd/libcmd.mk
#ident	"@(#)cmd.vxvm:unixware/libcmd/libcmd.mk	1.7"

# Copyright(C)1996 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
# UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
# LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
# IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
# OR DISCLOSURE.
# 
# THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
# TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
# OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
# EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
# 
#               RESTRICTED RIGHTS LEGEND
# USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
# SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
# (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
# COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
#               VERITAS SOFTWARE
# 1600 PLYMOUTH STREET, MOUNTAIN VIEW, CA 94043

COMMON_DIR = ../../common/libcmd
LINKDIR    = ../../unixware/libcmd

include $(COMMON_DIR)/libcmd.mk

include $(CMDRULES)

# Force cmd rules to include from $(ROOT)/$(MACH)
INC = $(ROOT)/$(MACH)/usr/include

# Pick a probe file as the key of restricted source
PROBEFILE = $(COMMON_DIR)/volmakesup.c
MAKEFILE = libcmd.mk

LIBCMD_OBJS = $(COM_OBJS)
LIBCMD_SRCS = $(COM_SRCS)
LIBCMD_HDRS = libcmd_sys.h

LIBRARY = libcmd.a
LINT_LIBRARY_SUFFIX=libcmd
LINT_LIBRARY=llib-l$(LINT_LIBRARY_SUFFIX).ln
LOCALDEF = $(LOCAL)

LOCALINC = -I$(COMMON_DIR) -I$(LINKDIR)

PIC =

.c.o:
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<
	@if [ "$(@D)" != "./" ]; then ln -s $(LINKDIR)/$(@F) $*.o; fi

.IGNORE:

all: headinstall
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(LIBRARY) $(MAKEARGS) ;\
	else \
		for f in $(LIBRARY); do \
			if [ ! -f $$f ]; then \
				echo "ERROR: $$f is missing" 1>&2 ;\
				false ;\
				break; \
			fi \
		done \
	fi

install: all

$(LIBRARY): $(LIBCMD_OBJS)
	echo "Making all in cmd/vxvm/unixware/libcmd"
	$(AR) $(ARFLAGS) $(LIBRARY) `$(LORDER) $(LIBCMD_OBJS) | $(TSORT)`

lint: $(LINT_LIBRARY)

$(LINT_LIBRARY): $(LIBCMD_SRCS)
	$(LINT) $(LINT_LIBRARY_FLAGS) -o$(LINT_LIBRARY_SUFFIX) $(LIBCMD_SRCS);

headinstall:
	@echo "Making headinstall in cmd/vxvm/unixware/libcmd" ; \
	if [ -f $(PROBEFILE) ]; then \
		$(INS) -f $(INC) -m 0644 -u root -g sys $(LIBCMD_HDRS) ; \
	fi

clean:
	@echo "Making clean in cmd/vxvm/unixware/libcmd"
	rm -f $(LIBCMD_OBJS)
	rm -f *.o

lintclean:
	rm -f $(LINT_LIBRARY)

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "Making clobber in cmd/vxvm/unixware/libcmd" ; \
		rm -f $(LIBRARY) ; \
		for h in $(LIBCMD_HDRS) ; \
		do \
			f=`basename $$h` ; \
			rm -f $(INC)/$$f ; \
		done \
	fi

