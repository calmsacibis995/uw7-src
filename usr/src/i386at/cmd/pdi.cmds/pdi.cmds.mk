#ident	"@(#)pdi.cmds:pdi.cmds.mk	1.2.10.1"
#ident	"$Header$"

include $(CMDRULES)

#       Makefile for pdi.cmds

OWN = root
GRP = sys

BUS	= AT386
LOCALDEF= -D$(BUS)

SYSTEMENV = 4 

INSDIR = $(ETC)/scsi
FMTDIR = $(ETC)/scsi/format.d
MDVDIR = $(ETC)/scsi/mkdev.d
TARGETDIR = $(ETC)/scsi/target.d
DFLTDIR = $(ETC)/default
DISKMGMT = $(ETC)/diskmgmt/s5dm

LDLIBS = -ladm -lgen -lcmd
SETUPLIBS = $(LDLIBS) -lelf

MAINS = bmkdev pdimkdev pdiconfig diskcfg tapecntl disksetup prtvtoc \
	edvtoc diskformat hbacompat mccntl pdi_hot pdi_timeout sdimkosr5 \
	sdipath fixroot
SCRIPTS = sdiadd diskadd diskrm diskaddrm
MSGS = disksetup.str tapecntl.str prtvtoc.str diskadd.str diskaddrm.str \
	diskrm.str edvtoc.str mccntl.str timeout.str sdimkosr5.str \
	sdipath.str pdi_hot.str

FORMAT_FILES = sd00.0 sd01.1
MKDEV_FILES = disk1 qtape1 cdrom1 worm1 9track1 hba1 changer1 cled1
TARGET_FILES = sc01 sd01 st01 sw01 mc01 cled
DFLT_FILES = bfs fstyp s5 sfs ufs vxfs dskmgmt

OFILES = script.o scsicomm.o scl.o scsi_setup.o ix_altsctr.o readxedt.o
BOBJECTS = boot_mkdev.o readxedt.o edt_sort.o readhbamap.o
MOBJECTS = mkdev.o tload.o readxedt.o edt_sort.o readhbamap.o edt_fix.o scsicomm.o
COBJECTS = config.o tload.o readxedt.o edt_sort.o readhbamap.o edt_fix.o scsicomm.o
DOBJECTS = diskcfg.o scsicomm.o
HOBJECTS = pdi_hot.o
FOBJECTS = fixroot.o
TOBJECTS = timeout.o ptFuncs.o readxedt.o

FORMAT = \
        tc.index \
        format.d/sd00.0 \
        format.d/sd01.1

MKDEV = \
	mkdev.d/disk1 \
	mkdev.d/qtape1 \
	mkdev.d/cdrom1 \
	mkdev.d/hba1 \
	mkdev.d/worm1 \
	mkdev.d/cled1 \
	mkdev.d/changer1

TARGETS = \
	target.d/sc01 \
	target.d/sd01 \
	target.d/st01 \
	target.d/sw01 \
	target.d/cled \
	target.d/mc01

all:	$(SCRIPTS) $(MAINS) $(FORMAT) $(MKDEV) $(TARGETS)
	echo "**** pdi.cmds build completes" > /dev/null

install:	all $(MSGS)
	-[ -d $(INSDIR) ] || mkdir $(INSDIR)
	-[ -d $(FMTDIR) ] || mkdir $(FMTDIR)
	-[ -d $(MDVDIR) ] || mkdir $(MDVDIR)
	-[ -d $(DFLTDIR) ] || mkdir $(DFLTDIR)
	-[ -d $(TARGETDIR) ] || mkdir $(TARGETDIR)
	-[ -d $(DISKMGMT) ] || mkdir -p $(DISKMGMT)
	-[ -d $(USRBIN) ] || mkdir -p $(USRBIN)
	-[ -d $(USRSBIN) ] || mkdir -p $(USRSBIN)
	-[ -d $(SBIN) ] || mkdir -p $(SBIN)
	-[ -d tmp ] || mkdir tmp
	-[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRSBIN) -m 0544 -u bin -g bin disksetup
	$(INS) -f $(USRSBIN) -m 0544 -u bin -g bin mccntl
	$(INS) -f $(USRSBIN) -m 0544 -u bin -g bin prtvtoc
	$(INS) -f $(USRSBIN) -m 0544 -u bin -g bin edvtoc
	$(INS) -f $(DISKMGMT) -m 0755 -u $(OWN) -g $(GRP) diskadd
	$(INS) -f $(DISKMGMT) -m 0755 -u $(OWN) -g $(GRP) diskrm
	$(INS) -f $(SBIN) -m 0755 -u $(OWN) -g $(GRP) diskaddrm
	$(CH)-$(RM) -f $(SBIN)/diskadd $(SBIN)/diskrm
	$(CH)-ln $(SBIN)/diskaddrm $(SBIN)/diskadd
	$(CH)-ln $(SBIN)/diskaddrm $(SBIN)/diskrm
	$(INS) -f $(USRSBIN) -m 0755 -u $(OWN) -g $(GRP) diskformat
	$(INS) -f $(SBIN) -m 0544 -u $(OWN) -g $(GRP) fixroot
	$(INS) -f $(INSDIR) -m 755 -u $(OWN) -g $(GRP) diskcfg
	$(INS) -f $(INSDIR) -m 755 -u $(OWN) -g $(GRP) pdiconfig
	$(INS) -f $(INSDIR) -m 755 -u $(OWN) -g $(GRP) bmkdev
	$(INS) -f $(INSDIR) -m 755 -u $(OWN) -g $(GRP) pdimkdev
	$(INS) -f $(INSDIR) -m 755 -u $(OWN) -g $(GRP) sdiadd
	$(INS) -f $(INSDIR) -m 755 -u $(OWN) -g $(GRP) pdi_hot
	$(INS) -f $(INSDIR) -m 755 -u $(OWN) -g $(GRP) hbacompat
	$(INS) -f $(INSDIR) -m 755 -u $(OWN) -g $(GRP) pdi_timeout
	$(INS) -f $(INSDIR) -m 755 -u $(OWN) -g $(GRP) sdimkosr5
	$(INS) -f $(INSDIR) -m 755 -u $(OWN) -g $(GRP) sdipath
	$(INS) -f $(INSDIR) -m 400 -u $(OWN) -g $(GRP) pditimetab.orig
	$(CP) $(INSDIR)/pditimetab.orig $(INSDIR)/pditimetab
	$(INS) -f $(USRBIN) -m 555 -u bin -g bin tapecntl
	grep -v "^#ident" tc.index > tmp/tc.index ;\
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin tmp/tc.index
	for i in $(FORMAT_FILES); \
	do \
		grep -v "^#ident" format.d/$$i > tmp/$$i ;\
		$(INS) -f $(FMTDIR) -m 0555 -u bin -g bin tmp/$$i ;\
	done
	for i in $(MKDEV_FILES); \
	do \
		grep -v "^#ident" mkdev.d/$$i > tmp/$$i ;\
		$(INS) -f $(MDVDIR) -m 0555 -u bin -g bin tmp/$$i ;\
	done
	for i in $(TARGET_FILES); \
	do \
		cp target.d/$$i tmp/$$i ;\
		$(INS) -f $(TARGETDIR) -m 0444 -u bin -g bin tmp/$$i ;\
	done
	for i in $(DFLT_FILES); \
	do \
		$(CP) dflt.d/$$i.dflt tmp/$$i ;\
		$(INS) -f $(DFLTDIR) -m 444 -u $(OWN) -g $(GRP) tmp/$$i ;\
	done
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 disksetup.str
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 mccntl.str
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 tapecntl.str
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 prtvtoc.str
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 diskadd.str
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 diskaddrm.str
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 diskrm.str
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 edvtoc.str
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 pdi_hot.str
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 timeout.str
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 sdimkosr5.str
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 sdipath.str

