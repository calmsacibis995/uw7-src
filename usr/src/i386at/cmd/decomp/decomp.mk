#ident	"@(#)decomp:decomp.mk	1.4.1.3"

# decomp.mk -- make a decompression dynamic shared library
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
# 
# You may contact UNIX System Laboratories by writing to
# UNIX System Laboratories, 190 River Road, Summit, NJ 07901, USA

include $(CMDRULES)

INSDIR = $(USRSADM)/install/bin
OWN=bin
GRP=bin

MAINS = libdecomp.so

OBJECTS = gzip.o util.o inflate.o munzip.o

SOURCES = gzip.c util.c inflate.c munzip.c gzip.h

LOCALDEF = -DUSER_LEVEL

LDFLAGS = -G

all:	$(MAINS)

libdecomp.so:	$(OBJECTS)
	$(CC) -o libdecomp.so $(OBJECTS) $(LDFLAGS) $(LDLIBS)

gzip.o: gzip.c \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	gzip.h

munzip.o: munzip.c \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	gzip.h

util.o: util.c \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	gzip.h

inflate.o: inflate.c \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	gzip.h

#
# Files are now local to the decomp component rather than sym links 
#
# $(SOURCES):
#	@ln -s $(ROOT)/usr/src/$(WORK)/boot/tools/zip/$@ $@

install: all
	@if [ ! -d $(INSDIR) ] ;\
	then \
		mkdir -p $(INSDIR) ;\
	fi
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) libdecomp.so

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

size: all
	$(SIZE) $(MAINS)

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(INSDIR) | tr ' ' '\012' | sort

product:
	@echo $(MAINS) | tr ' ' '\012' | \
	sed 's;^;$(INSDIR)/;'
