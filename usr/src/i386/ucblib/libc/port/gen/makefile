#ident	"@(#)ucb:i386/ucblib/libc/port/gen/makefile	1.12"
#ident	"$Header$"
#	copyright	"%c%"

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#


#		PROPRIETARY NOTICE (Combined)
#
#This source code is unpublished proprietary information
#constituting, or derived under license from AT&T's UNIX(r) System V.
#In addition, portions of such source code were derived from Berkeley
#4.3 BSD under license from the Regents of the University of
#California.
#
#
#
#		Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
#	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.

#
# makefile for ucblibc/port/gen
#
#

include $(LIBRULES)

PROF=
NONPROF=
INC1=$(ROOT)/$(MACH)/usr/ucbinclude
INCSYS=$(INC)/sys
INCSYS1=$(INC1)/sys
SDEFLIST=

OBJECTS =  setpriority.o getusershell.o index.o \
	random.o regex.o rindex.o ualarm.o usleep.o \
	bcmp.o bcopy.o bzero.o ndbm.o econvert.o gconvert.o \
	seconvert.o decimal_bin.o ftime.o timezone.o sleep.o \
	nice.o times.o rand.o stricmp.o \
	_psignal.o _swapFLAGS.o fp_class.o getwd.o ieee_func.o \
	ieee_globals.o ieee_handler.o mkstemp.o psignal.o \
	scandir.o setbuffer.o sigfpe.o siglist.o double_decim.o \
	float_decim.o readdir.o _base_S.o _base_il.o _base_sup.o \
	_big_power.o _small_power.o _sprintf_sup.o _times_power.o \
	_unpack_dble.o base_conv.o pack_float.o unpack_float.o


SOURCES =  setpriority.c getusershell.c index.c \
	random.c regex.c rindex.c ualarm.c usleep.c \
	bcmp.c bcopy.c bzero.c ndbm.c econvert.c gconvert.c \
	seconvert.c decimal_bin.c ftime.c timezone.c sleep.c \
	nice.c times.c rand.c stricmp.c \
	_psignal.c _swapFLAGS.c fp_class.c getwd.c ieee_func.c \
	ieee_globals.c ieee_handler.c mkstemp.c psignal.c \
	scandir.c setbuffer.c sigfpe.c siglist.c double_decim.c \
	float_decim.c readdir.c _base_S.c _base_il.c _base_sup.c \
	_big_power.c _small_power.c _sprintf_sup.c _times_power.c \
	_unpack_dble.c base_conv.c pack_float.c unpack_float.c


ALL:		 $(OBJECTS)

all:	ALL


double_decim.o: base_conv.h
		$(CC)  $(CFLAGS) -I. -I$(INC1) -c double_decim.c
 
float_decim.o:  base_conv.h
		$(CC)  $(CFLAGS) -I$(INC1) -c float_decim.c

_psignal.o:	$(INC)/signal.h \
		$(INCSYS)/signal.h

_swapFLAGS.o:	$(INC1)/math.h \
		$(INC1)/fp.h \
		$(INCSYS1)/ieeefp.h
		$(CC) -I$(INC1) $(CFLAGS) -c _swapFLAGS.c

fp_class.o:	$(INC1)/math.h \
		$(INC1)/fp.h \
		$(INCSYS1)/ieeefp.h \
		libm.h
		$(CC) -I$(INC1) $(CFLAGS) -c fp_class.c

getwd.o:	$(INCSYS)/param.h \
		$(INCSYS)/types.h \
		$(INCSYS)/select.h \
		$(INCSYS)/stat.h \
		$(INCSYS1)/dir.h \
		$(INC)/stdio.h \
		$(INCSYS)/fcntl.h \
		$(INC)/unistd.h \
		$(INCSYS)/mman.h \
		$(INCSYS)/mnttab.h \
		$(INCSYS)/errno.h
		$(CC) -I$(INC1) $(CFLAGS) -c getwd.c

ieee_func.o:	$(INC1)/math.h \
		$(INC1)/fp.h \
		$(INCSYS1)/ieeefp.h \
		libm.h
		$(CC) -I$(INC1) $(CFLAGS) -c ieee_func.c

ieee_globals.o:	$(INC1)/fp.h \
		$(INCSYS1)/ieeefp.h
		$(CC) -I$(INC1) $(CFLAGS) -c ieee_globals.c

ieee_handler.o:	$(INC1)/fp.h \
		$(INCSYS1)/ieeefp.h \
		libm.h
		$(CC) -I$(INC1) $(CFLAGS) -c ieee_handler.c

mkstemp.o:	$(INCSYS)/fcntl.h

psignal.o:	$(INC)/stdio.h
 
scandir.o:	$(INCSYS1)/types.h \
		$(INCSYS)/stat.h \
		$(INCSYS1)/dir.h \
		$(INC1)/strings.h
		$(CC) -I$(INC1) -I$(INC) $(CFLAGS) -c scandir.c

setbuffer.o:	$(INC)/stdio.h

sigfpe.o:	$(INC)/errno.h \
		$(INC)/signal.h \
		$(INCSYS)/signal.h \
		$(INC1)/fp.h \
		$(INCSYS1)/ieeefp.h \
		$(INCSYS)/types.h \
		$(INCSYS)/ucontext.h\
		$(INCSYS)/siginfo.h
		$(CC) -I$(INC1) -I$(INC) $(CFLAGS) -c sigfpe.c

