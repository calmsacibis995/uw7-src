#	copyright	"%c%"

#ident	"@(#)getclk:getclk.mk	1.1.3.3"
#ident  "$Header$"

include $(CMDRULES)

OWN = root
GRP = sys

all : getclk

getclk: getclk.o 
	$(CC) -o getclk getclk.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

getclk.o: getclk.c \
	$(INC)/stdio.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/sys/rtc.h

install: all
	$(INS) -f $(USRSBIN) -m 0744 -u $(OWN) -g $(GRP) getclk

clean:
	rm -f getclk.o

clobber: clean
	rm -f getclk

lintit:
	$(LINT) $(LINTFLAGS) getclk.c