clean:
	rm -f *.o tmp/*

clobber: clean
	rm -f $(SCRIPTS) $(MAINS)
	rm -rf tmp

sdiadd: sdiadd.sh

diskadd: diskadd.sh

diskrm: diskrm.sh

diskaddrm: diskaddrm.sh

fixroot: $(FOBJECTS)
	$(CC) -o fixroot $(FOBJECTS) $(LDFLAGS)

pdi_hot: $(HOBJECTS)
	$(CC) -o pdi_hot $(HOBJECTS) $(LDFLAGS) $(LDLIBS)

bmkdev: $(BOBJECTS)
	$(CC) -o bmkdev $(BOBJECTS) $(LDFLAGS)

pdimkdev: $(MOBJECTS)
	$(CC) -o pdimkdev $(MOBJECTS) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

diskcfg: $(DOBJECTS)
	$(CC) -o diskcfg $(DOBJECTS) $(LDFLAGS) $(LDLIBS) -lresmgr $(ROOTLIBS)

pdiconfig: $(COBJECTS)
	$(CC) -o pdiconfig $(COBJECTS) $(LDFLAGS) $(LDLIBS) -lresmgr $(ROOTLIBS)

tapecntl:	tapecntl.o
	$(CC) -o tapecntl tapecntl.o $(LDFLAGS) $(ROOTLIBS)

disksetup:	 disksetup.o diskinit.o boot.o $(OFILES) 
	$(CC) -o disksetup disksetup.o diskinit.o boot.o $(OFILES) $(LDFLAGS) $(SETUPLIBS) $(NOSHLIBS)

mccntl:			mccntl.o
	$(CC) -o mccntl mccntl.o $(LDFLAGS)

diskformat:	 diskformat.o $(OFILES) 
	$(CC) -o diskformat diskformat.o $(OFILES) $(LDFLAGS) $(LDLIBS)

prtvtoc: prtvtoc.o ix_altsctr.o readxedt.o scsicomm.o
	$(CC) -o prtvtoc prtvtoc.o ix_altsctr.o readxedt.o scsicomm.o $(LDFLAGS)
	
edvtoc: edvtoc.o
	$(CC) -o edvtoc edvtoc.o $(LDFLAGS)

hbacompat: hbacompat.o
	$(CC) -o hbacompat hbacompat.o $(LDFLAGS)

pdi_timeout: $(TOBJECTS)
	$(CC) -o pdi_timeout $(TOBJECTS) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

sdimkosr5: sdimkosr5.o
	$(CC) -o sdimkosr5 sdimkosr5.o $(LDFLAGS)

sdipath: sdipath.o
	$(CC) -o sdipath sdipath.o $(LDFLAGS) $(LDLIBS)

config.o: config.c \
	diskcfg.h \
	scsicomm.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/unistd.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/limits.h \
	$(INC)/dirent.h \
	$(INC)/nlist.h \
	$(INC)/fcntl.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/sdi_edt.h \
	$(INC)/sys/sdi.h

mkdev.o: mkdev.c \
	fixroot.h \
	scsicomm.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/statfs.h \
	$(INC)/ctype.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/vtoc.h \
	$(INC)/string.h \
	$(INC)/ftw.h \
	$(INC)/devmgmt.h \
	$(INC)/unistd.h \
	$(INC)/sys/vfstab.h \
	$(INC)/sys/sd01_ioctl.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/fs/s5filsys.h \
	$(INC)/sys/sdi_edt.h \
	$(INC)/sys/scsi.h \
	$(INC)/sys/sdi.h

scsicomm.o: scsicomm.c \
	scsicomm.h \
	$(INC)/sys/types.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sdi_edt.h \
	$(INC)/fcntl.h \
	$(INC)/sys/fcntl.h \
	$(INC)/errno.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/sd01_ioctl.h \
	$(INC)/string.h \
	$(INC)/stdio.h

diskcfg.o: diskcfg.c \
	diskcfg.h \
	scsicomm.h \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/sys/errno.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/unistd.h \
	$(INC)/limits.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/sdi_edt.h \
	$(INC)/sys/types.h \
	$(INC)/sys/wait.h \
	$(INC)/sys/stat.h

tapecntl.o:	tapecntl.c

disksetup.o:	badsec.h \
		$(INC)/stdio.h \
		$(INC)/fcntl.h \
		$(INC)/ctype.h \
		$(INC)/malloc.h \
		$(INC)/string.h \
		$(INC)/sys/types.h \
		$(INC)/sys/vtoc.h \
		$(INC)/sys/termios.h \
		$(INC)/sys/alttbl.h \
		$(INC)/sys/altsctr.h \
		$(INC)/sys/param.h \
		$(INC)/sys/fdisk.h \
		$(INC)/sys/fsid.h \
		$(INC)/sys/fstyp.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/swap.h \
		$(INC)/signal.h 

mccntl.o:	$(INC)/fcntl.h \
		$(INC)/sys/types.h \
		$(INC)/sys/errno.h \
		$(INC)/sys/signal.h \
		$(INC)/sys/param.h \
		$(INC)/sys/buf.h \
		$(INC)/sys/scsi.h \
		$(INC)/sys/sdi_edt.h \
		$(INC)/sys/sdi.h \
		$(INC)/sys/mc01.h \
		$(INC)/stdio.h \
		$(INC)/ctype.h

script.o:	

scl.o:	

scsi_setup.o:	

ix_altsctr.o:	badsec.h \
		$(INC)/stdio.h \
		$(INC)/fcntl.h \
		$(INC)/ctype.h \
		$(INC)/malloc.h \
		$(INC)/string.h \
		$(INC)/sys/types.h \
		$(INC)/sys/vtoc.h \
		$(INC)/sys/alttbl.h \
		$(INC)/sys/altsctr.h \
		$(INC)/sys/param.h \
		$(INC)/sys/fdisk.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/swap.h \
		$(INC)/signal.h 

diskinit.o:	diskinit.c

boot.o:		boot.c

hbacompat.o:	$(INC)/fcntl.h \
		$(INC)/sys/ksym.h

edt_sort.o: edt_sort.h

fixroot.o: fixroot.h
