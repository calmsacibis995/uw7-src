#	copyright	"%c%"

#ident	"@(#)fmtmsg:fmtmsg.mk	1.12.3.2"
#ident "$Header$"

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

INCSYS=$(INC)
HDRS=$(INC)/fmtmsg.h $(INC)/stdio.h $(INC)/string.h $(INC)/errno.h
FILE=fmtmsg
INSTALLS=fmtmsg
SRC=main.c
OBJ=$(SRC:.c=.o)
LOCALINC=-I.
LINTFLAGS=$(DEFLIST)

all		: $(FILE) 

install		: all
		$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) $(INSTALLS)

clobber		: clean
		rm -f $(FILE)

clean		:
		rm -f $(OBJ)

strip		: $(FILE)
		$(STRIP) $(FILE)

lintit		: $(SRC)
		$(LINT) $(LINTFLAGS) $(SRC)

$(FILE)		: $(OBJ)
		$(CC) $(OBJ) -o $@ $(LDFLAGS) $(LDLIBS) $(SHLIBS)

$(OBJ)		: $(HDRS)
