# Copyright (c) 1998 The Santa Cruz Operation, Inc.. All Rights Reserved. 
#                                                                         
#        THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF THE               
#                   SANTA CRUZ OPERATION INC.                             
#                                                                         
#   The copyright notice above does not evidence any actual or intended   
#   publication of such source code.                                      

# Copyright (c) 1998 The Santa Cruz Operation, Inc.. All Rights Reserved. 
#                                                                         
#        THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF THE               
#                   SANTA CRUZ OPERATION INC.                             
#                                                                         
#   The copyright notice above does not evidence any actual or intended   
#   publication of such source code.                                      

#ident "@(#)Makefile	1.7"
#ident "$Header$"

include $(CMDRULES)

RES		= ../res/libresolv.a
LDLIBS		= -lsocket -lnsl
PIDDIR = /etc/inet
INDOT = in.
PS = ps -p
IOT = IOT
INSDIR = $(USRSBIN)
INSDIR_NAMED_D = $(ETC)/inet/named.d
OWN = bin
GRP = bin

LOCALDEF =  	-DSVR4 -DUSE_POSIX -DBSD
LOCALINC =	-I../include

OBJS =		db_dump.o db_load.o db_lookup.o db_reload.o db_save.o db_update.o \
		db_secure.o db_glue.o ns_forw.o ns_init.o ns_main.o ns_maint.o \
		ns_req.o ns_resp.o ns_sort.o ns_stats.o ns_validate.o ns_ncache.o \
		storage.o tree.o ns_udp.o version.o

XFEROBJ =	named-xfer.o db_glue.o storage.o version.o
HDRS =		db_defs.h db_func.h db_glob.h named.h ns_defs.h ns_func.h ns_glob.h \
		pathnames.h tree.h

PRODUCTS = in.named named-xfer ndc

all : $(PRODUCTS)

install: all
	@for i in $(PRODUCTS);\
	do\
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $$i;\
	done
	@-[ -d $(INSDIR_NAMED_D) ] || mkdir -p $(INSDIR_NAMED_D)
	$(INS) -f $(INSDIR_NAMED_D) -m 0644 -u $(OWN) -g $(GRP) db.cache

version.c : Version.c
	cp Version.c version.c

in.named: $(OBJS) $(RES) $(COMPLIB) version.o
	$(CC) $(CFLAGS) $(INCLIST) -o in.named version.o $(OBJS) \
		$(RES) $(COMPLIB) $(LDLIBS)

named-xfer:	$(XFEROBJ) $(RES) $(COMPLIB)
	$(CC) $(CDEBUG) $(LDFLAGS) -o $@ $(XFEROBJ) \
		$(RES) $(COMPLIB) $(LDLIBS)

ndc: ndc.sh
	sed -e "s|%PIDDIR%|${PIDDIR}|" \
	    -e "s|%INDOT%|${INDOT}|" \
	    -e "s|%PS%|${PS}|" \
	    -e "s|%IOTPS%|${IOT}|" \
	    -e "s|%DESTSBIN%|${DESTSBIN}|" \
	    -e "s|%IOT%|${IOT}|" \
	    < ndc.sh > ndc
	chmod +x ndc

clean : 
	rm -f *.o version.c

clobber : clean
	rm -f in.named named-xfer named.reload named.restart ndc

$(OBJS) : $(HDRS)
