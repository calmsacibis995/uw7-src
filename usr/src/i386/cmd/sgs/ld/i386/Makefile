#ident	"@(#)ld:i386/Makefile	1.14"

# makefile for i386 ELF version of the link editor
include $(CMDRULES)

MOD=755
INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

LOCALDEF= -DPORTAR $(SGS_COMPAT)
LOCALINC=-I$(COMDIR) -I$(COMINC) -I$(CPUDIR) -I$(CPUINC)
LDFLAGS=

LOCALHEADS= $(COMDIR)/globals.h  $(COMDIR)/macros.h $(CPUDIR)/machdep.h

HFILES=$(LOCALHEADS) $(CPUINC)/paths.h $(CPUINC)/sgs.h

SOURCES= \
	$(COMDIR)/args.c \
	$(COMDIR)/files.c \
	$(COMDIR)/globals.c \
	$(COMDIR)/libs.c \
	$(CPUDIR)/machrel.c \
	$(COMDIR)/main.c \
	$(COMDIR)/map.c \
	$(CPUDIR)/note.c \
	$(COMDIR)/outfile.c \
	$(COMDIR)/relocate.c \
	$(COMDIR)/sections.c \
	$(COMDIR)/syms.c \
	$(COMDIR)/update.c \
	$(COMDIR)/util.c


OBJECTS= \
	args.o \
	files.o \
	globals.o \
	libs.o \
	machrel.o \
	main.o \
	map.o \
	note.o \
	outfile.o \
	relocate.o \
	sections.o \
	syms.o \
	util.o \
	update.o

PRODUCTS= ld

all: $(PRODUCTS)

ld: $(OBJECTS)
	$(CC) -o ld $(LDFLAGS) $(CFLAGS) $(OBJECTS) $(LIBELF) $(LIBSGS)

.c~.c:
	$(GET) $(GFLAGS) -p $< > $*.c
	chmod -w $*.c

args.o:		$(COMDIR)/args.c $(COMDIR)/globals.h $(CPUDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(COMDIR)/args.c

files.o: 	$(COMDIR)/files.c $(COMDIR)/globals.h $(CPUDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(COMDIR)/files.c

globals.o:	$(COMDIR)/globals.c $(COMDIR)/globals.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(COMDIR)/globals.c

libs.o:		$(COMDIR)/libs.c $(COMDIR)/globals.h $(CPUDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(COMDIR)/libs.c

main.o:		$(COMDIR)/main.c $(COMDIR)/globals.h $(CPUDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(COMDIR)/main.c

map.o:		$(COMDIR)/map.c $(COMDIR)/globals.h $(CPUDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(COMDIR)/map.c
			
relocate.o:	$(COMDIR)/relocate.c $(COMDIR)/globals.h  $(CPUDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(COMDIR)/relocate.c

outfile.o:	$(COMDIR)/outfile.c $(COMDIR)/globals.h $(COMDIR)/macros.h $(CPUDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(COMDIR)/outfile.c

sections.o:	$(COMDIR)/sections.c $(COMDIR)/globals.h $(COMDIR)/macros.h $(CPUDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(COMDIR)/sections.c

syms.o:		$(COMDIR)/syms.c $(COMDIR)/globals.h $(CPUDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(COMDIR)/syms.c

update.o:	$(COMDIR)/update.c $(COMDIR)/globals.h $(CPUDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(COMDIR)/update.c

util.o:		$(COMDIR)/util.c $(COMDIR)/globals.h $(CPUDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(COMDIR)/util.c

machrel.o:	$(CPUDIR)/machrel.c $(COMDIR)/globals.h  $(CPUDIR)/machdep.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(CPUDIR)/machrel.c

note.o:		$(CPUDIR)/note.c $(COMDIR)/globals.h
		$(CC) -c $(CFLAGS) $(DEFLIST) $(CPUDIR)/note.c


install:$(INSDIR)/$(SGS)ld

$(INSDIR)/$(SGS)ld:ld
	cp ld ld.bak
	$(STRIP) ld
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)ld ld
	mv ld.bak ld

lintit:	$(LOCALHEADS) $(SOURCES)
	$(LINT) $(LINTFLAGS) $(SOURCES)

printit: $(LOCALHEADS) $(SOURCES)
	pr -n -f $(LOCALHEADS) $(SOURCES) > print.out
clean:
	-rm -f $(OBJECTS)

clobber:	clean
	-rm -f $(PRODUCTS)

