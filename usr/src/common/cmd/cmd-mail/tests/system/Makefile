#ident	"@(#)Makefile	11.2"

SHELL =		/bin/sh

all:		mailgen mailread pty killer

mailgen:	mailgen.c
		cc -o mailgen mailgen.c -lsocket -lx

mailread:	mailread.c
		cc -o mailread mailread.c -lsocket

pty:		pty.c
		cc -o pty pty.c -lx

killer:		killer.c
		cc -DMAILSPOOL=\"/var/mail\" -I../.. -o killer killer.c \
		../../c-client/callbacks.c ../../c-client/libc-client.a \
		-lsocket -lgen

clean:

clobber:	clean
		rm -f mailgen mailread pty killer callbacks.o killer.o
