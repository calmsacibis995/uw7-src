#ident	"@(#)ksh93:src/lib/libast/Makefile	1.1"
#  ast library make

include $(LIBRULES)

CCFLAGS = -O $(CC.PIC)

INSTALLROOT = ../../..

LOCALINC = -I$(INSTALLROOT)/src/lib/libast/include \
	-I$(INSTALLROOT)/src/lib/libast/obsolete \
	-I$(INSTALLROOT)/src/lib/libast/sfio \
	-I$(INSTALLROOT)/src/lib/libast/misc \
	-I$(INSTALLROOT)/src/lib/libast/dir \
	-I$(INSTALLROOT)/src/lib/libast/re \
	-I$(INSTALLROOT)/src/lib/libast/hash \
	-I$(INSTALLROOT)/src/lib/libast/string \
	-I$(INSTALLROOT)/src/lib/libast/man \
	-I$(INSTALLROOT)/src/lib/libast

HEADERSRC = 

SOURCES = confmap.c confstr.c conftab.c pathconf.c sysconf.c

OBJECTS = $(SOURCES:.c=.o)

SFIOOBJECTS = sfclose.o sfclrlock.o sfdisc.o \
	sfdlen.o sfexcept.o sfcvt.o sfecvt.o sffcvt.o sfextern.o sffilbuf.o \
	sfflsbuf.o sfprints.o sfgetd.o sfgetl.o \
	sfgetr.o sfgetu.o sfllen.o sfmode.o \
	sfmove.o sfnew.o sfpkrd.o sfnotify.o \
	sfnputc.o sfopen.o sfpeek.o sfpoll.o \
	sfpool.o sfpopen.o sfprintf.o sfputd.o \
	sfputl.o sfputr.o sfputu.o sfrd.o \
	sfread.o sfreserve.o sfscanf.o sfseek.o \
	sfset.o sfsetbuf.o sfsetfd.o sfsize.o \
	sfsk.o sfstack.o sfstrtod.o sfsync.o \
	sfswap.o sftable.o sftell.o sftmp.o \
	sfungetc.o sfvprintf.o sfvscanf.o sfwr.o \
	sfwrite.o sfpurge.o _sfclrerr.o _sfeof.o \
	_sferror.o _sffileno.o _sfgetc.o \
	_sfgetl.o _sfgetu.o _sfstacked.o \
	_sfputc.o _sfputd.o _sfputl.o _sfputu.o \
	_sfslen.o _sfulen.o \
	stdgets.o stdprintf.o stdscanf.o stdvbuf.o stdsprnt.o \
	stdvsprnt.o stdvsnprnt.o stdvsscn.o stdopen.o

LIBASTOBJECTS = state.o \
        opendir.o readdir.o rewinddir.o seekdir.o telldir.o \
        getcwd.o fastfind.o \
        hashalloc.o hashdump.o hashfree.o hashlast.o hashlook.o \
        hashscan.o hashsize.o hashwalk.o \
        memhash.o memsum.o strhash.o strsum.o \
        mematoe.o memetoa.o \
        chresc.o chrtoi.o streval.o strexpr.o strmatch.o strcopy.o \
        modei.o modex.o strmode.o strlook.o \
        stresc.o stropt.o strtape.o tok.o tokline.o tokscan.o \
        pathaccess.o pathcat.o pathcanon.o pathcheck.o pathpath.o \
        pathkey.o pathprobe.o pathrepl.o pathtemp.o pathstat.o \
	pathgetlink.o pathsetlink.o pathbin.o pathshell.o pathcd.o \
        fs3d.o ftwalk.o ftwflags.o setenviron.o \
        optget.o optjoin.o strsort.o \
	astcopy.o astconf.o astquery.o astwinsize.o \
        magic.o signal.o systrace.o \
        error.o liberror.o \
        fmtbase.o fmtdev.o fmtelapsed.o fmterror.o fmtesc.o fmtfs.o \
	fmtls.o fmtmatch.o \
        fmtmode.o fmtperm.o fmtre.o fmttime.o fmtuid.o fmtgid.o fmtsignal.o \
        strelapsed.o strperm.o struid.o strgid.o stack.o stk.o \
        swapget.o swapmem.o swapop.o swapput.o \
        sigdata.o sigcrit.o sigunblock.o \
        procopen.o procclose.o procrun.o procfree.o \
        recomp.o reerror.o reexec.o resub.o ressub.o \
        tmdate.o tmfix.o tmfmt.o tmform.o tmgoff.o \
        tminit.o tmleap.o tmlex.o tmmake.o tmpoff.o \
        tmtime.o tmtype.o tmword.o tmzone.o \
        vecargs.o vecfile.o vecfree.o vecload.o vecstring.o \
	malloc.o vmbest.o vmclear.o vmclose.o vmdcheap.o \
        vmdcsbrk.o vmdebug.o vmdisc.o vmexit.o vmlast.o vmopen.o \
        vmpool.o vmprivate.o vmprofile.o vmregion.o vmsegment.o \
        vmset.o vmstat.o vmstrdup.o vmtrace.o \
        univdata.o touch.o \
        memccpy.o memchr.o memcmp.o memcpy.o memdup.o memmove.o memset.o \
        mkdir.o mkfifo.o mknod.o rmdir.o remove.o rename.o link.o unlink.o \
        strdup.o strchr.o strrchr.o strtod.o strtol.o strtoul.o strton.o \
        setlocale.o mktemp.o tmpnam.o \
        execve.o spawnve.o spawnveg.o vfork.o killpg.o \
        lstat.o readlink.o statvfs.o symlink.o \
        getpgrp.o setpgid.o setsid.o waitpid.o \
	confmap.o conftab.o confstr.o \
        pathconf.o sysconf.o fcntl.o open.o \
        atexit.o getdents.o getwd.o dup2.o errno.o \
        getpreroot.o ispreroot.o realopen.o setpreroot.o \
        getgroups.o mount.o system.o iblocks.o \
        modedata.o tmdata.o sfkeyprintf.o sfslowio.o

OBSOLETEOBJECTS = fdcopy.o getpath.o getshell.o getsymlink.o \
	getuniv.o getwinsize.o hsort.o lpstat.o putsymlink.o \
	query.o setcwd.o setenv.o setuniv.o slowio.o strerror.o \
	strsignal.o tablook.o astfeature.o

DIRS = comp.d disc.d hash.d misc.d obsolete.d path.d port.d preroot.d re.d \
	sfio.d stdio.d string.d tm.d vec.d vmalloc.d dir.d

all:	$(DIRS) $(OBJECTS) libast.a

clean:
	rm -f $(LIBASTOBJECTS) $(SFIOOBJECTS) $(OBSOLETEOBJECTS) 
	for i in $(DIRS:.d=); \
	do\
		cd $$i; make clean; cd ..;\
	done

clobber:	clean
	rm -f libast.a
	for i in $(DIRS:.d=); \
	do\
		cd $$i; make clobber; cd ..;\
	done

# miscellaneous files: README hash.3 ftwalk.3 re.3 sfio.3 stak.3 stk.3 tm.3

libast.a : $(LIBASTOBJECTS) $(SFIOOBJECTS) $(OBSOLETEOBJECTS) 
	$(AR) $(ARFLAGS) libast.a $(LIBASTOBJECTS) \
		$(SFIOOBJECTS) $(OBSOLETEOBJECTS) $(INCLIBS)

$(DIRS) :
	cd $(@:.d=); make all CCFLAGS=$(CCFLAGS)

.c.o:
	$(CC) $(CCFLAGS) $(LOCALINC) $(GLOBALINC) -c $< 
