#ident	"@(#)dfsck:dfsck.mk	1.4.3.2"

include	$(CMDRULES)

MAINS	= dfsck
OBJECTS = dfsck.o

ALL:		$(MAINS)

$(MAINS):	dfsck.o
		$(CC) -o dfsck dfsck.o $(LDFLAGS)

dfsck.o:	 $(INC)/stdio.h \
		 $(INC)/fcntl.h \
		 $(INC)/signal.h \
		 $(INC)/sys/signal.h \
		 $(INC)/errno.h \
		 $(INC)/sys/errno.h \
		 $(INC)/sys/types.h \
		 $(INC)/sys/stat.h 

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all : ALL

install: ALL
	$(INS) -f $(USRSBIN) -m 0555 -u bin -g bin $(MAINS)
