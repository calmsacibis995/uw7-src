#ident	"@(#)cplusfe:i386/Makefile	1.51"

#
#	C++ front end makefile
#
include $(CMDRULES)

LINTFLAGS=
LINK_MODE=

LDLIBS=

INS=$(SGSBASE)/sgs.install

FE_OBJECTS = fe/cfe.o fe/class_decl.o \
	fe/cmd_line.o fe/const_ints.o fe/debug.o fe/decl_inits.o fe/decls.o \
	fe/decl_spec.o fe/declarator.o fe/def_arg.o fe/disambig.o \
	fe/error.o fe/expr.o fe/exprutil.o fe/fe_init.o fe/fe_wrapup.o \
	fe/float_pt.o fe/folding.o fe/func_def.o fe/host_envir.o \
	fe/il.o fe/il_alloc.o fe/il_display.o fe/il_read.o fe/il_to_str.o \
	fe/il_walk.o fe/il_write.o fe/layout.o fe/lexical.o \
	fe/literals.o fe/lookup.o fe/lower_eh.o fe/lower_il.o fe/lower_init.o \
	fe/lower_name.o fe/macro.o fe/mem_manage.o \
	fe/overload.o fe/pch.o fe/pragma.o fe/preproc.o \
	fe/scope_stk.o fe/statements.o \
	fe/symbol_ref.o fe/symbol_tbl.o fe/sys_predef.o \
	fe/target.o fe/templates.o fe/types.o \
	fe/fpemu.o fe/dwarf_dbg.o fe/eh_query.o

LONG_LONG_OBJECTS = llasgmul.o lldivrem.o

READER_OBJECTS = disp/const_ints.o disp/debug.o disp/error.o \
	disp/float_pt.o disp/host_envir.o disp/il.o \
	disp/il_read.o disp/il_to_str.o disp/il_walk.o disp/mem_manage.o \
	disp/target.o disp/types.o \
	disp/fpemu.o \
	disp/eh_query.o

BE_OBJECTS = $(READER_OBJECTS)

DISP_OBJECTS = $(READER_OBJECTS) disp/il_display.o

CGENBE_OBJECTS = $(READER_OBJECTS) disp/c_gen_be.o

ERROR_FILES = err_codes.h err_data.h

INCLUDES = basic_hdrs.h basics.h c_gen_be.h class_decl.h \
	cmd_line.h const_ints.h \
	debug.h decl_inits.h decl_hdrs.h decls.h \
	defines.h def_arg.h error.h expr_hdrs.h expr.h exprutil.h \
	$(ERROR_FILES) \
	fe_common.h fe_init.h fe_wrapup.h float_pt.h \
	folding.h host_envir.h il_alloc.h il.h il_def.h \
	il_display.h il_file.h il_read.h \
	il_walk.h il_write.h \
	lang_feat.h layout.h lexical.h \
	literals.h lower_eh.h lower_hdrs.h \
	lower_il.h lower_init.h \
	lower_name.h macro.h mem_manage.h \
	mem_tables.h overload.h pch.h pragma.h preproc.h \
	statements.h symbol_tbl.h sys_predef.h \
	targ_def.h target.h templates.h \
	types.h version.h \
	dwarf_dbg.h eh_query.h \
	walk_entry.h

PROBEFILE = $(COMDIR)/cmd_line.c

PRODUCTS = c++fe c++disp c_gen_be
BINARIES = fe.o be.o disp.o fe.so

DEBUG=0

#
# The following define to set the __SCO_VERSION__ value is dependent on the
# version number in the form N.N in inc/i386/sgs.h being the 3rd field - 
# unfortunately immediately following the operning '"'.
#
VERSION_ID=-DVERSION_ID=\"`awk '$$2 == "CPLUS_REL" {split($$3,v,"."); split(v[1],w, "\""); printf("%d%.2d\n", w[2], v[2]);}' $(CPUINC)/sgs.h``date +\%Y\%mL`\"

FE_DEFLIST = -DDEBUG=$(DEBUG) -DCHECKING -DDEM_MAIN -DSHOBJ_SUPPORT \
	-DIL_SHOULD_BE_WRITTEN_TO_FILE -DBACK_END_SHOULD_BE_CALLED=0 \
	-DGENERATING_DEBUG_INFO -DGENERATE_EH_TABLES \
	-DDIRECTORY_PRAGMAS_ALLOWED

