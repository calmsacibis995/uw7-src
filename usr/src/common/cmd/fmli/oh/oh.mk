#ident	"@(#)fmli:oh/oh.mk	1.37.4.2"

include $(CMDRULES)

LIBRARY=liboh.a
HEADER1=../inc
LOCALINC=-I$(HEADER1)
LOCALDEF=-DWISH
OBJECTS= action.o \
	alias.o \
	cmd.o \
	detab.o \
	detect.o \
	dispfuncs.o \
	evstr.o \
	externoot.o \
	fm_mn_par.o \
	getval.o \
	if_ascii.o \
	if_dir.o \
	if_init.o \
	if_exec.o \
	if_form.o \
	if_help.o \
	if_menu.o \
	ifuncs.o \
	interrupt.o \
	is_objtype.o \
	misc.o \
	namecheck.o \
	nextpart.o \
	obj_to_opt.o \
	obj_to_par.o \
	odftread.o \
	odikey.o \
	oh_init.o \
	ootpart.o \
	ootread.o \
	opt_rename.o \
	optab.o \
	optabfuncs.o \
	ott_mv.o \
	partab.o \
	partabfunc.o \
	pathtitle.o \
	pathfstype.o \
	path_to_vp.o \
	pathott.o \
	scram.o \
	slk.o \
	suffuncs.o \
	typefuncs.o \
	typetab.o

$(LIBRARY): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)


action.o: $(HEADER1)/moremacros.h
action.o: $(HEADER1)/token.h
action.o: $(HEADER1)/var_arrays.h
action.o: $(HEADER1)/wish.h
action.o: action.c

alias.o: $(HEADER1)/actrec.h
alias.o: $(HEADER1)/ctl.h
alias.o: $(HEADER1)/moremacros.h
alias.o: $(HEADER1)/sizes.h
alias.o: $(HEADER1)/slk.h
alias.o: $(HEADER1)/token.h
alias.o: $(HEADER1)/wish.h
alias.o: alias.c

cmd.o: $(HEADER1)/actrec.h
cmd.o: $(HEADER1)/ctl.h
cmd.o: $(HEADER1)/eval.h
cmd.o: $(HEADER1)/helptext.h
cmd.o: $(HEADER1)/interrupt.h
cmd.o: $(HEADER1)/menudefs.h
cmd.o: $(HEADER1)/moremacros.h
cmd.o: $(HEADER1)/sizes.h
cmd.o: $(HEADER1)/slk.h
cmd.o: $(HEADER1)/terror.h
cmd.o: $(HEADER1)/token.h
cmd.o: $(HEADER1)/vtdefs.h
cmd.o: $(HEADER1)/wish.h
cmd.o: ./fm_mn_par.h
cmd.o: cmd.c

detab.o: $(HEADER1)/eft.types.h
detab.o: $(HEADER1)/inc.types.h
detab.o: $(HEADER1)/detabdefs.h
detab.o: $(HEADER1)/typetab.h
detab.o: $(HEADER1)/wish.h
detab.o: detab.c

detect.o: $(HEADER1)/eft.types.h
detect.o: $(HEADER1)/inc.types.h
detect.o: $(HEADER1)/detabdefs.h
detect.o: $(HEADER1)/optabdefs.h
detect.o: $(HEADER1)/parse.h
detect.o: $(HEADER1)/partabdefs.h
detect.o: $(HEADER1)/sizes.h
detect.o: $(HEADER1)/typetab.h
detect.o: $(HEADER1)/var_arrays.h
detect.o: $(HEADER1)/wish.h
detect.o: detect.c

dispfuncs.o: $(HEADER1)/eft.types.h
dispfuncs.o: $(HEADER1)/inc.types.h
dispfuncs.o: $(HEADER1)/moremacros.h
dispfuncs.o: $(HEADER1)/partabdefs.h
dispfuncs.o: $(HEADER1)/terror.h
dispfuncs.o: $(HEADER1)/typetab.h
dispfuncs.o: $(HEADER1)/var_arrays.h
dispfuncs.o: $(HEADER1)/wish.h
dispfuncs.o: dispfuncs.c