siglist.o:	$(INC)/signal.h \
		$(INCSYS)/signal.h

times.o:	$(INCSYS1)/resource.h \
		$(INCSYS)/time.h
		$(CC) $(CFLAGS) -I$(INC1) -I$(INC) -c times.c

nice.o:		$(INCSYS1)/resource.h \
		$(INCSYS)/time.h
		$(CC) $(CFLAGS) -I$(INC1) -I$(INC) -c nice.c

sleep.o:	$(INC1)/signal.h \
		$(INCSYS)/time.h
		$(CC) $(CFLAGS) -I$(INC1) -I$(INC) -c sleep.c

ftime.o:	$(INCSYS)/types.h \
		$(INCSYS)/time.h

ndbm.o:		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC1)/ndbm.h \
		$(INCSYS)/stat.h \
		$(INCSYS)/types.h \
		$(INCSYS)/file.h 
		$(CC) $(CFLAGS) -I$(INC1) -I$(INC) -c ndbm.c

getusershell.o:	 $(INC)/ctype.h \
		 $(INC)/stdio.h \
		 $(INCSYS)/file.h \
		 $(INCSYS)/param.h \
		 $(INCSYS)/select.h \
		 $(INCSYS)/stat.h \
		 $(INCSYS)/types.h 

setpriority.o:	 $(INC)/errno.h \
		 $(INC)/string.h	\
		 $(INCSYS)/errno.h \
		 $(INCSYS)/priocntl.h \
		 $(INCSYS)/procset.h \
		 $(INCSYS)/resource.h \
		 $(INCSYS)/select.h \
		 $(INCSYS)/time.h \
		 $(INCSYS)/tspriocntl.h	\
		 $(INCSYS)/types.h \
		 $(INC)/time.h 

readdir.o:	 $(INCSYS)/types.h \
		 $(INCSYS1)/dir.h \
		 $(INCSYS)/dirent.h
		$(CC) $(CFLAGS) -I$(INC1) -I$(INC) -c readdir.c

index.o:	 

random.o:	 $(INC)/stdio.h 

regex.o:	 

rindex.o:	 

ualarm.o:	 $(INCSYS)/time.h \
		 $(INC)/time.h 

usleep.o:	 $(INC1)/signal.h	\
		 $(INCSYS)/time.h
		$(CC) $(CFLAGS) -I$(INC1) -I$(INC) -c usleep.c

econvert.o:	$(INC1)/fp.h \
		$(INCSYS1)/ieeefp.h
		$(CC) $(CFLAGS) -I$(INC1) -I$(INC) -c econvert.c

gconvert.o:	$(INC1)/fp.h \
		$(INCSYS1)/ieeefp.h
		$(CC) $(CFLAGS) -I$(INC1) -I$(INC) -c gconvert.c

seconvert.o:	$(INC1)/fp.h \
		$(INCSYS1)/ieeefp.h
		$(CC) $(CFLAGS) -I$(INC1) -I$(INC) -c seconvert.c

decimal_bin.o:	$(INC1)/sunfp.h
		$(CC) $(CFLAGS) -I$(INC1) -I$(INC) -c decimal_bin.c

_base_S.o: base_conv.h
		$(CC)  $(CFLAGS) -I. -I$(INC1) -c _base_S.c

_base_il.o: base_conv.h
		$(CC)  $(CFLAGS) -I. -I$(INC1) -c _base_il.c

_base_sup.o: base_conv.h
		$(CC)  $(CFLAGS) -I. -I$(INC1) -c _base_sup.c

_big_power.o: base_conv.h
		$(CC)  $(CFLAGS) -I. -I$(INC1) -c _big_power.c

_small_power.o: base_conv.h
		$(CC)  $(CFLAGS) -I. -I$(INC1) -c _small_power.c

_sprintf_sup.o: base_conv.h
		$(CC)  $(CFLAGS) -I. -I$(INC1) -c _sprintf_sup.c

_times_power.o: base_conv.h
		$(CC)  $(CFLAGS) -I. -I$(INC1) -c _times_power.c

_unpack_dble.o: base_conv.h
		$(CC)  $(CFLAGS) -I. -I$(INC1) -c _unpack_dble.c

pack_float.o: base_conv.h
		$(CC)  $(CFLAGS) -I. -I$(INC1) -c pack_float.c

unpack_float.o: base_conv.h
		$(CC)  $(CFLAGS) -I. -I$(INC1) -c unpack_float.c

base_conv.o:

GLOBALINCS = $(INC)/errno.h \
	$(INC1)/signal.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INCSYS)/errno.h \
	$(INCSYS)/mau.h \
	$(INCSYS)/param.h \
	$(INCSYS)/pcb.h \
	$(INCSYS)/priocntl.h \
	$(INCSYS)/procset.h \
	$(INCSYS)/psw.h \
	$(INCSYS1)/resource.h \
	$(INCSYS)/select.h \
	$(INCSYS)/siginfo.h \
	$(INCSYS)/signal.h \
	$(INCSYS)/time.h \
	$(INCSYS)/tspriocntl.h \
	$(INCSYS)/types.h \
	$(INCSYS)/ucontext.h \
	$(INC)/time.h \
	$(INC)/ctype.h \
	$(INCSYS)/file.h \
	$(INCSYS)/stat.h \
	$(INC1)/ttyent.h 

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS)
