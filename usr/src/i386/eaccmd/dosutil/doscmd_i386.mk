#	copyright	"%c%"

#ident	"@(#)eac:i386/eaccmd/dosutil/doscmd_i386.mk	1.3.1.8"
#ident  "$Header$"

include $(CMDRULES)

#	@(#) dosutil.mk 22.1 89/11/14 
#
#	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987.
#	Copyright (C) Microsoft Corporation, 1984, 1985, 1986, 1987.
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, Microsoft Corporation
#	and AT&T, and should be treated as Confidential.

OWN = bin
GRP = dos

DIRS = $(USRBIN) $(ETC)/default
LOCALDEF = -DO_SYNCW=O_SYNC
LDLIBS = $(LIB) -lx -lgen

CMDS = doscat doscp dosdir dosmkdir dosrm subdirs
CMDS1 = doscat doscp dosdir dosmkdir dosrm
SRCS = doscat.c doscp.c dosdir.c dosmkdir.c dosrm.c
INCS = dosutil.h

OBJS = $(SRCS:.c=.o) $(DOSLIBCFILES:.c=.o)
LIB = libd.a

FRC =

DOSLIBCFILES=deflt.c cat.c child.c common.c dir.c dir2.c dir3.c \
	  disk.c disk2.c fat.c fat2.c fat3.c fat4.c interrupt.c \
	  machdep.c unlink.c parent.c pattern.c recursive.c makefuncs.c
DOSLIBOBJ = $(DOSLIBCFILES:.c=.o)

all: $(LIB) $(CMDS)

install: $(DIRS) all
	$(INS) -f $(USRBIN) -m 0711 -u $(OWN) -g $(GRP) doscat
	$(INS) -f $(USRBIN) -m 0711 -u $(OWN) -g $(GRP) doscp
	$(INS) -f $(USRBIN) -m 0711 -u $(OWN) -g $(GRP) dosdir
	$(INS) -f $(USRBIN) -m 0711 -u $(OWN) -g $(GRP) dosmkdir
	$(INS) -f $(USRBIN) -m 0711 -u $(OWN) -g $(GRP) dosrm
	$(INS) -f $(ETC)/default -m 0644 -u $(OWN) -g bin msdos.sh
	mv $(ETC)/default/msdos.sh $(ETC)/default/msdos
	rm -f $(USRBIN)/dosls $(USRBIN)/dosrmdir
	ln $(USRBIN)/dosdir $(USRBIN)/dosls
	ln $(USRBIN)/dosrm $(USRBIN)/dosrmdir
	for i in dosformat ;\
	do \
		( \
		echo "cd $$i && $(MAKE) -f $$i.mk $(MAKEARGS) install " ;\
		cd $$i && $(MAKE) -f $$i.mk $(MAKEARGS) install;\
		) \
	done ;

$(DIRS):
	[ -d $@ ] || mkdir -p $@ ;\
		$(CH)chmod 0755 $@ ;\
		$(CH)chown $(OWN) $@ ;\
		$(CH)chgrp bin $@

clean: 
	rm -f $(OBJS) $(LIB)
	for i in dosformat ;\
	do \
		( \
		echo "cd $$i && $(MAKE) -f $$i.mk $(MAKEARGS) clean " ;\
		cd $$i && $(MAKE) -f $$i.mk $(MAKEARGS) clean;\
		) \
	done ;

clobber: clean
	rm -f $(CMDS1)
	for i in dosformat ;\
	do \
		( \
		echo "cd $$i && $(MAKE) -f $$i.mk $(MAKEARGS) clobber " ;\
		cd $$i && $(MAKE) -f $$i.mk $(MAKEARGS) clobber;\
		) \
	done ;

lintit:
	$(LINT) $(LINTFLAGS) doscat.c
	$(LINT) $(LINTFLAGS) doscp.c
	$(LINT) $(LINTFLAGS) dosdir.c
	$(LINT) $(LINTFLAGS) dosmkdir.c
	$(LINT) $(LINTFLAGS) dosrm.c
	for i in dosformat ;\
	do \
		( \
		echo "cd $$i && $(MAKE) -f $$i.mk $(MAKEARGS) lintit " ;\
		cd $$i && $(MAKE) -f $$i.mk $(MAKEARGS) lintit;\
		) \
	done ;

$(LIB): $(DOSLIBOBJ)
	$(AR) $(ARFLAGS) $(LIB) $?

doscat: $(LIB) doscat.o
	$(CC) -o doscat doscat.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

doscp: $(LIB) doscp.o
	$(CC) -o doscp doscp.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

dosdir: $(LIB) dosdir.o
	$(CC) -o dosdir dosdir.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

dosls: dosdir
	rm -f dosls
	ln dosdir dosls

dosmkdir: $(LIB) dosmkdir.o
	$(CC) -o dosmkdir dosmkdir.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

dosrm: $(LIB) dosrm.o
	$(CC) -o dosrm dosrm.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

dosrmdir: dosrm
	rm -f dosrmdir
	ln dosrm dosrmdir

subdirs: 
	for i in dosformat ;\
	do \
		( \
		echo "cd $$i && $(MAKE) -f $$i.mk $(MAKEARGS) " ;\
		cd $$i && $(MAKE) -f $$i.mk $(MAKEARGS) ;\
		) \
	done ;

doscat.o: doscat.c \
	$(INC)/sys/types.h \
	$(INC)/unistd.h \
	$(INC)/sys/errno.h \
	$(INC)/fcntl.h \
	$(INC)/stdio.h \
	dosutil.h \
	$(INC)/string.h

doscp.o: doscp.c \
	$(INC)/sys/types.h \
	$(INC)/unistd.h \
	$(INC)/sys/errno.h \
	$(INC)/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	dosutil.h \
	$(INC)/string.h \
	$(INC)/ctype.h

dosdir.o: dosdir.c \
	$(INC)/sys/types.h \
	$(INC)/unistd.h \
	$(INC)/sys/errno.h \
	$(INC)/fcntl.h \
	$(INC)/stdio.h \
	dosutil.h \
	$(INC)/string.h

dosmkdir.o: dosmkdir.c \
	$(INC)/sys/types.h \
	$(INC)/unistd.h \
	$(INC)/sys/errno.h \
	$(INC)/fcntl.h \
	$(INC)/stdio.h \
	dosutil.h \
	$(INC)/string.h

dosrm.o: dosrm.c \
	$(INC)/sys/types.h \
	$(INC)/unistd.h \
	$(INC)/sys/errno.h \
	$(INC)/fcntl.h \
	$(INC)/stdio.h \
	dosutil.h \
	$(INC)/string.h

