#	copyright	"%c%"

#ident	"@(#)pt_chmod.mk	1.2"
#ident  "$Header$"

include $(CMDRULES)


OWN = root
GRP = bin

LDLIBS = -ladm
OBJECTS = pt_chmod.o

#
# Header dependencies
#

all: pt_chmod

pt_chmod: pt_chmod.o
	$(CC) pt_chmod.o -o pt_chmod $(LDFLAGS) $(LDLIBS) $(SHLIBS)

pt_chmod.o: pt_chmod.c \
	$(INC)/string.h \
	$(INC)/stdio.h \
	$(INC)/grp.h \
	$(INC)/unistd.h \
	$(INC)/errno.h \
	$(INC)/sys/mac.h \
	$(INC)/sys/types.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/stat.h

install: all
	$(INS) -f $(USRLIB) -m 04111 -u $(OWN) -g $(GRP) pt_chmod

clean:
	-rm -f $(OBJECTS)

clobber: clean
	-rm -f pt_chmod

lintit:
	$(LINT) $(LINTFLAGS) pt_chmod.c
