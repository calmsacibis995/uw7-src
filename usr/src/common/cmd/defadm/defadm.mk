#	copyright	"%c%"

#ident	"@(#)defadm:defadm.mk	1.1.1.3"

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = root
GRP = sys

LDLIBS = -lcmd

MAKEFILE = defadm.mk

MAINS = defadm defadm.dy

OBJECTS =  defadm.o

SOURCES =  defadm.c

all:		$(MAINS)

defadm:		defadm.o	
	$(CC) -o $@ defadm.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

defadm.dy:		defadm.o	
	$(CC) -o $@ defadm.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

defadm.o:	 $(INC)/sys/types.h $(INC)/deflt.h	\
		 $(INC)/dirent.h $(INC)/fcntl.h	\
		 $(INC)/errno.h	$(INC)/string.h \
		 $(INC)/unistd.h $(INC)/stdio.h  \
		 $(INC)/sys/stat.h $(INC)/sys/param.h \
		 $(INC)/signal.h 

clean:
	rm -f $(OBJECTS)
	
clobber: clean
	rm -f $(MAINS)

install:	all $(INSDIR)
	$(INS) -f $(SBIN) -m 0500 -u $(OWN) -g $(GRP) defadm
	$(INS) -f $(INSDIR) -m 0500 -u $(OWN) -g $(GRP) defadm.dy

strip:
	$(STRIP) $(MAINS)

remove:
	cd $(INSDIR);  rm -f $(MAINS)

$(INSDIR):
	mkdir $(INSDIR);  $(CH)chmod 755 $(INSDIR);  $(CH)chown bin $(INSDIR)

partslist:
	@echo $(MAKEFILE) $(LOCALINCS) $(SOURCES)  |  tr ' ' '\012'  |  sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
		sed -e 's;^;$(INSDIR)/;' -e 's;//*;/;g'

productdir:
	@echo $(INSDIR)
