#!/sbin/sh
#ident	"@(#)initpkg:i386/cmd/initpkg/ckmount.sh	1.1"

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

# This file has those commands necessary to check and mount
# the file systems that must be mounted early in init.

vfstab=/etc/vfstab

while read bdevice rdevice mntp fstype fsckpass automnt mntopts
do
	# check for comments
	case ${bdevice} in
	'#'*)	continue ;;
	esac

	case ${mntp} in
	${1})
		/sbin/mount ${mntp}
		if [ $? -ne 0 ]
		then
			/sbin/fsck -F ${fstype} -m  ${rdevice}
			if [ $? -ne 0 ]
			then
				pfmt -l $LABEL -s info -g $CAT:48 "The %s file system (%s) is being checked.\n" $mntp ${rdevice} 2>/dev/console
				/sbin/fsck -F ${fstype} -y  ${rdevice}
			fi
			/sbin/mount ${mntp}
		fi ;;
	esac
done < ${vfstab}
