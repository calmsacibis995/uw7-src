#ident	"@(#)instcmd.mk	15.1	98/03/04"

include	$(CMDRULES)

KB_OBJS = kb_remap.o kb_read_dk.o kb_read_kbd.o kb_read_font.o kb_misc.o 
OBJECTS = $(KB_OBJS) dkcomp.o lex.yy.o lex.yy.c

KB_SRC = $(ROOT)/$(MACH)/usr/lib/keyboard
DK_SRC = $(ROOT)/$(MACH)/usr/lib/mapchan

DIRS = C 8859-1 8859-2 8859-5 8859-7 8859-9 fontsrc

TARGET_MAINS = check_devs sflop kb_remap
HOST_MAINS = edsym sbfmkfs
LOCAL_MAINS = keycomp bmgr fcomp dkcomp
LOCALDEF = -D_KMEMUSER -D_KERNEL_HEADERS
LOCALINC = -I$(ROOT)/usr/src/$(WORK)/uts
MAINS = $(TARGET_MAINS) $(HOST_MAINS) $(LOCAL_MAINS)

all: $(MAINS)
	@for i in $(DIRS) ;\
	do \
		cd $$i ;\
		echo "Entering `pwd`" ;\
		make ;\
		echo "Leaving `pwd`" ;\
		cd .. ;\
	done

check_devs: $$@.c \
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
	$(CC)  $(CFLAGS) -I$(INC) -o $@ $@.c $(LDFLAGS)

sflop: $$@.c \
		$(INC)/stdio.h \
		$(INC)/stdlib.h \
		$(INC)/unistd.h \
		$(INC)/errno.h \
		$(INC)/string.h \
		$(INC)/sys/types.h \
		$(INC)/fcntl.h \
		$(INC)/sys/vtoc.h \
		$(INC)/sys/fd.h
	$(CC)  $(CFLAGS) -I$(INC) -o $@ $@.c $(LDFLAGS)

kb_remap: $(KB_OBJS)
	$(CC)  $(CFLAGS) -I$(INC) -o $@ $(KB_OBJS) $(LDFLAGS)

kb_remap.o: $(@:.o=.c) \
	kb_remap.h \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/sys/kd.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fcntl.h

kb_read_dk.o: $(@:.o=.c) \
	kb_remap.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h

kb_read_font.o: $(@:.o=.c) \
	kb_remap.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/kd.h

kb_read_kbd.o: $(@:.o=.c) \
	kb_remap.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/kd.h

kb_misc.o: $(@:.o=.c) \
	kb_remap.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/kd.h

edsym: $$@.c 
	$(HCC) $(CFLAGS) -o $@ $@.c $(LDFLAGS)

sbfmkfs: $$@.o
	$(HCC) -o $@ sbfmkfs.o $(LDFLAGS)

# Be sure to first search in /usr/include. 
# Later, obtain just a few cross-env .h files.
sbfmkfs.o: $(@:.o=.c)
	$(HCC) $(CFLAGS) -I/usr/include $(LOCALINC) -D_KMEMUSER -c sbfmkfs.c

keycomp: $$@.c keycomp.h key_remap.h 
	$(HCC) $(CFLAGS) -o $@ $@.c $(LDFLAGS)

dkcomp: $$@.o lex.yy.o
	$(HCC) $(CFLAGS) -o $@ $@.o lex.yy.o $(LDFLAGS)

dkcomp.o: $(@:.o=.c) defs.h key_remap.h 
	$(HCC) $(CFLAGS) -c $(@:.o=.c)

lex.yy.o: $(@:.o=.c) 
	$(HCC) $(CFLAGS) -c $(@:.o=.c) 

lex.yy.c: lex.l defs.h
	$(LEX) $(LFLAGS) lex.l

bmgr: $$@.c 
	$(HCC) $(CFLAGS) -o $@ $@.c $(LDFLAGS)

fcomp: $$@.c \
		fcomp.h \
		kb_remap.h
	$(HCC) $(CFLAGS) -o $@ $@.c $(LDFLAGS)

install: all
	@for i in $(TARGET_MAINS) $(HOST_MAINS) ;\
	do \
		$(INS) -f $(PROTO)/bin $$i ;\
	done
	@for i in $(DIRS) ;\
	do \
		cd $$i ;\
		make install ;\
		cd .. ;\
	done

clean:
	rm -f $(OBJECTS)
	@for i in $(DIRS) ;\
	do \
		cd $$i ;\
		make clean ;\
		cd .. ;\
	done

clobber: clean
	rm -f $(MAINS)
	@for i in $(DIRS) ;\
	do \
		cd $$i ;\
		make clobber ;\
		cd .. ;\
	done
