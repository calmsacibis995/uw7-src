#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lnttys.sh	1.2"
#ident "$Header$"
# install links to /dev sub-directories

cd /dev/term
for i in *
do
	rm -f /dev/tty$i
	ln /dev/term/$i /dev/tty$i
done
