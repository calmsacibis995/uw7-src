#	copyright	"%c%"

#ident	"@(#)more:more.mk	1.1.1.1"
#ident "$Header$"

#
# COPYRIGHT NOTICE
# 
# This source code is designated as Restricted Confidential Information
# and is subject to special restrictions in a confidential disclosure
# agreement between HP, IBM, SUN, NOVELL and OSF.  Do not distribute
# this source code outside your company without OSF's specific written
# approval.  This source code, and all copies and derivative works
# thereof, must be returned or destroyed at request. You must retain
# this notice on any copies which you make.
# 
# This source code is designated as Restricted Confidential Information
# and is subject to special restrictions in a confidential disclosure
# agreement between HP, IBM, SUN, NOVELL and OSF.  Do not distribute
# this source code outside your company without OSF's specific written
# approval.  This source code, and all copies and derivative works
# thereof, must be returned or destroyed at request. You must retain
# this notice on any copies which you make.
# 
# ALL RIGHTS RESERVED 
#
#
# OSF/1 1.2
#


include $(CMDRULES)

#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved
#	This Module contains Proprietary Information of Microsoft 
#	Corporation and should be treated as Confidential.

INSDIR=$(ROOT)/$(MACH)/usr/bin
LIBDIR=$(ROOT)/$(MACH)/usr/lib

OWN = bin
GRP = bin

.SUFFIXES: .ln .c

LDLIBS = -lcurses

OFILES = ch.o command.o decode.o help.o input.o line.o linenum.o main.o \
	option.o os.o output.o position.o prim.o screen.o signal.o tags.o \
	ttyin.o

LNFILES = ${OFILES:.o=.ln}

DATA = more.help

all: more

more: $(OFILES)
	$(CC) -o more $(OFILES) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: more 
	$(INS) -f $(INSDIR) -m 555 -u $(OWN) -g $(GRP) more
	$(INS) -f $(LIBDIR) -m 0644 -u $(OWN) -g $(GRP) $(DATA)

clean:
	rm -f $(OFILES) $(LNFILES)
	
clobber: clean
	rm -f more

lintit: $(LNFILES)
	$(LINT) $(LINTFLAGS) $(LNFILES) 

#	optional targets

save:
	cd $(INSDIR); set -x; for m in more; do cp $$m OLD$$m; done
	cd $(LIBDIR); set -x; for m in $(DATA); do cp $$m OLD$$m; done

restore:
	cd $(INSDIR); set -x; for m in more; do; cp OLD$$m $$m; done
	cd $(LIBDIR); set -x; for m in $(DATA); do; cp OLD$$m $$m; done

remove:
	cd $(INSDIR); rm -f more
	cd $(LIBDIR); rm -f $(DATA)

partslist:
	@echo more.mk $(LOCALINCS) more.c | tr ' ' '\012' | sort

product:
	@echo more | tr ' ' '\012' | \
	sed -e 's;^;$(INSDIR)/;' -e 's;//*;/;g'

productdir:
	@echo $(INSDIR)

.c.ln:
	$(LINT) $(LINTFLAGS) -c $<

# DO NOT DELETE THIS LINE -- make depend depends on it.

