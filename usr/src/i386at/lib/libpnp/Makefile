CC=$(TOOLS)/usr/ccs/bin/i386cc
RM =		rm -f
CP =		cp
MKDIR =		mkdir
LN =		ln
LD =		ld
AR =		ar
RANLIB =	@:
INS = $(TOOLS)/etc/install
INSDIR= $(ROOT)/$(MACH)/usr/lib

DFLAGS =	-DGEMINI
IFLAGS =	-I$(TOOLS)/usr/include	-I$(ROOT)/usr/src/$(WORK)/uts/io/autoconf/ca/isapnp
CFLAGS = -O -w3 -wx $(DFLAGS) $(IFLAGS) -belf -Kpic
OFILES = eisaconv.o tcl_interface.o pnpconfig.o hw_eisa.o
LFLAGS =

TARGET = libpnp.a libpnp.so libpnp.so.1

all: $(TARGET)

libpnp.a: $(OFILES)
	$(RM) libpnp.a
	$(AR) q libpnp.a $(OFILES)

libpnp.so: $(OFILES)
	$(RM) libpnp.so libpnp.so.1
	$(LD) -G $(OFILES) $(LFLAGS) -o libpnp.so

libpnp.so.1: libpnp.so
	$(LN) -s libpnp.so libpnp.so.1

install: $(TARGET)
	@if [ ! -d $(INSDIR) ]; then \
		mkdir -p $(INSDIR); \
  	fi
	$(INS) -f $(INSDIR) libpnp.so
	$(INS) -f $(INSDIR) libpnp.so.1
	$(INS) -f $(INSDIR) libpnp.a
	$(INS) -f $(INSDIR) PnP.tlib

clean:
	$(RM) *.o core

clobber: clean
	$(RM) $(TARGET)
