#
#	ident @(#) showpicture.sh 11.2 97/11/03 
#
#############################################################################
#
#	Copyright (c) 1997 The Santa Cruz Operation, Inc.. 
#		All Rights Reserved. 
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF 
#		THE SANTA CRUZ OPERATION INC.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.
#
#############################################################################
#
# Copyright (c) 1991 Bell Communications Research, Inc. (Bellcore)
# 
# Permission to use, copy, modify, and distribute this material 
# for any purpose and without fee is hereby granted, provided 
# that the above copyright notice and this permission notice 
# appear in all copies, and that the name of Bellcore not be 
# used in advertising or publicity pertaining to this 
# material without the specific, prior written permission 
# of an authorized representative of Bellcore.  BELLCORE 
# MAKES NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY 
# OF THIS MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS", 
# WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES.
#
#############################################################################
#
# Conversion from C shell to Bourne shell by Z-Code Software Corp.
# Conversion Copyright (c) 1992 Z-Code Software Corp.
# Permission to use, copy, modify, and distribute this material
# for any purpose and without fee is hereby granted, provided
# that the above copyright notice and this permission notice
# appear in all copies, and that the name of Z-Code Software not
# be used in advertising or publicity pertaining to this
# material without the specific, prior written permission
# of an authorized representative of Z-Code.  Z-CODE SOFTWARE
# MAKES NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY
# OF THIS MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS",
# WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES.
#

echo_n()
{
    echo "$@\c"
}

:

if [ -z "$METAMAIL_TMPDIR" ]
then
	METAMAIL_TMPDIR=/tmp
fi

if test -z "${X_VIEWER:-}"
then
	X_VIEWER="xloadimage -view -quiet -geometry +1+1"
# X_VIEWER="xv -geometry +1+1"
fi

if test "$1" = "-viewer" -a ! -z "$2"
then
	X_VIEWER="$2"
	shift
	shift
fi

if test -z "${DISPLAY:-}"
then
	echo ""
	dspmsg $MF_METAMAIL -s $MS_SHOWPICTURE $SHOWPICTURE_MSG_DISPLAY \
		"This message contains a picture, which can currently only be\nviewed when running X11.  If you read this message while running\nX11, you will be able to see the picture properly.\n"
	exit 0
fi

dspmsg $MF_METAMAIL -s $MS_SHOWPICTURE $SHOWPICTURE_MSG_NOTE \
	"NOTE: TO MAKE THE PICTURE WINDOW GO AWAY, JUST TYPE 'q' IN IT.\n" \
	"q"
echo ""

if test -z "$1"
then
	$X_VIEWER stdin
else
	for i
	do
		dir=`dirname $i`
		base=`basename $i`
		if test ! "$base" = "$i"
		then
			cd $dir
		fi
		if ln $i $$.PRESS-q-TO-EXIT  > /dev/null 2>&1
		then $X_VIEWER $$.PRESS-q-TO-EXIT
		    rm $$.PRESS-q-TO-EXIT 
		else $X_VIEWER "$i"
		fi
	done
fi
