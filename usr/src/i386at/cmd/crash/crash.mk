#ident	"@(#)crash:i386at/cmd/crash/crash.mk	1.1.4.5"

#
# Tool Section
#

include $(CMDRULES)
include $(UTSRULES)

.c.o:
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -UUNIPROC -c $<

KBASE=$(ROOT)/$(MACH)/usr/include

#
# Define Section
#
INSPERM  = -m 0555 -u $(OWN) -g $(GRP)
LOCALDEF = -D_KMEMUSER -U_KERNEL -U_KERNEL_HEADERS -DKVBASE_IS_VARIABLE \
	-DLD_WEAK_REFERENCE_BUG
MLDLIBS   = -lelf  -lcmd -lc
LDLIBS   = -lelf  -lia -lcmd -lc
FRC 	 =
MSGS 	 = memsize.str
OFFSCC	 = $(CC) $(CFLAGS) -I$(INC) $(GLOBALDEF) $(DEVDEF) -U_KERNEL_HEADERS -UUNIPROC

CMDOBJS= \
	abuf.o \
	base.o \
	buf.o \
	class.o \
	cg.o \
	dis.o \
	disp.o \
	engine.o \
	fpriv.o \
	i386.o \
	inode.o \
	lck.o \
	lidcache.o \
	main.o \
	map.o \
	misc.o \
	page.o \
	proc.o \
	pty.o \
	resmgr.o \
	search.o \
	sfs_inode.o \
	size.o \
	snode.o \
	stacktrace.o \
	stat.o \
	strcon.o \
	stream.o \
	ts.o \
	tty.o \
	u.o \
	var.o \
	vfs.o \
	vfssw.o \
	vxfs_inode.o

# If there were a libcrash.a, it would consist of these four objects
LIBOBJS= \
	init.o \
	symtab.o \
	util.o \
	vtop.o

# crash32 take from libc.a, so even an OSr5 machine can examine Gemini dumps
LIBCOBJS= \
	strtoull.o \
	getksym.o \
	getopt.o \
	pfmt_data.o

XFILES= \
	$(CMDOBJS) \
	$(LIBOBJS)

X32FILES= \
	$(CMDOBJS) \
	$(LIBOBJS) \
	crash32.o \
	$(LIBCOBJS)

OFFSHDRS= \
	asoffs.h \
	bdevswoffs.h \
	bufoffs.h \
	cdevswoffs.h \
	credoffs.h \
	databoffs.h \
	engineoffs.h \
	exdataoffs.h \
	execinfooffs.h \
	fifonodeoffs.h \
	fileoffs.h \
	filockoffs.h \
	linkblkoffs.h \
	lwpoffs.h \
	metsoffs.h \
	modctloffs.h \
	modctl_listoffs.h \
	modobjoffs.h \
	moduleoffs.h \
	msgboffs.h \
	pageoffs.h \
	pidoffs.h \
	plocaloffs.h \
	plocalmetoffs.h \
	procoffs.h \
	queueoffs.h \
	runqueoffs.h \
	snodeoffs.h \
	stdataoffs.h \
	strttyoffs.h \
	useroffs.h \
	vfsoffs.h \
	vfsswoffs.h \
	vnodeoffs.h

# addstruct uses one tmpstruct.?
.MUTEX:	$(OFFSHDRS)

all: 	addstruct crash crash32 ldsysdump memsize memsize.dy

crash:	$(XFILES)
	$(CC) -Wl,-Bexport -o $@ $(XFILES) $(LDFLAGS) $(LDLIBS)

# version of crash to examine Gemini dumps on earlier OS without 64-bit support
crash32: $(X32FILES)
	$(CC) -Wl,-Bexport -o $@ $(X32FILES) $(LDFLAGS) $(LDLIBS)

$(LIBCOBJS): $(TOOLS)/usr/ccs/lib/libc.a
	$(AR) x $? $@

size.o:	size.c $(OFFSHDRS)

asoffs.h: $(INC)/vm/as.h
	sh addstruct as $? $@ $(OFFSCC)

bdevswoffs.h: $(INC)/sys/conf.h
	sh addstruct bdevsw $? $@ $(OFFSCC)

#bootinfooffs.h: $(INC)/sys/bootinfo.h
#	sh addstruct bootinfo $? $@ $(OFFSCC)

bufoffs.h: $(INC)/sys/buf.h
	sh addstruct buf $? $@ $(OFFSCC)

cdevswoffs.h: $(INC)/sys/conf.h
	sh addstruct cdevsw $? $@ $(OFFSCC)

credoffs.h: $(INC)/sys/cred.h
	sh addstruct cred $? $@ $(OFFSCC)

databoffs.h: $(INC)/sys/stream.h
	sh addstruct datab $? $@ $(OFFSCC)

engineoffs.h: $(INC)/sys/engine.h
	sh addstruct engine $? $@ $(OFFSCC)

exdataoffs.h: $(INC)/sys/exec.h
	sh addstruct exdata $? $@ $(OFFSCC)

execinfooffs.h: $(INC)/sys/exec.h
	sh addstruct execinfo $? $@ $(OFFSCC)

fifonodeoffs.h: $(INC)/sys/fs/fifonode.h
	sh addstruct fifonode $? $@ $(OFFSCC)

fileoffs.h: $(INC)/sys/file.h
	sh addstruct file $? $@ $(OFFSCC)

filockoffs.h: $(INC)/sys/flock.h
	sh addstruct filock $? $@ $(OFFSCC)

linkblkoffs.h: $(INC)/sys/stream.h
	sh addstruct linkblk $? $@ $(OFFSCC)

lwpoffs.h: $(INC)/sys/lwp.h
	sh addstruct lwp $? $@ $(OFFSCC)

