#	copyright	"%c%"

#ident	"@(#)su.mk	1.5"
#ident  "$Header$"

include $(CMDRULES)

#	Copyright (c) 1987, 1988 Microsoft Corporation	
#	  All Rights Reserved	
#	This Module contains Proprietary Information of Microsoft  
#	Corporation and should be treated as Confidential.	   
#	su make file

OWN = root
GRP = sys
MAINS = su su.dy

LDLIBS = -lcrypt -lcmd -liaf -lgen

all: $(MAINS)

su: su.o
	$(CC) -o $@ su.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS) 

su.dy: su.o
	$(CC) -o $@ su.o $(LDFLAGS) $(LDLIBS) $(SHLIBS) -DNIS

su.o: su.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/crypt.h \
	$(INC)/time.h \
	$(INC)/signal.h \
	$(INC)/fcntl.h \
	$(INC)/string.h \
	$(INC)/ia.h \
	$(INC)/mac.h \
	$(INC)/deflt.h \
	$(INC)/libgen.h \
	$(INC)/errno.h \
	$(INC)/priv.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/sys/secsys.h \
	$(INC)/dlfcn.h

install: all
	-rm -f $(USRBIN)/su
	$(INS) -f $(SBIN) -m 04555 -u $(OWN) -g $(GRP) su
	$(INS) -f $(USRBIN) -m 04555 -u $(OWN) -g $(GRP) su.dy
	-/bin/mv $(USRBIN)/su.dy $(USRBIN)/su
	-mkdir ./tmp
	-$(CP) su.dfl ./tmp/su
	$(INS) -f $(ETC)/default -m 0444 -u $(OWN) -g $(GRP) ./tmp/su
	-rm -rf ./tmp

clean:
	rm -f su.o

clobber: clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) su.c
