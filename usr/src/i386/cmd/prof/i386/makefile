#ident	"@(#)prof:i386/makefile	1.14"

include $(CMDRULES)

PROF_SAVE	= 

CMDBASE		= ../..
SGSBASE		= ../../sgs
LPBASE		= $(SGSBASE)/lprof
PLBBASE		= $(LPBASE)/libprof
COMDIR		= ../common
CPUDIR		= ../$(CPU)

CPUINC		= $(SGSBASE)/inc/$(CPU)
INS		= $(CMDBASE)/install/install.sh
INSDIR		= $(CCSBIN)

HFILES		= \
		$(CPUINC)/sgs.h \
		$(SGSBASE)/inc/common/dprof.h \
		$(CPUDIR)/machrel.h
INCDIRS		= \
		-I $(CPUDIR) \
		-I $(SGSBASE)/inc/common \
		-I $(SGSBASE)/inc/$(CPU) \
		-I $(LPBASE)/hdr \
		-I $(PLBBASE)/$(CPU) \
		-I $(PLBBASE)/common
SOURCES		= $(COMDIR)/prof.c
OBJECTS		= prof.o
PRODUCTS	= prof

LIBELF		= -lelf
PROFLIBD	= $(PLBBASE)/$(CPU)
LIBSYMINT	= -L$(PROFLIBD) -lsymint
LDFLAGS		= -s
LIBS		= $(LIBELF)

ENVPARMS	= \
	ROOT="$(ROOT)" SGS="$(SGS)" OWN="$(OWN)" GRP="$(GRP)" \
	CC="$(CC)" CFLAGS="$(CFLAGS)" LDLIBS="$(LDLIBS)" \
	LINT="$(LINT)" LINTFLAGS="$(LINTFLAGS)" INS="$(INS)" \
	INSDIR="$(INSDIR)" STRIP="$(STRIP)" LIBELF="$(LIBELF)" \
	CPU="$(CPU)" CPUINC="$(CPUINC)" \
	CCSBIN="$(CCSBIN)" CCSLIB="$(CCSLIB)" \
	LINK_MODE="$(LINK_MODE)" \
	PROF_SAVE="$(PROF_SAVE)" LIBS="$(LIBS)" 

.MUTEX: libsymint $(PRODUCTS)

all:	
	$(MAKE) profmk CPU="$(CPU)"

profmk: $(PRODUCTS)

$(PRODUCTS): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) \
	$(LDLIBS) $(LIBS) -o prof $(LINK_MODE) -lgen

prof.o:	$(HFILES) $(COMDIR)/prof.c
	$(CC) $(CFLAGS) $(INCDIRS) -c $(COMDIR)/prof.c

libsymint:
	cd $(PROFLIBD); $(MAKE) -f libprof.mk $(ENVPARMS)

install: all
	cp $(PRODUCTS) prof.bak
	$(STRIP) $(PRODUCTS)
	sh $(INS) -f $(INSDIR) $(PRODUCTS)
	sh $(INS) -f $(UW_CCSBIN) $(PRODUCTS)
	sh $(INS) -f $(OSR5_CCSBIN) $(PRODUCTS)
	mv prof.bak $(PRODUCTS)

lintit:	$(SOURCES)
	$(LINT) $(LINTFLAGS) $(SOURCES)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(PRODUCTS)

