#	copyright	"%c%"

#ident	"@(#)unshareall.mk	1.2"
#ident "$Header$"

include $(CMDRULES)

INSDIR = $(USRSBIN)
OWN = bin
GRP = bin
FRC =

all: unshareall

unshareall: unshareall.sh 
	cp unshareall.sh unshareall
	chmod 555 unshareall

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) unshareall

clean:

clobber: clean
	rm -f unshareall
FRC:
