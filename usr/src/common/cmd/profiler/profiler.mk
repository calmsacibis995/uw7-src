#	copyright	"%c%"

#ident	"@(#)profiler:profiler.mk	1.4.7.2"
#ident	"$Header$"

include	$(CMDRULES)

OWN	=	bin
GRP	=	bin

INSDIR = $(USRSBIN)
LINSDIR = /usr/sbin

all:	prfdc prfpr prfsnap prfstat prfld

install:	all
	-rm -f $(ETC)/prfdc
	-rm -f $(ETC)/prfpr
	-rm -f $(ETC)/prfld
	-rm -f $(ETC)/prfsnap
	-rm -f $(ETC)/prfstat
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) prfdc
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) prfpr
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) prfld
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) prfsnap
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) prfstat
	-$(SYMLINK) $(LINSDIR)/prfdc $(ETC)/prfdc
	-$(SYMLINK) $(LINSDIR)/prfpr $(ETC)/prfpr
	-$(SYMLINK) $(LINSDIR)/prfld $(ETC)/prfld
	-$(SYMLINK) $(LINSDIR)/prfsnap $(ETC)/prfsnap
	-$(SYMLINK) $(LINSDIR)/prfstat $(ETC)/prfstat


prfdc:		prfdc.c prf_load.o uprf.o
	$(CC) $(CFLAGS) $(DEFLIST) -o prfdc   prf_load.o uprf.o prfdc.c $(LDFLAGS) $(LIBELF) $(SHLIBS)

prfpr:		prfpr.c
	$(CC) $(CFLAGS) $(DEFLIST) -o prfpr prfpr.c $(LDFLAGS) $(SHLIBS)

prfld:		prfld.c prf_load.o uprf.o
	$(CC) $(CFLAGS) $(DEFLIST) -o prfld   prf_load.o uprf.o prfld.c $(LDFLAGS) $(LIBELF) $(SHLIBS)

prfsnap:	prfsnap.c uprf.o prf_load.o
	$(CC) $(CFLAGS) $(DEFLIST) -o prfsnap  prf_load.o uprf.o prfsnap.c $(LDFLAGS) $(LIBELF) $(SHLIBS)

prfstat:	prfstat.c prf_load.o uprf.o
	$(CC) $(CFLAGS) $(DEFLIST) -o prfstat prfstat.c prf_load.o uprf.o $(LDFLAGS) $(LIBELF) $(SHLIBS)

prf_load.o:	prf_load.c
	$(CC) $(CFLAGS) $(DEFLIST) -c prf_load.c 

uprf.o:	uprf.c
	$(CC) $(CFLAGS) $(DEFLIST) -c uprf.c 

clean:
	-rm -f *.o

clobber:	clean
	-rm -f prfdc prfpr prfld prfsnap prfstat

lintit:
	$(LINT) $(LINTFLAGS) prfdc.c prfpr.c prfld.c prfsnap.c prfstat.c

prfdc.o: prfdc.c \
	$(INC)/time.h \
	$(INC)/signal.h \
	$(INC)/errno.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/fcntl.h \
	$(INC)/stdlib.h \
	$(INC)/sys/prf.h
prfpr.o: prfpr.c \
	$(INC)/stdio.h \
	$(INC)/time.h \
	$(INC)/a.out.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/unistd.h $(INC)/sys/unistd.h \
	$(INC)/stdlib.h $(INC)/sys/prf.h
prfsnap.o: prfsnap.c \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/stdlib.h $(INC)/sys/prf.h
prfstat.o: prfstat.c \
	$(INC)/stdio.h \
	$(INC)/sys/errno.h \
	$(INC)/a.out.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/unistd.h $(INC)/sys/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/sys/prf.h
prfld.o: prfld.c \
	$(INC)/stdio.h $(INC)/fcntl.h $(INC)/sys/fcntl.h $(INC)/sys/prf.h
prf_load.o: prf_load.c \
	$(INC)/errno.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/fcntl.h \
	$(INC)/signal.h \
	$(INC)/stdlib.h \
	$(INC)/sys/prf.h
uprf.o: uprf.c \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h \
	$(INC)/malloc.h \
	$(INC)/libelf.h \
	$(INC)/sys/prf.h
