#ident	"@(#)des.mk	1.5"
#ident	"$Header$"

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#	PROPRIETARY NOTICE (Combined)
#
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
#
#
#
#	Copyright Notice 
#
# Notice of copyright on this source code product does not indicate 
#  publication.
#
#	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#          All rights reserved.
# 
#

include $(LIBRULES)
include ../libnsl.rules

.SUFFIXES:	.c .o

LOCALDEF = $(NSL_LOCALDEF)
LOCALINC=-I./des
OBJS= des_crypt.o des_soft.o des_mt.o
INTLOBJS= intl_crypt.o intl_soft.o des_mt.o
SRCS = $(OBJS:.o=.c)
INTLSRCS = $(INTLOBJS:.o=.c)
INCLUDES=des_mt.h \
	 $(INC)/des/des.h \
	 $(INC)/des/softdes.h \
	 des/desdata.h \
	 $(INC)/des/des.h

all:	
	$(MAKE) -f des.mk intl $(MAKEARGS) NSL_LOCALDEF="$(NSL_LOCALDEF)"
	if [ -s des_crypt.c -a  -s des_soft.c ] ;\
	then \
		$(MAKE) -f des.mk usa $(MAKEARGS) \
			NSL_LOCALDEF="$(NSL_LOCALDEF)" ;\
	fi

usa: $(OBJS)
	cp des_crypt.o ../des_crypt.o_d
	cp des_soft.o ../des_soft.o_d
	cp des_mt.o ../des_mt.o_d

intl: $(INTLOBJS)
	cp intl_crypt.o ../des_crypt.o_i
	cp intl_soft.o ../des_soft.o_i
	cp des_mt.o ../des_mt.o_i

des_crypt.o intl_crypt.o: $(INC)/sys/types.h \
	$(INC)/rpc/des_crypt.h \
	$(INC)/des/des.h
 
des_soft.o intl_soft.o: $(INC)/sys/types.h \
	$(INC)/des/softdes.h \
	des/desdata.h \
	$(INC)/des/des.h 

lintit:	
	$(LINT) $(LINTFLAGS) $(INCLIST) $(DEFLIST) $(SRCS) $(INTLSRCS)

clean:
	rm -f $(OBJS) $(INTLOBJS)

clobber: clean

strip:	$(OBJS) $(INTLOBJS)
	$(STRIP) $(OBJS) $(INTLOBJS)

size:	$(OBJS) $(INTLOBJS)
	$(SIZE) $(OBJS) $(INTLOBJS)
