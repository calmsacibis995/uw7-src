#ident	"@(#)debugger:libutil/i386/makefile	1.10"

include $(CMDRULES)

TARGET = $(PRODLIB)/libutil.a

OSR5_TARGET = $(OSR5_LIB)/libutil.a

include ../../util/common/std.make

CCSOURCES = \
	$(COMMON)/callstack.C \
	$(COMMON)/cancel_sig.C \
	$(COMMON)/change.C \
	$(COMMON)/create.C \
	$(COMMON)/curr_frame.C \
	$(COMMON)/curr_loc.C \
	$(COMMON)/destroy.C \
	$(COMMON)/dis.C \
	$(COMMON)/dump_raw.C \
	$(COMMON)/fentry.C \
	$(COMMON)/find_fcn.C \
	$(COMMON)/functions.C \
	$(COMMON)/get_addr.C \
	$(COMMON)/grab_core.C \
	$(COMMON)/grab_proc.C \
	$(COMMON)/inform.C \
	$(COMMON)/input.C \
	$(COMMON)/jump.C \
	$(COMMON)/language.C \
	$(COMMON)/list_src.C \
	$(COMMON)/overload.C \
	$(COMMON)/print_map.C \
	$(COMMON)/print_expr.C \
	$(COMMON)/pfiles.C \
	$(COMMON)/print_path.C \
	$(COMMON)/pending.C \
	$(COMMON)/ps.C \
	$(COMMON)/regs.C \
	$(COMMON)/rel_proc.C \
	$(COMMON)/rename.C \
	$(COMMON)/run.C \
	$(COMMON)/set_eh.C \
	$(COMMON)/set_fcn.C \
	$(COMMON)/set_path.C \
	$(COMMON)/set_onstop.C \
	$(COMMON)/set_sig.C \
	$(COMMON)/set_stop.C \
	$(COMMON)/set_sys.C \
	$(COMMON)/set_val.C \
	$(COMMON)/sstep.C \
	$(COMMON)/send_sig.C \
	$(COMMON)/stop.C \
	$(COMMON)/symbols.C \
	$(COMMON)/wait.C \
	$(COMMON)/whatis.C

CSOURCES=\
	$(COMMON)/debug_open.c

include DEPEND

include ../../util/common/lib.make
