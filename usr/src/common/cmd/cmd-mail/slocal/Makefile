#ident	"@(#)Makefile	11.2"

SHELL	= /bin/sh

LOCALDEF=	-DSYS5 -DSYS5DIR -DSENDMTS -DTYPESIG=int -I../sendmail/src\
		-I.. -D_ptr=__ptr -D_cnt=__cnt

LIBS	=	

include $(CMDRULES)

LDLIBS	=	-L$(USRLIB) ../c-client/libc-client.a -lgen -lsocket

DESTDIR= $(USRBIN)

build:	slocal

OBJS =	slocal.o utils.o m_getfld.o callbacks.o

slocal:	$(OBJS) ../c-client/libc-client.a
	$(CC) $(LDFLAGS) -o slocal $(OBJS) $(LDLIBS) $(LIBS)

callbacks.o:	../c-client/callbacks.c
	$(CC) $(CFLAGS) $(DEFLIST) -c $?

install: slocal
	if [ ! -d $(ETC) ] ; then mkdir -p $(ETC) ; fi
	$(INS) -f $(ETC)/mail slocal

clean:
	rm -f $(OBJS)

clobber: clean
	-rm -f slocal

local-install:
	cp slocal /etc/mail/slocal

local-remove:
	rm -f /etc/mail/slocal
