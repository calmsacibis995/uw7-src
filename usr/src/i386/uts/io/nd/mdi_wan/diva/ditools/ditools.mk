#ident "@(#)ditools.mk	29.1"
#
#   Name : ditools.gemini.M
#
#   COPYRIGHT NOTICE:
#
#   Copyright (C) Eicon Technology Corporation, 1993.
#   This module contains Proprietary Information of
#   Eicon Technology Corporation, and should be treated
#   as confidential.
#
#   Description :
#   Gemini Makefile for ditools component

include $(UTSRULES)

# Directories of interest:

MAKEFILE = ditools.mk
SCP_SRC = ctable.c
KBASE = ../../../../..
# DEBUG = -DDEBUG
LOCALDEF = -DUNIX
EXE = ctable
OBJ = ctable.o
LOBJS = ctable.L

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@


all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(EXE) $(MAKEARGS); \
	fi

$(EXE): $(OBJ)
	$(CC) -o $@ $(OBJ)   


lintit: $(LOBJS)

install:

clean:
	rm -f $(OBJ) $(LOBJS) tags

clobber: clean
	rm -f $(EXE)


