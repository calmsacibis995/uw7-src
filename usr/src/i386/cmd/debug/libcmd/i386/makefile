#ident	"@(#)debugger:libcmd/i386/makefile	1.5"

include $(CMDRULES)

TARGET = $(PRODLIB)/libcmd.a

OSR5_TARGET = $(OSR5_LIB)/libcmd.a

include ../../util/common/std.make

CCSOURCES = \
	$(COMMON)/Execute.C \
	$(COMMON)/Help.C \
	$(COMMON)/Input.C \
	$(COMMON)/Keyword.C \
	$(COMMON)/Location.C \
	$(COMMON)/Parser.C \
	$(COMMON)/Scanner.C \
	$(COMMON)/Shell.C \
	$(COMMON)/systable.C \
	sysmach.C

CSOURCES = \
	Help_msg.c \
	Help_thr.c \
	Help_ptrace.c

include DEPEND

include ../../util/common/lib.make
