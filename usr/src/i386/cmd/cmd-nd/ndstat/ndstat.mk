#ident "@(#)ndstat.mk	5.1"
#ident "$Header$"

#
#	Copyright (C) The Santa Cruz Operation, 1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(CMDRULES)

MAKEFILE = ndstat.mk
DEVINC1 = -I$(ROOT)/usr/src/$(CPU)/uts/io/nd
LOCALDEF = -DUNIXWARE -D_KMEMUSER
LDLIBS = -lelf
INTL_DIR=../intl
INTL_OBJ=$(INTL_DIR)/intl.o
OBJS =	stats.o macioc.o nd.o hwdep.o sapstats.o srstat.o
LOBJS =	stats.L macioc.L nd.L hwdep.L sapstats.L srstat.L
TARGET = ndstat

.c.L:
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all: $(TARGET)

lintit: $(LOBJS)

$(INTL_OBJ):
	if [ -d $(INTL_DIR) ]; then \
	(cd $(INTL_DIR); echo "=== $(MAKE) -f intl.mk all"; \
	 $(MAKE) -f intl.mk all $(MAKEARGS)); \
	fi;

$(TARGET): $(INTL_OBJ) $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(INTL_OBJ) $(LDLIBS)

install: all

forum:
	cp $(TARGET) $(FORUMDIR)

clean:
	rm -f $(OBJS) $(LOBJS) tags

clobber: clean
	rm -f $(TARGET)
