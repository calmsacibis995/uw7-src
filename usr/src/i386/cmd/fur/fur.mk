#ident	"@(#)fur:i386/cmd/fur/fur.mk	1.6.2.8"
#ident	"$Header$"

include	$(CMDRULES)
SED=sed

OBJS = bits.o canonical.o check.o decode.o extn.o fcns.o fur.o gencode.o inline.o insertion.o intemu.o jumps.o opdecode.o opgencode.o opsubst.o rel.o tables.o util.o cte.o
COMPAT_OBJS = bits.o canonical.o check.o decode.o extn.o fcns.o compat_fur.o gencode.o inline.o compat_insertion.o intemu.o jumps.o opdecode.o opgencode.o opsubst.o rel.o tables.o util.o cte.o

LELF = $(LIBELF)
EXTRA_LINK =
CINC_CPU = $(SGSBASE)/inc/$(CPU)
CINC_COM = $(SGSBASE)/inc/common
LIBCINC = $(LIBBASE)/libC

INTEMU = $(SGSBASE)/intemu/common

DIS = $(SGSBASE)/dis/$(CPU)

DISCOM = $(SGSBASE)/dis/common

CCSLIB = $(ROOT)/$(MACH)/usr/ccs/lib

SGSBASE = ../sgs
LIBBASE = ../../lib

all:	$(SGS)fur compat_fur prof.o prologue.o epilogue.o block.o flow.o mkproflog mkblocklog compat_mkproflog compat_mkblocklog

install: all
	[ -d $(CCSBIN) ] || mkdir -p $(CCSBIN)
	[ -d $(UW_CCSBIN) ] || mkdir -p $(UW_CCSBIN)
	[ -d $(OSR5_CCSBIN) ] || mkdir -p $(OSR5_CCSBIN)
	$(INS) -f $(CCSBIN) -m 755 -u $(OWN) -g $(GRP) $(SGS)fur
	cp $(SGS)fur fur.bak
	cp compat_fur compat_fur.bak
	mv compat_fur fur
	$(INS) -f $(UW_CCSBIN) -m 755 -u $(OWN) -g $(GRP) fur
	$(INS) -f $(OSR5_CCSBIN) -m 755 -u $(OWN) -g $(GRP) fur
	mv compat_fur.bak compat_fur
	mv fur.bak $(SGS)fur
	[ -d $(CCSLIB)/fur ] || mkdir -p $(CCSLIB)/fur
	[ -d $(UW_CCSLIB)/fur ] || mkdir -p $(UW_CCSLIB)/fur
	[ -d $(OSR5_CCSLIB)/fur ] || mkdir -p $(OSR5_CCSLIB)/fur
	$(INS) -f $(CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) mkblocklog
	$(INS) -f $(CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) mkproflog
	cp mkblocklog mkblocklog.bak
	cp mkproflog mkproflog.bak
	cp compat_mkblocklog compat_mkblocklog.bak
	cp compat_mkproflog compat_mkproflog.bak
	mv compat_mkproflog mkproflog
	mv compat_mkblocklog mkblocklog
	$(INS) -f $(UW_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) mkblocklog
	$(INS) -f $(OSR5_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) mkblocklog
	$(INS) -f $(UW_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) mkproflog
	$(INS) -f $(OSR5_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) mkproflog
	mv mkproflog.bak mkproflog
	mv mkblocklog.bak mkblocklog
	mv compat_mkproflog.bak compat_mkproflog
	mv compat_mkblocklog.bak compat_mkblocklog
	$(INS) -f $(CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) log.h
	$(INS) -f $(CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) blocklog.c
	$(INS) -f $(CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) proflog.c
	$(INS) -f $(CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) prof.o
	$(INS) -f $(CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) prologue.o
	$(INS) -f $(CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) epilogue.o
	$(INS) -f $(CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) block.o
	$(INS) -f $(CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) flowlog.s
	$(INS) -f $(CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) flow.o
	$(INS) -f $(UW_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) log.h
	$(INS) -f $(UW_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) blocklog.c
	$(INS) -f $(UW_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) proflog.c
	$(INS) -f $(UW_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) prof.o
	$(INS) -f $(UW_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) prologue.o
	$(INS) -f $(UW_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) epilogue.o
	$(INS) -f $(UW_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) block.o
	$(INS) -f $(UW_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) flowlog.s
	$(INS) -f $(UW_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) flow.o
	$(INS) -f $(OSR5_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) log.h
	$(INS) -f $(OSR5_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) blocklog.c
	$(INS) -f $(OSR5_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) proflog.c
	$(INS) -f $(OSR5_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) prof.o
	$(INS) -f $(OSR5_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) prologue.o
	$(INS) -f $(OSR5_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) epilogue.o
	$(INS) -f $(OSR5_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) block.o
	$(INS) -f $(OSR5_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) flowlog.s
	$(INS) -f $(OSR5_CCSLIB)/fur -m 755 -u $(OWN) -g $(GRP) flow.o

