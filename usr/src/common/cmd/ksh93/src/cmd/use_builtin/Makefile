#ident	"@(#)ksh93:src/cmd/use_builtin/Makefile	1.1"

include $(CMDRULES)
OWN=bin
GRP=bin
INSDIR=$(USRBIN)

all:	use_builtin

use_builtin: use_builtin.sh
	cp use_builtin.sh  use_builtin

install:	all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) use_builtin

clean:

clobber:	clean
	rm -f use_builtin
