#!/bin/sh

# $XFree86: xc/programs/Xserver/hw/xfree86/etc/postinst.sh,v 3.3 1996/01/14 13:38:40 dawes Exp $
#
# postinst.sh
#
# This script should be run after installing a beta version into $NEWDIR.
# It makes a backup of the old version of files that will be replaced, and
# puts the backups into $SAVEDIR.  The beta version of the files is then
# linked into the normal installed location $RUNDIR.
#
# This script should be more portable.  On some OSs, 'ln -s' may need to
# be replaced with 'ln', and 'mkdir -p' may need to be replaced with
# '/usr/X11R6/bin/mkdirhier'.
#

ORIGVERSION=3.1.2
BETAVERSION=3.1.2C
NEWDIR=/usr/XFree86-$BETAVERSION
RUNDIR=/usr/X11R6
SAVEDIR=/usr/XFree86-$ORIGVERSION

if [ ! -d $NEWDIR/. ]; then
	echo $NEWDIR does not exist
	exit 1
fi
if [ ! -d $RUNDIR/. ]; then
	echo $RUNDIR does not exist
	exit 1
fi
cd $NEWDIR
for i in `find * -type f -print`; do
	d=`dirname $i`
	if [ ! -d $SAVEDIR/$d ]; then
		mkdir -p $SAVEDIR/$d
	fi
	if [ ! -d $RUNDIR/$d ]; then
		mkdir -p $RUNDIR/$d
	fi
	if [ -f $RUNDIR/$i -a ! -f $SAVEDIR/$i ]; then
		mv $RUNDIR/$i $SAVEDIR/$i
		echo saved $ORIGVERSION version of $i to $SAVEDIR
	fi
	rm -f $RUNDIR/$i
	ln -s $NEWDIR/$i $RUNDIR/$i
	echo installed $BETAVERSION version of $i in $RUNDIR
done
exit 0