evstr.o: $(HEADER1)/eft.types.h
evstr.o: $(HEADER1)/inc.types.h
evstr.o: $(HEADER1)/eval.h
evstr.o: $(HEADER1)/interrupt.h
evstr.o: $(HEADER1)/moremacros.h
evstr.o: $(HEADER1)/terror.h
evstr.o: $(HEADER1)/token.h
evstr.o: $(HEADER1)/var_arrays.h
evstr.o: $(HEADER1)/wish.h
evstr.o: ./fm_mn_par.h
evstr.o: evstr.c

externoot.o: $(HEADER1)/sizes.h
externoot.o: externoot.c

fm_mn_par.o: $(HEADER1)/eft.types.h
fm_mn_par.o: $(HEADER1)/inc.types.h
fm_mn_par.o: $(HEADER1)/actrec.h
fm_mn_par.o: $(HEADER1)/eval.h
fm_mn_par.o: $(HEADER1)/moremacros.h
fm_mn_par.o: $(HEADER1)/terror.h
fm_mn_par.o: $(HEADER1)/token.h
fm_mn_par.o: $(HEADER1)/var_arrays.h
fm_mn_par.o: $(HEADER1)/vtdefs.h
fm_mn_par.o: $(HEADER1)/wish.h
fm_mn_par.o: ./fm_mn_par.h
fm_mn_par.o: fm_mn_par.c

getval.o: $(HEADER1)/eft.types.h
getval.o: $(HEADER1)/inc.types.h
getval.o: $(HEADER1)/ctl.h
getval.o: $(HEADER1)/eval.h
getval.o: $(HEADER1)/form.h
getval.o: $(HEADER1)/interrupt.h
getval.o: $(HEADER1)/moremacros.h
getval.o: $(HEADER1)/terror.h
getval.o: $(HEADER1)/token.h
getval.o: $(HEADER1)/var_arrays.h
getval.o: $(HEADER1)/winp.h
getval.o: $(HEADER1)/wish.h
getval.o: ./fm_mn_par.h
getval.o: ./objform.h
getval.o: getval.c

if_ascii.o: $(HEADER1)/eft.types.h
if_ascii.o: $(HEADER1)/inc.types.h
if_ascii.o: $(HEADER1)/but.h
if_ascii.o: $(HEADER1)/obj.h
if_ascii.o: $(HEADER1)/procdefs.h
if_ascii.o: $(HEADER1)/sizes.h
if_ascii.o: $(HEADER1)/retcodes.h
if_ascii.o: $(HEADER1)/typetab.h
if_ascii.o: $(HEADER1)/wish.h
if_ascii.o: if_ascii.c

if_dir.o: $(HEADER1)/eft.types.h
if_dir.o: $(HEADER1)/inc.types.h
if_dir.o: $(HEADER1)/actrec.h
if_dir.o: $(HEADER1)/ctl.h
if_dir.o: $(HEADER1)/menudefs.h
if_dir.o: $(HEADER1)/message.h
if_dir.o: $(HEADER1)/moremacros.h
if_dir.o: $(HEADER1)/sizes.h
if_dir.o: $(HEADER1)/slk.h
if_dir.o: $(HEADER1)/terror.h
if_dir.o: $(HEADER1)/token.h
if_dir.o: $(HEADER1)/typetab.h
if_dir.o: $(HEADER1)/var_arrays.h
if_dir.o: $(HEADER1)/vtdefs.h
if_dir.o: $(HEADER1)/wish.h
if_dir.o: if_dir.c

if_exec.o: $(HEADER1)/terror.h
if_exec.o: $(HEADER1)/wish.h
if_exec.o: if_exec.c

