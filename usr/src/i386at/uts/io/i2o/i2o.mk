#ident  "%W%"

#
#	L001	16Feb1999	tonylo
#	- If no header files can be found in the i2osig directory
#	  then assume this is a SCP build and do not install 
#	  header files into the cross environment

include $(UTSRULES)

MAKEFILE=	i2o.mk
KBASE = ../..

SUBDIRS = osm txport msg ptosm
LINCDIRS = inc

all:	FRC
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk all"; \
		 $(MAKE) -f $$d.mk all $(MAKEARGS)); \
		fi; \
	 done

install: FRC
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk install"; \
		 $(MAKE) -f $$d.mk install $(MAKEARGS)); \
		fi; \
	 done

clean:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clean"; \
		 $(MAKE) -f $$d.mk clean $(MAKEARGS)); \
		fi; \
	 done

clobber:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; \
		 $(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
		fi; \
	 done


lintit:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk lintit"; \
		 $(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
		fi; \
	 done

fnames:
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
	    (cd $$d; \
		$(MAKE) -f $$d.mk fnames $(MAKEARGS) | \
		$(SED) -e "s;^;$$d/;"); \
		fi; \
	done

FRC:

#L001
RESTRICTEDHEADERS = `ls inc/i2osig/*.h 2>/dev/null`

headinstall:
	-@if [ -z "$(RESTRICTEDHEADERS)" ]; then \
		echo "Assuming SCP build: do not attempt to install restricted header files"; \
	else \
		for d in $(SUBDIRS) $(LINCDIRS); do \
			if [ -d $$d ]; then \
			(cd $$d; echo "=== $(MAKE) -f $$d.mk headinstall"; \
			 $(MAKE) -f $$d.mk headinstall $(MAKEARGS)); \
			fi; \
		done \
	fi


include $(UTSDEPEND)

depend::
	@for d in $(SUBDIRS); do \
		if [ -d $$d ]; then \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk depend";\
		 touch $$d.mk.dep;\
		 $(MAKE) -f $$d.mk depend $(MAKEARGS));\
		fi; \
	done

include $(MAKEFILE).dep
