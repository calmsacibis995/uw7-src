#	copyright	"%c%"

#ident	"@(#)creatiadb.mk	1.3"
#ident "$Header$"

include $(CMDRULES)

OWN = root
GRP = sys

#	Common Libraries and -l<lib> flags.
LDLIBS = -lia -liaf -lgen 

all: creatiadb

creatiadb: creatiadb.o 
	$(CC) -o creatiadb creatiadb.o $(LDFLAGS) $(LDLIBS) $(SHLIBS) $(ROOTLIBS)

creatiadb.o: creatiadb.c \
	$(INC)/stdio.h \
	$(INC)/pwd.h \
	$(INC)/shadow.h \
	$(INC)/grp.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/time.h \
	$(INC)/sys/time.h \
	$(INC)/sys/mac.h \
	$(INC)/audit.h \
	$(INC)/ia.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/stat.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h \
	$(INC)/stdlib.h \
	$(INC)/unistd.h \
	$(INC)/priv.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h

clean:
	rm -f creatiadb.o
	
clobber: clean
	rm -f creatiadb

lintit:
	$(LINT) $(LINTFLAGS) creatiadb.c

install: creatiadb
	 $(INS) -f $(SBIN) -m 0500 -u $(OWN) -g $(GRP) creatiadb

remove:
	cd $(SBIN); rm -f creatiadb


partslist:
	@echo creatiadb.mk $(LOCALINCS) creatiadb.c | tr ' ' '\012' | sort

product:
	@echo creatiadb | tr ' ' '\012' | \
	sed -e 's;^;$(SBIN)/;' -e 's;//*;/;g'

productdir:
	@echo $(SBIN)
