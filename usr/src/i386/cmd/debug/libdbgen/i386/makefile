#ident	"@(#)debugger:libdbgen/i386/makefile	1.5"

include $(CMDRULES)

TARGET = $(PRODLIB)/libdbgen.a

OSR5_TARGET = $(OSR5_LIB)/libdbgen.a

include ../../util/common/std.make

CCSOURCES = \
	$(COMMON)/Buffer.C \
	$(COMMON)/Link.C \
	$(COMMON)/List.C \
	$(COMMON)/NewHandle.C \
	$(COMMON)/Rbtree.C \
	$(COMMON)/RegExp.C \
	$(COMMON)/Vector.C \
	$(COMMON)/Wordstack.C \
	$(COMMON)/str.C

include DEPEND

include ../../util/common/lib.make
