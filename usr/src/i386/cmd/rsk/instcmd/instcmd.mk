#ident	"@(#)instcmd.mk	15.1"

include	$(CMDRULES)

MAINS	= check_devs kb_remap sbfmkfs keycomp dkcomp keymap
LOCAL_MAINS = bmgr fcomp
KB_OBJS	= kb_remap.o kb_read_dk.o kb_read_kbd.o kb_read_font.o kb_misc.o
OBJECTS	= check_devs.o sbfmkfs.o keycomp.o dkcomp.o \
	  lex.yy.o $(KB_OBJS)

SOURCES = check_devs.c sbfmkfs.c kb_remap.c kb_read_dk.c \
	  kb_read_kbd.c kb_read_font.c kb_misc.c keycomp.c keymap \
	  dkcomp.c lex.l defs.h key_remap.h keycomp.h kb_remap.h \
	  bmgr.c fcomp.c fcomp.h
FONT_INPUT = gr.8x14 gr.8x16 iso.8x14 iso.8x16

FONT = 88591

LOCALDEF = -D_KMEMUSER -D_KERNEL_HEADERS
LOCALINC = -I$(ROOT)/usr/src/$(WORK)/uts
INSDIR  = $(USRLIB)/rsk

all: $(MAINS) $(LOCAL_MAINS) $(FONT)

check_devs: $$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

check_devs.o: check_devs.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/unistd.h \
	$(INC)/errno.h \
	$(INC)/fcntl.h \
	$(INC)/sys/cram.h \
	$(INC)/sys/kd.h \
	$(INC)/sys/inline.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fd.h \
	$(INC)/sys/ddi.h

sbfmkfs: $$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

sbfmkfs.o: $(@:.o=.c)
	$(CC) $(CFLAGS) $(LOCALINC) $(LOCALDEF) -c sbfmkfs.c

kb_remap: $(KB_OBJS)
	$(CC) $(CFLAGS) -o $@ $(KB_OBJS) $(LDFLAGS)

kb_remap.o: kb_remap.c \
	kb_remap.h \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/sys/kd.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fcntl.h

kb_read_dk.p: kb_read_dk.c \
	kb_remap.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h

kb_read_font.p: kb_read_font.c \
	kb_remap.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/kd.h

kb_read_kbd.p: kb_read_kbd.c \
	kb_remap.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/kd.h

kb_misc.p: kb_misc.c \
	kb_remap.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/kd.h

keycomp: keycomp.c keycomp.h key_remap.h
	$(CC) -I$(INC) $(CFLAGS) -o $@ $@.c $(LDFLAGS)

dkcomp: $$@.o lex.yy.o
	$(CC) -I$(INC) $(CFLAGS) -o $@ $@.o lex.yy.o $(LDFLAGS)

dkcomp.o: dkcomp.c defs.h key_remap.h
	$(CC) -I$(INC) -c $(CFLAGS) dkcomp.c

lex.yy.o: lex.yy.c
	$(CC) -I$(INC) -c $(CFLAGS) lex.yy.c

lex.yy.c: lex.l defs.h
	$(LEX) $(LFLAGS) lex.l

bmgr: $$@.c \
		$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/fcntl.h
	$(HCC) $(CFLAGS) -I/usr/include -o $@ $@.c $(LDFLAGS)

fcomp: $$@.c \
		fcomp.h \
		kb_remap.h \
		$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/fcntl.h \
		$(INC)/sys/mman.h
	$(HCC) $(CFLAGS) -I/usr/include -o $@ $@.c $(LDFLAGS)

$(SOURCES):
	@ln -s $(PROTO)/desktop/instcmd/$@ $@

$(FONT_INPUT):
	@ln -s $(PROTO)/desktop/instcmd/fontsrc/$@ $@

$(FONT): bmgr fcomp $(FONT_INPUT)
	 ./bmgr gr.8x14
	 ./bmgr gr.8x16
	 mv gr.8x14.bm grbm.8x14
	 mv gr.8x16.bm grbm.8x16
	 ./fcomp iso $(FONT)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	@for i in $(MAINS) $(FONT);\
	do \
		$(INS) -f $(INSDIR) $$i ;\
	done

clean:
	rm -f $(OBJECTS) lex.yy.c $(FONT_INPUT) grbm.8x14 grbm.8x16

clobber: clean
	rm -f $(MAINS) $(LOCAL_MAINS) $(SOURCES) $(FONT)
