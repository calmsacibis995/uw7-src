#ident	"@(#)Makefile	1.2"
#ident	"$Header$"

include $(CMDRULES)

CFLAGS = -O -I$(INC) -Di386 -DSVR40

all:	layers

layers:
	$(CC) $(CFLAGS) -c layers.c
	$(CC) $(CFLAGS) -o layers layers.o $(LDLIBS) -los