# atypical rule: it is helpful if m.fields appear in nm listing
metsoffs.h: $(INC)/sys/metrics.h
	@rm -f $@
	sh addstruct mets $? tmpstruct.c $(OFFSCC)
	sed -e 's/,"/,"m./' -e 's/",/"+2,/' tmpstruct.c > $@
	@rm -f tmpstruct.c

modctloffs.h: $(INC)/sys/mod_k.h
	sh addstruct modctl $? $@ $(OFFSCC)

modctl_listoffs.h: $(INC)/sys/mod_k.h
	sh addstruct modctl_list $? $@ $(OFFSCC)

modobjoffs.h: $(INC)/sys/mod_obj.h
	sh addstruct modobj $? $@ $(OFFSCC)

moduleoffs.h: $(INC)/sys/mod_k.h
	sh addstruct module $? $@ $(OFFSCC)

msgboffs.h: $(INC)/sys/stream.h
	sh addstruct msgb $? $@ $(OFFSCC)

pageoffs.h: $(INC)/vm/page.h
	sh addstruct page $? $@ $(OFFSCC)

pidoffs.h: $(INC)/sys/pid.h
	sh addstruct pid $? $@ $(OFFSCC)

# atypical rule: it is helpful if l.fields appear in nm listing
plocaloffs.h: $(INC)/sys/plocal.h
	@rm -f plocaloffs.h
	sh addstruct plocal $? tmpstruct.c $(OFFSCC)
	sed -e 's/,"/,"l./' -e 's/",/"+2,/' tmpstruct.c > $@
	@rm -f tmpstruct.c

# atypical rule: it is helpful if lm.fields appear in nm listing
plocalmetoffs.h: $(INC)/sys/metrics.h
	@rm -f plocalmetoffs.h
	sh addstruct plocalmet $? tmpstruct.c $(OFFSCC)
	sed -e 's/,"/,"lm./' -e 's/",/"+3,/' tmpstruct.c > $@
	@rm -f tmpstruct.c

procoffs.h: $(INC)/sys/proc.h
	sh addstruct proc $? $@ $(OFFSCC)

queueoffs.h: $(INC)/sys/stream.h
	sh addstruct queue $? $@ $(OFFSCC)

runqueoffs.h: $(INC)/sys/disp.h
	sh addstruct runque $? $@ $(OFFSCC)

snodeoffs.h: $(INC)/sys/fs/snode.h
	sh addstruct snode $? $@ $(OFFSCC)

stdataoffs.h: $(INC)/sys/strsubr.h
	sh addstruct stdata $? $@ $(OFFSCC)

strttyoffs.h: $(INC)/sys/strtty.h
	sh addstruct strtty $? $@ $(OFFSCC)

useroffs.h: $(INC)/sys/user.h
	sh addstruct user $? $@ $(OFFSCC)

vfsoffs.h: $(INC)/sys/vfs.h
	sh addstruct vfs $? $@ $(OFFSCC)

vfsswoffs.h: $(INC)/sys/vfs.h
	sh addstruct vfssw $? $@ $(OFFSCC)

vnodeoffs.h: $(INC)/sys/vnode.h
	sh addstruct vnode $? $@ $(OFFSCC)

ldsysdump: 	ldsysdump.sh
		@-rm -f $@
		cp $? $@

# need to build memsize as a static binary since it can be run
# by dumpsave before all filesystem are mount. Since dynamic
# libraries live in /usr, if /usr is a different file system,
# then we can have a problem.

memsize: 	memsize.o
		$(CC) -o $@ memsize.o $(LDFLAGS) $(MLDLIBS) $(ROOTLIBS)

memsize.dy:	memsize.o
		$(CC) -o $@ memsize.o $(LDFLAGS) $(MLDLIBS)

install: 	ins_addstruct ins_crash ins_ldsysdump ins_memsize $(MSGS)
		-[ -d $(USRLIB)/locale/C/MSGFILES ] || \
			mkdir -p $(USRLIB)/locale/C/MSGFILES
		$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 memsize.str

ins_addstruct:	addstruct
		-[ -d $(USRLIB)/crash ] || mkdir -p $(USRLIB)/crash
		$(INS) -f $(USRLIB)/crash -m 0755 -u bin -g bin addstruct

ins_crash: 	crash
		-[ -d $(USRSBIN) ] || mkdir -p $(USRSBIN)
		-rm -f $(ETC)/crash
		$(INS) -f $(USRSBIN) -m 0555 -u bin -g bin crash
		-$(CH)$(SYMLINK) $(USRSBIN)/crash $(ETC)/crash

ins_ldsysdump: 	ldsysdump
		-[ -d $(USRSBIN) ] || mkdir -p $(USRSBIN)
		-rm -f $(ETC)/ldsysdump
		$(INS) -f $(USRSBIN) -m 0555 -u bin -g bin ldsysdump
		-$(CH)$(SYMLINK) $(USRSBIN)/ldsysdump $(ETC)/ldsysdump

ins_memsize:	memsize memsize.dy
		-[ -d $(SBIN) ] || mkdir -p $(SBIN)
		$(INS) -f $(SBIN) -m 0555 -u bin -g bin memsize
		$(INS) -f $(SBIN) -m 0555 -u bin -g bin memsize.dy

clean:
		-rm -f *.o
		-rm -f $(OFFSHDRS)
		-rm -f offstruct tmpstruct*

clobber: 	clean
		-rm -f crash
		-rm -f crash32
		-rm -f ldsysdump
		-rm -f memsize
		-rm -f memsize.dy

lint: 		$(CFILES) $(HFILES) 
		lint $(CPPFLAGS) -uh $(CFILES) 

#FRC:


