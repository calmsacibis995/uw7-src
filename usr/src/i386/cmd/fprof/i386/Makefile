#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.
#ident	"@(#)fprof:i386/Makefile	1.4"

include $(CMDRULES)

CMDBASE		= ../..
SGSBASE		= ../../sgs
COMDIR		= ../common
CPUDIR		= ../$(CPU)
SED		= sed

INS			= $(CMDBASE)/install/install.sh
INSDIR		= $(CCSBIN)

HFILES			= $(COMDIR)/fprof.h $(CPUDIR)/machrel.h
INCDIRS			= -I $(CPUDIR) -I $(COMDIR)
SOURCES			= $(COMDIR)/lib.c $(COMDIR)/fprof.c $(COMDIR)/scan.c $(COMDIR)/stats.c $(COMDIR)/xkfuncs.c
LIB_PRODUCTS	= libfprof.so.1 compat_libfprof.so.1
BIN_PRODUCTS	= fprof lrt_scan compat_fprof compat_lrt_scan
PRODUCTS		= $(LIB_PRODUCTS) $(BIN_PRODUCTS)
LIB_OBJECTS		= lib.o fprof.o stats.o xkfuncs.o
COMPAT_LIB_OBJECTS	= compat_lib.o fprof.o stats.o xkfuncs.o

OBJECTS			= scan.o compat_lib.o $(LIB_OBJECTS)

SH	= sh
RM	= rm
LN	= ln
CHMOD	= chmod

all: $(PRODUCTS)

profmk: libsymint $(PRODUCTS)

install: all
	cp fprof fprof.bak
	cp lrt_scan lrt_scan.bak
	cp compat_fprof compat_fprof.bak
	cp compat_lrt_scan compat_lrt_scan.bak
	$(STRIP) lrt_scan
	$(STRIP) compat_lrt_scan
	$(SH) $(INS) -f $(CCSBIN) fprof
	$(SH) $(INS) -f $(CCSBIN) lrt_scan
	mv compat_fprof fprof
	mv compat_lrt_scan lrt_scan
	$(SH) $(INS) -f $(UW_CCSBIN) fprof
	$(SH) $(INS) -f $(UW_CCSBIN) lrt_scan
	$(SH) $(INS) -f $(OSR5_CCSBIN) fprof
	$(SH) $(INS) -f $(OSR5_CCSBIN) lrt_scan
	mv fprof.bak fprof
	mv lrt_scan.bak lrt_scan
	mv compat_fprof.bak compat_fprof
	mv compat_lrt_scan.bak compat_lrt_scan
	cp libfprof.so.1 libfprof.so.1.bak
	cp compat_libfprof.so.1 compat_libfprof.so.1.bak
	$(SH) $(INS) -f $(CCSLIB) libfprof.so.1
	mv compat_libfprof.so.1 libfprof.so.1
	$(SH) $(INS) -f $(UW_CCSLIB) libfprof.so.1
	$(SH) $(INS) -f $(OSR5_CCSLIB) libfprof.so.1
	mv compat_libfprof.so.1.bak compat_libfprof.so.1
	mv libfprof.so.1.bak libfprof.so.1

lintit:	$(SOURCES)
	$(LINT) $(LINTFLAGS) $(SOURCES)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(PRODUCTS)

libfprof.so.1: $(LIB_OBJECTS)
	$(CC) -h /usr/ccs/lib/libfprof.so.1 $(LDFLAGS) -G -o libfprof.so.1 $(LIB_OBJECTS) -lgen
	$(CHMOD) +x $@

compat_libfprof.so.1: $(COMPAT_LIB_OBJECTS)
	$(CC) -h $(ALT_PREFIX)/usr/ccs/lib/libfprof.so.1 $(LDFLAGS) -G -o compat_libfprof.so.1 $(COMPAT_LIB_OBJECTS) -lgen
	$(CHMOD) +x $@

lrt_scan: scan
	$(RM) -f $@
	$(LN) -f $? $@

compat_lrt_scan: compat_scan
	$(RM) -f $@
	$(LN) -f $? $@

scan:	scan.o
	$(CC) $(CFLAGS) -o scan scan.o libfprof.so.1

compat_scan:	scan.o
	$(CC) $(CFLAGS) -o compat_scan scan.o compat_libfprof.so.1

dumplog:	dumplog.o
	$(CC) $(CFLAGS) -o dumplog dumplog.o libfprof.so.1 -lgen

dumplog.o: $(COMDIR)/dumplog.c $(COMDIR)/fprof.h
	$(CC) $(CFLAGS) $(INCDIRS) -c $(COMDIR)/dumplog.c

scan.o: $(COMDIR)/scan.c $(COMDIR)/fprof.h
	$(CC) $(CFLAGS) $(INCDIRS) -c $(COMDIR)/scan.c

fprof.o: $(COMDIR)/fprof.c $(COMDIR)/fprof.h
	$(CC) $(CFLAGS) $(INCDIRS) -c $(COMDIR)/fprof.c

stats.o: $(COMDIR)/stats.c $(COMDIR)/fprof.h
	$(CC) $(CFLAGS) $(INCDIRS) -c $(COMDIR)/stats.c

xkfuncs.o: $(COMDIR)/xkfuncs.c $(COMDIR)/fprof.h
	$(CC) $(CFLAGS) $(INCDIRS) -c $(COMDIR)/xkfuncs.c

lib.o: $(COMDIR)/lib.c $(COMDIR)/fprof.h
	$(CC) $(CFLAGS) $(INCDIRS) -c $(COMDIR)/lib.c

compat_lib.o: $(COMDIR)/lib.c $(COMDIR)/fprof.h
	$(CC) -Wa,"-ocompat_lib.o" $(CFLAGS) $(INCDIRS) \
	-DCFG_FILE=\"$(ALT_PREFIX)/usr/ccs/lib/fprof.cfg\" -c $(COMDIR)/lib.c

fprof: $(COMDIR)/fprof.sh
	$(RM) -f $@
	$(SED) -e "s+CCSLIB+/usr/ccs/lib+g" $? > $@
	$(CHMOD) +x fprof

compat_fprof: $(COMDIR)/fprof.sh
	$(RM) -f $@
	$(SED) -e "s+CCSLIB+$(ALT_PREFIX)/usr/ccs/lib+g" $? > $@
	$(CHMOD) +x compat_fprof
