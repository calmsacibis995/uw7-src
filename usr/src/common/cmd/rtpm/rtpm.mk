#ident	"@(#)rtpm:rtpm.mk	1.5"

include $(CMDRULES)

OWN = bin
GRP = bin

# how to use this makefile
# to make sure all files  are up to date: $(MAKE) -f rtmp.mk all
#
# to force recompilation of all files: $(MAKE) -f rtpm.mk all FRC=FRC
#
# rtpm must be able to read /dev/kmem,
# which standardly has restricted read permission.
# It must have set-group-ID mode
# and have the same group as /dev/kmem.
# The chmod and chgrp commmands below ensure this.
#
LOCALDEF = -D_KMEMUSER
LDLIBS = -lw -lgen -lmas -lnwutil -lnsl -lcurses
INSDIR = $(USRSBIN)
INSLIB = $(USRLIB)/locale/C/MSGFILES

MAINS = rtpm

OBJECTS = rtpm.o ether.o inet.o nswap.o proc.o mtbl.o input.o output.o metcook.o color.o netware.o

all: $(MAINS)

rtpm: $(OBJECTS)
	$(CC) -o rtpm $(OBJECTS) netware.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

dirs:
	[ -d $(USRSBIN) ] || mkdir -p $(USRSBIN)
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)

install: all dirs
	-rm -f $(USRSBIN)/rtpm
	$(INS) -o -f $(INSDIR) -m 02555 -u $(OWN) -g sys rtpm
	$(SYMLINK) /usr/sbin/rtpm $(USRBIN)/rtpm
	$(INS) -f $(INSLIB) -m 0644 -u $(OWN) -g sys RTPM.str
	$(INS) -f $(ETC) -m 0644 -u $(OWN) -g sys .rtpmrc

clean: 
	-rm -f *.o

clobber: clean
	-rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) rtpm.c ether.c inet.c nswap.c proc.c mtbl.c input.c output.c metcook.c color.c netware.c
