# @(#)cmd.vxvm:unixware/vold/vold.mk	1.7 10/10/97 15:59:03 - cmd.vxvm:unixware/vold/vold.mk
#ident	"@(#)cmd.vxvm:unixware/vold/vold.mk	1.7"

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

COMMON_DIR = ../../common/vold
LINKDIR    = ../../unixware/vold

include $(COMMON_DIR)/vold.mk

include $(CMDRULES)

# Force cmdrules include from $(ROOT)/$(MACH) instead of $(TOOLS)
INC = $(ROOT)/$(MACH)/usr/include

LIC_INC = ../../../../subsys/license/i4/iforpm/includes
LIC_LIB = ../../../../subsys/license/i4/libs

# Do not enable the optimizer, vxinstall may hang in kernel. 
CFLAGS =

# Pick a probe file as the key of restricted source
PROBEFILE = $(COMMON_DIR)/voldb.c
MAKEFILE = vold.mk

DIAG_DIR = $(ROOT)/$(MACH)/usr/lib/vxvm/diag.d

# no lint library for gen
LINTLIB  = $(LIBVXVM)

LOCALINC = $(COM_LOCALINC) -I. -I$(LIC_INC) -I$(LINKDIR)

VCONFIGD_OBJS = $(COM_CONFIGD_OBJS) \
	autostart.o \
	ap.o \
	apf.o \
	devintf.o \
	devmap.o \
	kernel_sys.o \
	ncopy.o \
	node.o \
	main.o \
	misc_sys.o \
	voldthreads.o

VCONFIGD_LINT_OBJS = $(COM_CONFIGD_LINT_OBJS) \
	autostart.ln \
	ap.ln \
	apf.ln \
	devintf.ln \
	devmap.ln \
	kernel_sys.ln \
	ncopy.ln \
	node.ln \
	main.ln \
	misc_sys.ln \
	voldthreads.ln

VCONFIGD_SRCS = $(COM_CONFIGD_SRCS) \
	autostart.c \
	ap.c \
	apf.c \
	devintf.c \
	devmap.c \
	kernel_sys.c \
	ncopy.c \
	node.c \
	main.c \
	misc_sys.c \
	voldthreads.c

VCONFIGDUMP_OBJS = $(COM_CONFIGDUMP_OBJS) \
	misc_sys.o \
	voldthreads.o

VCONFIGDUMP_LINT_OBJS = $(COM_CONFIGDUMP_LINT_OBJS) \
	misc_sys.ln \
	voldthreads.ln

VCONFIGDUMP_SRCS = $(COM_CONFIGDUMP_SRCS) \
	misc_sys.c \
	voldthreads.c

VPRIVUTIL_OBJS = $(COM_PRIVUTIL_OBJS) \
	misc_sys.o \
	voldthreads.o

VPRIVUTIL_LINT_OBJS = $(COM_PRIVUTIL_LINT_OBJS) \
	misc_sys.ln \
	voldthreads.ln

VPRIVUTIL_SRCS = $(COM_PRIVUTIL_SRCS) \
	misc_sys.c \
	voldthreads.c

VKPRINT_OBJS = $(COM_KPRINT_OBJS)
VKPRINT_LINT_OBJS = $(COM_KPRINT_LINT_OBJS)
VKPRINT_SRCS = $(COM_KPRINT_SRCS)

TESTPOLL_OBJS = $(COM_TESTPOLL_OBJS) \
	misc_sys.o \
	voldthreads.o

TESTPOLL_CMDS = $(COM_TESTPOLL_SRCS) \
	misc_sys.c \
	voldthreads.c

WRTEMPTY_OBJS = $(COM_WRTEMPTY_OBJS) \
	misc_sys.o \
	voldthreads.o

WRTEMPTY_SRCS = $(COM_WRTEMTPY_SRCS) \
	misc_sys.c \
	voldthreads.c

VOLD_OBJS = $(COM_OBJS) \
	autostart.o \
	ap.o \
	apf.o \
	devintf.o \
	devmap.o \
	kernel_sys.o \
	node.o \
	main.o \
	misc_sys.o \
	ncopy.o \
	voldthreads.o

VOLD_LINT_OBJS = $(COM_LINT_OBJS) \
	autostart.ln \
	ap.ln \
	apf.ln \
	devintf.ln \
	devmap.ln \
	kernel_sys.ln \
	node.ln \
	main.ln \
	misc_sys.ln \
	ncopy.ln \
	voldthreads.ln

VOLD_SRCS = $(COM_SRCS) \
	autostart.c \
	ap.c \
	apf.c \
	devintf.c \
	devmap.c \
	kernel_sys.c \
	node.c \
	main.c \
	misc_sys.c \
	ncopy.c \
	voldthreads.c

TEST_OBJS = $(COM_TEST_OBJS)

CONFIGD_TARGETS = $(COM_CONFIGD_TARGETS)
DIAG_TARGETS = $(COM_DIAG_TARGETS)
VOLD_TARGETS = $(CONFIGD_TARGETS) $(DIAG_TARGETS)
TEST_TARGETS = $(COM_TEST_TARGETS)

