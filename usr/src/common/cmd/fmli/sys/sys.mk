#ident	"@(#)fmli:sys/sys.mk	1.31.5.5"

include $(CMDRULES)

LIBRARY=libsys.a
HEADER1=../inc
LOCALINC=-I$(HEADER1)
OBJECTS= actrec.o \
	ar_mfuncs.o \
	backslash.o \
	chgenv.o \
	chgepenv.o \
	coproc.o \
	copyfile.o \
	estrtok.o \
	evfuncs.o \
	eval.o \
	exit.o \
	expand.o \
	expr.o \
	filename.o \
	genfind.o \
	getaltenv.o \
	getepenv.o \
	grep.o \
	io.o \
	itoa.o \
	memshift.o \
	mencmds.o \
	cut.o \
	nstrcat.o \
	onexit.o \
	parent.o \
	putaltenv.o \
	scrclean.o \
	spawn.o \
	strCcmp.o \
	stream.o \
	strsave.o \
	tempfiles.o \
	terror.o \
	test.o \
	varappend.o \
	varchkapnd.o \
	varcreate.o \
	vardelete.o \
	vargrow.o \
	varshrink.o \
	mcce.o 

#---	reg_compile.o 
#---	reg_step.o 


$(LIBRARY):  $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

actrec.o: $(HEADER1)/eft.types.h
actrec.o: $(HEADER1)/inc.types.h
actrec.o: $(HEADER1)/actrec.h
actrec.o: $(HEADER1)/ctl.h
actrec.o: $(HEADER1)/menudefs.h
actrec.o: $(HEADER1)/message.h
actrec.o: $(HEADER1)/moremacros.h
actrec.o: $(HEADER1)/slk.h
actrec.o: $(HEADER1)/terror.h
actrec.o: $(HEADER1)/token.h
actrec.o: $(HEADER1)/wish.h
actrec.o: actrec.c

ar_mfuncs.o: $(HEADER1)/actrec.h
ar_mfuncs.o: $(HEADER1)/slk.h
ar_mfuncs.o: $(HEADER1)/token.h
ar_mfuncs.o: $(HEADER1)/wish.h
ar_mfuncs.o: ar_mfuncs.c

backslash.o: $(HEADER1)/wish.h
backslash.o: backslash.c

chgenv.o: chgenv.c

chgepenv.o: $(HEADER1)/sizes.h
chgepenv.o: chgepenv.c

compile.o: compile.c
#--regex.o: regex.c   

coproc.o: $(HEADER1)/eft.types.h
coproc.o: $(HEADER1)/inc.types.h
coproc.o: $(HEADER1)/eval.h
coproc.o: $(HEADER1)/moremacros.h
coproc.o: $(HEADER1)/sizes.h
coproc.o: $(HEADER1)/terror.h
coproc.o: $(HEADER1)/var_arrays.h
coproc.o: $(HEADER1)/wish.h
coproc.o: coproc.c

copyfile.o: $(HEADER1)/eft.types.h
copyfile.o: $(HEADER1)/inc.types.h
copyfile.o: $(HEADER1)/exception.h
copyfile.o: $(HEADER1)/wish.h
copyfile.o: copyfile.c

cut.o: $(HEADER1)/ctl.h
cut.o: $(HEADER1)/eval.h
cut.o: $(HEADER1)/message.h
cut.o: $(HEADER1)/moremacros.h
cut.o: $(HEADER1)/sizes.h
cut.o: $(HEADER1)/wish.h
cut.o: cut.c

estrtok.o: estrtok.c

eval.o: $(HEADER1)/eval.h
eval.o: $(HEADER1)/interrupt.h
eval.o: $(HEADER1)/message.h
eval.o: $(HEADER1)/moremacros.h
eval.o: $(HEADER1)/terror.h
eval.o: $(HEADER1)/wish.h
eval.o: eval.c

evfuncs.o: $(HEADER1)/eft.types.h
evfuncs.o: $(HEADER1)/inc.types.h
evfuncs.o: $(HEADER1)/ctl.h
evfuncs.o: $(HEADER1)/eval.h
evfuncs.o: $(HEADER1)/interrupt.h
evfuncs.o: $(HEADER1)/message.h
evfuncs.o: $(HEADER1)/moremacros.h
evfuncs.o: $(HEADER1)/retcodes.h
evfuncs.o: $(HEADER1)/wish.h
evfuncs.o: $(HEADER1)/sizes.h
evfuncs.o: evfuncs.c

exit.o: $(HEADER1)/retcodes.h
exit.o: $(HEADER1)/var_arrays.h
exit.o: $(HEADER1)/wish.h
exit.o: exit.c

