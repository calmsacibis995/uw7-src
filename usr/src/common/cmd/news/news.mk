#	copyright	"%c%"

#ident	"@(#)news:news.mk	1.6.6.2"
#ident "$Header$"

include $(CMDRULES)

#	Makefile for news

OWN = bin
GRP = bin

all: news

news: news.o 
	$(CC) -o news news.o  $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

news.o: news.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/stdlib.h \
	$(INC)/unistd.h \
	$(INC)/sys/stat.h \
	$(INC)/setjmp.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/dirent.h \
	$(INC)/pwd.h \
	$(INC)/time.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h \
	$(INC)/utime.h

install: all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) news

clean:
	rm -f news.o

clobber: clean
	rm -f news

lintit:
	$(LINT) $(LINTFLAGS) news.c

#	These targets are useful but optional

partslist:
	@echo news.mk news.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo news | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit news.mk $(LOCALINCS) news.c -o news.o news
