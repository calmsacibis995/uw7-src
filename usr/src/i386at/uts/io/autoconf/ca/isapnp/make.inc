#	@(#)make.inc	6.1	9/9/97	15:50:55
# ROOT=/u/stevegi/root
__DSROOT=
# CONF=/u/stevegi/root/etc/conf
CONF=./etc/conf
INCRT=/u/stevegi/root/usr/src/uts/i386
INC=/usr/include

IDDEV=/u/stevegi/root/xtra/suppds
XNXDEV=/usr/ods30
DINCRT=/usr/include
LIBC=

CDIR=.
CDDIR=.
XDIR=

PATH=/bin:/usr/bin
SHELL=/bin/sh
CC=$(TOOLS)/usr/ccs/bin/i386cc
# CC=ROOT=/u/stevegi/root __DSROOT= /u/stevegi/root/usr/src/xenv/cc
XENIXCC=ROOT=/u/stevegi/root __DSROOT= /usr/ods30/bin/cc
COMPFMT=
CMDOPT=
KERNELOPT=$(EMPTY)
AR=/bin/ar
AS=/bin/as
CPP=/lib/cpp
KSURF=
LD=/bin/ld
MAKE=/bin/make
MASM=/bin/masm
STRIP=/bin/strip
UNSYM=/u/stevegi/root/usr/sco/bin/unsym -da

PRODUCT=ALL
MORECPP= -DKPRF -DVPIX -DWEITEK -DSCO_VAL_ADDED -DSCO_ONLY -DSecureWare -DFFS -DMERGE386 -D_KERNEL -D_SVID3 -D_NBREAD
