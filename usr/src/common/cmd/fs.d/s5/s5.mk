#	copyright	"%c%"

#ident	"@(#)s5.cmds:common/cmd/fs.d/s5/s5.mk	1.2.7.11"
#ident "$Header$"
#  /usr/src/cmd/lib/fs/s5 is the directory of all s5 specific commands
#  whose executable reside in $(INSDIR1) and $(INSDIR2).

include $(CMDRULES)

INSDIR1 = $(USRLIB)/fs/s5
INSDIR2 = $(ETC)/fs/s5
OWN = bin
GRP = bin
LOCALDEF = -D_KMEMUSER
LDLIBS = -lgen

LIBELF =
#
#	Libelf is required for mkfs if ELF_BOOT is defined.  This would allow
#	mkfs to open, parse, and load ELF boot strap code to the disk.  That
#	is, if the name of an ELF boot file was given as the first line of
#	a proto file.
#
# LIBELF = $(LIBELF)

LINTFLAGS = $(DEFLIST)

THISFILE= s5.mk

all:	mount df  ff fsck  fsdb labelit mkfs ncheck volcopy dcopy

clean:
	rm -f *.o

clobber: clean		clobber_mount \
	clobber_dcopy\
	clobber_df	clobber_fsck\
	clobber_fsdb    clobber_ff \
	clobber_labelit clobber_mkfs \
	clobber_ncheck clobber_volcopy

install: install_mount install_ncheck \
	install_dcopy\
	install_df	install_fsck\
	install_fsdb    install_ff \
	install_labelit install_mkfs \
	install_volcopy

#
#  This is to build s5 specific mount command
#

mount:	mount.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)
	$(CC) -o $@.dy $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

mount.o:mount.c\
	$(INC)/stdio.h\
	$(INC)/sys/signal.h\
	$(INC)/unistd.h\
	$(INC)/sys/errno.h\
	$(INC)/sys/mnttab.h\
	$(INC)/sys/mount.h\
	$(INC)/sys/types.h\
	$(INC)/sys/statvfs.h

install_mount: mount $(INSDIR1) $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mount
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mount
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mount.dy
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mount.dy

clobber_mount:	clean
	rm -f mount mount.dy

#
#  This is to build the s5 specific df command
#

df:	df.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

df.o:	df.c \
 	$(INC)/sys/param.h $(INC)/sys/types.h \
	$(INC)/sys/mnttab.h $(INC)/sys/stat.h \
	$(INC)/sys/statfs.h $(INC)/sys/errno.h \
	$(INC)/sys/vnode.h $(INC)/sys/fs/s5param.h \
	$(INC)/sys/fs/s5filsys.h $(INC)/sys/fs/s5fblk.h \
	$(INC)/sys/fs/s5dir.h $(INC)/sys/fs/s5ino.h \
	$(INC)/setjmp.h \
	$(INC)/string.h $(INC)/locale.h $(INC)/pfmt.h

install_df: df $(INSDIR1) $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) df
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) df

clobber_df:
	rm -f df

#
# This is s5 specific fsck
#

.MUTEX: fsck.o fsck2.o fsck3.o fsck4.o
	

fsck: fsck.o fsck2.o fsck3.o fsck4.o
	$(CC) $(LDFLAGS) -o $@ fsck.o fsck2.o fsck3.o fsck4.o $(LDLIBS) $(ROOTLIBS)
	$(CC) $(LDFLAGS) -o $@.dy fsck.o fsck2.o fsck3.o fsck4.o $(LDLIBS) $(SHLIBS)

fsck2.o: $(INC)/sys/types.h\
	$(INC)/sys/fs/s5param.h\
	fsckinit.c\
	$(FRC)
	$(CC) $(CFLAGS) $(DEFLIST) -DFsTYPE=1 -c fsckinit.c
	mv fsckinit.o fsck2.o

fsck3.o: $(INC)/sys/types.h\
	$(INC)/sys/fs/s5param.h\
	fsckinit.c\
	$(FRC)
	$(CC) $(CFLAGS) $(DEFLIST) -DFsTYPE=2 -c fsckinit.c
	mv fsckinit.o fsck3.o

fsck4.o: $(INC)/sys/types.h\
	$(INC)/sys/fs/s5param.h\
	fsckinit.c\
	$(FRC)
	$(CC) $(CFLAGS) $(DEFLIST) -DFsTYPE=4 -c fsckinit.c
	mv fsckinit.o fsck4.o

fsck.o: $(INC)/stdio.h\
	$(INC)/ctype.h\
	$(INC)/string.h \
	$(INC)/sys/fcntl.h\
	$(INC)/stand.h\
	$(INC)/sys/param.h\
	$(INC)/sys/types.h\
	$(INC)/signal.h\
	$(INC)/sys/uadmin.h\
	$(INC)/sys/vnode.h\
	$(INC)/sys/fs/s5param.h\
	$(INC)/sys/fs/s5ino.h\
	$(INC)/sys/fs/s5inode.h\
	$(INC)/sys/fs/s5filsys.h\
	$(INC)/sys/fs/s5dir.h\
	$(INC)/sys/fs/s5fblk.h\
	$(INC)/sys/stat.h\
	fsck.c\
	$(FRC)
	$(CC) $(CFLAGS) $(DEFLIST) -DFsTYPE=4 -D$(CPU) -c fsck.c 