if_form.o: $(HEADER1)/eft.types.h
if_form.o: $(HEADER1)/inc.types.h
if_form.o: $(HEADER1)/actrec.h
if_form.o: $(HEADER1)/ctl.h
if_form.o: $(HEADER1)/eval.h
if_form.o: ./fm_mn_par.h
if_form.o: $(HEADER1)/form.h
if_form.o: $(HEADER1)/interrupt.h
if_form.o: $(HEADER1)/menudefs.h
if_form.o: $(HEADER1)/message.h
if_form.o: $(HEADER1)/moremacros.h
if_form.o: $(HEADER1)/slk.h
if_form.o: $(HEADER1)/terror.h
if_form.o: $(HEADER1)/token.h
if_form.o: $(HEADER1)/typetab.h
if_form.o: $(HEADER1)/var_arrays.h
if_form.o: $(HEADER1)/vtdefs.h
if_form.o: $(HEADER1)/winp.h
if_form.o: $(HEADER1)/wish.h
if_form.o: $(HEADER1)/sizes.h
if_form.o: ./objform.h
if_form.o: if_form.c

if_help.o: $(HEADER1)/eft.types.h
if_help.o: $(HEADER1)/inc.types.h
if_help.o: $(HEADER1)/actrec.h
if_help.o: $(HEADER1)/ctl.h
if_help.o: $(HEADER1)/form.h
if_help.o: $(HEADER1)/interrupt.h
if_help.o: $(HEADER1)/message.h
if_help.o: $(HEADER1)/moremacros.h
if_help.o: $(HEADER1)/slk.h
if_help.o: $(HEADER1)/terror.h
if_help.o: $(HEADER1)/token.h
if_help.o: $(HEADER1)/typetab.h
if_help.o: $(HEADER1)/var_arrays.h
if_help.o: $(HEADER1)/vt.h
if_help.o: $(HEADER1)/vtdefs.h
if_help.o: $(HEADER1)/winp.h
if_help.o: $(HEADER1)/wish.h
if_help.o: $(HEADER1)/sizes.h
if_help.o: ./fm_mn_par.h
if_help.o: ./objhelp.h
if_help.o: if_help.c

if_init.o: $(HEADER1)/eft.types.h
if_init.o: $(HEADER1)/inc.types.h
if_init.o: $(HEADER1)/attrs.h
if_init.o: $(HEADER1)/color_pair.h
if_init.o: $(HEADER1)/ctl.h
if_init.o: $(HEADER1)/interrupt.h
if_init.o: $(HEADER1)/moremacros.h
if_init.o: $(HEADER1)/slk.h
if_init.o: $(HEADER1)/terror.h
if_init.o: $(HEADER1)/token.h
if_init.o: $(HEADER1)/var_arrays.h
if_init.o: $(HEADER1)/vtdefs.h
if_init.o: $(HEADER1)/wish.h
if_init.o: ./fm_mn_par.h
if_init.o: if_init.c

if_menu.o: $(HEADER1)/eft.types.h
if_menu.o: $(HEADER1)/inc.types.h
if_menu.o: $(HEADER1)/actrec.h
if_menu.o: $(HEADER1)/ctl.h
if_menu.o: $(HEADER1)/interrupt.h
if_menu.o: $(HEADER1)/menudefs.h
if_menu.o: $(HEADER1)/message.h
if_menu.o: $(HEADER1)/moremacros.h
if_menu.o: $(HEADER1)/slk.h
if_menu.o: $(HEADER1)/terror.h
if_menu.o: $(HEADER1)/token.h
if_menu.o: $(HEADER1)/typetab.h
if_menu.o: $(HEADER1)/var_arrays.h
if_menu.o: $(HEADER1)/vtdefs.h
if_menu.o: $(HEADER1)/wish.h
if_menu.o: $(HEADER1)/sizes.h
if_menu.o: ./fm_mn_par.h
if_menu.o: ./objmenu.h
if_menu.o: if_menu.c

