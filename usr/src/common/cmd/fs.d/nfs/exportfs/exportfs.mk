#ident	"@(#)exportfs.mk	1.2"
#ident	"$Header$"

include $(CMDRULES)
INSDIR = $(USRSBIN)
OWN = bin
GRP = bin

all: exportfs.sh
	cp exportfs.sh exportfs && \
	chmod 0755 exportfs

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) exportfs

lintit:

tags:

clean:

clobber: clean
	-rm -f exportfs