$(SGS)fur: $(OBJS)
	$(CC) $(LDFLAGS) -o $(SGS)fur $(OBJS) $(LELF) $(EXTRA_LINK)

compat_fur: $(COMPAT_OBJS)
	$(CC) $(LDFLAGS) -o compat_fur $(COMPAT_OBJS) $(LELF) $(EXTRA_LINK)

prof.o: prof.s

prologue.o: prologue.s

epilogue.o: epilogue.s

flow.o: flow.s

block.o: block.s

fill.o: fill.c 

rel.o: rel.c \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/stdlib.h \
		$(INC)/stdarg.h \
		$(INC)/string.h \
		$(INC)/stdio.h \
		$(INC)/libelf.h \
		./fur.h 
	$(CC) $(CFLAGS) -I$(LIBCINC) -I$(INTEMU) -I$(CINC_COM) -I$(CINC_CPU) $(DEFLIST) -c rel.c

canonical.o: canonical.c \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/stdlib.h \
		$(INC)/stdarg.h \
		$(INC)/string.h \
		$(INC)/stdio.h \
		$(INC)/libelf.h \
		./fur.h 
	$(CC) $(CFLAGS) -I$(LIBCINC) -I$(INTEMU) -I$(CINC_COM) -I$(CINC_CPU) $(DEFLIST) -c canonical.c

jumps.o: jumps.c \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/stdlib.h \
		$(INC)/stdarg.h \
		$(INC)/string.h \
		$(INC)/stdio.h \
		$(INC)/libelf.h \
		./fur.h 
	$(CC) $(CFLAGS) -I$(LIBCINC) -I$(INTEMU) -I$(CINC_COM) -I$(CINC_CPU) $(DEFLIST) -c jumps.c

opdecode.o: opdecode.c \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/stdlib.h \
		$(INC)/stdarg.h \
		$(INC)/string.h \
		$(INC)/stdio.h \
		$(INC)/libelf.h \
		$(DIS)/dis.h \
		./op.h  \
		./fur.h 
	$(CC) $(CFLAGS) -I$(DIS) -I$(LIBCINC) -I$(INTEMU) -I$(CINC_COM) -I$(CINC_CPU) $(DEFLIST) -c opdecode.c

opgencode.o: opgencode.c \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/stdlib.h \
		$(INC)/stdarg.h \
		$(INC)/string.h \
		$(INC)/stdio.h \
		$(INC)/libelf.h \
		$(DIS)/dis.h \
		./op.h  \
		./fur.h 
	$(CC) $(CFLAGS) -I$(DIS) -I$(LIBCINC) -I$(INTEMU) -I$(CINC_COM) -I$(CINC_CPU) $(DEFLIST) -c opgencode.c

opsubst.o: opsubst.c \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/stdlib.h \
		$(INC)/stdarg.h \
		$(INC)/string.h \
		$(INC)/stdio.h \
		$(INC)/libelf.h \
		$(DIS)/dis.h \
		./op.h  \
		./fur.h 
	$(CC) $(CFLAGS) -I$(DIS) -I$(LIBCINC) -I$(INTEMU) -I$(CINC_COM) -I$(CINC_CPU) $(DEFLIST) -c opsubst.c

fur.o: fur.c \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/stdlib.h \
		$(INC)/stdarg.h \
		$(INC)/string.h \
		$(INC)/stdio.h \
		$(INC)/libelf.h \
		./op.h  \
		./fur.h 
	$(CC) $(CFLAGS) -I$(DIS) -I$(LIBCINC) -I$(INTEMU) -I$(CINC_COM) -I$(CINC_CPU) $(DEFLIST) -c fur.c

compat_fur.o: fur.c \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/stdlib.h \
		$(INC)/stdarg.h \
		$(INC)/string.h \
		$(INC)/stdio.h \
		$(INC)/libelf.h \
		./fur.h 
	$(CC) $(CFLAGS) -Wa,"-ocompat_fur.o" -I$(DIS) -I$(LIBCINC) -I$(INTEMU) \
	-DFUR_DIR=\"$(ALT_PREFIX)/usr/ccs/lib/fur\" -I$(CINC_COM) \
	-I$(CINC_CPU) $(DEFLIST) -c fur.c

check.o: check.c \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/stdlib.h \
		$(INC)/stdarg.h \
		$(INC)/string.h \
		$(INC)/stdio.h \
		$(INC)/libelf.h \
		./fur.h 
	$(CC) $(CFLAGS) -I$(LIBCINC) -I$(INTEMU) -I$(CINC_COM) -I$(CINC_CPU) $(DEFLIST) -c check.c

decode.o: decode.c \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/stdlib.h \
		$(INC)/stdarg.h \
		$(INC)/string.h \
		$(INC)/stdio.h \
		$(INC)/libelf.h \
		$(DIS)/dis.h \
		./fur.h 
	$(CC) $(CFLAGS) -I$(DIS) -I$(LIBCINC) -I$(INTEMU) -I$(CINC_COM) -I$(CINC_CPU) $(DEFLIST) -c decode.c

gencode.o: gencode.c \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/stdlib.h \
		$(INC)/stdarg.h \
		$(INC)/string.h \
		$(INC)/stdio.h \
		$(INC)/libelf.h \
		./fur.h 
	$(CC) $(CFLAGS) -I$(LIBCINC) -I$(INTEMU) -I$(CINC_COM) -I$(CINC_CPU) $(DEFLIST) -c gencode.c

inline.o: inline.c \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/stdlib.h \
		$(INC)/stdarg.h \
		$(INC)/string.h \
		$(INC)/stdio.h \
		$(INC)/libelf.h \
		./fur.h  \
		./op.h 
	$(CC) $(CFLAGS) -I$(DIS) -I$(LIBCINC) -I$(INTEMU) -I$(CINC_COM) -I$(CINC_CPU) $(DEFLIST) -c inline.c

insertion.o: insertion.c \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/stdlib.h \
		$(INC)/stdarg.h \
		$(INC)/string.h \
		$(INC)/stdio.h \
		$(INC)/libelf.h \
		./fur.h 
	$(CC) $(CFLAGS) -I$(LIBCINC) -I$(INTEMU) -I$(CINC_COM) -I$(CINC_CPU) $(DEFLIST) -c insertion.c

compat_insertion.o: insertion.c \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/stdlib.h \
		$(INC)/stdarg.h \
		$(INC)/string.h \
		$(INC)/stdio.h \
		$(INC)/libelf.h \
		./fur.h 
	$(CC) -Wa,"-ocompat_insertion.o" $(CFLAGS) -I$(LIBCINC) -I$(INTEMU) \
	-DFUR_DIR=\"$(ALT_PREFIX)/usr/ccs/lib/fur\" \
	-I$(CINC_COM) -I$(CINC_CPU) $(DEFLIST) -c insertion.c

util.o: util.c \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/stdlib.h \
		$(INC)/stdarg.h \
		$(INC)/string.h \
		$(INC)/stdio.h \
		$(INC)/libelf.h \
		./fur.h 
	$(CC) $(CFLAGS) -I$(LIBCINC) -I$(INTEMU) -I$(CINC_COM) -I$(CINC_CPU) $(DEFLIST) -c util.c

mkproflog:	mkproflog.sh
	$(SED) -e "s+FURDIR+/usr/ccs/lib/fur+g" mkproflog.sh > mkproflog

mkblocklog:	mkblocklog.sh
	$(SED) -e "s+FURDIR+/usr/ccs/lib/fur+g" mkblocklog.sh > mkblocklog

compat_mkproflog:	mkproflog.sh
	$(SED) -e "s+FURDIR+$(ALT_PREFIX)/usr/ccs/lib/fur+g" mkproflog.sh > compat_mkproflog

compat_mkblocklog:	mkblocklog.sh
	$(SED) -e "s+FURDIR+$(ALT_PREFIX)/usr/ccs/lib/fur+g" mkblocklog.sh > compat_mkblocklog

clean:
	-rm -f $(OBJS) compat_insertion.o compat_fur.o

clobber: clean
	-rm -f $(SGS)fur mkproflog mkblocklog compat_mkproflog compat_mkblocklog

intemu.o: $(INTEMU)/intemu.c $(INTEMU)/intemu.h
	$(CC) -I$(INTEMU) -I$(SGSBASE)/inc/$(CPU) -c $(CFLAGS) $(INTEMU)/intemu.c

bits.o: $(DIS)/dis.h $(INC)/libelf.h $(SGSBASE)/inc/$(CPU/sgs.h
	$(CC) -I$(DIS) -I$(SGSBASE)/inc/$(CPU) -c $(CFLAGS) -DFUR $(DIS)/bits.c

fcns.o: $(DIS)/fcns.c $(DIS)/dis.h $(INC)/libelf.h $(SGSBASE)/inc/$(CPU)/sgs.h $(DISCOM)/structs.h
	$(CC) -I$(SGSBASE)/inc/common -I$(DISCOM) -I$(DIS) -I$(SGSBASE)/inc/$(CPU) -c $(CFLAGS) -DFUR $(DIS)/fcns.c

tables.o: $(DIS)/tables.c $(DIS)/dis.h
	$(CC) -I$(DIS) -I$(SGSBASE)/inc/$(CPU) -c $(CFLAGS) -DFUR $(DIS)/tables.c

extn.o: $(DISCOM)/extn.c $(DIS)/dis.h $(DISCOM)/structs.h
	$(CC) -I$(SGSBASE)/inc/common -I$(DIS) -I$(DISCOM) -c $(CFLAGS) -DFUR $(DISCOM)/extn.c