ifuncs.o: $(HEADER1)/eft.types.h
ifuncs.o: $(HEADER1)/inc.types.h
ifuncs.o: $(HEADER1)/but.h
ifuncs.o: $(HEADER1)/ifuncdefs.h
ifuncs.o: $(HEADER1)/message.h
ifuncs.o: $(HEADER1)/moremacros.h
ifuncs.o: $(HEADER1)/obj.h
ifuncs.o: $(HEADER1)/optabdefs.h
ifuncs.o: $(HEADER1)/partabdefs.h
ifuncs.o: $(HEADER1)/retcodes.h
ifuncs.o: $(HEADER1)/sizes.h
ifuncs.o: $(HEADER1)/terror.h
ifuncs.o: $(HEADER1)/token.h
ifuncs.o: $(HEADER1)/typetab.h
ifuncs.o: $(HEADER1)/var_arrays.h
ifuncs.o: $(HEADER1)/windefs.h
ifuncs.o: $(HEADER1)/wish.h
ifuncs.o: ifuncs.c

interrupt.o: $(HEADER1)/wish.h
interrupt.o: interrupt.c

is_objtype.o: $(HEADER1)/eft.types.h
is_objtype.o: $(HEADER1)/inc.types.h
is_objtype.o: $(HEADER1)/detabdefs.h
is_objtype.o: $(HEADER1)/terror.h
is_objtype.o: $(HEADER1)/typetab.h
is_objtype.o: $(HEADER1)/wish.h
is_objtype.o: is_objtype.c

misc.o: $(HEADER1)/moremacros.h
misc.o: $(HEADER1)/wish.h
misc.o: misc.c

namecheck.o: $(HEADER1)/eft.types.h
namecheck.o: $(HEADER1)/inc.types.h
namecheck.o: $(HEADER1)/message.h
namecheck.o: $(HEADER1)/mio.h
namecheck.o: $(HEADER1)/partabdefs.h
namecheck.o: $(HEADER1)/sizes.h
namecheck.o: $(HEADER1)/typetab.h
namecheck.o: $(HEADER1)/wish.h
namecheck.o: namecheck.c

nextpart.o: $(HEADER1)/eft.types.h
nextpart.o: $(HEADER1)/inc.types.h
nextpart.o: $(HEADER1)/but.h
nextpart.o: $(HEADER1)/optabdefs.h
nextpart.o: $(HEADER1)/partabdefs.h
nextpart.o: $(HEADER1)/typetab.h
nextpart.o: $(HEADER1)/wish.h
nextpart.o: nextpart.c

obj_to_opt.o: $(HEADER1)/eft.types.h
obj_to_opt.o: $(HEADER1)/inc.types.h
obj_to_opt.o: $(HEADER1)/but.h
obj_to_opt.o: $(HEADER1)/ifuncdefs.h
obj_to_opt.o: $(HEADER1)/optabdefs.h
obj_to_opt.o: $(HEADER1)/partabdefs.h
obj_to_opt.o: $(HEADER1)/typetab.h
obj_to_opt.o: $(HEADER1)/wish.h
obj_to_opt.o: obj_to_opt.c

obj_to_par.o: $(HEADER1)/eft.types.h
obj_to_par.o: $(HEADER1)/inc.types.h
obj_to_par.o: $(HEADER1)/but.h
obj_to_par.o: $(HEADER1)/ifuncdefs.h
obj_to_par.o: $(HEADER1)/optabdefs.h
obj_to_par.o: $(HEADER1)/partabdefs.h
obj_to_par.o: $(HEADER1)/typetab.h
obj_to_par.o: $(HEADER1)/wish.h
obj_to_par.o: obj_to_par.c

odftread.o: $(HEADER1)/eft.types.h
odftread.o: $(HEADER1)/inc.types.h
odftread.o: $(HEADER1)/detabdefs.h
odftread.o: $(HEADER1)/mio.h
odftread.o: $(HEADER1)/optabdefs.h
odftread.o: $(HEADER1)/retcodes.h
odftread.o: $(HEADER1)/sizes.h
odftread.o: $(HEADER1)/terror.h
odftread.o: $(HEADER1)/typetab.h
odftread.o: $(HEADER1)/wish.h
odftread.o: odftread.c

odikey.o: $(HEADER1)/eft.types.h
odikey.o: $(HEADER1)/inc.types.h
odikey.o: $(HEADER1)/moremacros.h
odikey.o: $(HEADER1)/sizes.h
odikey.o: $(HEADER1)/typetab.h
odikey.o: $(HEADER1)/var_arrays.h
odikey.o: $(HEADER1)/wish.h
odikey.o: odikey.c

