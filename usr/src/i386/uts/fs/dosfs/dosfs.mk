#
# 
#  Written for System V Release 4	(ESIX 4.0.4)	
#
#  Gerard van Dorth	(gdorth@nl.oracle.com)
#  Paul Bauwens	(paul@pphbau.atr.bso.nl)
#
#  May 1993
#
# This software is provided "as is".
#
# The author supplies this software to be publicly
# redistributed on the understanding that the author
# is not responsible for the correct functioning of
# this software in any circumstances and is not liable
# for any damages caused by this software.
#
#ident	"@(#)kern-i386:fs/dosfs/dosfs.mk	1.4.1.1"

include $(UTSRULES)

KBASE   = ../..
INSPERM = -m 644 -u $(OWN) -g $(GRP)
DOSFS      = dosfs.cf/Driver.o

LOCALDEF=-D_FSKI=1


SRCS = \
	dosfs_vfsops.c \
	dosfs_conv.c \
	dosfs_fat.c \
	dosfs_vnops.c \
	dosfs_lookup.c \
	dosfs_denode.c \
	dosfs_lbuf.c \
	dosfs_data.c \
	$(FRC)

OBJS = \
	dosfs_vfsops.o \
	dosfs_conv.o \
	dosfs_fat.o \
	dosfs_vnops.o \
	dosfs_lookup.o \
	dosfs_denode.o \
	dosfs_lbuf.o \
	dosfs_data.o \
	$(FRC)

all:	$(DOSFS)

install: all
	(cd dosfs.cf; $(IDINSTALL) -R$(CONF) -M dosfs)

$(DOSFS):	$(OBJS)
	$(LD) -r -o $(DOSFS) $(OBJS)

clean:
	-rm -f *.o $(DOSFS)

clobber:        clean
	-$(IDINSTALL) -R$(CONF) -d -e dosfs

FRC:

fnames:
	@for i in $(SRCS);	\
	do \
		echo $$i; \
	done

sysfsHeaders = \
	bootsect.h \
	bpb.h \
	denode.h \
	direntry.h \
	dosfs.h \
	dosfs_data.h \
	dosfs_filsys.h \
	dosfs_hier.h \
	dosfs_lbuf.h \
	fat.h

headinstall: $(sysfsHeaders)
	@-[ -d $(INC)/sys/fs ] || mkdir -p $(INC)/sys/fs
	@for f in $(sysfsHeaders); \
	 do \
	    $(INS) -f $(INC)/sys/fs -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)
#
#       Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)