DISP_DEFLIST = -DDEBUG=$(DEBUG) -DCHECKING \
	-DSTANDALONE_IL_DISPLAY -DGENERATING_DEBUG_INFO -DGENERATE_EH_TABLES \
	-DDIRECTORY_PRAGMAS_ALLOWED

FPE = ../../fpemu/common
FMD = ../../fpemu/$(CPU)

INCLIST= -I. -I$(COMDIR) -I$(COMINC) -I$(CPUINC) -I$(FMD) -I$(FPE)

FE_CC_CMD=$(CC) $(CFLAGS) $(FE_DEFLIST) $(VERSION_ID) $(INCLIST)
DISP_CC_CMD=$(CC) $(CFLAGS) $(DISP_DEFLIST) $(INCLIST)

all:
	if [ -f $(PROBEFILE) ]; \
	then \
		$(MAKE) binaries $(MAKEARGS) DEBUG=$(DEBUG) ; \
	else \
		if test "$(NATIVE)" = "yes"; \
		then \
			dir=native; \
		else \
			dir=cross; \
		fi; \
		for file in $(BINARIES); \
		do \
			rm -f $$file; \
			if [ -r $$dir/$$file ]; \
			then \
				cp $$dir/$$file $$file; \
			else \
				echo "ERROR: $$dir/$$file is missing" 1>&2; \
				false; \
				break; \
			fi \
		done \
	fi

.MUTEX: fe disp error_files fe.o c++fe be.o disp.o c++disp c_gen_be fe.so

binaries : fe disp error_files c++fe c++disp be.o c_gen_be fe.so
	if test "$(NATIVE)" = "yes"; \
	then \
		rm -rf native; \
		mkdir native; \
		cp $(BINARIES) native; \
	else \
		rm -rf cross; \
		mkdir cross; \
		cp $(BINARIES) cross; \
	fi

fe.o : $(FE_OBJECTS) 
	$(LD) -r $(FE_OBJECTS) -o fe.o $(LIBDWARF2);

c++fe : fe.o usr_include.o $(LONG_LONG_OBJECTS) $(LDLIBS)
	if test "$(NATIVE)" = "yes" ; then \
	    $(CC) fe.o usr_include.o $(LDLIBS) $(LINK_MODE) $(LIBSGS) -o $(@); \
	else \
	    $(CC) fe.o usr_include.o $(LONG_LONG_OBJECTS) $(LDLIBS) $(LINK_MODE) $(LIBSGS) -o $(@); \
	fi

disp.o: $(DISP_OBJECTS)
	$(LD) -r $(DISP_OBJECTS) -o disp.o;

be.o:	$(BE_OBJECTS)
	$(LD) -r $(BE_OBJECTS) -o be.o;

c++disp : disp.o usr_include.o $(LONG_LONG_OBJECTS) $(LDLIBS)
	if test "$(NATIVE)" = "yes" ; then \
	    $(CC) disp.o usr_include.o $(LDLIBS) $(LINK_MODE) $(LIBSGS) -o $(@); \
	else \
	    $(CC) disp.o usr_include.o $(LONG_LONG_OBJECTS) $(LDLIBS) $(LINK_MODE) $(LIBSGS) -o $(@); \
	fi

c_gen_be : $(CGENBE_OBJECTS) usr_include.o $(LONG_LONG_OBJECTS) $(LDLIBS)
	if test "$(NATIVE)" = "yes" ; then \
		$(CC) $(CGENBE_OBJECTS) usr_include.o $(LDLIBS) $(LINK_MODE) $(LIBSGS) -o $(@); \
	else \
		$(CC) $(CGENBE_OBJECTS) usr_include.o $(LONG_LONG_OBJECTS) $(LDLIBS) $(LINK_MODE) $(LIBSGS) -o $(@); \
	fi

# Too bad if there are plain files named fe or disp here...

fe:
	mkdir $(@)

disp:
	mkdir $(@)

error_files: $(ERROR_FILES)

.MUTEX: err_codes.h err_data.h

err_codes.h err_data.h: mk_errinfo $(COMDIR)/error_msg.txt $(COMDIR)/error_tag.txt
	./mk_errinfo $(COMDIR)/error_msg.txt $(COMDIR)/error_tag.txt err_codes.h err_data.h

mk_errinfo: $(COMDIR)/mk_errinfo.c
	$(HCC) -o mk_errinfo $(INCLIST) $?

.MUTEX: all disp.o fe.o usr_include.o

