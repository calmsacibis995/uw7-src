#ident	"@(#)drf:hcomp/hcomp.mk	1.4.1.3"

include $(CMDRULES)

SOURCES = hcomp.c wslib.c wslib.h
HELPFILES = na.hcf drf_help.hcf drf_sh.hcf drf_rst.hcf \
	    drf_mount.hcf drf_umount.hcf drf_snum.hcf drf_rbt.hcf
HELPCPIO = locale_hcf.z
KSH = /usr/bin/ksh

INSDIR = $(USRLIB)/drf/locale/C

all: hcomp $(HELPCPIO)

$(HELPCPIO): $(HELPFILES) $(PROTO)/bin/wrt $(PROTO)/bin/bzip
	
	ls *.hcf | PATH=$(PATH):$(PROTO)/bin $(KSH) $(PROTO)/desktop/buildscripts/cpioout >$(HELPCPIO)

$(SOURCES):
	@ln -s $(ROOT)/usr/src/$(WORK)/cmd/winxksh/libwin/$@ $@

hcomp:	hcomp.o wslib.o
	$(HCC) $(CFLAGS) $? -o $@ -lw

hcomp.o: hcomp.c wslib.h
	$(HCC) -I/usr/include -c $(CFLAGS) hcomp.c

wslib.o:	wslib.c wslib.h
	$(HCC) -I/usr/include -c $(CFLAGS) wslib.c


$(HELPFILES): $(@:.hcf=)
	./hcomp $(@:.hcf=)

$(PROTO)/bin/wrt:	$(PROTO)/cmd/wrt.c $(PROTO)/bin
	(cd $(PROTO)/cmd; \
	make -f cmd.mk wrt; \
	install -f $(PROTO)/bin wrt )

$(PROTO)/bin/bzip:	$(PROTO)/cmd/zip/gzip
	cp $? $@

$(PROTO)/cmd/zip/gzip:	$(PROTO)/cmd/zip/gzip.c $(PROTO)/bin
	(cd $(PROTO)/cmd; \
	make -f cmd.mk bzip; \
	install -f $(PROTO)/bin bzip )

$(PROTO)/bin:
	mkdir -p $(PROTO)/bin 2>/dev/null 

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	@for i in $(HELPCPIO);\
	do \
		$(INS) -f $(INSDIR) $$i ; \
	done

clean:
	rm -f *.o 

clobber: clean
	rm -f hcomp $(SOURCES) $(HELPFILES) 
