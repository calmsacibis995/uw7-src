#ident	"@(#)ctrace:i386/makefile	1.10"
#	ctrace makefile

include $(CMDRULES)

# installation directories:
INSDIR=$(CCSBIN)
CTLIB=$(CCSLIB)/ctrace
CRLIB=/usr/ccs/lib/ctrace

DEFLIST=
DFLT_RUNTIME = -DRUNTIME=\"$(CRLIB)/runtime.c\"
COMPAT_RUNTIME = -DRUNTIME=\"$(ALT_PREFIX)$(CRLIB)/runtime.c\"
COMDIR=../common
CPUINC=../../sgs/inc/$(CPU)
INCLIST=-I . -I $(CPUINC) -I $(COMDIR)
CC_CMD = $(CC) -c $(CFLAGS) $(DEFLIST) -I$(INC)
INS=../../install/install.sh
LIBELF=
LINK_MODE=

# Must derive y.tab.h from acgram.y
# add -t for yacc debug (YYDEBUG)
# add -v for yacc y.output file
YFLAGS=
YYDEBUG=
YACC_CMD=       $(YACC) $(YFLAGS) -d

LFLAGS=

LINTFLAGS=-b

SOURCE	 = $(COMDIR)/constants.h $(COMDIR)/global.h $(COMDIR)/main.c \
	   $(COMDIR)/parser.y $(COMDIR)/scanner.l $(COMDIR)/lookup.c \
	   $(COMDIR)/trace.c ./runtime.c $(COMDIR)/ctcr
CFILES =   $(COMDIR)/main.c parser.c scanner.c \
	   $(COMDIR)/lookup.c $(COMDIR)/trace.c
HFILE =    $(COMDIR)/constants.h $(COMDIR)/global.h
OBJECTS =  main.O parser.O scanner.O lookup.O trace.O
COMPAT_OBJECTS =  compat_main.O parser.O scanner.O lookup.O trace.O
PRODUCT=ctrace
COMPAT_PRODUCT=compat_ctrace

.MUTEX:		parser.h $(OBJECTS) $(COMPAT_OBJECTS)

all:	$(PRODUCT) $(COMPAT_PRODUCT)

$(PRODUCT):	parser.h $(OBJECTS:O=o)
		$(CC) $(CFLAGS) $(OBJECTS:O=o) $(LINK_MODE) -o $(PRODUCT)

$(COMPAT_PRODUCT):	parser.h $(COMPAT_OBJECTS:O=o)
		$(CC) $(CFLAGS) $(COMPAT_OBJECTS:O=o) $(LINK_MODE) -o $(COMPAT_PRODUCT)

main.o:		$(COMDIR)/main.c $(HFILE) $(CPUINC)/sgs.h
		$(CC_CMD) $(DFLT_RUNTIME) '-DTRACEVERS="01.01"' $(INCLIST) $(COMDIR)/main.c

compat_main.o:		$(COMDIR)/main.c $(HFILE) $(CPUINC)/sgs.h
		$(CC_CMD) $(COMPAT_RUNTIME) '-DTRACEVERS="01.01"' -Wa,"-ocompat_main.o" $(INCLIST) $(COMDIR)/main.c

parser.c:	$(COMDIR)/parser.y $(HFILE)
		$(YACC_CMD) $(COMDIR)/parser.y
		mv y.tab.c parser.c
		if cmp -s y.tab.h parser.h; then rm y.tab.h; \
		else cp y.tab.h parser.h; fi

parser.h:	parser.c

scanner.c:	$(COMDIR)/scanner.l
		$(LEX) $(LFLAGS) $(COMDIR)/scanner.l
		mv lex.yy.c scanner.c

parser.o:	parser.c
		$(CC_CMD) $(INCLIST) $(YYDEBUG) parser.c

scanner.o:	parser.h scanner.c
		$(CC_CMD) $(INCLIST) scanner.c	

lookup.o:	parser.h $(COMDIR)/lookup.c
		$(CC_CMD) $(INCLIST) $(COMDIR)/lookup.c

trace.o:	$(COMDIR)/trace.c
		$(CC_CMD) $(INCLIST) $(COMDIR)/trace.c
	
install: 	all
		cp ctrace ctrace.bak
		cp compat_ctrace compat_ctrace.bak
		rm -f ctcr
		cp $(COMDIR)/ctcr ctcr
		$(STRIP) ctrace
		$(STRIP) compat_ctrace
		/bin/sh $(INS) -f $(INSDIR) ctrace
		mv compat_ctrace ctrace
		/bin/sh $(INS) -f $(UW_CCSBIN) ctrace
		/bin/sh $(INS) -f $(OSR5_CCSBIN) ctrace
		/bin/sh $(INS) -f $(INSDIR) ctcr
		/bin/sh $(INS) -f $(UW_CCSBIN) ctcr
		/bin/sh $(INS) -f $(OSR5_CCSBIN) ctcr
		rm -f $(INSDIR)/ctc
		ln $(INSDIR)/ctcr $(INSDIR)/ctc
		rm -f $(UW_CCSBIN)/ctc
		ln $(UW_CCSBIN)/ctcr $(UW_CCSBIN)/ctc
		rm -f $(OSR5_CCSBIN)/ctc
		ln $(OSR5_CCSBIN)/ctcr $(OSR5_CCSBIN)/ctc
		if [ ! -d $(CTLIB) ] ;\
		then \
			mkdir $(CTLIB);\
		fi
		/bin/sh $(INS) -f $(CTLIB) runtime.c
		if [ ! -d $(UW_CCSLIB)/ctrace ] ;\
		then \
			mkdir $(UW_CCSLIB)/ctrace ;\
		fi
		/bin/sh $(INS) -f $(UW_CCSLIB)/ctrace runtime.c
		if [ ! -d $(OSR5_CCSLIB)/ctrace ] ;\
		then \
			mkdir $(OSR5_CCSLIB)/ctrace ;\
		fi
		/bin/sh $(INS) -f $(OSR5_CCSLIB)/ctrace runtime.c
		mv ctrace.bak ctrace
		mv compat_ctrace.bak compat_ctrace
		rm -f ctcr

clean:
		rm -f *.o y.output
		rm -f lint.out
		rm -f *.ln

clobber: 	clean
		rm -f ctrace compat_ctrace parser.[ch] scanner.c y.tab.h

lintit:		$(CFILES)
		$(LINT) $(LINTFLAGS) $(INCLIST) -I$(INC) $(CFILES)

ctrace.ln:	$(CFILES)
		rm -f $(OBJECTS:O=ln)
		$(LINT) $(LINTFLAGS) -c $(INCLIST) -I$(INC) $(CFILES)
		cat $(OBJECTS:O=ln) >ctrace.ln
