#	copyright	"%c%"

#ident	"@(#)users:users.mk	1.12.8.4"
#ident  "$Header$"

include $(CMDRULES)

LDLIBS = -liaf -lgen

OWN = bin
GRP = bin

all: listusers

listusers : users.o 
	$(CC) -o listusers users.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

users.o: users.c \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/grp.h \
	$(INC)/pwd.h \
	$(INC)/varargs.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/sys/time.h \
	$(INC)/mac.h \
	$(INC)/priv.h \
	$(INC)/ia.h

install: all 
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) listusers

clean :
	rm -f users.o

clobber : clean
	rm -f listusers

lintit :
	$(LINT) $(LINTFLAGS) users.c
