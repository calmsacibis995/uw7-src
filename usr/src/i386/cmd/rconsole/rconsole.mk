#	copyright	"%c%"

#ident	"@(#)rconsole:i386/cmd/rconsole/rconsole.mk	1.1.3.7"

include $(CMDRULES)


OWN = 
GRP = 

INSDIR = $(USRSBIN)

MAINS = chkconsole isat386 adpt_type

OBJECTS = conflgs.o chkconsole.o isat386.o adpt_type.o

SOURCES = conflgs.c chkconsole.c isat386.c adpt_type.c

MSGS	= adpt_type.str

all: $(MAINS)

conflgs: conflgs.o 
	$(CC) -o conflgs conflgs.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

chkconsole: chkconsole.o 
	$(CC) -o chkconsole chkconsole.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

isat386: isat386.o 
	$(CC) -o isat386 isat386.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

adpt_type: adpt_type.o 
	$(CC) -o adpt_type adpt_type.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

chkconsole.o: chkconsole.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/sys/stat.h

conflgs.o: conflgs.c \
	$(INC)/stdio.h

isat386.o: isat386.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/errno.h $(INC)/sys/errno.h

adpt_type.o: adpt_type.c \
	$(INC)/stdio.h $(INC)/stdlib.h $(INC)/fcntl.h \
	$(INC)/errno.h $(INC)/sys/types.h $(INC)/sys/param.h \
	$(INC)/sys/at_ansi.h $(INC)/sys/kd.h

install: all $(MSGS)
	$(INS) -f $(INSDIR) chkconsole
	$(INS) -f $(INSDIR) isat386
	$(INS) -f $(INSDIR) adpt_type
	-[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 adpt_type.str

clean:
	-rm -f $(OBJECTS)

clobber: clean
	-rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)





