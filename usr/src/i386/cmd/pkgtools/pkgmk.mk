#		copyright	"%c%"

#ident	"@(#)pkgmk.mk	15.1"

include $(CMDRULES)

PROC = pkgmk
OBJECTS = $(PROC)

## options used to build this command
LOCALINC = -I ../hdrs
LDLIBS = -L ../libinst -linst -L $(BASE)/libpkg -lpkg -L $(BASE)/libadm -ladm -L $(BASE)/libcmd -lcmd -lgen -L $(BASE)/include -l stubs -lw

## objects which make up this process
OFILES=\
	splpkgmap.o main.o quit.o mkpkgmap.o getinst.o

all:	$(PROC)

$(PROC): $(OFILES)
	$(CC) -o $(PROC) $(OFILES) $(LDFLAGS) $(LDLIBS) $(SHLIBS)
	chmod 775 $(PROC)

clean:
	rm -f $(OFILES)

clobber: clean
	rm -f $(PROC)

