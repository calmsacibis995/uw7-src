#! /sbin/sh

#	Copyright (c) 1993 UNIX System Laboratories, Inc.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.   	
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	copyright	"%c%"

#ident	"@(#)initpkg:i386/cmd/initpkg/mountall.sh	1.18.24.9"

if [ -z "$LC_ALL" -a -z "$LC_MESSAGES" ]
then
	if [ -z "$LANG" ]
	then
		LNG=`defadm locale LANG 2>/dev/null`
		if [ "$?" != 0 ]
		then LANG=C
		else eval $LNG
		fi
	fi
	LC_MESSAGES=/etc/inst/locale/$LANG
	export LANG LC_MESSAGES
fi
LABEL="UX:$0"

CAT=uxrc; export CAT

usage()
{
FARG=`gettxt $CAT:57 "FSType"`
ARG=`gettxt $CAT:58 "file_system_table"`
USAGE="mountall [-F $FARG] [-l|-r] [$ARG]"
pfmt -l $LABEL -s action -g $CAT:4 "Usage: %s\n" "$USAGE"
exit 2
}
TYPES="all"

while getopts ?rlF: c
do
	case $c in
	r)	RFLAG="r";;
	l)	LFLAG="l";;
	F)	FSType=$OPTARG;
		if [ "$TYPES" = "one" ]
		then
			pfmt -l $LABEL -s error -g $CAT:59 "more than one FSType specified\n"
			exit 2
		else
			TYPES="one"
		fi;
		case $FSType in
		?????????*) 
			pfmt -l $LABEL -s error -u $CAT:60 "FSType %s exceeds 8 characters\n" $FSType
			exit 2
		esac
		;;
	\?)	usage
		;;
	esac
done
shift `expr $OPTIND - 1`
if [ "$RFLAG" = "r" -a "$LFLAG" = "l" ]
then
	pfmt -l $LABEL -s error -g $CAT:61 "options -r and -l incompatible\n"
	usage
fi
if [ $# -gt 1 ]
then
	pfmt -l $LABEL -s error -g $CAT:62 "multiple arguments not supported\n"
	usage
fi

# get file system table name and make sure file exists
case $1 in
	"-")	FSTAB=""
		;;
	"")	FSTAB=/etc/vfstab
		;;
	*)	FSTAB=$1
		;;
esac
if [ "$FSTAB" != ""  -a  ! -s "$FSTAB" ]
then
	pfmt -l $LABEL -s error -g $CAT:63 "file system table (%s) not found\n" $FSTAB
	exit 1
fi

if [ \( "$FSType" = "rfs" -o "$FSType" = "nfs" \) -a "$LFLAG" = "l" ]
then
	pfmt -l $LABEL -s error -g $CAT:64 "option -l and FSType are incompatible\n"
	usage
fi
if [ \( "$FSType" = "s5" -o "$FSType" = "ufs" -o "$FSType" = "bfs" -o "$FSType" = "sfs" \) -a "$RFLAG" = "r" ]
then
	pfmt -l $LABEL -s error -g $CAT:65 "option -r and FSType are incompatible\n"
	usage
fi

#	file-system-table format:
#
#	column 1:	special- block special device or resource name
#	column 2: 	fsckdev- char special device for fsck 
#	column 3:	mountp- mount point
#	column 4:	fstype- File system type
#	column 5:	fsckpass- number if to be checked automatically
#	column 6:	automnt-	yes/no for automatic mount
#	column 7:	mntopts- -o specific mount options
#	column 8:	macceiling when security is installed, fs MAC ceiling

