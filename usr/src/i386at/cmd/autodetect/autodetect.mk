#ident	"@(#)autodetect:autodetect.mk	1.3"
#
# autodetect.mk:
# makefile for autodetect command used only in 4.2MP MP packages installation
#

include $(CMDRULES)
OWN=root
GRP=sys
INCSYS=$(INC)

all:	autodetect

install:	all
		$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) autodetect

autodetect:	autodetect.o
		$(CC) -o autodetect autodetect.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

clean:
	-rm -f *.o

clobber: clean
	-rm -f autodetect


autodetect.o:	autodetect.c \
		$(INC)/fcntl.h \
		$(INC)/stdlib.h \
		$(INC)/string.h
		$(CC) $(CFLAGS) $(DEFLIST) -c autodetect.c 
