#	copyright	"%c%"

#ident	"@(#)pm_cmds.mk	1.3"

include $(CMDRULES)


OWN = sys
GRP = priv

TOOLSDIR = $(ETC)/security/tools
DEFDIR = $(ROOT)/$(MACH)/etc/default
LDLIBS = -lcmd

MAINS = filepriv initprivs priv_upd

OBJECTS = filepriv.o initprivs.o priv_upd.o

SOURCES = $(OBJECTS:.o=.c)

all: $(MAINS)

filepriv: filepriv.o
	$(CC) -o $@ filepriv.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

initprivs: initprivs.o
	$(CC) -o $@ initprivs.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

priv_upd: priv_upd.o
	$(CC) -o $@ priv_upd.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

filepriv.o: filepriv.c \
	$(INC)/libcmd.h \
	$(INC)/pfmt.h \
	$(INC)/priv.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/limits.h \
	$(INC)/locale.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/unistd.h \
	$(INC)/sys/mac.h \
	$(INC)/sys/time.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/secsys.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/resource.h

initprivs.o: initprivs.c \
	$(INC)/libcmd.h \
	$(INC)/pfmt.h \
	$(INC)/priv.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/limits.h \
	$(INC)/locale.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/unistd.h \
	$(INC)/sys/mac.h \
	$(INC)/sys/time.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/secsys.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/resource.h

priv_upd.o: priv_upd.c \
	$(INC)/libcmd.h 

install: all
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) filepriv
	$(INS) -f $(SBIN) -m 0550 -u $(OWN) -g $(GRP) initprivs
	-mkdir ./tmp
	-$(CP) privcmds.dfl ./tmp/privcmds
	$(INS) -f $(DEFDIR) -m 0444 -u root -g sys ./tmp/privcmds
	-rm -rf ./tmp
	- [ -d $(TOOLSDIR) ] || mkdir -p $(TOOLSDIR)
	$(INS) -f $(TOOLSDIR) -m 0550 -u $(OWN) -g $(GRP) priv_upd
	$(INS) -f $(TOOLSDIR) -m 0550 -u $(OWN) -g $(GRP) setpriv

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo  | tr ' ' '\012' | sort

product:
	@echo $(MAINS) | tr ' ' '\012' | \
	sed 's;^;/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)
