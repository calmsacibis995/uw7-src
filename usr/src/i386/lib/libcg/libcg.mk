#ident	"@(#)libcg:i386/lib/libcg/libcg.mk	1.3"

# Top-level makefile for libcg.
#
#

LIBCGRULES = libcg.rules

include $(LIBRULES)
include $(LIBCGRULES)

MAKEFILE = libcg.mk

LDFLAGS=-G -dy
LIBSODIR=/usr/lib
INSTALLDIR=$(USRLIB)


all: libso  

libso: 
	cd $(CPU); $(MAKE) -f archdep.mk pic
	-rm -rf object
	mkdir object
	find $(CPU) -name '*.O' -print | \
	xargs sh -sc 'ln "$$@" object'
	$(LD) $(LDFLAGS) -h $(LIBSODIR)/libcg.so.1 -o libcg.so.1 object/*.O
	-@rm -f libcg.so
	@ln libcg.so.1 libcg.so
	-@rm -rf object

libprof: 
	cd $(CPU); $(MAKE) -f archdep.mk prof
	-rm -rf object
	mkdir object
	find $(CPU) -name '*.P' -print | \
	xargs sh -sc 'ln "$$@" object'
	$(LD) $(LDFLAGS) -h $(LIBSODIR)/libcg_p.so.1 -o libcg_p.so.1 object/*.P
	-@rm -f libcg_p.so
	@ln libcg_p.so.1 libcg_p.so
	-@rm -rf object

install: all
	$(INS) -f $(INSTALLDIR) -u $(OWN) -g $(GRP) -m 755 libcg.so.1
	-rm -f $(INSTALLDIR)/libcg.so
	ln $(INSTALLDIR)/libcg.so.1 $(INSTALLDIR)/libcg.so
	if [ -f libcg_p.so.1 ]; then \
	  $(INS) -f $(INSTALLDIR) -u $(OWN) -g $(GRP) -m 755 libcg_p.so.1; \
	  rm -f $(INSTALLDIR)/libcg_p.so; \
	  ln $(INSTALLDIR)/libcg_p.so.1 $(INSTALLDIR)/libcg_p.so; \
	fi

clean:
	-cd $(CPU);  $(MAKE) -f archdep.mk clean

clobber: clean
	-rm -f libcg.so libcg_p.so libcg.so.1 libcg_p.so.1

