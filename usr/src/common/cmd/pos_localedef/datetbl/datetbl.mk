#	copyright	"%c%"

#ident	"@(#)pos_localedef:common/cmd/pos_localedef/datetbl/datetbl.mk	1.1.8.2"

include $(CMDRULES)

#	Makefile for datetbl

OWN = bin
GRP = bin

all: $(MAINS)

install: all
	$(INS) -f $(USRLIB)/locale/C -m 0555 -u $(OWN) -g $(GRP) time_C
	$(INS) -f $(USRLIB)/locale/C LC_TIME
	$(INS) -f $(USRLIB)/locale/POSIX -m 0555 -u $(OWN) -g $(GRP) time_POSIX
	$(INS) -f $(USRLIB)/locale/POSIX LC_TIME