oh_init.o: $(HEADER1)/typetab.h
oh_init.o: $(HEADER1)/sizes.h
oh_init.o: $(HEADER1)/wish.h
oh_init.o: oh_init.c

ootpart.o: $(HEADER1)/eft.types.h
ootpart.o: $(HEADER1)/inc.types.h
ootpart.o: $(HEADER1)/optabdefs.h
ootpart.o: $(HEADER1)/partabdefs.h
ootpart.o: $(HEADER1)/typetab.h
ootpart.o: $(HEADER1)/wish.h
ootpart.o: ootpart.c

ootread.o: $(HEADER1)/eft.types.h
ootread.o: $(HEADER1)/inc.types.h
ootread.o: $(HEADER1)/ifuncdefs.h
ootread.o: $(HEADER1)/mio.h
ootread.o: $(HEADER1)/optabdefs.h
ootread.o: $(HEADER1)/partabdefs.h
ootread.o: $(HEADER1)/sizes.h
ootread.o: $(HEADER1)/terror.h
ootread.o: $(HEADER1)/typetab.h
ootread.o: $(HEADER1)/var_arrays.h
ootread.o: $(HEADER1)/wish.h
ootread.o: ootread.c

opt_rename.o: $(HEADER1)/eft.types.h
opt_rename.o: $(HEADER1)/inc.types.h
opt_rename.o: $(HEADER1)/but.h
opt_rename.o: $(HEADER1)/ifuncdefs.h
opt_rename.o: $(HEADER1)/optabdefs.h
opt_rename.o: $(HEADER1)/partabdefs.h
opt_rename.o: $(HEADER1)/sizes.h
opt_rename.o: $(HEADER1)/typetab.h
opt_rename.o: $(HEADER1)/wish.h
opt_rename.o: opt_rename.c

optab.o: $(HEADER1)/eft.types.h
optab.o: $(HEADER1)/inc.types.h
optab.o: $(HEADER1)/but.h
optab.o: $(HEADER1)/ifuncdefs.h
optab.o: $(HEADER1)/optabdefs.h
optab.o: $(HEADER1)/typetab.h
optab.o: $(HEADER1)/wish.h
optab.o: optab.c

optabfuncs.o: $(HEADER1)/eft.types.h
optabfuncs.o: $(HEADER1)/inc.types.h
optabfuncs.o: $(HEADER1)/but.h
optabfuncs.o: $(HEADER1)/ifuncdefs.h
optabfuncs.o: $(HEADER1)/optabdefs.h
optabfuncs.o: $(HEADER1)/partabdefs.h
optabfuncs.o: $(HEADER1)/typetab.h
optabfuncs.o: $(HEADER1)/wish.h
optabfuncs.o: optabfuncs.c

ott_mv.o: $(HEADER1)/eft.types.h
ott_mv.o: $(HEADER1)/inc.types.h
ott_mv.o: $(HEADER1)/partabdefs.h
ott_mv.o: $(HEADER1)/sizes.h
ott_mv.o: $(HEADER1)/typetab.h
ott_mv.o: $(HEADER1)/wish.h
ott_mv.o: ott_mv.c

partab.o: $(HEADER1)/eft.types.h
partab.o: $(HEADER1)/inc.types.h
partab.o: $(HEADER1)/but.h
partab.o: $(HEADER1)/ifuncdefs.h
partab.o: $(HEADER1)/optabdefs.h
partab.o: $(HEADER1)/partabdefs.h
partab.o: $(HEADER1)/typetab.h
partab.o: $(HEADER1)/wish.h
partab.o: partab.c

partabfunc.o: $(HEADER1)/eft.types.h
partabfunc.o: $(HEADER1)/inc.types.h
partabfunc.o: $(HEADER1)/but.h
partabfunc.o: $(HEADER1)/ifuncdefs.h
partabfunc.o: $(HEADER1)/mio.h
partabfunc.o: $(HEADER1)/optabdefs.h
partabfunc.o: $(HEADER1)/partabdefs.h
partabfunc.o: $(HEADER1)/sizes.h
partabfunc.o: $(HEADER1)/typetab.h
partabfunc.o: $(HEADER1)/wish.h
partabfunc.o: partabfunc.c

