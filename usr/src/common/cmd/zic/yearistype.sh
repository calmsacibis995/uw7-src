#! /bin/sh
#       Copyright (c) 1993
#         All Rights Reserved

#       THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Univel
#       The copyright notice above does not evidence any
#       actual or intended publication of such source code.
 
#ident	"@(#)yearistype.sh	1.2"

case $#-$2 in
	2-odd)	case $1 in
			*[13579])	exit 0 ;;
			*)		exit 1 ;;
		esac ;;
	2-even)	case $1 in
			*[24680])	exit 0 ;;
			*)		exit 1 ;;
		esac ;;
	2-*)	pfmt -l "UX:yearistype" -s error -g "uxzic:53" "wild type - %s\n" $2 >&2
		exit 1 ;;
	*)	pfmt -l "UX:yearistype" -s error -g "uxzic:54" "usage: %s year type\n" $0 >&2
		exit 1 ;;
esac
