#ident	"@(#)hcomp.mk	15.1"

include $(CMDRULES)

SOURCES = hcomp.c wslib.c wslib.h
HELPFILES = initsysname.hcf

INSDIR = $(USRLIB)/rsk/locale/C

all: hcomp $(HELPFILES)

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

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	@for i in $(HELPFILES);\
	do \
		$(INS) -f $(INSDIR) $$i ; \
	done
clean:
	rm -f *.o 

clobber: clean
	rm -f hcomp $(SOURCES) $(HELPFILES) 
