#ident	"@(#)usemouse.mk	1.4"

include	$(CMDRULES)

OBJS= source.o merge.o parse.o

LDLIBS= -levent

LOCALINC = -I./ 

DATS	= default vi vsh rogue sysadmsh
DATSRCS	= default.src vi.src vsh.src rogue.src sysadmsh.src

all:	usemouse $(DATS) 

install:	all 
	[ -d $(USRBIN) ] || mkdir -p $(USRBIN)
	$(INS) -f $(USRBIN) usemouse
	[ -d $(USRLIB)/mouse ] || mkdir -p $(USRLIB)/mouse
	cp $(DATS) $(USRLIB)/mouse
	[ -d $(ETC)/default ] || mkdir -p $(ETC)/default
	cp default $(ETC)/default/usemouse

$(DATS):	$(DATSRCS)
	cp $@.src $@

clean:
	rm -f *.o
	rm -f $(DATS)

clobber: clean
	rm -f usemouse

usemouse: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

