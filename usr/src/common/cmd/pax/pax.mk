
#ident	"@(#)pax:pax.mk	1.2.1.1"

include $(CMDRULES)

#	Makefile for pax

OWN = bin
GRP = bin

LOCAL_LDLIBS = $(LDLIBS)

all: pax

OBJECTS = pax.o append.o buffer.o charmap.o cpio.o \
	create.o extract.o fileio.o \
	hash.o link.o list.o mem.o namelist.o names.o \
	pass.o pathname.o replace.o tar.o \
	ttyio.o warn.o

pax: pax.o append.o buffer.o charmap.o cpio.o \
	create.o extract.o fileio.o \
	hash.o link.o list.o mem.o namelist.o names.o \
	pass.o pathname.o replace.o tar.o \
	ttyio.o warn.o
	$(CC) $(CFLAGS) -o pax $(OBJECTS) $(LDFLAGS) $(LOCAL_LDLIBS) $(ROOTLIBS)


install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) pax

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f pax


append.o: $(INC)/sys/types.h $(INC)/sys/ioctl.h
append.o: $(INC)/locale.h config.h pax.h $(INC)/limits.h
append.o: $(INC)/regex.h $(INC)/stdio.h $(INC)/sys/stat.h
append.o: $(INC)/sys/time.h $(INC)/sys/errno.h
append.o: $(INC)/fcntl.h $(INC)/sys/fcntl.h $(INC)/unistd.h
append.o: $(INC)/sys/unistd.h $(INC)/stdlib.h
append.o: $(INC)/string.h $(INC)/pfmt.h charmap.h func.h
append.o: pax_msgids.h
buffer.o: $(INC)/sys/types.h $(INC)/sys/stat.h
buffer.o: $(INC)/sys/time.h $(INC)/sys/uio.h
buffer.o: $(INC)/sys/utime.h $(INC)/locale.h config.h pax.h
buffer.o: $(INC)/limits.h $(INC)/regex.h $(INC)/stdio.h
buffer.o: $(INC)/sys/errno.h $(INC)/fcntl.h
buffer.o: $(INC)/sys/fcntl.h $(INC)/unistd.h
buffer.o: $(INC)/sys/unistd.h $(INC)/stdlib.h
buffer.o: $(INC)/string.h $(INC)/pfmt.h charmap.h func.h
buffer.o: pax_msgids.h
charmap.o: config.h pax.h $(INC)/limits.h $(INC)/regex.h
charmap.o: $(INC)/stdio.h $(INC)/sys/stat.h
charmap.o: $(INC)/sys/types.h $(INC)/sys/time.h
charmap.o: $(INC)/sys/errno.h $(INC)/fcntl.h
charmap.o: $(INC)/sys/fcntl.h $(INC)/unistd.h
charmap.o: $(INC)/sys/unistd.h $(INC)/stdlib.h
charmap.o: $(INC)/string.h $(INC)/pfmt.h charmap.h func.h
charmap.o: pax_msgids.h
cpio.o: config.h pax.h $(INC)/limits.h $(INC)/regex.h
cpio.o: $(INC)/stdio.h $(INC)/sys/stat.h $(INC)/sys/types.h
cpio.o: $(INC)/sys/time.h $(INC)/sys/errno.h $(INC)/fcntl.h
cpio.o: $(INC)/sys/fcntl.h $(INC)/unistd.h
cpio.o: $(INC)/sys/unistd.h $(INC)/stdlib.h $(INC)/string.h
cpio.o: $(INC)/pfmt.h charmap.h func.h pax_msgids.h
create.o: config.h pax.h $(INC)/limits.h $(INC)/regex.h
create.o: $(INC)/stdio.h $(INC)/sys/stat.h
create.o: $(INC)/sys/types.h $(INC)/sys/time.h
create.o: $(INC)/sys/errno.h $(INC)/fcntl.h
create.o: $(INC)/sys/fcntl.h $(INC)/unistd.h
create.o: $(INC)/sys/unistd.h $(INC)/stdlib.h
create.o: $(INC)/string.h $(INC)/pfmt.h charmap.h func.h
create.o: pax_msgids.h $(INC)/sys/mkdev.h
extract.o: $(INC)/sys/types.h $(INC)/sys/utime.h config.h pax.h
extract.o: $(INC)/limits.h $(INC)/regex.h $(INC)/stdio.h
extract.o: $(INC)/sys/stat.h $(INC)/sys/time.h
extract.o: $(INC)/sys/errno.h $(INC)/fcntl.h
extract.o: $(INC)/sys/fcntl.h $(INC)/unistd.h
extract.o: $(INC)/sys/unistd.h $(INC)/stdlib.h
extract.o: $(INC)/string.h $(INC)/pfmt.h charmap.h func.h
extract.o: pax_msgids.h
fileio.o: config.h pax.h $(INC)/limits.h $(INC)/regex.h
fileio.o: $(INC)/stdio.h $(INC)/sys/stat.h
fileio.o: $(INC)/sys/types.h $(INC)/sys/time.h
fileio.o: $(INC)/sys/errno.h $(INC)/fcntl.h
fileio.o: $(INC)/sys/fcntl.h $(INC)/unistd.h
fileio.o: $(INC)/sys/unistd.h $(INC)/stdlib.h
fileio.o: $(INC)/string.h $(INC)/pfmt.h charmap.h func.h
fileio.o: pax_msgids.h
hash.o: config.h pax.h $(INC)/limits.h $(INC)/regex.h
hash.o: $(INC)/stdio.h $(INC)/sys/stat.h $(INC)/sys/types.h
hash.o: $(INC)/sys/time.h $(INC)/sys/errno.h $(INC)/fcntl.h
hash.o: $(INC)/sys/fcntl.h $(INC)/unistd.h
hash.o: $(INC)/sys/unistd.h $(INC)/stdlib.h $(INC)/string.h
hash.o: $(INC)/pfmt.h charmap.h func.h pax_msgids.h
link.o: config.h pax.h $(INC)/limits.h $(INC)/regex.h
link.o: $(INC)/stdio.h $(INC)/sys/stat.h $(INC)/sys/types.h
link.o: $(INC)/sys/time.h $(INC)/sys/errno.h $(INC)/fcntl.h
link.o: $(INC)/sys/fcntl.h $(INC)/unistd.h
link.o: $(INC)/sys/unistd.h $(INC)/stdlib.h $(INC)/string.h
link.o: $(INC)/pfmt.h charmap.h func.h pax_msgids.h
list.o: $(INC)/pwd.h $(INC)/sys/types.h $(INC)/grp.h
list.o: $(INC)/sys/mkdev.h config.h pax.h $(INC)/limits.h
list.o: $(INC)/regex.h $(INC)/stdio.h $(INC)/sys/stat.h
list.o: $(INC)/sys/time.h $(INC)/sys/errno.h $(INC)/fcntl.h
list.o: $(INC)/sys/fcntl.h $(INC)/unistd.h
list.o: $(INC)/sys/unistd.h $(INC)/stdlib.h $(INC)/string.h
list.o: $(INC)/pfmt.h charmap.h func.h pax_msgids.h
mem.o: config.h pax.h $(INC)/limits.h $(INC)/regex.h
mem.o: $(INC)/stdio.h $(INC)/sys/stat.h $(INC)/sys/types.h
mem.o: $(INC)/sys/time.h $(INC)/sys/errno.h $(INC)/fcntl.h
mem.o: $(INC)/sys/fcntl.h $(INC)/unistd.h
mem.o: $(INC)/sys/unistd.h $(INC)/stdlib.h $(INC)/string.h
mem.o: $(INC)/pfmt.h charmap.h func.h pax_msgids.h
namelist.o: $(INC)/sys/types.h $(INC)/sys/utime.h
namelist.o: $(INC)/dirent.h $(INC)/sys/dirent.h
namelist.o: $(INC)/fnmatch.h config.h pax.h $(INC)/limits.h
namelist.o: $(INC)/regex.h $(INC)/stdio.h $(INC)/sys/stat.h
namelist.o: $(INC)/sys/time.h $(INC)/sys/errno.h
namelist.o: $(INC)/fcntl.h $(INC)/sys/fcntl.h
namelist.o: $(INC)/unistd.h $(INC)/sys/unistd.h
namelist.o: $(INC)/stdlib.h $(INC)/string.h $(INC)/pfmt.h
namelist.o: charmap.h func.h pax_msgids.h
names.o: $(INC)/pwd.h $(INC)/sys/types.h $(INC)/grp.h
names.o: config.h pax.h $(INC)/limits.h $(INC)/regex.h
names.o: $(INC)/stdio.h $(INC)/sys/stat.h $(INC)/sys/time.h
names.o: $(INC)/sys/errno.h $(INC)/fcntl.h
names.o: $(INC)/sys/fcntl.h $(INC)/unistd.h
names.o: $(INC)/sys/unistd.h $(INC)/stdlib.h
names.o: $(INC)/string.h $(INC)/pfmt.h charmap.h func.h
names.o: pax_msgids.h
pass.o: $(INC)/sys/types.h $(INC)/sys/utime.h config.h pax.h
pass.o: $(INC)/limits.h $(INC)/regex.h $(INC)/stdio.h
pass.o: $(INC)/sys/stat.h $(INC)/sys/time.h
pass.o: $(INC)/sys/errno.h $(INC)/fcntl.h
pass.o: $(INC)/sys/fcntl.h $(INC)/unistd.h
pass.o: $(INC)/sys/unistd.h $(INC)/stdlib.h $(INC)/string.h
pass.o: $(INC)/pfmt.h charmap.h func.h pax_msgids.h
pathname.o: config.h pax.h $(INC)/limits.h $(INC)/regex.h
pathname.o: $(INC)/stdio.h $(INC)/sys/stat.h
pathname.o: $(INC)/sys/types.h $(INC)/sys/time.h
pathname.o: $(INC)/sys/errno.h $(INC)/fcntl.h
pathname.o: $(INC)/sys/fcntl.h $(INC)/unistd.h
pathname.o: $(INC)/sys/unistd.h $(INC)/stdlib.h
pathname.o: $(INC)/string.h $(INC)/pfmt.h charmap.h func.h
pathname.o: pax_msgids.h
pax.o: $(INC)/sys/stat.h $(INC)/sys/types.h
pax.o: $(INC)/sys/time.h $(INC)/locale.h config.h pax.h
pax.o: $(INC)/limits.h $(INC)/regex.h $(INC)/stdio.h
pax.o: $(INC)/sys/errno.h $(INC)/fcntl.h $(INC)/sys/fcntl.h
pax.o: $(INC)/unistd.h $(INC)/sys/unistd.h $(INC)/stdlib.h
pax.o: $(INC)/string.h $(INC)/pfmt.h charmap.h func.h
pax.o: pax_msgids.h
replace.o: config.h pax.h $(INC)/limits.h $(INC)/regex.h
replace.o: $(INC)/stdio.h $(INC)/sys/stat.h
replace.o: $(INC)/sys/types.h $(INC)/sys/time.h
replace.o: $(INC)/sys/errno.h $(INC)/fcntl.h
replace.o: $(INC)/sys/fcntl.h $(INC)/unistd.h
replace.o: $(INC)/sys/unistd.h $(INC)/stdlib.h
replace.o: $(INC)/string.h $(INC)/pfmt.h charmap.h func.h
replace.o: pax_msgids.h $(INC)/nl_types.h $(INC)/langinfo.h
tar.o: config.h pax.h $(INC)/limits.h $(INC)/regex.h
tar.o: $(INC)/stdio.h $(INC)/sys/stat.h $(INC)/sys/types.h
tar.o: $(INC)/sys/time.h $(INC)/sys/errno.h $(INC)/fcntl.h
tar.o: $(INC)/sys/fcntl.h $(INC)/unistd.h
tar.o: $(INC)/sys/unistd.h $(INC)/stdlib.h $(INC)/string.h
tar.o: $(INC)/pfmt.h charmap.h func.h pax_msgids.h
ttyio.o: config.h pax.h $(INC)/limits.h $(INC)/regex.h
ttyio.o: $(INC)/stdio.h $(INC)/sys/stat.h
ttyio.o: $(INC)/sys/types.h $(INC)/sys/time.h
ttyio.o: $(INC)/sys/errno.h $(INC)/fcntl.h
ttyio.o: $(INC)/sys/fcntl.h $(INC)/unistd.h
ttyio.o: $(INC)/sys/unistd.h $(INC)/stdlib.h
ttyio.o: $(INC)/string.h $(INC)/pfmt.h charmap.h func.h
ttyio.o: pax_msgids.h $(INC)/signal.h $(INC)/sys/signal.h
ttyio.o: $(INC)/sys/bitmasks.h $(INC)/sys/siginfo.h
ttyio.o: $(INC)/sys/ksynch.h $(INC)/sys/dl.h
ttyio.o: $(INC)/sys/ipl.h $(INC)/sys/disp_p.h
ttyio.o: $(INC)/sys/trap.h $(INC)/sys/ksynch_p.h
ttyio.o: $(INC)/sys/list.h $(INC)/sys/listasm.h
warn.o: config.h pax.h $(INC)/limits.h $(INC)/regex.h
warn.o: $(INC)/stdio.h $(INC)/sys/stat.h $(INC)/sys/types.h
warn.o: $(INC)/sys/time.h $(INC)/sys/errno.h $(INC)/fcntl.h
warn.o: $(INC)/sys/fcntl.h $(INC)/unistd.h
warn.o: $(INC)/sys/unistd.h $(INC)/stdlib.h $(INC)/string.h
warn.o: $(INC)/pfmt.h charmap.h func.h pax_msgids.h
