#ident	"@(#)eac:i386/eaccmd/fdinit/fdinit.mk	1.3"

include	$(CMDRULES)

FILE	= fdinit
SRC	= fdinit.c
OBJ	= $(SRC:.c=.o)
LDFLAGS	= -s

all		: $(FILE) 

install: all
	$(INS) -f $(SBIN) -m 755 -u bin -g bin $(FILE)

clobber: clean
	rm -f $(FILE)

clean:
	rm -f $(OBJ)

$(FILE): $(OBJ) $(LIBS)
	$(CC) $(OBJ) -o $(FILE) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

$(OBJ): $(HDRS)
