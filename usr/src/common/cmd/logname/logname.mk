#	copyright	"%c%"

#ident	"@(#)logname:logname.mk	1.4.4.1"

include $(CMDRULES)

#	logname make file

OWN = bin
GRP = bin

SOURCES = logname.c

all: logname

logname: logname.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

logname.o: logname.c \
	$(INC)/stdio.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) logname

clean:
	rm -f logname.o

clobber: clean
	 rm -f logname

lintit:
	$(LINT) $(LINTFLAGS$(SOURCES)
