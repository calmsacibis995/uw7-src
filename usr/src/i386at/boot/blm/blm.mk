#ident	"@(#)stand:i386at/boot/blm/blm.mk	1.1"
#ident	"$Header$"

include $(CMDRULES)

MAKEFILE = blm.mk

INSDIR = $(ROOT)/$(MACH)/etc/.boot

TARGS = platform.blm bfs.blm smallfs.blm dcmp.blm hd.blm

OPTFLAGS = -O -Ki486 -Kno_lu -Kno_inline -W0,-Lb
LOCALCFLAGS = $(OPTFLAGS) -Kno_host
LOCALDEF = -D_BLM -DNO_STDLIB_H -DMEMDEBUG
LOCALINC = -I. -Izip -I../h


all:	$(TARGS)

.c.o:
	$(CC) -c $(LOCALCFLAGS) $(INCLIST) $(DEFLIST) $<


PLAT_OFILES = platform.o bioscall.o video.o memsizer.o bios.o rawboot.o timer.o
platform.blm: $(PLAT_OFILES)
	$(LD) -r -o $@ $(PLAT_OFILES)
	$(MCS) -d $@

bfs.blm: bfs.o
	$(LD) -r -o $@ bfs.o
	$(MCS) -d $@
bfs.o: ../stage2/bfs.c
	$(CC) $(LOCALCFLAGS) $(DEFLIST) $(INCLIST) -c ../stage2/bfs.c

smallfs.blm: smallfs.o
	$(LD) -r -o $@ smallfs.o
	$(MCS) -d $@
smallfs.o: ../stage2/smallfs.c
	$(CC) $(LOCALCFLAGS) $(DEFLIST) $(INCLIST) -c ../stage2/smallfs.c

dcmp.blm: dcmp.o dcmp_zip.o inflate.o
	$(LD) -r -o $@ dcmp.o dcmp_zip.o inflate.o
	$(MCS) -d $@
dcmp_zip.o: zip/dcmp_zip.c
	$(CC) $(LOCALCFLAGS) $(DEFLIST) $(INCLIST) -c zip/dcmp_zip.c
inflate.o: zip/inflate.c
	$(CC) $(LOCALCFLAGS) $(DEFLIST) $(INCLIST) -c zip/inflate.c

hd.blm: hd.o
	$(LD) -r -o $@ hd.o
	$(MCS) -d $@

install: all
	@[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	for f in $(TARGS); \
	do $(INS) -f $(INSDIR) -m 644 -u $(OWN) -g $(GRP) $$f; \
	done

clean:
	rm -f *.o

clobber: clean
	rm -f $(TARGS)

depend:
