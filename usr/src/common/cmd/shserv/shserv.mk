#	copyright	"%c%"

#ident	"@(#)shserv.mk	1.3"
#ident "$Header$"

include $(CMDRULES)

OWN = root
GRP = bin

LDLIBS = -lcmd -lnsl -lgen -liaf 

all: shserv

shserv: shserv.o 
	$(CC) -o shserv shserv.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

shserv.o: shserv.c \
	$(INC)/stdio.h \
	$(INC)/iaf.h \
	$(INC)/unistd.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h \
	$(INC)/sys/secsys.h \
	$(INC)/priv.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h

install: shserv 
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) shserv

clean:
	rm -f shserv.o
	
clobber: clean
	rm -f shserv

lintit:
	$(LINT) $(LINTFLAGS) shserv.c

# optional targets

save:
	cd $(USRBIN); set -x; for m in shserv; do cp $$m OLD$$m; done

restore:
	cd $(USRBIN); set -x; for m in shserv; do; cp OLD$$m $$m; done

remove:
	cd $(USRBIN); rm -f shserv

partslist:
	@echo shserv.mk $(LOCALINCS) shserv.c | tr ' ' '\012' | sort

product:
	@echo shserv | tr ' ' '\012' | \
	sed -e 's;^;$(USRBIN)/;' -e 's;//*;/;g'

productdir:
	@echo $(USRBIN)

srcaudit: # will not report missing nor present object or product files.
	@fileaudit shserv.mk $(LOCALINCS) shserv.c -o shserv.o shserv