install : all usr_include.o $(LONG_LONG_OBJECTS)
	if test "$(NATIVE)" = "yes" ; then \
	    $(CC) fe.o usr_include.o $(LDLIBS) $(LINK_MODE) $(LIBSGS) -o c++fe; \
	else \
	    $(CC) fe.o usr_include.o $(LONG_LONG_OBJECTS) $(LDLIBS) $(LINK_MODE) $(LIBSGS) -o c++fe; \
	fi
	$(MV) c++fe c++fe.bak
	$(CP) c++fe.bak c++fe
	$(STRIP) c++fe
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(CCSLIB)/$(SGS)c++fe c++fe
	$(MV) c++fe.bak c++fe
	if test "$(NATIVE)" = "yes" ; then \
	    $(CC) disp.o usr_include.o $(LDLIBS) $(LINK_MODE) $(LIBSGS) -o c++disp; \
	else \
	    $(CC) disp.o usr_include.o $(LONG_LONG_OBJECTS) $(LDLIBS) $(LINK_MODE) $(LIBSGS) -o c++disp; \
	fi
	$(MV) c++disp c++disp.bak
	$(CP) c++disp.bak c++disp
	$(STRIP) c++disp
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(CCSBIN)/$(SGS)c++disp c++disp
	$(MV) c++disp.bak c++disp
	/bin/sh $(INS) 555 $(OWN) $(GRP) $(CCSLIB)/fe.so fe.so

lintit : $(HFILES) $(SOURCES)
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $(SOURCES)

clean:
	-rm -f $(FE_OBJECTS) $(DISP_OBJECTS) $(CGENBE_OBJECTS) $(SHOBJ_OBJECTS) usr_include.o $(LONG_LONG_OBJECTS)
	-rm -f mk_errinfo $(ERROR_FILES)

# Note - native and cross subdirectories don't get clobbered!
# Needed for source code product build
shrink clobber : clean
	rm -f $(BINARIES)
	rm -f $(PRODUCTS);


#
# The following libc runtime modules are included explicitly in the C++ build directory
# for the Gemini cross-development build.  The frontend needs these routine before
# the C librray is built.  They can be removed after the Gemini release.
# 
llasgmul.o: llasgmul.s
	$(CC) -c $?

lldivrem.o: lldivrem.s
	$(CC) -c $?

fe/c_gen_be.o : $(COMDIR)/c_gen_be.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/c_gen_be.o: $(COMDIR)/c_gen_be.c
	$(DISP_CC_CMD) -DSTANDALONE_C_GEN_BE -DBACK_END_IS_C_GEN_BE=1 \
	-DDUMP_LOWERED_EH_CONSTRUCTS_IN_C_GEN_BE -c $? && $(MV) $(@F) $(@)

fe/cfe.o : $(COMDIR)/cfe.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/class_decl.o : $(COMDIR)/class_decl.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/cmd_line.o : $(COMDIR)/cmd_line.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/const_ints.o : $(COMDIR)/const_ints.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/const_ints.o : $(COMDIR)/const_ints.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/debug.o : $(COMDIR)/debug.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/debug.o : $(COMDIR)/debug.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/decl_inits.o : $(COMDIR)/decl_inits.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/decl_spec.o : $(COMDIR)/decl_spec.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/declarator.o : $(COMDIR)/declarator.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/decls.o : $(COMDIR)/decls.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/def_arg.o : $(COMDIR)/def_arg.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/disambig.o : $(COMDIR)/disambig.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/dwarf_dbg.o : $(COMDIR)/dwarf_dbg.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/eh_query.o : $(COMDIR)/eh_query.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/eh_query.o : $(COMDIR)/eh_query.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/error.o : $(COMDIR)/error.c err_data.h
	$(FE_CC_CMD) -c $(COMDIR)/error.c && $(MV) $(@F) $(@)

disp/error.o : $(COMDIR)/error.c err_data.h
	$(DISP_CC_CMD) -c $(COMDIR)/error.c && $(MV) $(@F) $(@)

