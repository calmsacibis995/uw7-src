#copyright	"%c%"

#ident	"@(#)adm.mk	15.1"

include $(LIBRULES)

.c.a:;

LIBADM = libadm.a
OBJECTS = $(SOURCES:.c=.o)

SOURCES= \
	pkginfo.c pkgnmchk.c pkgparam.c \
	getinput.c ckint.c ckitem.c \
	ckpath.c ckrange.c ckstr.c \
	ckyorn.c puterror.c puthelp.c \
	puttext.c ckkeywd.c getvol.c \
	devattr.c putprmpt.c ckgid.c \
	ckdate.c cktime.c ckuid.c \
	dgrpent.c getdev.c \
	devtab.c data.c getdgrp.c \
	listdev.c listdgrp.c regexp.c \
	devreserv.c putdev.c putdgrp.c \
	ddb_dsf.c ddb_gen.c \
	ddb_lib.c ddb_sec.c \
	devalloc.c devdealloc.c \
	space.c ckquit.c strarray.c

LOCALINC = -I.

all: $(LIBADM)

.PRECIOUS: $(LIBADM)

$(LIBADM): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBADM) $(OBJECTS)

clean:
	rm -f lint.out $(LINTLIBADM) *.o

clobber: clean
	rm -f $(LIBADM)


