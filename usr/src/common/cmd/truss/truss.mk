#	copyright	"%c%"

#ident	"@(#)truss:common/cmd/truss/truss.mk	1.6.8.2"
#ident  "$Header$"
#
# makefile for truss(1) command
#
# make		- make truss in local directory
# make install	- make truss and install in $(INSDIR)
# make lint	- check program consistency
# make clean	- as your mother told you
# make clobber	- make it squeaky clean

include $(CMDRULES)

PROCISSUE = PROC Issue 2 Version 1

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

LINTFLAGS=$(DEFLIST)

HEADERS = pcontrol.h ioc.h ramdata.h proto.h systable.h print.h \
	machdep.h registers.h codes.h

SOURCES = main.c listopts.c ipc.c actions.c expound.c incdec.c \
	codes.c print.c pcontrol.c ramdata.c systable.c procset.c xstat.c \
	args.c syscall.c name.c prt.c

OBJECTS = main.o listopts.o ipc.o actions.o expound.o incdec.o \
	codes.o print.o pcontrol.o ramdata.o systable.o procset.o xstat.o \
	args.o syscall.o name.o prt.o

all:	truss

truss:	$(OBJECTS)
	$(CC) -o truss $(OBJECTS) $(SHLIBS) $(LDFLAGS) $(LDLIBS)
#	$(MCS) -d truss
#	$(MCS) -a "@(#)/bin/truss $(PROCISSUE) `date +%m/%d/%y`" truss

actions.o:	pcontrol.h ramdata.h systable.h print.h proto.h machdep.h
codes.o:	pcontrol.h ioc.h codes.h ramdata.h systable.h proto.h
expound.o:	pcontrol.h ramdata.h systable.h proto.h
incdec.o:	pcontrol.h ramdata.h proto.h
ipc.o:		pcontrol.h ramdata.h systable.h proto.h machdep.h
listopts.o:	pcontrol.h ramdata.h systable.h proto.h
main.o:		pcontrol.h ramdata.h systable.h proto.h machdep.h
pcontrol.o:	pcontrol.h ramdata.h systable.h proto.h machdep.h
print.o:	pcontrol.h print.h ramdata.h systable.h proto.h machdep.h
procset.o:	pcontrol.h ramdata.h systable.h proto.h
systable.o:	pcontrol.h ramdata.h systable.h print.h proto.h
xstat.o:	pcontrol.h ramdata.h systable.h proto.h
ramdata.o:	pcontrol.h ramdata.h systable.h proto.h
args.o:		pcontrol.h ramdata.h systable.h proto.h machdep.h
syscall.o:	pcontrol.h ramdata.h proto.h machdep.h
name.o:		pcontrol.h ramdata.h proto.h machdep.h
prt.o:		pcontrol.h ramdata.h systable.h proto.h machdep.h

install:	truss
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) truss

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

clean:
	rm -f *.o

clobber:	clean
	rm -f truss
