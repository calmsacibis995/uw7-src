#ident	"@(#)cmd.mk	15.1"
include $(CMDRULES)

CMDS    = bootstrap wrt tapeop getcylsize bzip
SCRIPTS1 = rsk_image rsk_boot prep_flop asknodename inet_getvals S03RSK S01rsk_nws
OTRFILES = rskram.proto rsk_inst.gen

LOCALDEF  = -DDRF_FLOP
INSDIR  = $(USRLIB)/rsk
INSDIRL = $(USRLIB)/rsk/locale/C
MSGDIR  = $(USRLIB)/locale/C/MSGFILES

SOURCES = bootstrap.c wrt.c tapeop.c getcylsize.c

all: $(CMDS) $(SCRIPTS1) $(TXTSTR)

$(SOURCES):
	@ln -s $(PROTO)/cmd/$@ $@

bootstrap: $$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

bootstrap.o: $(@:.o=.c)
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -D_KMEMUSER -c bootstrap.c

wrt:	$$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

tapeop:	$$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

getcylsize:	$$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

bzip:	zip/gzip
	cp $? $@

zip/gzip:	
	(cd zip; \
	 $(MAKE) -f zip.mk gzip)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	@for i in $(CMDS) $(SCRIPTS1) $(OTRFILES);\
	do \
		$(INS) -f $(INSDIR) $$i ;\
	done

	[ -d $(INSDIRL) ] || mkdir -p $(INSDIRL)
	$(INS) -f $(INSDIRL) txtstr.C

	[ -d $(MSGDIR) ] || mkdir -p $(MSGDIR)
	$(INS) -f $(MSGDIR) rsk.str

clean:
	rm -f $(SCRIPTS1)
	rm -f *.o
	(cd zip; $(MAKE) -f zip.mk clean)

clobber: clean
	rm -f $(CMDS) $(SOURCES)  $(SCRIPTS1)
	(cd zip; $(MAKE) -f zip.mk clobber)
