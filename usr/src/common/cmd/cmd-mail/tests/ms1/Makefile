#ident	"@(#)Makefile	11.2"

SHELL =		/bin/sh

LOCALDEF =	-I../..

include		$(CMDRULES)

LIB =		../../c-client/libc-client.a

LDLIBS =	-L$(USRLIB) $(LIB) -lgen -lsocket

TARGET =	ms1tb

OBJS =		ms1tb.o callbacks.o

$(TARGET):	$(OBJS) $(LIB)
		$(CC) $(CFLAGS) $(DEFLIST) -o $(TARGET) $(OBJS) $(LDFLAGS) $(LDLIBS)

callbacks.o:	../../c-client/callbacks.c
		$(CC) $(CFLAGS) $(DEFLIST) -c $?

clean:
		rm -f $(OBJS)

clobber:	clean
		rm -f $(TARGET)
