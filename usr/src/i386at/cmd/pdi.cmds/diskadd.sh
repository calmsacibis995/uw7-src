#!/sbin/sh
#ident	"@(#)pdi.cmds:diskadd.sh	1.3.7.1"

# This script is used for adding disk drives to the UNIX System.

label=UX:diskadd
msgdb=uxdiskadd

usage() {
	pfmt -l $label -s action -g $msgdb:1 "\nusage: diskadd [-F s5dm] [1] ( for Second Disk )\n"
	pfmt -l $label -s nostd -g $msgdb:2 "usage: diskadd [-F s5dm] cCCbBtTTdDD ( for additional Disks )\n\n"
	pfmt -l $label -s nostd -g $msgdb:3 "Issued with [1] or without an argument diskadd will\nadd the Second disk with the appropriate name.\n"
	pfmt -l $label -s nostd -g $msgdb:4 "For any additional disks, the argument must be in the form:\n\n"
	pfmt -l $label -s nostd -g $msgdb:5 "        cCCbBtTTdDD\n\n"
	pfmt -l $label -s nostd -g $msgdb:6 "        Where CC is the PDI Controller number (1-2 digits)\n"
	pfmt -l $label -s nostd -g $msgdb:7 "              B  is the Bus ID on that Controller (1 digit)\n"
	pfmt -l $label -s nostd -g $msgdb:8 "              TT is the Target ID on that Bus (1-2 digits)\n"
	pfmt -l $label -s nostd -g $msgdb:9 "        and   DD is the Logical Unit on that Target (1-2 digits).\n"
}

lockfile="/var/tmp/DISKADD.LOCK"

trap 'trap "" 1 2 3 9 15;
	pfmt -l $label -s info -g $msgdb:10 "You have canceled the diskadd program.  \n"
	rm -f $lockfile
exit 2' 1 2 3 15

if [ -f $lockfile ]
then
	pfmt -l $label -s error -g $msgdb:11 "The diskadd program is currently being run and cannot be run concurrently.\n"
	pfmt -l $label -s action -g $msgdb:12 "Please retry this at a later time.\n"
	exit 1
else
	>$lockfile
fi

y=`gettxt $msgdb:13 "y"`
Y=`gettxt $msgdb:14 "Y"`
n=`gettxt $msgdb:15 "n"`
N=`gettxt $msgdb:16 "N"`
pfmt -l $label -s info -g $msgdb:17 "You have invoked the System V disk management (s5dm) diskadd utility.\n"
pfmt -l $label -s nostd -g $msgdb:18 "The purpose of this utility is to set up additional disk drives.\n"
pfmt -l $label -s nostd -g $msgdb:19 "This utility can destroy the existing data on the disk.\n"
pfmt -l $label -s nostd -g $msgdb:20 "Do you wish to continue?\n"
pfmt -l $label -s nostd -g $msgdb:21 "(Type %s for yes or %s for no followed by ENTER): \n" "$y" "$n"
read cont
if  [ "$cont" != "$y" ] && [ "$cont" != "$Y" ] 
then
	rm -f $lockfile
	exit 0
fi

if [ -n "$1" ]
then
	drive=$1
else
	drive=1
fi

case $drive in
1)	# add the default second disk device
	dn=`devattr disk2 desc 2>&1`
	ret=$?
	if [ $ret = 2 ]
	then
		echo "$dn"
		rm -f $lockfile
		exit 1
	elif [ $ret != 0 ]
	then
		pfmt -l $label -s error -g $msgdb:22 "\nThere does not seem to be a second disk present on your system.\n"
		pfmt -l $label -s action -g $msgdb:23 "Please verify your disk is connected correctly.\n"
		usage
		rm -f $lockfile
		exit 1
	fi
	devnm=`devattr disk2 cdevice`
	t_mnt=`devattr disk2 bdevice`
	length=`expr length $t_mnt - 1`
	t_mnt=`expr substr $t_mnt 1 $length`
	;;

c?b?t?d? | \
c?b?t?d?? | \
c?b?t??d? | \
c?b?t??d?? | \
c??b?t?d? | \
c??b?t?d?? | \
c??b?t??d? | \
c??b?t??d??) # added for scsi devices

	dn=`devattr /dev/dsk/${drive}s0 desc 2>&1`
	ret=$?
	if [ $ret = 2 ]
	then
		echo "$dn"
		rm -f $lockfile
		exit 1
	elif [ $ret != 0 ]
	then
		pfmt -l $label -s error -g $msgdb:24 "\n%s does not seem to be present on your system.\n" "$drive"
		pfmt -l $label -s action -g $msgdb:23 "Please verify your disk is connected correctly.\n"
		usage
		rm -f $lockfile
		exit 1
	fi
	devnm="/dev/rdsk/${drive}s0"
	t_mnt="/dev/dsk/${drive}s"
	;;

*)	usage
	rm -f $lockfile
	exit 1 # added for scsi
	;;

esac

/etc/mount | grep ${t_mnt} > /dev/null 2>&1
if [ $? = 0 ]
then
	pfmt -l $label -s error -g $msgdb:25 "The device you wish to add cannot be added.\n"
	pfmt -l $label -s info -g $msgdb:26 "It already has a mounted filesystem on it.\n"
	rm -f $lockfile
	exit 1
fi

/usr/sbin/fdisk -I $devnm 
if [ $? != 0 ]
then
	pfmt -l $label -s error -g $msgdb:27 "\nThe Installation of the disk has failed.\n"
	pfmt -l $label -s info -g $msgdb:28 "Received error return value from %s.\n" "/usr/sbin/fdisk"
	pfmt -l $label -s action -g $msgdb:23 "Please verify your disk is connected correctly.\n"
	usage
	rm -f $lockfile
	exit 1
fi
 
echo

/usr/sbin/disksetup -I $devnm
if [ $? != 0 ]
then
	pfmt -l $label -s error -g $msgdb:27 "\nThe Installation of the disk has failed.\n"
	pfmt -l $label -s info -g $msgdb:28 "Received error return value from %s.\n" "/usr/sbin/disksetup"
	pfmt -l $label -s action -g $msgdb:23 "Please verify your disk is connected correctly.\n"
	usage
	rm -f $lockfile
	exit 1
fi

rm -f /etc/scsi/pdi_edt $lockfile
pfmt -l $label -s info -g $msgdb:29 "Diskadd for %s DONE at %s\n" "$dn" "`date`"
exit 0