path_to_vp.o: $(HEADER1)/eft.types.h
path_to_vp.o: $(HEADER1)/inc.types.h
path_to_vp.o: $(HEADER1)/obj.h
path_to_vp.o: $(HEADER1)/optabdefs.h
path_to_vp.o: $(HEADER1)/sizes.h
path_to_vp.o: $(HEADER1)/typetab.h
path_to_vp.o: $(HEADER1)/wish.h
path_to_vp.o: path_to_vp.c

pathott.o: $(HEADER1)/eft.types.h
pathott.o: $(HEADER1)/inc.types.h
pathott.o: $(HEADER1)/typetab.h
pathott.o: $(HEADER1)/sizes.h
pathott.o: pathott.c

pathtitle.o: $(HEADER1)/sizes.h
pathtitle.o: $(HEADER1)/wish.h
pathtitle.o: pathtitle.c

pathfstype.o: $(HEADER1)/eft.types.h
pathfstype.o: $(HEADER1)/inc.types.h
pathfstype.o: pathfstype.c

scram.o: $(HEADER1)/eft.types.h
scram.o: $(HEADER1)/inc.types.h
scram.o: $(HEADER1)/exception.h
scram.o: $(HEADER1)/moremacros.h
scram.o: $(HEADER1)/obj.h
scram.o: $(HEADER1)/parse.h
scram.o: $(HEADER1)/partabdefs.h
scram.o: $(HEADER1)/sizes.h
scram.o: $(HEADER1)/retcodes.h
scram.o: $(HEADER1)/terror.h
scram.o: $(HEADER1)/token.h
scram.o: $(HEADER1)/typetab.h
scram.o: $(HEADER1)/vtdefs.h
scram.o: $(HEADER1)/winp.h
scram.o: $(HEADER1)/wish.h
scram.o: scram.c

slk.o: $(HEADER1)/eft.types.h
slk.o: $(HEADER1)/inc.types.h
slk.o: $(HEADER1)/ctl.h
slk.o: $(HEADER1)/interrupt.h
slk.o: $(HEADER1)/moremacros.h
slk.o: $(HEADER1)/sizes.h
slk.o: $(HEADER1)/slk.h
slk.o: $(HEADER1)/token.h
slk.o: $(HEADER1)/wish.h
slk.o: ./fm_mn_par.h
slk.o: slk.c

suffuncs.o: $(HEADER1)/eft.types.h
suffuncs.o: $(HEADER1)/inc.types.h
suffuncs.o: suffuncs.c

typefuncs.o: $(HEADER1)/eft.types.h
typefuncs.o: $(HEADER1)/inc.types.h
typefuncs.o: $(HEADER1)/mio.h
typefuncs.o: $(HEADER1)/moremacros.h
typefuncs.o: $(HEADER1)/partabdefs.h
typefuncs.o: $(HEADER1)/sizes.h
typefuncs.o: $(HEADER1)/typetab.h
typefuncs.o: $(HEADER1)/var_arrays.h
typefuncs.o: $(HEADER1)/wish.h
typefuncs.o: typefuncs.c

typetab.o: $(HEADER1)/eft.types.h
typetab.o: $(HEADER1)/inc.types.h
typetab.o: $(HEADER1)/mio.h
typetab.o: $(HEADER1)/moremacros.h
typetab.o: $(HEADER1)/partabdefs.h
typetab.o: $(HEADER1)/sizes.h
typetab.o: $(HEADER1)/typetab.h
typetab.o: $(HEADER1)/var_arrays.h
typetab.o: $(HEADER1)/wish.h
typetab.o: typetab.c

###### Standard makefile targets #####

all:		$(LIBRARY)

install:	all

clean:
		/bin/rm -f *.o

clobber:	clean
		/bin/rm -f $(LIBRARY)

.PRECIOUS:	$(LIBRARY)
