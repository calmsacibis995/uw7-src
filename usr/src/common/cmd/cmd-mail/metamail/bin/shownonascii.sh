#
#	ident @(#) shownonascii.sh 11.3 97/11/12 
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

#MYFONTDIR=/this/directory/COOPER/almost/certainly/COOPER/does/not/exist

CHARSET=$1
shift

if test "$1" = "-e"
then
	shift
	CMD="$*"
	RIGHTTERMCMD="$*"
else
	CMD="more $* /dev/null"
	RIGHTTERMCMD="more $*"
fi

if test ! -z "${MM_CHARSET:-}"
then
	if test "$MM_CHARSET" = "$CHARSET"
	then
		$RIGHTTERMCMD
		exit 0
	fi
fi

#if test ! -d "$MYFONTDIR"
#then
#	echo This message contains non-ASCII text, but the $CHARSET font
#	echo has not yet been installed on this machine.  What follows
#	echo "may be partially unreadable, but the English (ASCII) parts"
#	echo "should still be readable."
#	cat $*
#	exit 0
#fi

if test -z "${DISPLAY:-}"
then
	dspmsg $MF_METAMAIL -s $MS_SHOWNONASCII $SHOWNONASCII_MSG_DISPLAY \
		"This message contains non-ASCII text, which can only be displayed\nproperly if you are running X11.  What follows\nmay be partially unreadable, but the English (ASCII) parts\nshould still be readable.\n"
	cat $*
	exit 0
fi

#FPGREP=`xset q | grep $MYFONTDIR`
#if test -z "${FPGREP:-}"
#then
#    echo Adding $MYFONTDIR to your font path.
#    xset +fp "$MYFONTDIR"
#else
#    echo Your font path appears to be correctly set.
#fi


dspmsg $MF_METAMAIL -s $MS_SHOWNONASCII $SHOWNONASCII_MSG_XTERM \
	"Running xterm to display text in $CHARSET, please wait...\n" "$CHARSET"

# Bogus -- need to unsetenv MM_NOTTTY, but can't in Bourne shell.  --bobg.
MM_NOTTTY=''
xterm -fn $CHARSET -e $CMD