expand.o: $(HEADER1)/moremacros.h
expand.o: $(HEADER1)/sizes.h
expand.o: $(HEADER1)/terror.h
expand.o: $(HEADER1)/wish.h
expand.o: expand.c

expr.o: $(HEADER1)/eval.h
expr.o: $(HEADER1)/terror.h
expr.o: $(HEADER1)/wish.h
expr.o: expr.c

filename.o: filename.c

genfind.o: $(HEADER1)/eft.types.h
genfind.o: $(HEADER1)/inc.types.h
genfind.o: $(HEADER1)/eval.h
genfind.o: $(HEADER1)/partabdefs.h
genfind.o: $(HEADER1)/sizes.h
genfind.o: $(HEADER1)/typetab.h
genfind.o: $(HEADER1)/var_arrays.h
genfind.o: $(HEADER1)/wish.h
genfind.o: genfind.c

getaltenv.o: $(HEADER1)/var_arrays.h
getaltenv.o: $(HEADER1)/wish.h
getaltenv.o: getaltenv.c

getepenv.o: $(HEADER1)/moremacros.h
getepenv.o: $(HEADER1)/sizes.h
getepenv.o: $(HEADER1)/wish.h
getepenv.o: getepenv.c

grep.o: $(HEADER1)/ctl.h
grep.o: $(HEADER1)/eval.h
grep.o: $(HEADER1)/message.h
grep.o: $(HEADER1)/moremacros.h
grep.o: $(HEADER1)/wish.h
grep.o: grep.c

io.o: $(HEADER1)/eft.types.h
io.o: $(HEADER1)/inc.types.h
io.o: $(HEADER1)/eval.h
io.o: $(HEADER1)/moremacros.h
io.o: $(HEADER1)/terror.h
io.o: $(HEADER1)/wish.h
io.o: io.c

itoa.o: itoa.c

memshift.o: $(HEADER1)/wish.h
memshift.o: memshift.c

mencmds.o: $(HEADER1)/eft.types.h
mencmds.o: $(HEADER1)/inc.types.h
mencmds.o: $(HEADER1)/ctl.h
mencmds.o: $(HEADER1)/eval.h
mencmds.o: $(HEADER1)/message.h
mencmds.o: $(HEADER1)/moremacros.h
mencmds.o: $(HEADER1)/procdefs.h
mencmds.o: $(HEADER1)/sizes.h
mencmds.o: $(HEADER1)/terror.h
mencmds.o: $(HEADER1)/typetab.h
mencmds.o: $(HEADER1)/wish.h
mencmds.o: mencmds.c

nstrcat.o: $(HEADER1)/sizes.h
nstrcat.o: nstrcat.c

onexit.o: $(HEADER1)/var_arrays.h
onexit.o: $(HEADER1)/wish.h
onexit.o: onexit.c

parent.o: $(HEADER1)/sizes.h
parent.o: $(HEADER1)/wish.h
parent.o: parent.c

putaltenv.o: $(HEADER1)/moremacros.h
putaltenv.o: $(HEADER1)/var_arrays.h
putaltenv.o: $(HEADER1)/wish.h
putaltenv.o: putaltenv.c

scrclean.o: scrclean.c

spawn.o: $(HEADER1)/eft.types.h
spawn.o: $(HEADER1)/inc.types.h
spawn.o: $(HEADER1)/moremacros.h
spawn.o: $(HEADER1)/sizes.h
spawn.o: $(HEADER1)/wish.h
spawn.o: spawn.c

strCcmp.o: strCcmp.c

stream.o: $(HEADER1)/token.h
stream.o: $(HEADER1)/wish.h
stream.o: stream.c

strsave.o: $(HEADER1)/terror.h
strsave.o: $(HEADER1)/wish.h
strsave.o: strsave.c

tempfiles.o: $(HEADER1)/eft.types.h
tempfiles.o: $(HEADER1)/inc.types.h
tempfiles.o: $(HEADER1)/moremacros.h
tempfiles.o: $(HEADER1)/retcodes.h
tempfiles.o: $(HEADER1)/terror.h
tempfiles.o: $(HEADER1)/var_arrays.h
tempfiles.o: $(HEADER1)/wish.h
tempfiles.o: tempfiles.c

terror.o: $(HEADER1)/eft.types.h
terror.o: $(HEADER1)/inc.types.h
terror.o: $(HEADER1)/message.h
terror.o: $(HEADER1)/retcodes.h
terror.o: $(HEADER1)/sizes.h
terror.o: $(HEADER1)/terrmess.h
terror.o: $(HEADER1)/terror.h
terror.o: $(HEADER1)/vtdefs.h
terror.o: $(HEADER1)/wish.h
terror.o: terror.c

