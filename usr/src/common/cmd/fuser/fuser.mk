#	copyright	"%c%"

#ident	"@(#)fuser:fuser.mk	1.1.17.1"

include $(CMDRULES)

#	Makefile for fuser

LOCALDEF = -D_KMEMUSER
LDLIBS = -lgen
OWN = bin
GRP = bin

all: fuser

fuser: fuser.o
	$(CC) -o fuser fuser.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

fuser.o: fuser.c \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/pwd.h \
	$(INC)/stdio.h \
	$(INC)/priv.h $(INC)/sys/privilege.h \
	$(INC)/sys/mnttab.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/param.h \
	$(INC)/sys/var.h \
	$(INC)/sys/utssys.h \
	$(INC)/sys/ksym.h

install: all
	-rm -f $(ETC)/fuser
	 $(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) fuser
	-$(SYMLINK) /usr/sbin/fuser $(ETC)/fuser

clean:
	rm -f fuser.o

clobber: clean
	rm -f fuser

lintit:
	$(LINT) $(LINTFLAGS) fuser.c

#	These targets are useful but optional

partslist:
	@echo fuser.mk fuser.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRSBIN) | tr ' ' '\012' | sort

product:
	@echo fuser | tr ' ' '\012' | \
	sed 's;^;$(USRSBIN)/;'

srcaudit:
	@fileaudit fuser.mk $(LOCALINCS) fuser.c -o fuser.o fuser
