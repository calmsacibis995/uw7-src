#ident	"@(#)debugger:libedit/i386/makefile	1.3"

include $(CMDRULES)

TARGET = $(PRODLIB)/libedit.a

OSR5_TARGET = $(OSR5_LIB)/libedit.a

include ../../util/common/std.make

CSOURCES = \
	../common/edit.c \
	../common/editlib.c \
	../common/emacs.c \
	../common/history.c \
	../common/vi.c \
	../common/defs.c

CCSOURCES = \
	../common/ksh_fc.C \
	../common/debug_read.C

include DEPEND

include ../../util/common/lib.make
