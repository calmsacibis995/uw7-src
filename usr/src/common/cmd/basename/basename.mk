#ident	"@(#)basename:basename.mk	1.3.6.1"
#ident	"$Header$"

include $(CMDRULES)
OWN=bin
GRP=bin
INSDIR=$(USRBIN)

all:	basename.sh
	cp basename.sh  basename

install:	all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) basename

clean:

clobber:	clean
	rm -f basename
