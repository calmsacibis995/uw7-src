#
#	ident @(#) showexternal.sh 11.3 97/11/12 
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

whoami()
{
    whoami=${LOGNAME:-${USER:-`who am i | awk '{print $1}' -`}}
    whoami=${whoami:-`grep ":$HOME:" /etc/passwd | sed 's/:.*//'`}
    if [ -z "$whoami" ]
    then
	dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL $SHOWEXTERNAL_ERR_NAME \
		"Cannot determine user name\n" 1>&2
	exit 1
    fi
    echo $whoami
}

:

# if test -f /usr/lib/sendmail
# then
#     MAILCOMMAND=/usr/lib/sendmail
# else
#     MAILCOMMAND=/bin/mail
# fi
MAILCOMMAND=/usr/bin/mailx

if test -z "$3"
then
	dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL $SHOWEXTERNAL_ERR_USAGE \
		 "Usage: showexternal body-file access-type name [site [directory [mode]]]\n"
	exit 1
fi

if [ -z "$METAMAIL_TMPDIR" ]
then
	METAMAIL_TMPDIR=/tmp
fi

bodyfile=$1
atype=`echo $2 | tr ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz`
name=$3

site=$4

dir=$5

mode=$6

ctype=`grep -i content-type: $bodyfile | sed -e 's/............: //'`
cenc=`grep -i content-transfer-encoding: $bodyfile | sed -e 's/.........................: //'`
username=""
pass=""
TMPDIR=$METAMAIL_TMPDIR/XXXternal.$$
trap 'rmdir "$TMPDIR" >/dev/null 2>&1' 1 2 3 15
mkdir $TMPDIR
cd $TMPDIR
NEWNAME="mm.ext.$$"
NEEDSCONFIRMATION=1

case $atype in
	anon-ftp)
		dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL $SHOWEXTERNAL_MSG_ANON \
		"This mail message contains a POINTER (reference) to data that is\nnot included in the message itself.  Rather, the data can be retrieved\nautomatically using anonymous FTP to a site on the network.\n"
		;;
		
	ftp)
		dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL $SHOWEXTERNAL_MSG_FTP \
		"This mail message contains a POINTER (reference) to data that is\nnot included in the message itself.  Rather, the data can be retrieved\nautomatically using the FTP protocol to a site on the network.\n"
		;;

	mail-server)
		dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL $SHOWEXTERNAL_MSG_SRVR \
		"This mail message contains a POINTER (reference) to data that is not\nincluded in the message itself.  Rather, the data can be retrieved by\nsending a special mail message to a mail server on the network.\nHowever, doing this automatically is slightly dangerous, because\nsomeone might be using this mechanism to cause YOU to send obnoxious\nmail.  For that reason, the mail message that WOULD be sent is being\nshown to you first for your approval.\n\nThis is the message that will be sent if you choose to go ahead and\nretrieve the external data:\n\n" \
		> $METAMAIL_TMPDIR/ext.junk.$$
		echo "To: ${name}@${site}" >> $METAMAIL_TMPDIR/ext.junk.$$
		echo "Subject: Automated Mail Server Request" >> $METAMAIL_TMPDIR/ext.junk.$$
		echo "" >> $METAMAIL_TMPDIR/ext.junk.$$
		cat $bodyfile >> $METAMAIL_TMPDIR/ext.junk.$$
		more $METAMAIL_TMPDIR/ext.junk.$$
		rm $METAMAIL_TMPDIR/ext.junk.$$ ;;

	*)
		NEEDSCONFIRMATION=0 ;;
esac

if test $NEEDSCONFIRMATION -ne 0
then
	echo ""
	yes="`gettxt Xopen_info:4 'yes'`"; no="`gettxt Xopen_info:5 'no'`"
	yeschar="`echo $yes | sed 's/\(.\)\(.*\)/\1/'`"
	nochar="`echo $no | sed 's/\(.\)\(.*\)/\1/'`"
	msg="echo `dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL $SHOWEXTERNAL_MSG_DATA \"Do you want to proceed with retrieving the external data? [$yes] \" \"$yes\"`"
	while
		eval "$msg"
		read ANS
	do
		case "$ANS" in
			${nochar}*)rm -rf $TMPDIR; exit 0 ;;
			""|${yeschar}*) break ;;
			*) dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL $SHOWEXTERNAL_ERR_ENTER "Please enter yes or no.\n" "$yes" "$no" ;;
		esac
	done
fi

case "$atype" in
	anon-ftp | ftp)
		case "$atype" in
		anon-ftp )
			username=anonymous
			pass=`whoami`@`hostname`
			;;
		esac

		if test -z "$site"
		then
			dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL \
				$SHOWEXTERNAL_MSG_SITE "Site for ftp access: "
			read site
		fi
		if test -z "$username"
		then
			dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL \
				$SHOWEXTERNAL_MSG_USER \
				"User name at site ${site}: " "$site"
			read username
		fi
		if test -z "$pass"
		then
			dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL \
				$SHOWEXTERNAL_MSG_PASSWD \
				"Password for user $username at site ${site}: "\
				"$username" "$site"
			stty -echo
			read pass
			stty echo
			echo ""
		fi
		if test -z "$dir"
		then
			DIRCMD=""
		else
			DIRCMD="cd $dir"
		fi
		if test -z "$mode"
		then
			MODECMD=""
		else
			MODECMD="type $mode"
		fi
		dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL $SHOWEXTERNAL_MSG_BODY \
			"OBTAINING MESSAGE BODY USING FTP\n"
		dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL $SHOWEXTERNAL_MSG_SU \
			"SITE: $site USER: $username\n" "$site" "$username"
		${FTP:-ftp} -n <<!
open $site
user $username $pass
$DIRCMD
$MODECMD
get $name $NEWNAME
quit
!
		if test ! -r "$NEWNAME"
		then
			dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL \
				$SHOWEXTERNAL_ERR_FTP "FTP failed.\n"
			rm -rf $TMPDIR
			exit 1
		fi
		;;

	afs|local-file)
		if test ! -r $name
		then
			dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL \
				$SHOWEXTERNAL_ERR_FOUND "File not found\n"
			rm -rf $TMPDIR
			exit 1
		fi
		NEWNAME=$name
		dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL \
			$SHOWEXTERNAL_MSG_GETTING \
			"GETTING BODY FROM FILE NAMED: $NEWNAME\n" "$NEWNAME"
		;;

	mail-server)
		if test -z "$bodyfile"
		then
			dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL \ 
				$SHOWEXTERNAL_ERR_BODYFILE \
				"mail-server access-type requires a body file\n"
			rm -rf $TMPDIR
			exit 1
		fi
		echo Subject: Automated Mail Server Request > $NEWNAME
		echo To: ${name}@${site} >> $NEWNAME
		echo "" >> $NEWNAME
		cat $bodyfile >> $NEWNAME
		$MAILCOMMAND -t < $NEWNAME
		if test $? -ne 0
		then
			dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL \
				$SHOWEXTERNAL_ERR_MAIL \
				"$MAILCOMMAND failed\n" "$MAILCOMMAND"
			rm -rf $TMPDIR
			exit 1
		fi
		rm -rf $TMPDIR
		dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL \
			$SHOWEXTERNAL_MSG_REQUESTED \
			"Your $ctype data has been requested from a mail server.\n" \
			"$ctype"
		exit 0 ;;
	*)
		dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL $SHOWEXTERNAL_ERR_TYPE \
			"UNRECOGNIZED ACCESS-TYPE\n"
		rm -rf $TMPDIR
		exit 1 ;;
esac

if test "$cenc" = base64
then
	mmencode -u -b < $NEWNAME > OUT
	mv OUT $NEWNAME
elif test "$cenc" = quoted-printable
then
	mmencode -u -q < $NEWNAME > OUT
	mv OUT $NEWNAME
fi

case "$atype" in
    local-file ) metamail -b -c $ctype $NEWNAME ;;
    * ) metamail -b -c "$ctype" $TMPDIR/$NEWNAME ;;
esac

if test $? -ne 0
then
	dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL $SHOWEXTERNAL_ERR_METAMAIL \
		"metamail failed\n"
	rm -rf $TMPDIR
	exit 1
fi

if test ! "$NEWNAME" = "$name"
then
	echo ""
	dspmsg $MF_METAMAIL -s $MS_SHOWEXTERNAL $SHOWEXTERNAL_MSG_RM \
		"The data just displayed is stored in the file $TMPDIR/$NEWNAME\nDo you want to delete it?\n" "$TMPDIR" "$NEWNAME"
	rm -i $NEWNAME
fi

if test ! -r ${TMPDIR}/${NEWNAME}
then
	cd /
	rmdir $TMPDIR
fi