test.o: $(HEADER1)/eft.types.h
test.o: $(HEADER1)/inc.types.h
test.o: $(HEADER1)/wish.h
test.o: ./test.h
test.o: test.c

varappend.o: $(HEADER1)/var_arrays.h
varappend.o: $(HEADER1)/wish.h
varappend.o: varappend.c

varchkapnd.o: $(HEADER1)/var_arrays.h
varchkapnd.o: $(HEADER1)/wish.h
varchkapnd.o: varchkapnd.c

varcreate.o: $(HEADER1)/var_arrays.h
varcreate.o: $(HEADER1)/terror.h
varcreate.o: $(HEADER1)/wish.h
varcreate.o: varcreate.c

vardelete.o: $(HEADER1)/var_arrays.h
vardelete.o: $(HEADER1)/wish.h
vardelete.o: vardelete.c

vargrow.o: $(HEADER1)/var_arrays.h
vargrow.o: $(HEADER1)/terror.h
vargrow.o: $(HEADER1)/wish.h
vargrow.o: vargrow.c

varshrink.o: $(HEADER1)/var_arrays.h
varshrink.o: $(HEADER1)/terror.h
varshrink.o: $(HEADER1)/wish.h
varshrink.o: varshrink.c

mcce.o: mcce.c

#--- reg_compile.o: _regexp.h
#--- reg_compile.o: _wchar.h
#--- reg_compile.o: _range.h
#--- reg_compile.o: synonyms.h
#--- reg_compile.o: reg_compile.c

#--- reg_step.o: _regexp.h
#--- reg_step.o: _wchar.h
#--- reg_step.o: synonyms.h
#--- reg_step.o: reg_step.c

####### Standard makefile targets ########

all:		$(LIBRARY)

install:	all

clean:
		/bin/rm -f *.o

clobber:	clean
		/bin/rm -f $(LIBRARY)

profile:	libprof.a
#
# PROFILING: ---------------------------------------------
#  method 0: ignore the exit and suffer the conseqences
#
VERDICT = libp0

libp0.a:  libsys.a noeggs.o
	/bin/rm -f $@; cp libsys.a $@;
	$(AR) d $@ exit.o
	$(AR) $(ARFLAGS) $@ noeggs.o
noeggs.c: exit.c
	sed 's/^exit/_noeggs/' $? >$@
#	
#  method 1: Rename the profiling and unix exits with
#      one extra leading under-bar.
# the library itself may not be profiled, but it doesnt
#   need the "exit" module; "exit" has to be recrafted
#   from csu/mcrt0.s
#
# VERDICT = libp1
libprof.a:	$(VERDICT).a
	/bin/rm -f $@; ln $? $@
#
# mcrt0 is viewmaster's profiling "crt0" file
# 
libp1.a:	libsys.a mcrt0.o vexit.o
	cp libsys.a $@
	$(AR) $(ARFLAGS) $@ vexit.o
#
#  machine, libc, environment tools
#
LIBC = $(ROOT)/usr/src/common/lib/libc
M_LIB = $(ROOT)/usr/src/$(MACH)/lib/libc
MM4	= $(M4) $(M_LIB)/m4.def	
MCDEF	= $(M_LIB)/mcount.def
#
# turn exit into _exit and _exit into __exit
#
CRT0	= $(M_LIB)/csu/mcrt0.s
MCOUNT	= $(M_LIB)/crt/mcount.s
mcrt0.s: $(CRT0)
	sed 's/exit/_exit/g' $?|$(MM4) $(MCDEF) ->$@
vexit.s: $(M_LIB)/sys/exit.s
	sed 's/exit/_exit/g' $? |$(MM4) $(MCDEF) -> $@

#
#  method 2. define a cleanup routine from the 
#   local exit routine and the unix cleanup (stdio/flsbuf.c)
#
libp2.a:	libsys.a v_clean.o
	/bin/rm -f $@; cp libsys.a $@; $(AR) d libsys.a exit.0
	$(AR) $(ARFLAGS) libsys.a v_clean.o
#
#  ONLY when L*PROFiling, 
# FACE Cleanup ={ port/stdio/flsbuf.c exit.c }{ ... }.
# where _cleanup => __cleanup
#  &    exit     => _cleanup
#		
v_clean.c:	exit.c $(LIBC)/port/stdio/flsbuf.c sys.mk
	(echo "void _cleanup();" ;\
	 sed -e 's/^exit.*/cleanup(n)/'\
	     -e '/_exit/d' exit.c;\
	 echo "void";\
	 sed -n '/cleanup/,/}/p' $(LIBC)/port/stdio/flsbuf.c;\
	)|\
	 sed -e 's/cleanup/_&/g' >$@
#

.PRECIOUS:	$(LIBRARY)
