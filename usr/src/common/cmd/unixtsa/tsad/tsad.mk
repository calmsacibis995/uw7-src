#ident	"@(#)tsad.mk	1.2"
#
# @(#)tsad.mk	1.2	10/9/96
#

TOP=..
include $(TOP)/config.mk

INSDIR=$(OPTBIN)
TARGET=tsad
MAKEFILE=tsad.mk
PROBEFILE=tsad.C

OBJECTS = tsad.o\
	config_file.o

all:
	@if [ -f $(PROBEFILE) ]; then \
#		$(C++C) $(CFLAGS) -o $(TARGETS) -c $(OBJECTS) $(LIBLIST);\
	$(MAKE) -f $(MAKEFILE) $(TARGET);\
	fi

install: all 
	[ -d $(ROOT)/$(MACH)/usr/sbin ] || mkdir -p $(ROOT)/$(MACH)/usr/sbin
	$(INS) -f $(ROOT)/$(MACH)/usr/sbin  -g $(GRP) -u $(OWN) -m $(BINMODE) tsad

clean : do_clean

clobber : clean do_clobber

$(TARGET) : $(OBJECTS)
	$(C++C) $(CFLAGS) -o $@ $(OBJECTS) $(LIBLIST)

$(BINDIR)/$(TARGET) : $(TARGET)
	[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
	$(INS) -f $(BINDIR) -g $(GRP) -u $(OWN) -m $(BINMODE) $(TARGET)

tsad.o : config_file.h ../include/tsad.h ../include/tsad_msgs.h