fe/expr.o : $(COMDIR)/expr.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/exprutil.o : $(COMDIR)/exprutil.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/fe_init.o : $(COMDIR)/fe_init.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/fe_wrapup.o : $(COMDIR)/fe_wrapup.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/float_pt.o : $(COMDIR)/float_pt.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/float_pt.o : $(COMDIR)/float_pt.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/folding.o : $(COMDIR)/folding.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/func_def.o : $(COMDIR)/func_def.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/host_envir.o : $(COMDIR)/host_envir.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/host_envir.o : $(COMDIR)/host_envir.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/il_alloc.o : $(COMDIR)/il_alloc.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/il.o : $(COMDIR)/il.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/il.o : $(COMDIR)/il.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/il_display.o : $(COMDIR)/il_display.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/il_display.o : $(COMDIR)/il_display.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/il_read.o : $(COMDIR)/il_read.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/il_read.o : $(COMDIR)/il_read.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/il_to_str.o : $(COMDIR)/il_to_str.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/il_to_str.o : $(COMDIR)/il_to_str.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/il_walk.o : $(COMDIR)/il_walk.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/il_walk.o : $(COMDIR)/il_walk.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/target.o : $(COMDIR)/target.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/il_write.o : $(COMDIR)/il_write.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/layout.o : $(COMDIR)/layout.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/lexical.o : $(COMDIR)/lexical.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/literals.o : $(COMDIR)/literals.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/lookup.o : $(COMDIR)/lookup.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/lower_eh.o : $(COMDIR)/lower_eh.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/lower_il.o : $(COMDIR)/lower_il.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/lower_init.o : $(COMDIR)/lower_init.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/lower_name.o : $(COMDIR)/lower_name.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/macro.o : $(COMDIR)/macro.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/mem_manage.o : $(COMDIR)/mem_manage.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/mem_manage.o : $(COMDIR)/mem_manage.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/mem_tables.o : $(COMDIR)/mem_tables.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/mem_tables.o : $(COMDIR)/mem_tables.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/overload.o : $(COMDIR)/overload.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/pch.o : $(COMDIR)/pch.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/pragma.o : $(COMDIR)/pragma.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/preproc.o : $(COMDIR)/preproc.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/scope_stk.o : $(COMDIR)/scope_stk.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/statements.o : $(COMDIR)/statements.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/symbol_ref.o : $(COMDIR)/symbol_ref.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/symbol_tbl.o : $(COMDIR)/symbol_tbl.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/sys_predef.o : sys_predef.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/target.o : $(COMDIR)/target.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/templates.o : $(COMDIR)/templates.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

fe/types.o : $(COMDIR)/types.c
	$(FE_CC_CMD) -c $? && $(MV) $(@F) $(@)

disp/types.o : $(COMDIR)/types.c
	$(DISP_CC_CMD) -c $? && $(MV) $(@F) $(@)

usr_include.o : $(COMDIR)/usr_include.c
	$(CC) $(CFLAGS) $(INCLIST) -c $?

FPEDEF= -DFPEFATAL=fp_error

fe/fpemu.o:	$(FPE)/fpemu.c $(FPE)/fpemu.h $(FMD)/fpemu_md.h
		$(CC) -c $(CFLAGS) $(FPEDEF) -I$(FPE) -I$(FMD) $(FPE)/fpemu.c \
		&& $(MV) $(@F) $(@)

disp/fpemu.o:	$(FPE)/fpemu.c $(FPE)/fpemu.h $(FMD)/fpemu_md.h
		$(CC) -c $(CFLAGS) $(FPEDEF) -I$(FPE) -I$(FMD) $(FPE)/fpemu.c \
		&& $(MV) $(@F) $(@)

# for building as a shared object

SHOBJ_OBJECTS = fe.pic/class_decl.o \
	fe.pic/const_ints.o \
	fe.pic/debug.o \
	fe.pic/decl_inits.o \
	fe.pic/decls.o \
	fe.pic/decl_spec.o \
	fe.pic/declarator.o \
	fe.pic/def_arg.o \
	fe.pic/disambig.o \
	fe.pic/error.o \
	fe.pic/expr.o \
	fe.pic/exprutil.o \
	fe.pic/fe_init.o \
	fe.pic/fe_wrapup.o \
	fe.pic/float_pt.o \
	fe.pic/folding.o \
	fe.pic/func_def.o \
	fe.pic/host_envir.o \
	fe.pic/il.o \
	fe.pic/il_alloc.o \
	fe.pic/il_to_str.o \
	fe.pic/il_walk.o \
	fe.pic/il_write.o \
	fe.pic/layout.o \
	fe.pic/lexical.o \
	fe.pic/literals.o \
	fe.pic/lookup.o \
	fe.pic/lower_eh.o \
	fe.pic/lower_il.o \
	fe.pic/lower_init.o \
	fe.pic/lower_name.o \
	fe.pic/macro.o \
	fe.pic/mem_manage.o \
	fe.pic/overload.o \
	fe.pic/pch.o \
	fe.pic/pragma.o \
	fe.pic/preproc.o \
	fe.pic/statements.o \
	fe.pic/scope_stk.o \
	fe.pic/statements.o \
	fe.pic/symbol_ref.o \
	fe.pic/symbol_tbl.o \
	fe.pic/sys_predef.o \
	fe.pic/target.o \
	fe.pic/templates.o \
	fe.pic/types.o \
	fe.pic/fpemu.o

