#	copyright	"%c%"

#ident	"@(#)mp.cmds:i386/cmd/sysdef/sysdef.mk	1.1"
#ident "$Header$"

include $(CMDRULES)

OWN = bin
GRP = bin

LDLIBS = $(LIBELF)
DEFLIST = -D_KMEMUSER

all: sysdef

sysdef: sysdef.o
	$(CC) sysdef.o -o sysdef $(LDFLAGS) $(LDLIBS) $(SHLIBS)

sysdef.o: sysdef.c \
	$(INC)/stdio.h \
	$(INC)/nlist.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/sysi86.h \
	$(INC)/sys/var.h \
	$(INC)/sys/tuneable.h \
	$(INC)/sys/ipc.h \
	$(INC)/sys/msg.h \
	$(INC)/sys/sem.h \
	$(INC)/sys/shm.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/utsname.h \
	$(INC)/sys/resource.h \
	$(INC)/sys/conf.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/hrtcntl.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/procset.h \
	$(INC)/ctype.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/acl.h $(INC)/sys/acl.h \
	$(INC)/sys/swap.h \
	$(INC)/libelf.h \
	$(INC)/sys/elf_386.h \
	$(INC)/sys/param.h \
	$(INC)/sys/ksym.h

install: all
	-rm -f $(ETC)/sysdef
	 $(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) sysdef
	-$(SYMLINK) /usr/sbin/sysdef $(ETC)/sysdef

clean:
	-rm -f sysdef.o

clobber: clean
	-rm -f sysdef

lintit:
	$(LINT) $(LINTFLAGS) sysdef.c
