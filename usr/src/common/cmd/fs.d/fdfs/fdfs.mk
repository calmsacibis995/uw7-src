#	copyright	"%c%"

#ident	"@(#)fd.cmds:fdfs/fdfs.mk	1.2.3.2"
#ident "$Header$"

include $(CMDRULES)

INSDIR = $(ETC)/fs/fdfs
FRC =

all:	mount 

mount:	mount.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

mount.o:mount.c\
	$(INC)/stdio.h\
	$(INC)/signal.h\
	$(INC)/unistd.h\
	$(INC)/errno.h\
	$(INC)/sys/mnttab.h\
	$(INC)/sys/mount.h\
	$(INC)/sys/types.h\
	$(FRC)

install: all
	[ -d $(ETC)/fs ] || mkdir $(ETC)/fs
	[ -d $(INSDIR) ] || mkdir $(INSDIR)
	[ -d $(USRLIB)/fs/fdfs ] || mkdir -p $(USRLIB)/fs/fdfs
	$(INS) -f $(INSDIR) mount
	cp $(INSDIR)/mount $(USRLIB)/fs/fdfs/mount
	cp /dev/null fsck
	$(INS) -f $(INSDIR) fsck
	cp $(INSDIR)/fsck $(USRLIB)/fs/fdfs/fsck

clean:
	rm -f *.o

clobber:	clean
	rm -f mount
FRC:
