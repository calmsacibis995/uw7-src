#ident	"@(#)biod.mk	1.2"
#ident	"$Header$"

include $(CMDRULES)

INCSYS = $(INC)
LOCALDEF = -Dnetselstrings
INSDIR = $(USRLIB)/nfs
OWN = bin
GRP = bin

all: biod

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) biod

clean:
	-rm -f biod.o

clobber: clean
	-rm -f biod

biod:	biod.o
	$(CC) -o $@ $(LDFLAGS) $@.o $(LDLIBS) $(SHLIBS)

biod.o:	biod.c $(INC)/stdio.h \
	$(INCSYS)/sys/types.h \
	$(INCSYS)/sys/file.h \
	$(INCSYS)/sys/ioctl.h
