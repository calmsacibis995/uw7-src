#ident	"@(#)debugger:libint/i386/makefile	1.5"

include $(CMDRULES)

TARGET = $(PRODLIB)/libint.a

OSR5_TARGET = $(OSR5_LIB)/libint.a

include ../../util/common/std.make

CCSOURCES = \
	$(COMMON)/Interface.C \
	$(COMMON)/SrcFile.C \
	$(COMMON)/Output.C \
	$(COMMON)/Manager.C \
	$(COMMON)/GManager.C \
	$(COMMON)/Msgtab.C \
	$(COMMON)/Message.C \
	$(COMMON)/Mformat.C \
	$(COMMON)/Path.C \
	$(COMMON)/Transport.C \
	$(COMMON)/UIutil.C \
	print.C \
	Msgtypes.C \
	Unbundle.C

CSOURCES=\
	Mtable.c

include DEPEND

include ../../util/common/lib.make
