# @(#)cmd.vxvm:unixware/volassist/volassist.mk	1.6 10/10/97 15:59:02 - cmd.vxvm:unixware/volassist/volassist.mk
#ident	"@(#)cmd.vxvm:unixware/volassist/volassist.mk	1.6"

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

COMMON_DIR = ../../common/volassist
LINKDIR    = ../../unixware/volassist

include $(COMMON_DIR)/volassist.mk

include $(CMDRULES)

# Force cmdrules include from $(TOOLS) to $(ROOT)/$(MACH)
INC = $(ROOT)/$(MACH)/usr/include

# Pick a probe file as the key of restricted source
PROBEFILE = $(COMMON_DIR)/mirror.c
MAKEFILE = volassist.mk

LIBCMD_DIR = ../libcmd

LDLIBS = $(LIBCMD_DIR)/libcmd.a $(LIBVXVM) -lgen
# no lint library for gen
LINTLIB = -L$(LIBCMD_DIR) -llibcmd $(LIBVXVM) 
LOCALINC = $(COM_LOCALINC) -I$(LIBCMD_DIR) -I$(LINKDIR)
#LOCALDEF = -DUW_2_1 -DLIC $(I18N)
LOCALDEF = $(LOCAL)

VASSIST_OBJS = $(COM_VASSIST_OBJS) sysdep.o
VASSIST_LINT_OBJS = $(COM_VASSIST_LINT_OBJS) sysdep.ln
VASSIST_SRCS = $(COM_VASSIST_SRCS) sysdep.c

VASSIST_TARGETS = $(COM_TARGETS) 

.c.o:
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<
	@if [ "$(@D)" != "./" ]; then ln -s $(LINKDIR)/$(@F) $*.o; fi

.IGNORE:

all:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(VASSIST_TARGETS) $(MAKEARGS) ;\
	else \
		for f in $(VASSIST_TARGETS); do \
			if [ ! -f $$f ]; then \
				echo "ERROR: $$f is missing" 1>&2 ;\
				false ;\
				break; \
			fi \
		done \
	fi

install: all
	@echo "Making install in cmd/vxvm/unixware/volassist"
	for f in $(VASSIST_TARGETS) ; \
	do \
		rm -f $(USRSBIN)/$$f ; \
		$(INS) -f $(USRSBIN) -m 0755 -u root -g sys $$f ; \
	done

lint:	$(VASSIST_LINT_OBJS)
	$(LINT) $(LINT_LINK_FLAGS) $(LINTFLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VASSIST_LINT_OBJS)
	touch lint


vxassist: $(VASSIST_OBJS)
	$(CC) -o $@ $(VASSIST_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) $(LDLIBS)

headinstall:
	@echo "Making headinstall in cmd/vxvm/unixware/volassist"

clean:
	rm -f $(VASSIST_OBJS)
	rm -f *.o

lintclean:
	rm -f $(VASSIST_LINT_OBJS) lint

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		rm -f $(VASSIST_TARGETS) ; \
	fi

$(VASSIST_OBJS): $(COMMON_DIR)/volassist.h $(COMMON_DIR)/proto.h

