#	copyright	"%c%"

#ident	"@(#)cpset:i386/cmd/cpset/cpset.mk	1.1.5.2"
#ident  "$Header$"

include $(CMDRULES)

# Makefile for cpset
# to install when not privileged
# set $(CH) in the environment to #

OWN = bin
GRP = bin

all: cpset

cpset: cpset.o
	$(CC) -o cpset cpset.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

cpset.o: cpset.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/sys/errno.h \
	$(INC)/string.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) cpset

clean:
	rm -f cpset.o

clobber: clean
	rm -f cpset

lintit: 
	$(LINT) $(LINTFLAGS) cpset.c
