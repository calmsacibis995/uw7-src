#ident	"@(#)debugger:libsymbol/i386/makefile	1.5"

include $(CMDRULES)

TARGET = $(PRODLIB)/libsymbol.a

OSR5_TARGET = $(OSR5_LIB)/libsymbol.a

include ../../util/common/std.make

CCSOURCES = \
	$(COMMON)/AddrList.C \
	$(COMMON)/Build.C \
	$(COMMON)/Coff.C \
	$(COMMON)/Coffbuild.C \
	$(COMMON)/Dwarfbuild.C \
	$(COMMON)/Dwarf2.C \
	$(COMMON)/Dwarf1.C \
	$(COMMON)/ELF.C \
	$(COMMON)/Elfbuild.C \
	$(COMMON)/Evaluator.C \
	$(COMMON)/Protoline.C \
	$(COMMON)/Protorec.C \
	$(COMMON)/Locdesc.C \
	$(COMMON)/NameList.C \
	$(COMMON)/Object.C \
	$(COMMON)/Reflist.C \
	$(COMMON)/Source.C \
	$(COMMON)/Symbol.C \
	$(COMMON)/Symtab.C \
	$(COMMON)/Symtable.C \
	$(COMMON)/TYPE.C \
	$(COMMON)/TypeList.C \
	TYPE.Mach.C \
	Cvtaddr.C \
	Coff_Bit.C \
	get_stype.C

include DEPEND

include ../../util/common/lib.make
