# @(#)cmd.vxvm:unixware/fsgen/fsgen.mk	1.6 10/10/97 16:57:05 - cmd.vxvm:unixware/fsgen/fsgen.mk
#ident	"@(#)cmd.vxvm:unixware/fsgen/fsgen.mk	1.6"

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

COMMON_DIR = ../../common/fsgen
LINKDIR    = ../../unixware/fsgen

include $(COMMON_DIR)/fsgen.mk

include $(CMDRULES)

# Force cmdrules include from $(TOOLS) to $(ROOT)/$(MACH)
INC = $(ROOT)/$(MACH)/usr/include

# Pick a probe file as the key of restricted source
PROBEFILE = $(COMMON_DIR)/fsplxopts.c
MAKEFILE = fsgen.mk

LIBCMD_DIR = ../libcmd
VOLD_DIR = ../vold
GEN_DIR = ../gen

LDLIBS = $(LIBCMD_DIR)/libcmd.a $(LIBVXVM) -lgen
# no lint library for gen
LINTLIB = -L$(LIBCMD_DIR) -llibcmd $(LIBVXVM)  
LOCALINC = $(COM_LOCALINC) -I$(LIBCMD_DIR) -I$(GEN_DIR) -I$(LINKDIR)
LOCALDEF = $(COM_LOCALDEF) $(LOCAL)

VPLEX_OBJS = $(COM_VPLEX_OBJS) fstype.o
VPLEX_LINT_OBJS = $(COM_VPLEX_LINT_OBJS) fstype.ln
VPLEX_SRCS = $(COM_VPLEX_SRCS) fstype.c


FSGEN_OBJS = $(COM_OBJS) fstype.o
FSGEN_TARGETS = $(COM_TARGETS)
FSGEN_HDRS = $(COM_HDRS)

VXDIR = $(ROOT)/$(MACH)/usr/lib/vxvm
VXTYPE = $(VXDIR)/type

SUBDIRS = fs

.sh:
	cat $< > $@; chmod +x $@

.c.o:
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<
	@if [ "$(@D)" != "./" ]; then ln -s $(LINKDIR)/$(@F) $*.o; fi

.IGNORE:

all: headinstall
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(FSGEN_TARGETS) $(MAKEARGS) ;\
	else \
		for f in $(FSGEN_TARGETS); do \
			if [ ! -f $$f ]; then \
				echo "ERROR: $$f is missing" 1>&2 ;\
				false ;\
				break; \
			fi \
		done \
	fi

	@for d in $(SUBDIRS) ; \
	do \
		(cd $$d ; \
		 $(MAKE) -f $$d.mk LIBVXVM="$(LIBVXVM)" \
			LOCAL="$(LOCAL)" $@ ); \
	done

install: all
	[ -d $(VXDIR) ] || mkdir -p $(VXDIR)
	[ -d $(VXTYPE) ] || mkdir -p $(VXTYPE)
	[ -d $(VXTYPE)/gen ] || mkdir -p $(VXTYPE)/gen
	[ -d $(VXTYPE)/fsgen ] || mkdir -p $(VXTYPE)/fsgen
	[ -d $(VXTYPE)/raid5 ] || mkdir -p $(VXTYPE)/raid5
	rm -rf $(VXTYPE)/root
	ln -s fsgen $(VXTYPE)/root
	for f in $(FSGEN_TARGETS) ; \
	do \
		rm -f $(VXTYPE)/fsgen/$$f ; \
		$(INS) -f $(VXTYPE)/fsgen -m 0755 -u root -g sys $$f ; \
	done
	for d in $(SUBDIRS) ; \
	do \
		(cd $$d ; \
		 $(MAKE) -f $$d.mk LIBVXVM="$(LIBVXVM)" \
			LOCAL="$(LOCAL)" $@ ); \
	done

lint: $(VPLEX_LINT_OBJS) lintdirs
	$(LINT) $(LINT_LINK_FLAGS) $(LINTFLAGS) $(LDFLAGS) $(LOCAL_LDDIR) \
	$(LINTLIB) $(VPLEX_LINT_OBJS)
	touch lint

lintdirs: 
	for d in $(SUBDIRS) ; \
	do \
		(cd $$d ; \
		echo "\n\t\tMaking Linting in $$d\n" ; \
		$(MAKE) -f $$d.mk LIBVXVM="$(LIBVXVM)" \
			LOCAL="$(LOCAL)" lint ); \
	done

headinstall:
	for f in $(FSGEN_HDRS) ; \
	do \
		$(INS) -f $(INC) -m 0644 -u root -g sys $$f ; \
	done
	for d in $(SUBDIRS) ; \
	do \
		(cd $$d ; \
		 $(MAKE) -f $$d.mk $@ ); \
	done

clean:
	rm -f $(FSGEN_OBJS)
	rm -f *.o
	for d in $(SUBDIRS) ; \
	do \
		(cd $$d ; \
		 $(MAKE) -f $$d.mk $@ ); \
	done

lintclean:
	rm -f $(VPLEX_LINT_OBJS) lint
	for d in $(SUBDIRS) ; \
	do \
	(cd $$d ; \
		echo "\t\tMaking $@ in $$d" ; \
		$(MAKE) -f $$d.mk $@ ) ; \
	done

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		rm -f $(FSGEN_TARGETS) ; \
	fi
	@for d in $(SUBDIRS) ; \
	do \
		(cd $$d ; \
		 $(MAKE) -f $$d.mk $@ ); \
	done

# lint objects
volplex.ln: $(COMMON_GEN_DIR)/volplex.c
	$(LINT) $(LINTFLAGS) $(CFLAGS)  $(DEFLIST) -c \
	$(COMMON_GEN_DIR)/volplex.c
 
gencommon.ln: $(COMMON_GEN_DIR)/gencommon.c
	$(LINT) $(LINTFLAGS) $(CFLAGS)  $(DEFLIST) -c \
	$(COMMON_GEN_DIR)/gencommon.c
 
genextern.ln: $(COMMON_GEN_DIR)/genextern.c
	$(LINT) $(LINTFLAGS) $(CFLAGS)  $(DEFLIST) -c \
	$(COMMON_GEN_DIR)/genextern.c
 
genlog.ln: $(COMMON_GEN_DIR)/genlog.c
	$(LINT) $(LINTFLAGS) $(CFLAGS)  $(DEFLIST) -c \
	$(COMMON_GEN_DIR)/genlog.c

vxresize: volresize.sh
	cat volresize.sh > $@; chmod +x $@

volplex.o: $(COMMON_GEN_DIR)/volplex.c
	$(CC) $(CFLAGS) $(DEFLIST) -c $(COMMON_GEN_DIR)/volplex.c

gencommon.o: $(COMMON_GEN_DIR)/gencommon.c
	$(CC) $(CFLAGS) $(DEFLIST) -c $(COMMON_GEN_DIR)/gencommon.c

genextern.o: $(COMMON_GEN_DIR)/genextern.c
	$(CC) $(CFLAGS) $(DEFLIST) -c $(COMMON_GEN_DIR)/genextern.c

genlog.o: $(COMMON_GEN_DIR)/genlog.c
	$(CC) $(CFLAGS) $(DEFLIST) -c $(COMMON_GEN_DIR)/genlog.c

vxplex: $(VPLEX_OBJS)
	$(CC) -o $@ $(VPLEX_OBJS) $(LDFLAGS) $(LOCAL_LDDIR) $(LDLIBS)