#  VERY IMPORTANT: fsck.c MUST be compiled with FsTYPE set to the
#  largest blocksize to allocate the biggest buffer possible.


install_fsck: fsck $(INSDIR1) $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fsck
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fsck
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fsck.dy
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fsck.dy

FRC :

clobber_fsck:
	rm -f fsck fsck.dy

lintit:
	$(LINT) $(DEFLIST) -DFsTYPE=1 fsck2.c 
	$(LINT) $(DEFLIST) -DFsTYPE=2 fsck3.c 
	$(LINT) $(DEFLIST) -DFsTYPE=4 -D$(CPU) fsck.c 

#
#  This is to build s5 specific fsdb command
#

fsdb: fsdb.o
	$(CC) $@.o -o $@ $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

fsdb.o:	fsdb.c \
	$(INC)/sys/param.h\
	$(INC)/signal.h\
	$(INC)/time.h\
	$(INC)/sys/types.h\
	$(INC)/sys/sysmacros.h\
	$(INC)/sys/vnode.h\
	$(INC)/sys/fs/s5param.h\
	$(INC)/sys/fs/s5ino.h\
	$(INC)/sys/fs/s5inode.h\
	$(INC)/sys/fs/s5dir.h\
	$(INC)/stdio.h\
	$(INC)/setjmp.h\
	$(INC)/sys/fs/s5filsys.h

install_fsdb: fsdb $(INSDIR1) $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fsdb
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fsdb

clobber_fsdb:
	rm -f fsdb

#
#  This is to build s5 specific labelit command
#

labelit: labelit.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

labelit.o: labelit.c \
	$(INC)/stdio.h $(INC)/sys/param.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/types.h $(INC)/sys/sysmacros.h \
	$(INC)/sys/filsys.h

clobber_labelit:
	rm -f labelit

install_labelit: labelit $(INSDIR1)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) labelit
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) labelit

#
#  This is to build s5 specific mkfs command
#

mkfs:	mkfs.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)
	$(CC) -o $@.dy $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

mkfs.o:	mkfs.c \
	$(INC)/stdio.h  \
	$(INC)/a.out.h \
	$(INC)/signal.h  \
	$(INC)/sys/types.h \
	$(INC)/sys/fs/s5macros.h  \
	$(INC)/sys/vnode.h \
	$(INC)/sys/param.h  \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/fs/s5fblk.h  \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/sys/fs/s5filsys.h  \
	$(INC)/sys/fs/s5ino.h \
	$(INC)/sys/stat.h  \
	$(INC)/sys/fs/s5inode.h

install_mkfs: mkfs $(INSDIR1) $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mkfs 
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mkfs 
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mkfs.dy
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mkfs.dy

clobber_mkfs: 
	rm -f mkfs mkfs.dy

#
#  This is to build s5 specific ncheck command
#

ncheck: ncheck.c 
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install_ncheck: ncheck $(INSDIR1)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) ncheck

clobber_ncheck: clean
	rm -f ncheck

#
# This is to build the s5 specific volcopy
#

volcopy: volcopy.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) -lgenIO $(ROOTLIBS)

volcopy.o: volcopy.c \
	$(INC)/sys/param.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/signal.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/filsys.h \
	$(INC)/sys/fs/s5filsys.h \
	$(INC)/sys/stat.h \
	$(INC)/stdio.h \
	$(INC)/archives.h \
	$(INC)/libgenIO.h \
	volcopy.h

install_volcopy: volcopy $(INSDIR1)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) volcopy

clobber_volcopy: clean
	rm -f volcopy

#
# This is to build the s5 specific ff 
#

ff:	ff.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ff.o:	ff.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/fs/s5filsys.h \
	$(INC)/sys/fs/s5ino.h \
	$(INC)/sys/fs/s5inode.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/sys/stat.h \
	$(INC)/stdio.h \
	$(INC)/pwd.h \
	$(INC)/malloc.h

install_ff: ff $(INSDIR1)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) ff

clobber_ff: clean
	rm -f ff

# 
#  This is to build s5 specific dcopy command 
# 

dcopy:	dcopy.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

dcopy.o: dcopy.c \
	$(INC)/sys/signal.h\
	$(INC)/sys/fcntl.h\
	$(INC)/sys/param.h\
	$(INC)/sys/types.h\
	$(INC)/sys/vnode.h\
	$(INC)/sys/stat.h \
	$(INC)/sys/fs/s5param.h\
	$(INC)/sys/fs/s5filsys.h\
	$(INC)/sys/fs/s5ino.h\
	$(INC)/sys/fs/s5inode.h\
	$(INC)/sys/fs/s5dir.h\
	$(INC)/sys/fs/s5fblk.h\
	$(INC)/stdio.h

install_dcopy: dcopy $(INSDIR1)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) dcopy

clobber_dcopy:
	rm -f dcopy

$(INSDIR1):
	mkdir -p $(INSDIR1)

$(INSDIR2):
	mkdir -p $(INSDIR2)
