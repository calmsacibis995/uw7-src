#	"@(#)aas.mk	1.3"	
#
# Copyrighted as an unpublished work.
# (c) Copyright 1987-1996 Computer Associates International, Inc.
# All rights reserved.
#

include $(CMDRULES)

INSDIR= $(USRSBIN)
INSFLAGS= -f

LDFLAGS = -s
LDLIBS	= -laas -lsocket -lnsl

OBJS	= \
	aas_pool_query.o \
	aas_addr_query.o \
	aas_release.o \
	aas_disable.o \
	aas_reconfig.o \
	aas_client_query.o \
	aas_alloc.o \
	aas_get_server.o \
	util.o

SRCS= $(OBJS:.o=.c)

all: aas_pool_query aas_addr_query aas_release aas_release_all aas_disable \
	aas_reconfig aas_client_query aas_alloc aas_get_server

aas_pool_query: aas_pool_query.o util.o
	$(CC) -o $@ aas_pool_query.o util.o $(LDFLAGS) $(LDLIBS)

aas_addr_query: aas_addr_query.o util.o
	$(CC) -o $@ aas_addr_query.o util.o $(LDFLAGS) $(LDLIBS)

aas_release: aas_release.o util.o
	$(CC) -o $@ aas_release.o util.o $(LDFLAGS) $(LDLIBS)

aas_release_all: aas_release_all.o util.o
	$(CC) -o $@ aas_release_all.o util.o $(LDFLAGS) $(LDLIBS)

aas_disable: aas_disable.o util.o
	$(CC) -o $@ aas_disable.o util.o $(LDFLAGS) $(LDLIBS)

aas_reconfig: aas_reconfig.o util.o
	$(CC) -o $@ aas_reconfig.o util.o $(LDFLAGS) $(LDLIBS)

aas_client_query: aas_client_query.o util.o
	$(CC) -o $@ aas_client_query.o util.o $(LDFLAGS) $(LDLIBS)

aas_alloc: aas_alloc.o util.o
	$(CC) -o $@ aas_alloc.o util.o $(LDFLAGS) $(LDLIBS)

aas_get_server: aas_get_server.o util.o
	$(CC) -o $@ aas_get_server.o util.o $(LDFLAGS) $(LDLIBS)

install: all
	$(INS) $(INSFLAGS) $(INSDIR) -m 0755 -u $(OWN) -g $(GRP) aas_pool_query
	$(INS) $(INSFLAGS) $(INSDIR) -m 0755 -u $(OWN) -g $(GRP) aas_addr_query
	$(INS) $(INSFLAGS) $(INSDIR) -m 0755 -u $(OWN) -g $(GRP) aas_release
	$(INS) $(INSFLAGS) $(INSDIR) -m 0755 -u $(OWN) -g $(GRP) aas_release_all
	$(INS) $(INSFLAGS) $(INSDIR) -m 0755 -u $(OWN) -g $(GRP) aas_disable
	$(INS) $(INSFLAGS) $(INSDIR) -m 0755 -u $(OWN) -g $(GRP) aas_reconfig
	$(INS) $(INSFLAGS) $(INSDIR) -m 0755 -u $(OWN) -g $(GRP) aas_client_query
	$(INS) $(INSFLAGS) $(INSDIR) -m 0755 -u $(OWN) -g $(GRP) aas_alloc
	$(INS) $(INSFLAGS) $(INSDIR) -m 0755 -u $(OWN) -g $(GRP) aas_get_server

clean:
	rm -f *.o 

clobber: clean
	rm -f aas_pool_query aas_addr_query \
		aas_release aas_release_all \
		aas_disable aas_reconfig \
		aas_client_query aas_alloc \
		aas_get_server

depend:
	${MKDEPEND} -f *.mk -- ${CFLAGS} *.c
