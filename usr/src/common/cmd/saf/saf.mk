#ident	"@(#)saf.mk	1.3"
#ident  "$Header$"

include $(CMDRULES)

#
# saf.mk: makefile for the Service Access Facility
#
# if debug is needed then add -DDEBUG to following line
LOCALDEF =
SACDIR = $(USRLIB)/saf
LDLIBS = -lgen -liaf

OWN = root
GRP = sys

# uncomment the next two lines to compile with -g
# CFLAGS = -g
# LDFLAGS =

SACSRC = \
	sac.c \
	readtab.c \
	global.c \
	log.c \
	misc.c \
	util.c

SACADMSRC = \
	sacadm.c \
	log.c \
	admutil.c \
	util.c

PMADMSRC = \
	pmadm.c \
	log.c \
	admutil.c \
	util.c

SACOBJ = \
	sac.o \
	readtab.o \
	global.o \
	log.o \
	misc.o \
	util1.o

SACADMOBJ = \
	sacadm.o \
	log.o \
	admutil.o \
	util2.o

PMADMOBJ = \
	pmadm.o \
	log.o \
	admutil.o \
	util2.o

MAINS = sac sacadm pmadm

all: $(MAINS)

sac: $(SACOBJ)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
	$(CC) -o sac $(SACOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS) ;\
	else \
	$(CC) -o sac $(SACOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS) ;\
	fi

sacadm: $(SACADMOBJ)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
	$(CC) -o sacadm $(SACADMOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS) ;\
	else \
	$(CC) -o sacadm $(SACADMOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS) ;\
	fi

pmadm: $(PMADMOBJ)
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
	$(CC) -o pmadm $(PMADMOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS) ;\
	else \
	$(CC) -o pmadm $(PMADMOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS) ;\
	fi

# To share as much code as possible, util.c is compiled into two
# forms, defining SAC for the sac's version of the file and undefining
# it for the administrative commands version

util1.o: util.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/sys/types.h \
	$(INC)/unistd.h \
	extern.h \
	misc.h \
	$(INC)/sac.h \
	structs.h \
	msgs.h
	$(CC) -c $(CFLAGS) $(DEFLIST) -DSAC util.c
	mv util.o util1.o

util2.o: util.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/sys/types.h \
	$(INC)/unistd.h \
	extern.h \
	misc.h \
	$(INC)/sac.h \
	structs.h \
	msgs.h
	$(CC) -c $(CFLAGS) $(DEFLIST) -USAC util.c
	mv util.o util2.o

admutil.o: admutil.c \
	$(INC)/stdio.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sac.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/unistd.h \
	$(INC)/priv.h \
	$(INC)/mac.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	misc.h \
	structs.h \
	extern.h

global.o: global.c \
	$(INC)/stdio.h \
	$(INC)/sac.h \
	$(INC)/sys/types.h \
	misc.h \
	structs.h

log.o: log.c \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/sys/types.h \
	$(INC)/sac.h \
	$(INC)/priv.h \
	extern.h \
	misc.h \
	msgs.h \
	structs.h

misc.o: misc.c \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/fcntl.h \
	$(INC)/sys/types.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/stat.h \
	$(INC)/poll.h \
	misc.h \
	msgs.h \
	extern.h \
	$(INC)/sac.h \
	adm.h \
	structs.h

pmadm.o: pmadm.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/unistd.h \
	$(INC)/sac.h \
	$(INC)/pwd.h \
	$(INC)/priv.h \
	$(INC)/mac.h \
	$(INC)/sys/secsys.h \
	extern.h \
	misc.h \
	structs.h

readtab.o: readtab.c \
	$(INC)/stdio.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	misc.h \
	msgs.h \
	$(INC)/sac.h \
	structs.h \
	$(INC)/sys/types.h \
	$(INC)/unistd.h \
	extern.h

sac.o: sac.c \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/ctype.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/stropts.h \
	$(INC)/unistd.h \
	$(INC)/utmp.h \
	$(INC)/memory.h \
	$(INC)/sac.h \
	$(INC)/priv.h \
	$(INC)/mac.h \
	$(INC)/sys/secsys.h \
	msgs.h \
	extern.h \
	misc.h \
	structs.h

sacadm.o: sacadm.c \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/unistd.h \
	$(INC)/sac.h \
	$(INC)/priv.h \
	$(INC)/mac.h \
	$(INC)/sys/secsys.h \
	misc.h \
	structs.h \
	adm.h \
	extern.h

install: all $(SACDIR)
	$(INS) -o -f $(SACDIR) -u $(OWN) -g $(GRP) sac
	$(INS) -f $(USRSBIN) -u $(OWN) -g $(GRP) pmadm
	$(INS) -f $(USRSBIN) -u $(OWN) -g $(GRP) -m 04755 sacadm

$(SACDIR):
	mkdir $@

clean:
	-rm -f *.o

clobber: clean
	-rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SACSRC)
	$(LINT) $(LINTFLAGS) $(SACADMSRC)
	$(LINT) $(LINTFLAGS) $(PMADMSRC)
