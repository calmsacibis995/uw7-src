#ident	"@(#)ihvkit:net/dlpi_ether_2.x/el16.mk	1.1"
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

MAKEFILE= el16.mk
INC = /usr/include
INCLIST = -I$(INC) -I$(INC)/sys
INS = install
OWN = bin
GRP = bin
HINSPERM = -m 644 -u $(OWN) -g $(GRP)
INSPERM = -m 644 -u root -g sys

GLOBALDEF = -D_KERNEL 
LOCALDEF = -DEL16 -DESMP -DALLOW_SET_EADDR

DEFLIST = \
	$(GLOBALDEF) \
	$(LOCALDEF)

ETC = /etc
CONF = $(ETC)/conf
CONFBIN = $(CONF)/bin
IDINSTALL = $(CONFBIN)/idinstall
IDBUILD = $(CONFBIN)/idbuild
EL16 = el16.cf/Driver.o
DRIVER = el16

CFGDIR = $(DRIVER).cf

LINT = /usr/bin/lint
LINTFLAGS = -k -n -s
LINTDIR = ./lintdir
LFILE = $(LINTDIR)/el16.ln

FILES = \
	dlpi_el16.o \
	el16hrdw.o \
	el16init.o

CFILES = \
	el16hrdw.c \
	el16init.c

SRCFILES = $(CFILES) dlpi_ether.c

LFILES = \
	el16hrdw.ln \
	el16init.ln \
	dlpi_el16.ln


.SUFFIXES: .ln

.c.ln:
	echo "\n$(DIR)/$*.c:" > $*.L
	-$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) \
		-DEL16 -c -u $*.c >> $*.L


install: headinstall all
	( cd $(CFGDIR); \
	$(IDINSTALL) -R$(CONF) -M $(DRIVER); \
	$(IDBUILD)  -M $(DRIVER); \
	)

all: $(EL16)

$(EL16): $(FILES)
	$(LD) -r -o $(EL16) $(FILES)

clean:
	-rm -f *.o $(EL16)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d $(DRIVER)

$(LINTDIR):
	-mkdir -p $@

lintit: $(LFILE)
	-rm -f dlpi_el16.c

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done


fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

#
# Header Install Section
#

HEADERS= \
	dlpi_el16.h \
	el16.h

headinstall: $(HEADERS)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(HEADERS); \
	 do \
	    $(INS) -f $(INC)/sys $(HINSPERM) $$f; \
	 done

# Special header dependencies
#
dlpi_el16.o: dlpi_ether.c \
	$(INC)/sys/dlpi_el16.h \
	$(INC)/sys/el16.h
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c dlpi_ether.c && \
		mv dlpi_ether.o dlpi_el16.o

el16hrdw.o: el16hrdw.c \
	$(INC)/sys/dlpi_el16.h \
	$(INC)/sys/el16.h
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c el16hrdw.c
 
el16init.o: el16init.c \
	$(INC)/sys/dlpi_el16.h \
	$(INC)/sys/el16.h
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c el16init.c
