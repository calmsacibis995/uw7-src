#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)q-rm.sh	1.2"
#ident	"$Header$"

# PURPOSE: Remove currently configured RS232 port
# ---------------------------------------------------------------------

for i in $*
do

	if [ `echo $i | /usr/bin/grep "term"` ]
	then
		TTY=`echo $i | /usr/bin/cut -c11-14`
	elif [ `echo $i | /usr/bin/grep "tty"` ]
	then
		TTY=`echo $i | /usr/bin/cut -c9-12`
	else
		TTY=`echo $i | /usr/bin/cut -c6-14`
	fi

	/usr/sbin/pmadm -l -p ttymon3 -s $TTY >/usr/tmp/tmp.$VPID
	if [ $? = 0 ]
	then
		/usr/sbin/pmadm -r -p ttymon3 -s $TTY >/dev/null 2>&1
		echo "Confirmation" > /usr/tmp/title.$VPID
		echo "$i was successfully removed.\n" >>/usr/tmp/ap.$VPID
	else
		echo "Request Denied" > /usr/tmp/title.$VPID
		cat /usr/tmp/tmp.$VPID >> /usr/tmp/ap.$VPID
		echo "... was not removed.\n" >>/usr/tmp/ap.$VPID
	fi
done

echo 0
exit 0
