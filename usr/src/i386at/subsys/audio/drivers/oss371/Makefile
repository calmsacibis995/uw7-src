#-----------------------------------------------------------------------------
#
#       Copyright (C) The Santa Cruz Operation, 1996-1997.
#       This Module contains Proprietary Information of
#       The Santa Cruz Operation, and should be treated as Confidential.
#---------------------------------------------------------------------------
# @(#)Makefile	7.3	1/15/98	11:19:08
#
# Revision History:
# 1997-Jun-27, shawnm, change for use with oss371
# 1997-Feb-06, shawnm, change from uw2.0 to gemini for port from ddi1 to ddi6
# 1997-Jan-21, shawnm, don't use -m -u -g on install
# 1997-Jan-06, shawnm, created
#===========================================================================

TARGET_OS	= gemini
TARGET_ID	= gemini/oss

INCLSYS = $(ROOT)/$(MACH)/usr/include/sys
AUDRVDIR = $(ROOT)/$(MACH)/usr/lib/audio/drivers
RCDIR = $(ROOT)/$(MACH)/etc/init.d
COPYRIGHTDIR = $(ROOT)/$(MACH)/usr/lib/audio/copyright
DRVRNAME = $(TARGET_ID)/Driver.o

CC=$(TOOLS)/usr/ccs/bin/i386cc
CFLAGS = -O -DINKERNEL -DSVR42 -Kpentium -I.

INS = $(TOOLS)/etc/install

FIXEDOBJS       = soundcard.o dev_table.o sound_switch.o
OBJS            = $(FIXEDOBJS) \
                  audio.o dmabuf.o opl3.o sequencer.o midibuf.o sb_card.o \
                  adlib_card.o \
                  sb_mixer.o sb_common.o midi_synth.o \
                  uart401.o sb_midi.o sb_audio.o sys_timer.o \
                  sound_pnp.o pnp.o

SNDCONFOBJS     = sndconf.o ui.o conflib.o ui_curses.o
SNDCONF         = sndconf

IDFILES = $(TARGET_ID)/Master $(TARGET_ID)/System $(TARGET_ID)/Space.c $(TARGET_ID)/Node $(TARGET_ID)/Drvmap

all:
	@if [ -f adlib_card.c ]; then \
		make -f Makefile $(SNDCONF) $(DRVRNAME); \
	fi

sndconf.o: sndconf.c os.h interface.h
	$(CC) -DSVR42 -I. -c sndconf.c

conflib.o: parser/conflib.c os.h interface.h
	$(CC) -DSVR42 -I. -c parser/conflib.c

$(SNDCONF): $(SNDCONFOBJS)
	$(CC) $(SNDCONFOBJS) -DNOPNP -DINKERNEL -DSVR42 -Kpentium -lcurses -o sndconf

$(DRVRNAME): $(OBJS)
	@if [ ! -d oss ]; then \
		mkdir -p oss; \
	fi
	$(LD) -r -o $(DRVRNAME) $(OBJS)

$(OBJS): os.h interface.h

os.h: $(TARGET_OS)/os.h
	-ln -s $(TARGET_OS)/os.h os.h

interface.h: $(TARGET_OS)/interface.h
	-ln -s $(TARGET_OS)/interface.h interface.h

soundcard.o: soundcard.c

soundcard.c: $(TARGET_OS)/soundcard.c
	-@ln -s $(TARGET_OS)/soundcard.c soundcard.c

install: all
	@if [ ! -d $(INCLSYS) ]; then \
		mkdir -p $(INCLSYS); \
	fi
	$(INS) -f $(INCLSYS) soundcard.h
	@if [ ! -d $(AUDRVDIR) ]; then \
		mkdir -p $(AUDRVDIR); \
	fi
	@if [ ! -d $(RCDIR) ]; then \
		mkdir -p $(RCDIR); \
	fi
	$(INS) -f $(AUDRVDIR) $(DRVRNAME)
	$(INS) -f $(AUDRVDIR) $(SNDCONF)
	$(INS) -f $(RCDIR) ${TARGET_ID}/soundon
	$(INS) -f $(AUDRVDIR) $(TARGET_ID)/Master
	$(INS) -f $(AUDRVDIR) $(TARGET_ID)/System
	$(INS) -f $(AUDRVDIR) $(TARGET_ID)/Space.c
	$(INS) -f $(AUDRVDIR) $(TARGET_ID)/Node
	$(INS) -f $(AUDRVDIR) $(TARGET_ID)/Drvmap
	@if [ ! -d $(COPYRIGHTDIR) ]; then \
		mkdir -p $(COPYRIGHTDIR); \
	fi
	cp -f COPYRIGHT $(COPYRIGHTDIR)/COPYRIGHT.drivers

clobber:	clean
	rm -rf oss
	rm -f interface.h os.h soundcard.c
	rm -f $(DRVRNAME) $(SNDCONF)

clean:
	rm -f core *.o *.a
	for i in *.c;\
	do \
	(rm -f `basename $$i .c`.s); \
	done

