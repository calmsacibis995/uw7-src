#!/sbin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)basename:basename.sh	1.8.1.6"
#ident  "$Header$"

# ignore -- as first argument
[ "$1" = -- ] && shift

if [ $# -gt 2 ]
then
	catalog=uxcore
	label=UX:basename
	/sbin/pfmt -l $label -g $catalog:1 "Incorrect usage\\n"
	/sbin/pfmt -l $label -g $catalog:2 -s action "Usage: basename [ path [ suffix-pattern ] ]\\n"
	exit 1
fi
#	If no first argument or first argument is null, make first argument
#	"."  Add beginning slash, then remove trailing slashes, then remove 
#	everything up through last slash, then remove suffix pattern if 
#	second argument is present.
#	If nothing is left, first argument must be of form //*, in which
# 	case the basename is /.
exec /usr/bin/sed \
	-e 's!/*$!!' \
	-e 's!.*/!!' \
	-e "s!\\(.\\)$2\$!\\1!" \
	-e 's!^$!/!' <<!
/${1:-.}
!
