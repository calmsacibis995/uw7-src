#ident	"@(#)makefile	%B"
#ident  "$Header$"

include $(CMDRULES)

SRC1=putgrent.c errmsg.c file.c vgid.c vgname.c vgroup.c vuid.c \
	get_ngm.c vlogin.c dates.c vexpire.c prerrno.c adumprec.c \
	ypents.c getxopt.c messages.c execScript.c

OBJ=$(SRC1:.c=.o)

GENERAL=../inc
LOCALINC=-I. -I$(GENERAL)
LINTFLAGS=-u $(DEFLIST)

GDATE=$(USRSADM)

PRODUCT=lib.a ugdates

all: $(PRODUCT)

lib.a: $(OBJ)
	$(AR) cr $(@) `$(LORDER) $(OBJ) | tsort`

ugdates: ugdates.dat
	grep -v "^#ident" ugdates.dat > ugdates

touch: 
	touch $(SRC1) 

clean:
	rm -f $(OBJ) 

clobber: clean
	rm -f $(PRODUCT)

lintit:
	$(LINT) $(LINTFLAGS) $(SRC1)

install: $(PRODUCT)
	$(INS) -f $(GDATE) -u bin -g bin -m 444 ugdates

strip: $(PRODUCT)

size: $(PRODUCT)
	size $(PRODUCT)

errmsg.o: $(INC)/stdio.h
errmsg.o: $(INC)/varargs.h
file.o: $(INC)/sys/stat.h
file.o: $(INC)/sys/types.h
get_ngm.o: $(INC)/sys/param.h
get_ngm.o: $(INC)/unistd.h
putgrent.o: $(INC)/stdio.h
putgrent.o: $(INC)/grp.h
putgrent.o: $(INC)/unistd.h
vgid.o: $(INC)/sys/types.h
vgid.o: $(INC)/stdio.h
vgid.o: $(INC)/grp.h
vgid.o: $(GENERAL)/users.h
vgid.o: $(INC)/sys/param.h
vgid.o: $(INC)/userdefs.h
vgid.o: $(INC)/limits.h
vgname.o: $(INC)/sys/types.h
vgname.o: $(INC)/stdio.h
vgname.o: $(INC)/ctype.h
vgname.o: $(INC)/grp.h
vgname.o: $(INC)/userdefs.h
vgname.o: $(GENERAL)/users.h
vgroup.o: $(INC)/sys/types.h
vgroup.o: $(INC)/stdio.h
vgroup.o: $(INC)/ctype.h
vgroup.o: $(INC)/grp.h
vgroup.o: $(GENERAL)/users.h
vuid.o: $(INC)/sys/types.h
vuid.o: $(INC)/stdio.h
vuid.o: $(INC)/pwd.h
vuid.o: $(INC)/userdefs.h
vuid.o: $(GENERAL)/users.h
vuid.o: $(INC)/sys/param.h
vuid.o: $(INC)/limits.h
vlogin.o: $(INC)/sys/types.h
vlogin.o: $(INC)/stdio.h
vlogin.o: $(INC)/ctype.h
vlogin.o: $(INC)/pwd.h
vlogin.o: $(INC)/userdefs.h
vlogin.o: $(GENERAL)/users.h
dates.o: $(INC)/time.h
prerrno.o: $(INC)/errno.h
getgxopt.o:$(INC)/stdio.h
getgxopt.o:$(INC)/stdlib.h
getgxopt.o:$(INC)/string.h
getgxopt.o:$(GENERAL)/getxopt.h
