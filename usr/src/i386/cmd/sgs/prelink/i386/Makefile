#ident	"@(#)prelink:i386/Makefile	1.10"

#
#	Prelinker demangler makefile
#
include $(CMDRULES)

LINTFLAGS=
LINK_MODE=

LDLIBS=

INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSLIB)

HFILES=$(COMDIR)/edg_prelink.h

SOURCES=$(COMDIR)/edg_prelink.c
OBJECTS=edg_prelink.o
PRODUCTS=prelink
PROBEFILE=$(COMDIR)/edg_prelink.c

DEFLIST = -D__SYSV__

# We have to specify the cplusfe directories this way so the we get 
# the right thing whether $(COMDIR) and $(CPUDIR) are specified as
# relative or absolute pathnames.
INCLIST=-I. -I$(COMDIR) -I$(CPUINC) -I$(COMINC) -I$(CPUDIR)/../../cplusfe/$(CPU) -I$(COMDIR)/../../cplusfe/common
CC_CMD=$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST)

all:
	if [ -f $(PROBEFILE) ]; \
	then \
		$(MAKE) $(PRODUCTS) LIBELF=$(LIBELF) $(MAKEARGS); \
	else \
		if test "$(NATIVE)" = "yes"; \
		then \
			dir=native; \
		else \
			dir=cross; \
		fi; \
		for file in $(PRODUCTS); \
		do \
			rm -f $$file; \
			if [ -r $$dir/$$file ]; \
			then \
				cp $$dir/$$file $$file; \
			else \
				echo "ERROR: $$dir/$$file is missing" 1>&2; \
				false; \
				break; \
			fi \
		done \
	fi 

$(PRODUCTS): 	$(OBJECTS) $(LIBELF) $(LDLIBS)
	$(CC) $(OBJECTS) $(LIBELF) $(LDLIBS) $(LINK_MODE) $(LIBSGS) -o $(PRODUCTS)
	if test "$(NATIVE)" = "yes"; \
	then \
		rm -rf native; \
		mkdir native; \
		cp $(PRODUCTS) native; \
	else \
		rm -rf cross; \
		mkdir cross; \
		cp $(PRODUCTS) cross; \
	fi

edg_prelink.o:	$(HFILES) $(COMDIR)/edg_prelink.c
	$(CC_CMD) -c $(COMDIR)/edg_prelink.c

install : all
	$(MV) $(PRODUCTS) $(PRODUCTS).bak
	$(CP) $(PRODUCTS).bak $(PRODUCTS)
	$(STRIP) $(PRODUCTS)
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)$(PRODUCTS) $(PRODUCTS)
	$(MV) $(PRODUCTS).bak $(PRODUCTS)

lintit:	$(HFILES) $(SOURCES)
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $(SOURCES)

clean:
	-rm -f $(OBJECTS)

shrink clobber: clean
	rm -f $(PRODUCTS);
