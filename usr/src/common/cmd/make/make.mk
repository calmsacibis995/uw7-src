#ident	"@(#)make:make.mk	1.20.1.5"
########
#
#	MAKE MAKEFILE
#
########

include $(CMDRULES)

AUX_CLEAN = 
CC_CMD = $(CC) -c $(PPDEFS) $(CFLAGS) $(INC_LIST) $(LINK_MODE)
CMDBASE=..
INS=$(CMDBASE)/install/install.sh
INSDIR=$(CCSBIN)
DEFS = 
PPDEFS = $(DEFS)
SGSBASE=../sgs

OWN= bin
GRP= bin

INC_LIST	=\
	-I. -I$(SGSBASE)/inc/common -I$(SGSBASE)/inc/$(CPU)


CLEAN =\
	doname.o\
	dosys.o\
	dyndep.o\
	files.o\
	gram.c\
	gram.o\
	main.o\
	misc.o\
	parallel.o\
	rules.o

PRODUCTS = make 

HELPLIB = $(CCSLIB)/help
UW_HELPLIB = $(UW_CCSLIB)/help
OSR5_HELPLIB = $(OSR5_CCSLIB)/help
TESTDIR = .
YACC_CMD = $(YACC)
MKLIB =

OBJECTS = main.o doname.o misc.o files.o rules.o dosys.o \
	gram.o dyndep.o parallel.o

SOURCES = main.c doname.c misc.c files.c rules.c dosys.c \
	gram.y dyndep.c parallel.c  make.mk

all:	make 
	@echo "make is up to date"

make:	$(OBJECTS)
	$(CC) $(LINK_MODE) $(LDFLAGS) $(OBJECTS) $(MKLIB) $(LIBSGS) -o $(TESTDIR)/make


install:	all
	cp make	make.bak
	$(STRIP) make
	# remove make script installed by compatibility build
	-cp $(INSDIR)/make $(INSDIR)/NEWmake
	cp make $(INSDIR)/NEWmake
	-mv $(INSDIR)/make $(INSDIR)/OLDmake
	mv $(INSDIR)/NEWmake $(INSDIR)/make
	-rm -f $(INSDIR)/OLDmake
	mv make.bak make
	/bin/sh $(INS) -f $(UW_CCSBIN) -m 555 -u $(OWN) -g $(GRP) make
	/bin/sh $(INS) -f $(OSR5_CCSBIN) -m 555 -u $(OWN) -g $(GRP) make
	[ -d $(HELPLIB) ] || mkdir -p $(HELPLIB)
	-chmod 775 $(HELPLIB)
	/bin/sh $(INS) -f $(HELPLIB) -g $(GRP) -u $(OWN) -m 0444 bu 
	[ -d $(UW_HELPLIB) ] || mkdir -p $(UW_HELPLIB)
	-chmod 775 $(UW_HELPLIB)
	/bin/sh $(INS) -f $(UW_HELPLIB) -g $(GRP) -u $(OWN) -m 0444 bu 
	[ -d $(OSR5_HELPLIB) ] || mkdir -p $(OSR5_HELPLIB)
	-chmod 775 $(OSR5_HELPLIB)
	/bin/sh $(INS) -f $(OSR5_HELPLIB) -g $(GRP) -u $(OWN) -m 0444 bu 


########
#
#	All dependencies and rules not explicitly stated
#	(including header and nested header dependencies)
#
########

doname.o:	defs
doname.o:	doname.c
doname.o:	$(INC)/errno.h
doname.o:	$(INC)/stdio.h # nested include from defs
doname.o:	$(INC)/sys/errno.h # nested include from errno.h
doname.o:	$(INC)/sys/stat.h
doname.o:	$(INC)/sys/types.h
doname.o:	$(INC)/time.h
	$(CC_CMD) doname.c

dosys.o:	defs
dosys.o:	dosys.c
dosys.o:	$(INC)/stdio.h # nested include from defs
dosys.o:	$(INC)/sys/types.h
	if [ X$(NATIVE) = Xno ] ;\
	then \
		$(CC_CMD) -DMAKE_SHELL="\"/bin/sh\"" dosys.c ;\
	else \
		$(CC_CMD) dosys.c ;\
	fi


dyndep.o:	defs
dyndep.o:	dyndep.c
dyndep.o:	$(INC)/stdio.h # nested include from defs
	$(CC_CMD) dyndep.c

files.o:	$(INC)/ar.h
files.o:	defs
files.o:	files.c
files.o:	$(INC)/pwd.h
files.o:	$(INC)/stdio.h # nested include from defs
files.o:	$(INC)/sys/stat.h
files.o:	$(INC)/sys/types.h
	$(CC_CMD) files.c

gram.c:	gram.y
gram.c: defs
	$(YACC_CMD) gram.y
	$(MV) y.tab.c gram.c

gram.o:	$(INC)/ctype.h
gram.o:	gram.c
gram.o:	$(INC)/stdio.h # nested include from defs
	$(CC_CMD) gram.c

main.o:	defs
main.o:	main.c
main.o:	$(INC)/signal.h
main.o:	$(INC)/stdio.h # nested include from defs
main.o:	$(INC)/sys/signal.h # nested include from signal.h
main.o:	$(INC)/time.h
	$(CC_CMD) main.c

misc.o:	$(INC)/ctype.h
misc.o:	defs
misc.o:	$(INC)/errno.h
misc.o:	misc.c
misc.o:	$(INC)/signal.h
misc.o:	$(INC)/stdio.h # nested include from defs
misc.o:	$(INC)/sys/errno.h # nested include from errno.h
misc.o:	$(INC)/sys/signal.h # nested include from signal.h
misc.o:	$(INC)/sys/stat.h
misc.o:	$(INC)/sys/types.h
	$(CC_CMD) misc.c

parallel.o:	defs
parallel.o:	parallel.c
	$(CC_CMD) parallel.c

rules.o:	rules.c
rules.o:	$(INC)/stdio.h # nested include from defs
	$(CC_CMD) rules.c

########
#
#	Standard Targets
#
#	all		builds all the products specified by PRODUCTS
#	install		installs products; user defined in make.lo 
#	clean		removes all temporary files (ex. installable object)
#	clobber		"cleans", and then removes $(PRODUCTS)
#	remove		remove all installed files
#	product		lists all files installed
#	productdir	lists all required directories for this component
#	partslist	produces a list of all source files used in the
#			  construction of the component (including makefiles)
#	strip		allows for the stripping of the final executable
#	makefile	regenerates makefile
#
########

install: 	# rules, if any, specified above

clean:
		-rm -f $(CLEAN) $(AUX_CLEAN)

clobber:	clean
		-rm -f $(PRODUCTS)

partslist:
		@echo $(SOURCES)

product:
		@echo $(PRODUCTS)

strip:

makefile:	$(MAKE.LO) $(MAKE.ROOT)
		$(MKGEN) >make.out
		if [ -s make.out ]; then mv make.out makefile; fi

makefile_all:	makefile

save:
	cp $(CCSBIN)/make $(CCSBIN)/sv.make
