
#
#	@(#)Makefile.new	6.1	9/9/97	15:46:17
#

CURDIR = audio/drivers/audioconfig/audinfo
SUBDIRS=AdLib Aztech BTC CreativeLabs Generic Intel Logitech MediaVision
AUDINFODIR = $(ROOT)/$(MACH)/usr/lib/audio/audioconfig/audinfo
CC	= $(TOOLS)/usr/ccs/bin/i386cc
CFLAGS	= -belf -dy -s
YACC	= $(TOOLS)/usr/ccs/bin/i386yacc -d
LEX	= flex-2.5.3/flex -l
INS = $(TOOLS)/etc/install

all:	$(LEX) config.Z reader lscards ossres

$(LEX): flex-2.5.3
	@cd flex-2.5.3; $(MAKE)

config.Z: parser config.asc $(LEX)
	rm -f config config.gz config.Z
	./parser config < config.asc
	compress config

parser:	parser.o scan.o confdata.h
	$(CC) -o parser parser.o scan.o 

scan.o: scan.l $(LEX)

install:  config.Z reader lscards ossres
	@if [ ! -d $(AUDINFODIR) ]; then \
		mkdir -p $(AUDINFODIR); \
	fi
	@case '${MFLAGS}' in *[ik]*) set +e;; esac; \
	for i in $(SUBDIRS) ;\
	do \
		(cd $$i ; echo "making" $@ "in $(CURDIR)/$$i..."; \
		$(MAKE) $(MFLAGS) 'CDEBUGFLAGS=$(CDEBUGFLAGS)' $@); \
	done
	$(INS) -f $(AUDINFODIR) ossres
	$(INS) -f $(AUDINFODIR) lscards
	$(INS) -f $(AUDINFODIR) config.Z
	$(INS) -f $(AUDINFODIR) reader

clean:
	rm -f config.tmp  core *.o y.tab.h *~ *.old *.orig x y z 
	@cd flex-2.5.3; $(MAKE) clean
	@case '${MFLAGS}' in *[ik]*) set +e;; esac; \
	for i in $(SUBDIRS) ;\
	do \
	(cd $$i ; echo "making" $@ "in $(CURDIR)/$$i..."; \
	$(MAKE) $(MFLAGS) 'CDEBUGFLAGS=$(CDEBUGFLAGS)' $@); \
	done

clobber: clean
	rm -f parser config config.gz config.Z
	rm -f reader lscards ossres
	@cd flex-2.5.3; $(MAKE) clobber

conflib.o: confdata.h

reader: reader.o conflib.o
	$(CC) -o reader reader.o conflib.o

ossres.o: ossres.c confdata.h

ossres: ossres.o conflib.o
	$(CC) -o ossres ossres.o conflib.o

lscards: lscards.o conflib.o
	$(CC) -o lscards lscards.o conflib.o