ch.o: $(INC)/stdio.h $(INC)/stdlib.h $(INC)/unistd.h
ch.o: $(INC)/sys/types.h $(INC)/sys/unistd.h less.h
ch.o: $(INC)/nl_types.h
command.o: $(INC)/ctype.h $(INC)/stdio.h $(INC)/stdlib.h
command.o: $(INC)/sys/param.h $(INC)/sys/types.h
command.o: $(INC)/sys/param_p.h less.h $(INC)/nl_types.h
decode.o: less.h $(INC)/nl_types.h $(INC)/sys/types.h
help.o: $(INC)/fcntl.h $(INC)/sys/types.h
help.o: $(INC)/sys/fcntl.h $(INC)/stdio.h $(INC)/unistd.h
help.o: $(INC)/sys/unistd.h $(INC)/sys/param.h
help.o: $(INC)/sys/param_p.h less.h $(INC)/nl_types.h
input.o: less.h $(INC)/nl_types.h $(INC)/sys/types.h
less.o: $(INC)/nl_types.h $(INC)/sys/types.h
line.o: $(INC)/ctype.h $(INC)/locale.h $(INC)/sys/types.h
line.o: $(INC)/time.h less.h $(INC)/nl_types.h
linenum.o: less.h $(INC)/nl_types.h $(INC)/sys/types.h
main.o: $(INC)/string.h $(INC)/locale.h $(INC)/stdlib.h
main.o: $(INC)/limits.h $(INC)/fcntl.h $(INC)/sys/types.h
main.o: $(INC)/sys/fcntl.h $(INC)/errno.h
main.o: $(INC)/sys/errno.h $(INC)/unistd.h
main.o: $(INC)/sys/unistd.h $(INC)/stdio.h less.h
main.o: $(INC)/nl_types.h
option.o: $(INC)/stdio.h $(INC)/ctype.h $(INC)/stdlib.h
option.o: $(INC)/errno.h $(INC)/sys/errno.h less.h
option.o: $(INC)/nl_types.h $(INC)/sys/types.h
os.o: $(INC)/errno.h $(INC)/sys/errno.h $(INC)/fcntl.h
os.o: $(INC)/sys/types.h $(INC)/sys/fcntl.h $(INC)/signal.h
os.o: $(INC)/sys/signal.h $(INC)/sys/bitmasks.h
os.o: $(INC)/sys/siginfo.h $(INC)/sys/ksynch.h
os.o: $(INC)/sys/dl.h $(INC)/sys/ipl.h $(INC)/sys/disp_p.h
os.o: $(INC)/sys/trap.h $(INC)/sys/ksynch_p.h
os.o: $(INC)/sys/list.h $(INC)/sys/listasm.h $(INC)/stdio.h
os.o: $(INC)/stdlib.h $(INC)/setjmp.h $(INC)/unistd.h
os.o: $(INC)/sys/unistd.h $(INC)/sys/param.h
os.o: $(INC)/sys/param_p.h $(INC)/sys/stat.h
os.o: $(INC)/sys/time.h less.h $(INC)/nl_types.h
output.o: $(INC)/ctype.h $(INC)/errno.h $(INC)/sys/errno.h
output.o: $(INC)/limits.h $(INC)/unistd.h
output.o: $(INC)/sys/types.h $(INC)/sys/unistd.h
output.o: $(INC)/stdio.h less.h $(INC)/nl_types.h
output.o: $(INC)/wchar.h $(INC)/wctype.h
position.o: less.h $(INC)/nl_types.h $(INC)/sys/types.h
prim.o: $(INC)/regex.h $(INC)/stdio.h less.h
prim.o: $(INC)/nl_types.h $(INC)/sys/types.h
screen.o: $(INC)/termios.h $(INC)/sys/termios.h
screen.o: $(INC)/sys/ttydev.h $(INC)/sys/types.h
screen.o: $(INC)/termio.h $(INC)/sys/termio.h
screen.o: $(INC)/unistd.h $(INC)/sys/unistd.h
screen.o: $(INC)/sys/ioctl.h $(INC)/stdlib.h less.h
screen.o: $(INC)/nl_types.h
signal.o: $(INC)/signal.h $(INC)/sys/signal.h
signal.o: $(INC)/sys/types.h $(INC)/sys/bitmasks.h
signal.o: $(INC)/sys/siginfo.h $(INC)/sys/ksynch.h
signal.o: $(INC)/sys/dl.h $(INC)/sys/ipl.h
signal.o: $(INC)/sys/disp_p.h $(INC)/sys/trap.h
signal.o: $(INC)/sys/ksynch_p.h $(INC)/sys/list.h
signal.o: $(INC)/sys/listasm.h $(INC)/unistd.h
signal.o: $(INC)/sys/unistd.h less.h $(INC)/nl_types.h
tags.o: $(INC)/stdio.h less.h $(INC)/nl_types.h
tags.o: $(INC)/sys/types.h
ttyin.o: less.h $(INC)/nl_types.h $(INC)/sys/types.h
