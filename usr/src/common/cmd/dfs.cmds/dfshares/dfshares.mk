#	copyright	"%c%"

#ident	"@(#)dfshares.mk	1.2"
#ident "$Header$"

include $(CMDRULES)

INSDIR = $(USRSBIN)
FRC =

all: dfshares

dfshares: dfshares.c 
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ dfshares.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 dfshares

clean:
	rm -f dfshares.o

clobber: clean
	rm -f dfshares
FRC:
