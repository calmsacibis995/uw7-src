#
#	ident @(#) showaudio.sh 11.2 97/11/03 
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

hostname()
{
    /usr/ucb/hostname
}

:

thishost=`hostname`

AUDIOBINDIR=/u/andrew/phone-sau/bin
AUDIOPHONEHOST=greenbush
AUDIOPHONEHOSTLONG=greenbush.bellcore.com
ORG=Bellcore

if test "$1" = "-p"
then
	AUDIOPHONE=$2
	shift
	shift
fi

if test "$1" = "-s"
then
	AUDIOSPEAKERFORCE=1
fi

playphone=0
if test ! -z "${AUDIOPHONE:-}" -o ! -z "${AUDIOPHONEFORCE:-}"
then
	playphone=1
fi

if test $playphone -eq 0
then
	if test ! -d "$AUDIOBINDIR"
	then
		AUDIOSPEAKERFORCE=1
	fi

	if test -z "${AUDIOSPEAKERFORCE:-}" -a -z "${AUDIOPHONEFORCE:-}"
	then
		if test ! -z "${MM_NOTTTY:-}"
		then
			if test $MM_NOTTTY -eq 1
			then
				MM_NOTTTY=0; export MM_NOTTTY
				xterm -e showaudio $*
				exit 0
			fi
		fi
		dspmsg $MF_METAMAIL -s $MS_SHOWAUDIO $SHOWAUDIO_MSG_CHOOSE \
			 "This program can display audio on the speakers of some workstations,\nor (at some sites) it can call you on the telephone.  Please choose one:\n"
		echo ""
		dspmsg $MF_METAMAIL -s $MS_SHOWAUDIO $SHOWAUDIO_MSG_SPEAKER \
			"1 -- Use the computer's speaker\n"
		dspmsg $MF_METAMAIL -s $MS_SHOWAUDIO $SHOWAUDIO_MSG_TELEPHONE \
			"2 -- Call me on the telephone\n"
		echo ""
		dspmsg $MF_METAMAIL -s $MS_SHOWAUDIO $SHOWAUDIO_MSG_PREFER \
			"Which do you prefer (1 or 2)? [1] "
		read ans
		if test "$ans" -eq 2
		then
			dspmsg $MF_METAMAIL -s $MS_SHOWAUDIO $SHOWAUDIO_MSG_USE_PHONE \
				"OK, we'll use the telephone...\n"
			AUDIOPHONEFORCE=1
			dspmsg $MF_METAMAIL -s $MS_SHOWAUDIO $SHOWAUDIO_MSG_FUTURE \
				"In the future, you can avoid this question by setting the environment variable\nAUDIOPHONEFORCE to 1\n" "AUDIOPHONEFORCE"
			playphone=1
		else
			dspmsg $MF_METAMAIL -s $MS_SHOWAUDIO $SHOWAUDIO_MSG_USE_SPEAKER \
				"OK, Attempting to play the audio using your computer's speaker..\n"
			AUDIOSPEAKERFORCE=1
			dspmsg $MF_METAMAIL -s $MS_SHOWAUDIO $SHOWAUDIO_MSG_FUTURE \
				"In the future, you can avoid this question by setting the environment variable\nAUDIOSPEAKERFORCE to 1" "AUDIOSPEAKERFORCE"
		fi
	fi
fi

if test $playphone -eq 0
then
	audiohost=$thishost
	if test ! -z "${DISPLAY:-}"
	then
		audiohost=`echo $DISPLAY | sed -e 's/:.*//'`
		if test "$audiohost" = unix
		then
			audiohost=$thishost
		fi
		if test -z "$audiohost"
		then
			audiohost=$thishost
		fi
	fi
	if test ! -z "${AUDIOHOST:-}"
	then
		audiohost=$AUDIOHOST
	fi

	if test ! "$audiohost" = "$thishost"
	then
		dspmsg $MF_METAMAIL -s $MS_SHOWAUDIO $SHOWAUDIO_MSG_SENDING \
			"Sending audio to $audiohost...\n" "$audiohost"
		thisprog=`(cd; which showaudio)`
		cat $* | rsh $audiohost $thisprog -s
		exit 0
	fi

	if test -d /usr/sony
	then
		dev=/dev/sb0
	else
		dev=/dev/audio
	fi

	if test -f /usr/sbin/sfplay
	then
		file $* | grep AIFF > /dev/null 2>&1
		if test $? -eq 0
		then
			dspmsg $MF_METAMAIL -s $MS_SHOWAUDIO $SHOWAUDIO_MSG_AIFF\
				"Playing AIFF audio on $thishost using /usr/sbin/sfplay, one moment...\n" "$thishost"
			/usr/sbin/sfplay $*
		else
			dspmsg $MF_METAMAIL -s $MS_SHOWAUDIO $SHOWAUDIO_MSG_NEXT\
				"Playing NeXT/Sun-format audio on $thishost using /usr/sbin/sfplay...\n" "$thishost"
			/usr/sbin/sfplay -i format next end $*
		fi
		exit 0
	fi

	if test -w $dev
	then
		dspmsg $MF_METAMAIL -s $MS_SHOWAUDIO $SHOWAUDIO_MSG_DEV \
			"Playing audio on $thishost using $dev, one moment please...\n" "$thishost" "$dev"
		cat $* > $dev
		exit 0
	fi
fi

if test -d "$AUDIOBINDIR"
then
	thisprog=`which showaudio`
	if test -z "${AUDIOPHONE:-}"
	then
		if test ! -z "${MM_NOTTTY:-}"
		then
			if test $MM_NOTTTY -eq 1
			then
				MM_NOTTTY=0; export MM_NOTTTY
				xterm -e $thisprog $*
				exit 0
			fi
		fi
		dspmsg $MF_METAMAIL -s $MS_SHOWAUDIO $SHOWAUDIO_MSG_NUMBER \
			"This message contains audio, which can be sent to your telephone.\nPlease enter the telephone number at which you would like to hear this\naudio message as you would dial it from inside ${ORG}: " \
			"${ORG}"
		read AUDIOPHONE
	fi

	if test "$thishost" == "$AUDIOPHONEHOST" \
		-o "$thishost" == "$AUDIOPHONEHOSTLONG"
	then
		dspmsg $MF_METAMAIL -s $MS_SHOWAUDIO $SHOWAUDIO_MSG_CALLING \
			"Calling Phone number $AUDIOPHONE\nIf the process seems stuck after you hang up,\nthen please interrupt with ^C or whatever your interrupt key is\n" \
			"$AUDIOPHONE"
		cat $AUDIOBINDIR/../GREET.au $* - | $AUDIOBINDIR/play -\# $AUDIOPHONE -
		dspmsg $MF_METAMAIL -s $MS_SHOWAUDIO $SHOWAUDIO_MSG_DONE \
			"All done\n"
		exit 0
	else
		dspmsg $MF_METAMAIL -s $MS_SHOWAUDIO $SHOWAUDIO_MSG_RSH \
			"Trying to rsh to $AUDIOPHONEHOST to send audio via telephone\n" \
			"$AUDIOPHONEHOST"
		cat $* | rsh $AUDIOPHONEHOST $thisprog -p $AUDIOPHONE
		exit 0
	fi
fi
echo ""
dspmsg $MF_METAMAIL -s $MS_SHOWAUDIO $SHOWAUDIO_MSG_CANT_PLAY \
	"This message contains an audio message, which can not currently be\nplayed on this type of workstation.   If you log into an appropriate\nmachine (currently a SPARCstation or Sony News workstation)\nand read this message there, you should be able to hear the audio\nmessage.\n"
