#	copyright	"%c%"

#ident	"@(#)mvdir:mvdir.mk	1.5.4.2"
#ident "$Header$"

include $(CMDRULES)


OWN = root
GRP = bin

all: mvdir.sh
	cp mvdir.sh mvdir

install: all
	-rm -f $(ETC)/mvdir
	 $(INS) -f $(USRSBIN) -m 0544 -u $(OWN) -g $(GRP) mvdir
	-$(SYMLINK) /usr/sbin/mvdir $(ETC)/mvdir

clean:
	rm -f mvdir

clobber: clean

