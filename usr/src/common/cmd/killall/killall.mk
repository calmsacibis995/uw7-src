#	copyright	"%c%"

#ident	"@(#)killall:killall.mk	1.5.7.2"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for killall 

OWN = bin
GRP = bin

all: killall

killall: killall.o 
	$(CC) -o killall killall.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

killall.o: killall.c \
	$(INC)/sys/types.h \
	$(INC)/sys/procset.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/priv.h $(INC)/sys/privilege.h

install: all
	-rm -f $(ETC)/killall
	 $(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) killall
	-$(SYMLINK) /usr/sbin/killall $(ETC)/killall

clean:
	rm -f killall.o

clobber: clean
	rm -f killall

lintit:
	$(LINT) $(LINTFLAGS) killall.c

#	These targets are useful but optional

partslist:
	@echo killall.mk killall.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRSBIN) | tr ' ' '\012' | sort

product:
	@echo killall | tr ' ' '\012' | \
	sed 's;^;$(USRSBIN)/;'

srcaudit:
	@fileaudit killall.mk $(LOCALINCS) killall.c -o killall.o killall
