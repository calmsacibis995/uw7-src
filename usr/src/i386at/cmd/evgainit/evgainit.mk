#ident	"@(#)evgainit.mk	1.2"

#
# 	evgainit.mk:
# 	makefile for the evgainit command
#

include	$(CMDRULES)

LOCALDEF = -DEVGA -D_LTYPES

all:	evgainit

install:	all
		$(INS) -f $(SBIN) -m 0554 -u 0 -g 3 ./evgainit

clean:
	rm -f *.o

clobber: clean
	rm -f evgainit

evgainit.o:	evgainit.c \
	$(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/kd.h \
	$(FRC)
evgainit:	evgainit.o 
	$(CC) -o evgainit evgainit.o $(LDFLAGS) $(ROOTLIBS)
