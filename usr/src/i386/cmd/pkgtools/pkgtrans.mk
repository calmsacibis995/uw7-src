#		copyright	"%c%"

#ident	"@(#)pkgtrans.mk	15.1"

include $(CMDRULES)

PROC = pkgtrans
OBJECTS = $(PROC)

## options used to build this command
LOCALINC = -I ../hdrs
LDLIBS = -L ../libinst -linst -L $(BASE)/libpkg -lpkg -L $(BASE)/libadm -ladm -L $(BASE)/libcmd -lcmd -L $(BASE)/include -l stubs -lw -lgen

## objects which make up this process
OFILES=\
	main.o

all:	$(PROC)

$(PROC): $(OFILES)
	$(CC) -o $(PROC) $(OFILES) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)
	chmod 775 $(PROC)

clean:
	rm -f $(OFILES)

clobber: clean
	rm -f $(PROC)

