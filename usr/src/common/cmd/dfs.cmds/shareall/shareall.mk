#	copyright	"%c%"

#ident	"@(#)shareall.mk	1.2"
#ident "$Header$"

include $(CMDRULES)

INSDIR = $(USRSBIN)
OWN = bin
GRP = bin
FRC =

all: shareall

shareall: shareall.sh 
	cp shareall.sh shareall
	chmod 555 shareall

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) shareall

clean:

clobber: clean
	rm -f shareall
FRC:
