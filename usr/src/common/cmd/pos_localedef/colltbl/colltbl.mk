#	copyright	"%c%"

#ident	"@(#)pos_localedef:common/cmd/pos_localedef/colltbl/colltbl.mk	1.1.9.3"

include $(CMDRULES)

#	Makefile for colltbl

OWN = bin
GRP = bin

LDLIBS = -ly
YFLAGS = -d

OBJECTS = collfcns.o colltbl.o diag.o parse.o lex.o
SOURCES = $(OBJECTS:.o=.c)

.MUTEX: y.tab.h parse.c

all: colltbl

colltbl: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

collfcns.o: collfcns.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/stdlib.h \
	$(INC)/stddef.h \
	colltbl.h \
	$(INC)/regexp.h

colltbl.o: colltbl.c \
	$(INC)/stdio.h \
	colltbl.h

diag.o: diag.c \
	$(INC)/stdio.h \
	$(INC)/varargs.h \
	colltbl.h

lex.o: lex.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	colltbl.h \
	y.tab.h

parse.o: parse.c \
	colltbl.h \
	$(INC)/malloc.h \
	$(INC)/memory.h \
	$(INC)/values.h

parse.c y.tab.h: parse.y
	$(YACC) $(YFLAGS) parse.y
	mv y.tab.c parse.c

install: all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) colltbl ;\
	$(INS) -f $(USRLIB)/locale/C -m 0555 -u $(OWN) -g $(GRP) colltbl_C ;\
	$(CH)./colltbl colltbl_C ;\
	$(CH)$(INS) -f $(USRLIB)/locale/C LC_COLLATE ;\
	$(CH)rm -f LC_COLLATE
	$(INS) -f $(USRLIB)/locale/POSIX -m 0555 -u $(OWN) -g $(GRP) colltbl_POSIX ;\
	$(CH)./colltbl colltbl_POSIX ;\
	$(CH)$(INS) -f $(USRLIB)/locale/POSIX LC_COLLATE ;\
	$(CH)rm -f LC_COLLATE

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f colltbl y.tab.h parse.c

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

#	These targets are useful but optional

partslist:
	@echo colltbl.mk $(SOURCES) $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo  | tr ' ' '\012' | sort

product:
	@echo colltbl | tr ' ' '\012' | \
	sed 's;^;/;'

srcaudit:
	@fileaudit colltbl.mk $(LOCALINCS) $(SOURCES) -o $(OBJECTS) colltbl