# Very localized stuff to get vxconfigd to be runnable without
# /usr mounted -- mainly, link statically as much as possible
#
# Licensing is an option in this makefile, to turn it on -DLIC
# must be in one of the compile flag vars and VXVM_LICENSE=LICENSE
# ELMLIBS is commented in this file as the license manager is not
# known anyway for uw. Once that is known, it should be re-introduced.
#

THREADFLAGS	= -D_REENTRANT
DYNAMIC		= -Bdynamic
STATIC		= -Bstatic
SPEC_LD_SO	= -Wl,-I/etc/lib/ld.so.1

LIC_LIBS_LICENSE	= -L $(LIC_LIB) -lpmapi
LIC_LIBS_NOLICENSE	=

VOLD_LDLIBS	= \
	$(SPEC_LD_SO)\
	$(STATIC) $(LIBVXVM) -lgen $(VOLD_SPEC_LIBS) \
	$(LIC_LIBS_LICENSE) \
	$(DYNAMIC) -lthread -lc -ldl -lsocket -lnsl

DIAG_LDLIBS	= \
	$(LIBVXVM) -lgen -lelf -lthread \
	$(VOLD_SPEC_LIBS) \
	$(LIC_LIBS_LICENSE) $(DYNAMIC) -lsocket -lnsl

VKPRINT_LDLIBS	= \
	$(LIBVXVM) \
	$(VOLD_SPEC_LIBS) \
	$(LIC_LIBS_LICENSE) $(DYNAMIC) -lsocket -lnsl

# Flags to be set for all compilations
LOCALDEF = $(LOCAL) $(THREADFLAGS)

.c.o:
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<
	@if [ "$(@D)" != "./" ]; then ln -s $(LINKDIR)/$(@F) $*.o; fi

.IGNORE:

all:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(VOLD_TARGETS) $(MAKEARGS) ;\
	else \
		for f in $(VOLD_TARGETS); do \
			if [ ! -f $$f ]; then \
				echo "ERROR: $$f is missing" 1>&2 ;\
				false ;\
				break; \
			fi \
		done \
	fi

install: all
	@echo "Making install in cmd/vxvm/unixware/vold"
	for f in $(CONFIGD_TARGETS) ; \
	do \
		rm -f $(SBIN)/$$f ; \
		$(INS) -f $(SBIN) -m 0755 -u root -g sys $$f ; \
	done
	[ -d $(DIAG_DIR) ] || mkdir -p $(DIAG_DIR)
	for f in $(DIAG_TARGETS) ; \
	do \
		rm -f $(DIAG_DIR)/$$f ; \
		$(INS) -f $(DIAG_DIR) -m 0755 -u root -g sys $$f ; \
	done

headinstall:
	@echo "Making headinstall in cmd/vxvm/unixware/vold"

clean:
	@rm -rf $(VOLD_OBJS) $(TEST_OBJS)
	@rm -f *.o

lintclean:
	rm -rf $(VOLD_LINT_OBJS) lint

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		rm -f $(VOLD_TARGETS) $(TEST_TARGETS) ; \
	fi

vxconfigd: $(VCONFIGD_OBJS)
	$(C++C) -o $@ $(VCONFIGD_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) $(VOLD_LDLIBS)

lint: $(VOLD_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VCONFIGD_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VCONFIGDUMP_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VPRIVUTIL_LINT_OBJS)
	$(LINT) $(LINTFLAGS) $(LINT_LINK_FLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VKPRINT_LINT_OBJS)
	touch lint

vxkprint.ln: vxkprint.c
	$(LINT) $(CFLAGS) $(DEFLIST) -U_REENTRANT -c vxkprint.c

vxconfigdump: $(VCONFIGDUMP_OBJS)
	$(C++C) -o vxconfigdump $(VCONFIGDUMP_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) \
		$(DIAG_LDLIBS)

vxprivutil: $(VPRIVUTIL_OBJS)
	$(C++C) -o vxprivutil $(VPRIVUTIL_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) \
		$(DIAG_LDLIBS)

vxkprint: $(VKPRINT_OBJS)
	$(C++C) -o vxkprint $(VKPRINT_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) \
		$(VKPRINT_LDLIBS)

# Special -- vxkprint need not be reentrant
vxkprint.o: vxkprint.c
	$(CC) $(CFLAGS) $(DEFLIST) -U_REENTRANT -c vxkprint.c

# Some testing and debugging programs
testpoll: $(TESTPOLL_OBJS)
	$(CC) -o testpoll $(TESTPOLL_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) \
		$(VOLD_LDLIBS) $(LIC_LIBS_LICENSE)

vxwriteempty: $(WRTEMPTY_OBJS)
	$(CC) -o vxwriteempty $(WRTEMPTY_OBJS) $(LDFLAGS) \
		$(LOCAL_LDDIR) $(DIAG_LDLIBS)
