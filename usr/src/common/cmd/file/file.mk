#	copyright	"%c%"

#ident	"@(#)file:common/cmd/file/file.mk	1.11.1.2"
#ident "$Header$"
#

include $(CMDRULES)

INSDIR = $(USRBIN)
MINSDIR = $(ETC)
OWN = bin
GRP = bin

LDLIBS= -lcmd  -lw 

all:	file

file:	file.c
	$(CC) $(CFLAGS) $(DEFLIST) file.c -o $@ $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) file
	$(INS) -f $(MINSDIR) -m 0444 -u $(OWN) -g $(GRP) magic

clean:
	-rm -f file.o

clobber: clean
	-rm -f file
