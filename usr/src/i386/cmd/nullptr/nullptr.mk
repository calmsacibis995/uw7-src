#ident	"@(#)nullptr:nullptr.mk	1.2"
#ident	"$Header$"

include $(CMDRULES)

OWN = bin
GRP = bin

all: nullptr

nullptr: nullptr.o
	$(CC) -o nullptr nullptr.o $(LDFLAGS) $(LDLIBS) $(SHLIBS) 

nullptr.o: nullptr.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/sysi86.h

install: all
	$(INS) -f $(USRBIN) -m 555 -u $(OWN) -g $(GRP) nullptr

clean:
	-rm -f nullptr.o

clobber: clean
	-rm -f nullptr

lintit:
	$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) nullptr.c