#	White-space separates columns.
#	Lines beginning with \"#\" are comments.  Empty lines are ignored.
#	a '-' in any field is a no-op.

	exec < $FSTAB
	while  read special fsckdev mountp fstype fsckpass automnt mntopts macceiling
	do
		case $special in
		'#'* | '')	#  Ignore comments, empty lines
				continue ;;
		'-')		#  Ignore no-action lines
				continue
		esac 
		
		if  [ "$FSType" ]
		then			# ignore different fstypes
			if [ "$FSType" != "$fstype" ]
			then
				continue
			fi
		fi

		if [ "$LFLAG" ]
		then
			if [ "$fstype" = "rfs"  -o  "$fstype" = "nfs" ]
			then
				continue
			fi
		fi
		if [ "$RFLAG" ]
		then
			if [ "$fstype" != "rfs" -a  "$fstype" != "nfs" ]
			then
				continue
			fi
		fi
		if [ "$automnt" != "yes" ]
		then
			continue
		fi
		if [ "$fstype" = "-" ]
		then
			pfmt -l $LABEL -s error -g $CAT:66 "FSType of %s cannot be identified\n" $special
			continue
		fi
		# 	Use mount options if any
		if  [ "$mntopts" != "-" ]
		then
			OPTIONS="-o $mntopts"
		else
			OPTIONS=""
		fi

		# check if mac is installed and if macceiling in vfstab
		# is not equal to null or "-"
		mldmode >/dev/null 2>&1
		if [ "$?" != "0" ]
		then
			lflg=""
		else
			if [ -n "$macceiling"  -a "$macceiling" != "-" ]
			then
				lflg="-l $macceiling"
			else
				lflg=""
			fi
		fi


		#	First check file system state and repair if necessary.

		if [ "$fsckdev" = "-" ]
		then
			/sbin/mount $lflg "-F" $fstype $OPTIONS $special $mountp
			continue
		fi

		/sbin/mount $lflg "-F" $fstype $OPTIONS $special $mountp
		if [ $? -eq 0 ]
		then
			[ "$mountp" = "/usr" ] && /sbin/initprivs 2> /dev/null
			continue
		fi
		#msg=`/sbin/fsck "-m" "-F" $fstype $special 2>&1 </dev/null`
		/sbin/fsck "-m" "-F" $fstype $special >/dev/null 2>&1
		case $? in
		0)	/sbin/mount $lflg "-F" $fstype $OPTIONS $special $mountp
			[ "$mountp" = "/usr" ] && /sbin/initprivs 2> /dev/null
			;;

		32)	if [ ! -f /etc/.fscklog ]
			then
				echo > /etc/.fscklog
				pfmt -l $LABEL -s info -g $CAT:53 "\nPlease wait while the system is examined.  This may take a few minutes.\n\n"
			fi
			#echo "$msg\n"
			#pfmt -s nostd -g $CAT:24 "\t %s is being checked\n" $fsckdev
			if [ "$fstype" != "s5" ]
			then
				/sbin/fsck "-F" $fstype -y $fsckdev >/dev/null
			else 
				/sbin/fsck "-F" $fstype -y -t /var/tmp/tmp$$ -D $fsckdev >/dev/null
			fi
			if [ $? -eq 0 ]
			then
				/sbin/mount $lflg "-F" $fstype $OPTIONS $special $mountp
				[ "$mountp" = "/usr" ] && /sbin/initprivs 2> /dev/null
			else
				pfmt -l $LABEL -s info -g $CAT:67 "%s not mounted\n" $special
			fi
			;;

		33)	# already mounted
			#
			# if the mount point is "/usr" execute /sbin/initprivs
			# here since the administrator may have mounted "/usr"
			# without doing a manual /sbin/initprivs.  The reason this
			# is required here is because once "/usr" is mounted the
			# file system dependent commands are executed from the
			# /usr/lib/fs/[fs_type] directory instead of the
			# /etc/fs/[fs_type] directory.
			#
			# If this is not done here the possibility exists that
			# some file systems may not get mounted and commands
			# that require privilege may not execute properly.
			#
			[ "$mountp" = "/usr" ] && /sbin/initprivs 2> /dev/null
			pfmt -l $LABEL -s info -g $CAT:68 "%s already mounted\n" $special
			;;

		39)	pfmt -l $LABEL -s error -g $CAT:69 "%s could not be read\n" $special
			;;

		1|36)	pfmt -l $LABEL -s error -g $CAT:70 "fsck failed for %s\n" $special
			;;
		esac

	done

# execute ``/sbin/initprivs'' one  more  time.  This shouldn't
# take too long and will report  via  diagnostic  messages any
# discrepancies found between the Privilege Data File and  the
# files on disk.

wait
/sbin/initprivs
exit 0
