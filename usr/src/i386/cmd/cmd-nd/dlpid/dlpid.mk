#ident "@(#)dlpid.mk	22.1"
#ident "$Header$"

#
#	Copyright (C) The Santa Cruz Operation, 1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(CMDRULES)

# because our signal handler calls multiple stdio routines (boo hiss), 
# use re-entrant libc rather than fix each occurance.
# Note that acomp automatically adds -D_REENTRANT when you use -Kthread
DEFLIST_THREAD = -Kthread
MAKEFILE = dlpid.mk
DRVDIR = $(ROOT)/usr/src/$(CPU)/uts/io/nd
DEVINC1 = -I$(DRVDIR)
DEBUG = -DDEBUG -DDEBUG_PIPE
LOCALDEF = -DUNIXWARE $(DEFLIST_THREAD) $(DEBUG)

DLPID_OBJS = main.o pipe.o if.o error.o
LOBJS = main.L pipe.L if.L error.L
INTL_DIR = ../intl
INTL_OBJ = $(INTL_DIR)/intl.o
OBJS = $(DLPID_OBJS)
TARGET = dlpid

.SUFFIXES:.c .i .L

.c.i:
	$(CC) $(CFLAGS) -P $<

.c.L:
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all: $(TARGET)

lintit: $(LOBJS)

clean:
	rm -f $(OBJS) $(LOBJS) tags

clobber: clean
	rm -f $(TARGET) Log dlpidPIPE tags

$(INTL_OBJ):
	if [ -d $(INTL_DIR) ]; then \
	(cd $(INTL_DIR); echo "=== $(MAKE) -f intl.mk all"; \
	 $(MAKE) -f intl.mk all $(MAKEARGS)); \
	fi;

$(TARGET): $(INTL_OBJ) $(DLPID_OBJS)
	$(CC) $(CFLAGS) $(DEFLIST_THREAD) -o $(TARGET) $(DLPID_OBJS) $(INTL_OBJ) -lthread

install: all

forum:
	[ -d $(FORUMDIR) ] || mkdir -p $(FORUMDIR)
	cp -f $(TARGET) $(FORUMDIR)
	cp -f nd $(FORUMDIR)/nd

