#ident	"@(#)ca:ca/ca.mk	1.2"

include $(CMDRULES)

MAKEFILE = ca.mk

BINS = ca

all:	$(BINS)

clean:
	rm -f *.o ca 


ca: ca.o
	$(CC) -O -s -o $@ ca.o

ca.o: ca.c \
	$(INC)/sys/ca.h \
	$(INC)/sys/nvm.h

install: $(BINS)
	$(INS) -f $(SBIN) -m 0555 -u bin -g bin ca
