#ident	"@(#)Makefile	11.1"

SHELL =		/bin/sh

LOCALDEF =	 -DSCO -I../..

include		$(CMDRULES)

LIB =		

LDLIBS =	-L$(USRLIB) $(LIB) -lgen -lsocket

TARGET =	socket

OBJS =		socket.o

$(TARGET):	$(OBJS) $(LIB)
		$(CC) $(CFLAGS) $(DEFLIST) -o $(TARGET) $(OBJS) $(LDFLAGS) $(LDLIBS)

callbacks.o:	../../c-client/callbacks.c
		$(CC) $(CFLAGS) $(DEFLIST) -c $?

clean:
		rm -f $(OBJS)

clobber:	clean
		rm -f $(TARGET)
