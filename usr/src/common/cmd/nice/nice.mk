#	copyright	"%c%"

#ident	"@(#)nice:nice.mk	1.4.4.2"
#ident "$Header$"

include $(CMDRULES)

#	nice make file

OWN = bin
GRP = bin

INSDIR = $(USRBIN)
SOURCE = nice.c
LDLIBS = -lm

all:	nice

nice: nice.o
	$(CC) -o nice nice.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

nice.o: $(SOURCE) \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/priv.h $(INC)/sys/privilege.h \
	$(INC)/mac.h $(INC)/sys/mac.h \
	$(INC)/sys/errno.h

install: all
	 $(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) nice

clean:
	rm -f nice.o

clobber: clean
	 rm -f nice

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCE)

