#ident	"@(#)yacc:i386/makefile	1.14"

include $(CMDRULES)


INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

SOURCES=$(COMDIR)/y1.c $(COMDIR)/y2.c $(COMDIR)/y3.c $(COMDIR)/y4.c
OBJECTS=y1.o y2.o y3.o y4.o
PRODUCTS=yacc

#.MUTEX: $(OBJECTS)
all:	$(PRODUCTS)

yacc:	$(OBJECTS)
	$(CC) $(OBJECTS) $(LINK_MODE) $(LDLIBS) $(LIBSGS) -o yacc 

$(OBJECTS):	$(COMDIR)/dextern 

y1.o: $(COMDIR)/y1.c
	$(CC) -c $(CFLAGS) -DPARSER=\"$(DFLTCLIB)/yaccpar\" -I$(INC) -I$(CPUINC) $(COMDIR)/y1.c

y2.o: $(COMDIR)/y2.c
	$(CC) -c $(CFLAGS) -DPARSER=\"$(DFLTCLIB)/yaccpar\" -I$(INC) -I$(CPUINC) $(COMDIR)/y2.c

y3.o: $(COMDIR)/y3.c
	$(CC) -c $(CFLAGS) -DPARSER=\"$(DFLTCLIB)/yaccpar\" -I$(INC) -I$(CPUINC) $(COMDIR)/y3.c

y4.o: $(COMDIR)/y4.c
	$(CC) -c $(CFLAGS) -DPARSER=\"$(DFLTCLIB)/yaccpar\" -I$(INC) -I$(CPUINC) $(COMDIR)/y4.c

install:	all
		cp yacc yacc.bak
		$(STRIP) yacc
		/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)yacc yacc
		mv yacc.bak yacc
		/bin/sh $(INS) 644 $(OWN) $(GRP) $(CCSLIB)/yaccpar $(COMDIR)/yaccpar

lintit:	$(SOURCES)	
	$(LINT) $(LINTFLAGS) -I$(INC) -I$(CPUINC) $(SOURCES)

clean:
		-rm -f $(OBJECTS)

clobber:	clean
		-rm -f $(PRODUCTS)
