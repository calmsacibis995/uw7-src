#ident	"@(#)unixtsa.mk	1.2"
###
#
#  name		unixtsa.mk - build unix TSA
#		@(#)unixtsa.mk	1.2	10/9/96
#
###

TOP=.
include config.mk

SUBDIRS = \
	catalogs \
	tsalib \
	tsaunix \
	tsad

all install clean :
	Dir=`pwd`; $(MAKE) MTARG=$@ $(MAKEARGS) -f unixtsa.mk do_subdirs

 
force :
	touch */*.C
	$(MAKE) $(MAKEARGS) -f unixtsa.mk all


clobber :
	Dir=`pwd`; $(MAKE) MTARG=$@ $(MAKEARGS) -f unixtsa.mk do_subdirs
	rm -rf bin lib