SHOBJ_DEFLIST=-DUSL_DEBUGGER_SUPPORT -D_REENTRANT \
	-DDEBUG=$(DEBUG) -DCHECKING -DDEM_MAIN -DSHOBJ_SUPPORT \
	-DIL_SHOULD_BE_WRITTEN_TO_FILE=0 \
	-DBACK_END_IS_C_GEN_BE=0 \
	-DBACK_END_SHOULD_BE_CALLED=0 \
	-DGENERATE_EH_TABLES=0

FE_PIC_CMD=$(CC) -KPIC $(CFLAGS) $(SHOBJ_DEFLIST) $(VERSION_ID) $(INCLIST)

fe.pic:
	mkdir $(@)

fe.so:	fe.pic $(SHOBJ_OBJECTS) usr_include.o
	$(CC) $(CFLAGS) -G -o fe.so -h fe.so $(SHOBJ_OBJECTS) \
		usr_include.o $(LDLIBS) $(LIBSGS) $(LINK_MODE) -o $(@)

fe.pic/class_decl.o : $(COMDIR)/class_decl.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/cmd_line.o : $(COMDIR)/cmd_line.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/const_ints.o : $(COMDIR)/const_ints.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/debug.o : $(COMDIR)/debug.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/decl_inits.o : $(COMDIR)/decl_inits.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/decl_spec.o : $(COMDIR)/decl_spec.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/declarator.o : $(COMDIR)/declarator.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/decls.o : $(COMDIR)/decls.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/def_arg.o : $(COMDIR)/def_arg.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/disambig.o : $(COMDIR)/disambig.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/error.o : $(COMDIR)/error.c err_data.h
	$(FE_PIC_CMD) -c $(COMDIR)/error.c && $(MV) $(@F) $(@)

fe.pic/expr.o : $(COMDIR)/expr.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/exprutil.o : $(COMDIR)/exprutil.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/fe_init.o : $(COMDIR)/fe_init.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/fe_wrapup.o : $(COMDIR)/fe_wrapup.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/float_pt.o : $(COMDIR)/float_pt.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/folding.o : $(COMDIR)/folding.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/func_def.o : $(COMDIR)/func_def.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/host_envir.o : $(COMDIR)/host_envir.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/il.o : $(COMDIR)/il.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/il_alloc.o : $(COMDIR)/il_alloc.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/il_to_str.o : $(COMDIR)/il_to_str.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/il_walk.o : $(COMDIR)/il_walk.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/il_write.o : $(COMDIR)/il_write.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/layout.o : $(COMDIR)/layout.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/lexical.o : $(COMDIR)/lexical.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/literals.o : $(COMDIR)/literals.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/lookup.o : $(COMDIR)/lookup.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/lower_eh.o : $(COMDIR)/lower_eh.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/lower_il.o : $(COMDIR)/lower_il.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/lower_init.o : $(COMDIR)/lower_init.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/lower_name.o : $(COMDIR)/lower_name.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/macro.o : $(COMDIR)/macro.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/mem_manage.o : $(COMDIR)/mem_manage.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/overload.o : $(COMDIR)/overload.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/pch.o : $(COMDIR)/pch.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/pragma.o : $(COMDIR)/pragma.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/preproc.o : $(COMDIR)/preproc.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/scope_stk.o : $(COMDIR)/scope_stk.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/statements.o : $(COMDIR)/statements.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/symbol_ref.o : $(COMDIR)/symbol_ref.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/symbol_tbl.o : $(COMDIR)/symbol_tbl.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/sys_predef.o : sys_predef.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/target.o : $(COMDIR)/target.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/templates.o : $(COMDIR)/templates.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/types.o : $(COMDIR)/types.c
	$(FE_PIC_CMD) -c $? && $(MV) $(@F) $(@)

fe.pic/fpemu.o:	$(FPE)/fpemu.c $(FPE)/fpemu.h $(FMD)/fpemu_md.h
	$(CC) -KPIC -c $(CFLAGS) $(FPEDEF) -I$(FPE) -I$(FMD) $(FPE)/fpemu.c \
	&& $(MV) $(@F) $(@)
