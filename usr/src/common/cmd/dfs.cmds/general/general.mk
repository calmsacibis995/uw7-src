#	copyright	"%c%"

#ident	"@(#)general.mk	1.2"
#ident "$Header$"

include $(CMDRULES)

INSDIR = $(USRSBIN)
FRC =

all: general

general: general.c 
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ general.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 general

clean:
	rm -f general.o

clobber: clean
	rm -f general
FRC:
