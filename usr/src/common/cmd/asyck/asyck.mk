#ident	"@(#)asyck.mk	1.2"

include	$(CMDRULES)

OBJS= asyck.o

LOCALINC=.

all: 	asyck 

install:	all 
	[ -d $(USRBIN) ] || mkdir -p $(USRBIN)
	$(INS) -f $(USRBIN) asyck 

clean:
	rm -f *.o

clobber: clean
	rm -f asyck 

asyck: $(OBJS) 
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)
