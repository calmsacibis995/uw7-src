#ident	"@(#)libelf:i386/makefile	1.11.2.6"

include $(CMDRULES)

COM=		../common
COFF=		../coff
MISC=		../misc
MMAP=
DEFLIST=	-D$(ARFORMAT)
DEFCOM=		$(MMAP)
DEFCOFF=
DEFMISC=	-D$(CPU) -DELF
PRODUCTS=	libelf.a

ARFLAGS=	r
AR_CMD=		$(AR) $(ARFLAGS)
CC_CMD=		$(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(ENV)
ARFORMAT=	PORTAR

ENV=
INCLIST=	-I$(COM) -I$(COMINC) -I$(CPUINC) -I$(COFF)
INS=		$(SGSBASE)/sgs.install

COFFHDR=\
	$(COFF)/coff.h\
	cofftab.h

COFFSRC=\
	$(COFF)/coff.c\
	$(COFF)/coffscn.c\
	$(COFF)/coffshdr.c\
	$(COFF)/coffsym.c

COFFOBJ=\
	coff.o\
	coffscn.o\
	coffshdr.o\
	coffsym.o

COMHDR=\
	$(COM)/decl.h\
	$(COM)/error.h\
	$(COM)/foreign.h\
	$(COM)/member.h\
	$(COM)/syn.h\
	$(COM)/tune.h

COMSRC=\
	$(COM)/ar.c\
	$(COM)/begin.c \
	$(COM)/cntl.c\
	$(COM)/cook.c\
	$(COM)/data.c\
	$(COM)/end.c \
	$(COM)/error.c \
	$(COM)/fill.c\
	$(COM)/flag.c \
	$(COM)/getarhdr.c \
	$(COM)/getarsym.c\
	$(COM)/getbase.c \
	$(COM)/getdata.c\
	$(COM)/getehdr.c\
	$(COM)/getident.c\
	$(COM)/getphdr.c\
	$(COM)/getscn.c \
	$(COM)/getshdr.c\
	$(COM)/hash.c\
	$(COM)/input.c\
	$(COM)/kind.c\
	$(COM)/ndxscn.c\
	$(COM)/newdata.c\
	$(COM)/newehdr.c\
	$(COM)/newphdr.c\
	$(COM)/newscn.c\
	$(COM)/next.c\
	$(COM)/nextscn.c\
	$(COM)/rand.c\
	$(COM)/rawdata.c\
	$(COM)/rawfile.c\
	$(COM)/rawput.c\
	$(COM)/strptr.c\
	$(COM)/update.c\
	$(COM)/xlate.c

MACHSRC=\
	foreign.c\
	coffstr.c\
	coff386.c\
	cofftab.c\
	svr4.c

COMOBJ=\
	ar.o\
	begin.o\
	cntl.o\
	cook.o\
	data.o\
	end.o\
	error.o\
	fill.o\
	flag.o\
	getarhdr.o\
	getarsym.o\
	getbase.o\
	getdata.o\
	getehdr.o\
	getident.o\
	getphdr.o\
	getscn.o\
	getshdr.o\
	hash.o\
	input.o\
	kind.o\
	ndxscn.o\
	newdata.o\
	newehdr.o\
	newphdr.o\
	newscn.o\
	next.o\
	nextscn.o\
	rand.o\
	rawdata.o\
	rawfile.o\
	rawput.o\
	strptr.o\
	update.o\
	xlate.o

MACHOBJ=\
	foreign.o\
	coffstr.o\
	coff386.o\
	cofftab.o\
	svr4.o

MISCSRC=\
	$(MISC)/decode.c\
	$(MISC)/nlist.c

MISCOBJ=\
	decode.o\
	nlist.o


HDR=	$(COFFHDR) $(COMHDR) $(MISCHDR)
OBJ=	$(COFFOBJ) $(COMOBJ) $(MACHOBJ) $(MISCOBJ)
SRC=	$(COFFSRC) $(COMSRC) $(MACHSRC) $(MISCSRC)

.MUTEX:	$(COM)/error.h objects
.MUTEX:	$(COM)/error.c error.o
.MUTEX:	$(COM)/xlate.c xlate.o

mmap:		$(COM)/mmap.c
	@ echo ======================================;\
	echo Checking system call availability.;\
	echo Test compilation failure is ok.;\
	echo $(CC) $(INCLIST) $(DEFLIST) -o mmap $(COM)/mmap.c;\
	if $(CC) $(INCLIST) $(DEFLIST) -o mmap $(COM)/mmap.c;\
	then	rm -f mmap;\
		echo Successful: enabling memory mapping;\
		echo ======================================;\
		$(MAKE) -e MMAP=-DMMAP_IS_AVAIL all;\
	else	rm -f mmap;\
		echo Unsuccessful: disabling memory mapping;\
		echo ======================================;\
		$(MAKE) -e MMAP= all;\
	fi;\
	rm -f mmap

all:		$(PRODUCTS)

libelf.a:	$(COM)/error.h objects
	$(AR_CMD) libelf.a `$(LORDER) $(OBJ) | tsort`

objects:	$(OBJ)

$(COFFOBJ):	$(COFF)/coff.h\
		$(COM)/decl.h\
		$(COM)/error.h\
		$(COM)/syn.h
	$(CC_CMD) $(DEFCOFF) $(COFF)/$(@F:.o=.c)

$(COMOBJ):	$(COM)/decl.h\
		$(COM)/error.h\
		$(COM)/syn.h
	$(CC_CMD) $(DEFCOM) $(COM)/$(@F:.o=.c)

$(MACHOBJ):	$(COM)/foreign.h\
		$(COFF)/coff.h
	$(CC_CMD) $(DEFCOM) $(@F:.o=.c)

$(MISCOBJ):
	$(CC_CMD) $(DEFMISC) $(MISC)/$(@F:.o=.c)

clean:
	-rm -f $(OBJ) $(COM)/error.c $(COM)/error.h $(COM)/elf_str $(COM)/xlate.c

clobber:	clean
	-rm -f libelf.a
	-rm -f llib-lelf.ln

install:	mmap $(COM)/elf_str
	/bin/sh $(INS) 644 $(OWN) $(GRP) $(CCSLIB)/libelf.a libelf.a
	#/bin/sh $(INS) 644 $(OWN) $(GRP) $(CCSLIB)/llib-lelf.ln llib-lelf.ln
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	/bin/sh $(INS) 444 $(OWN) $(GRP) $(USRLIB)/locale/C/MSGFILES/libelf.str $(COM)/elf_str

lintit:	$(SRC) $(HDR)
	$(LINT)  $(INCLIST) $(DEFLIST) -u $(SRC) >lint.out

llib-lelf.ln:	$(COM)/llib-lelf.c
	$(LINT) -oelf $(INCLIST) $(COM)/llib-lelf.c

ar.o:		$(COM)/ar.c	$(COM)/member.h
begin.o:	$(COM)/begin.c	$(COM)/foreign.h $(COM)/member.h
coff.o:		$(COFF)/coff.c	$(COM)/foreign.h
coff386.o:	coff386.c	cofftab.h
coffscn.o:	$(COFF)/coffscn.c
coffshdr.o:	$(COFF)/coffshdr.c
coffstr.o:	coffstr.c	$(COFF)/coff.h
coffsym.o:	$(COFF)/coffsym.c
cofftab.o:	cofftab.c	cofftab.h
cntl.o:		$(COM)/cntl.c
cook.o:		$(COM)/cook.c
data.o:		$(COM)/data.c
decode.o:	$(MISC)/decode.c
end.o:		$(COM)/end.c	$(COM)/member.h
error.o:	$(COM)/error.c
fill.o:		$(COM)/fill.c
flag.o:		$(COM)/flag.c
foreign.o:	foreign.c	$(COM)/foreign.h
getarhdr.o:	$(COM)/getarhdr.c	$(COM)/member.h
getarsym.o:	$(COM)/getarsym.c
getbase.o:	$(COM)/getbase.c
getdata.o:	$(COM)/getdata.c
getehdr.o:	$(COM)/getehdr.c
getident.o:	$(COM)/getident.c
getphdr.o:	$(COM)/getphdr.c
getscn.o:	$(COM)/getscn.c
getshdr.o:	$(COM)/getshdr.c
input.o:	$(COM)/input.c	$(COM)/tune.h
hash.o:		$(COM)/hash.c
kind.o:		$(COM)/kind.c
ndxscn.o:	$(COM)/ndxscn.c
newdata.o:	$(COM)/newdata.c
newehdr.o:	$(COM)/newehdr.c
newphdr.o:	$(COM)/newphdr.c
newscn.o:	$(COM)/newscn.c
next.o:		$(COM)/next.c
nextscn.o:	$(COM)/nextscn.c
nlist.o:	$(MISC)/nlist.c
rand.o:		$(COM)/rand.c
rawdata.o:	$(COM)/rawdata.c
rawfile.o:	$(COM)/rawfile.c
rawput.o:	$(COM)/rawput.c
puthdr.o:	$(COM)/puthdr.c
strptr.o:	$(COM)/strptr.c
svr4.o:		svr4.c
update.o:	$(COM)/update.c
version.o:	$(COM)/version.c
xlate.o:	$(COM)/xlate.c

$(COM)/error.c:	$(COM)/error.list $(COM)/error.c.awk
	awk -f $(COM)/error.c.awk $(COM)/error.list >$(COM)/error.c

$(COM)/error.h:	$(COM)/error.list $(COM)/error.h.awk
	awk -f $(COM)/error.h.awk $(COM)/error.list >$(COM)/error.h

$(COM)/elf_str: $(COM)/error.list $(COM)/elf_str.awk
	awk -f $(COM)/elf_str.awk $(COM)/error.list >$(COM)/elf_str

$(COM)/xlate.c:	$(COM)/xlate.m4
	m4 < $(COM)/xlate.m4 > $(COM)/xlate.c
