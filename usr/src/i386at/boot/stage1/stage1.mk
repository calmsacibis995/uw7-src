#ident	"@(#)stand:i386at/boot/stage1/stage1.mk	1.3"
#ident	"$Header$"

include $(CMDRULES)

MAKEFILE = stage1.mk

INSDIR = $(ROOT)/$(MACH)/etc/.boot
MKLDIMG = ../tools/mkldimg

TARGS = fdboot hdboot

OPTFLAGS = -O -Ki486 -Kno_lu -Kno_inline -W0,-Lb
LOCALCFLAGS = $(OPTFLAGS) -Kno_host
LOCALINC = -I../h


all:	$(TARGS)


fdboot:	fdboot.elf
	$(MKLDIMG) -r512 fdboot.elf fdboot

fdboot.elf: _fdboot.o fdmap
	$(LD) -o fdboot.elf -dn -M fdmap _fdboot.o

_fdboot.s: fdboot.s prot.s bootcmn.s
	cat fdboot.s prot.s bootcmn.s >_fdboot.s


hdboot:	hdboot.elf
	$(MKLDIMG) -r512 hdboot.elf hdboot

hdboot.elf: _hdboot.o bootcmn.o partnum.o hdmap
	$(LD) -o hdboot.elf -dn -M hdmap _hdboot.o bootcmn.o partnum.o

_hdboot.s: hdboot.s prot.s diskmark.s
	cat hdboot.s prot.s diskmark.s >_hdboot.s


install: all
	@[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	for f in $(TARGS); \
	do $(INS) -f $(INSDIR) -m 644 -u $(OWN) -g $(GRP) $$f; \
	done

clean:
	rm -f *.o *.elf _fdboot.s _hdboot.s

clobber: clean
	rm -f $(TARGS)

depend:
