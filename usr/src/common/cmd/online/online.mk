#ident	"@(#)mp.cmds:common/cmd/online/online.mk	1.2"
#ident	"$Header$"

include $(CMDRULES)
OWN=root
GRP=sys
INSDIR=$(USRBIN)

all:	online.sh offline.sh
	cp online.sh  online
	cp offline.sh offline

install:	all
	-rm -f $(ETC)/online $(ETC)/offline
	-rm -f $(USRSBIN)/online $(USRBIN)/offline
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) online
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) offline
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) online
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) offline
	-$(SYMLINK) /sbin/online $(ETC)/online
	-$(SYMLINK) /sbin/offline $(ETC)/offline

clean:

clobber:	clean
	rm -f online offline

