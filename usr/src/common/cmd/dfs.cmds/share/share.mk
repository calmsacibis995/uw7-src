#	copyright	"%c%"

#ident	"@(#)share.mk	1.2"
#ident "$Header$"

include $(CMDRULES)

INSDIR = $(USRSBIN)
FRC =

all: share

share: share.c 
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ share.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 share

clean:
	rm -f share.o

clobber: clean
	rm -f share
FRC:
