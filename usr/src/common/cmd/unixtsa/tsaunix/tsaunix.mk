#ident	"@(#)tsaunix.mk	1.2"
#
# @(#)tsaunix.mk	1.2	10/9/96
#

TOP=..
include $(TOP)/config.mk

INSDIR=$(BINDIR)
TARGET=tsaunix
LDLIBS=-lcrypt -lgen
MAKEFILE = tsaunix.mk
PROBEFILE = tsamain.C

OBJECTS = \
	backup1.o \
	backup2.o \
	backup3.o \
	encrypt.o \
	filesys.o \
	hardlink.o \
	isvalid.o \
	mapusers.o \
	read.o \
	respond.o \
	restore1.o \
	restore2.o \
	restore3.o \
	scanutil.o \
	sequtil1.o \
	sequtil2.o \
	smsapi.o \
	smspcall.o \
	tsamain.o \
	tsamsgs.o \
	tsapis.o \
	tsasmsp.o \
	write.o

all:
	@if [ -f $(PROBEFILE) ]; \
	then \
		$(MAKE) -f $(MAKEFILE) $(TARGET); \
	fi

install: all
	[ -d $(ROOT)/$(MACH)/usr/sbin ] || mkdir -p $(ROOT)/$(MACH)/usr/sbin
	$(INS) -f $(ROOT)/$(MACH)/usr/sbin -g $(GRP) -u $(OWN) -m $(BINMODE) tsaunix

clean : do_clean

clobber : clean do_clobber

$(TARGET) : $(OBJECTS) ../lib/libsms.a
	$(C++C) $(CFLAGS) -o $@ $(OBJECTS) $(LIBLIST)

$(BINDIR)/$(TARGET) : $(TARGET)
	[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
	$(INS) -f $(BINDIR) -g $(GRP) -u $(OWN) -m $(BINMODE) $(TARGET)
