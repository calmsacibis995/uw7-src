#ident	"@(#)drf:cmd/cmd.mk	1.5.2.4"
include $(CMDRULES)

CMDS    = bootstrap wrt tapeop bzip checkwhite
SCRIPTS1 = prep_flop odm_vfs conframdfs cpioout cut_flop mini_kernel funcrc
SCRIPTS2 = emergency_rec emergency_disk
SCRIPTS3 = S01drf_nws 
OTRFILES = drfram.proto drf_inst.gen disk2.files

INSDIR  = $(USRLIB)/drf
INSDIRL = $(USRLIB)/drf/locale/C
MSGDIR  = $(USRLIB)/locale/C/MSGFILES

SOURCES = bootstrap.c wrt.c tapeop.c  checkwhite.c

all: $(CMDS) $(SCRIPTS1) $(SCRIPTS2) $(TXTSTR)

$(SOURCES):
	@ln -s $(PROTO)/cmd/$@ $@

bootstrap: $$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

bootstrap.o: $(@:.o=.c)
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -D_KMEMUSER -c bootstrap.c

checkwhite: $$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

wrt:	$$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

tapeop:	$$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

bzip:	zip/gzip
	cp $? $@

zip/gzip:	
	(cd zip; \
	 $(MAKE) -f zip.mk gzip)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	@for i in $(CMDS) $(SCRIPTS1) $(SCRIPTS3) $(OTRFILES);\
	do \
		$(INS) -f $(INSDIR) $$i ;\
	done

	[ -d $(INSDIRL) ] || mkdir -p $(INSDIRL)
	$(INS) -f $(INSDIRL) txtstr.C

	[ -d $(MSGDIR) ] || mkdir -p $(MSGDIR)
	$(INS) -f $(MSGDIR) drf.str

	[ -d $(SBIN) ] || mkdir -p $(SBIN)
	@for i in $(SCRIPTS2) ;\
	do \
		$(INS) -f $(SBIN) $$i ;\
	done
	@cp $(ROOT)/usr/src/$(WORK)/sysinst/cmd/rmwhite.sh ./rmwhite
	$(INS) -f $(INSDIR) rmwhite

clean:
	rm -f *.o
	(cd zip; $(MAKE) -f zip.mk clean)

clobber: clean
	rm -f $(CMDS) $(SOURCES)  $(SCRIPTS1) $(SCRIPTS2) 
	(cd zip; $(MAKE) -f zip.mk clobber)
