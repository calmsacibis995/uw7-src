#!/sbin/sh
#ident	"@(#)pdi.cmds:diskrm.sh	1.5"

# This script is used to remove disk drives nodes from /etc/vfstab

label=UX:diskrm
msgdb=uxdiskrm

lockfile="/var/tmp/.DISKRM.LOCK"

trap 'trap "" 1 2 3 9 15;
	set +e;
	cd /;
	pfmt -l $label -s info -g $msgdb:1 "You have canceled the diskrm program.\n"
	rm -f $lockfile
exit 2' 1 2 3 15

if [ -f $lockfile ]
then
	pfmt -l $label -s error -g $msgdb:2 "The diskrm program is currently being run and cannot be run concurrently.\n"
	pfmt -l $label -s action -g $msgdb:3 "Please retry this at a later time.\n"
	exit 1
else
	>$lockfile
fi

if [ -n "$1" ]
then
	if [ "$1" = "c0b0t0d0" ]
	then
		pfmt -l $label -s error -g $msgdb:4 "Disk 0 not removable\n"
		rm -f $lockfile
		exit 1
	fi
	drive=$1
	driv_arg=$1
else
	drive="NODEV"
fi
case "$drive" in
c?b?t?d? | \
c?b?t?d?? | \
c?b?t??d? | \
c?b?t??d?? | \
c??b?t?d? | \
c??b?t?d?? | \
c??b?t??d? | \
c??b?t??d??)

	y=`gettxt $msgdb:5 "y"`
	Y=`gettxt $msgdb:6 "Y"`
	n=`gettxt $msgdb:7 "n"`
	N=`gettxt $msgdb:8 "N"`
	pfmt -l $label -s info -g $msgdb:9 "You have invoked the System V disk management (s5dm) diskrm utility.\n"
	pfmt -l $label -s nostd -g $msgdb:10 "The purpose of this utility is to remove entries from the /etc/vfstab file.\n"
	pfmt -l $label -s nostd -g $msgdb:11 "Do you wish to continue?\n"
	pfmt -l $label -s nostd -g $msgdb:12 "(Type %s for yes or %s for no followed by ENTER): \n" "$y" "$n"
	read cont
	if  [ "$cont" != "$y" ] && [ "$cont" != "$Y" ] 
	then
		rm -f $lockfile
		exit 0
	fi
	/usr/bin/grep "$drive" /etc/vfstab >/dev/null 2>&1
	ret=`echo $?`
	if [ "$ret" = 0 ]
	then
		>/tmp/$$vfstab
		IFS="
"
		for i
		in `cat /etc/vfstab`
		do
			echo $i|grep $drive >/dev/null 2>&1
			ret=`echo $?`
			if [ "$ret" = 0 ]
			then
				pfmt -l $label -s info -g $msgdb:13 "\nDo you want to delete the following entry?\n\n"
				pfmt -l $label -s nostd -g $msgdb:14 "%s\n" "$i"
				pfmt -l $label -s nostd -g $msgdb:15 "\n(Type %s for yes or %s for no and press <ENTER>):\n" "$y" "$n"
				read cont
				if  [ "$cont" = "$n" ] || [ "$cont" = "$N" ] ||
				    [ "$cont" = "" ] 
				then
					echo "$i" >>/tmp/$$vfstab
				else
					CHANGE=yes
				fi
			else
				echo $i >>/tmp/$$vfstab
			fi
		done
		IFS="	"
		if [ "$CHANGE" = "yes" ]
		then
			pfmt -l $label -s info -g $msgdb:16 "saving /etc/vfstab to /etc/Ovfstab\n"
			cp /etc/vfstab /etc/Ovfstab
			pfmt -l $label -s info -g $msgdb:17 "creating a new /etc/vfstab\n"
			cp /tmp/$$vfstab /etc/vfstab
		fi
		rm /tmp/$$vfstab
	fi
	;;
*)
   pfmt -l $label -s error -g $msgdb:18 "usage: diskrm [-F s5dm]  cCCbBtTTdDD\n\n";
   rm -f $lockfile
   exit 1;;
esac
rm -f $lockfile
if [ "$CHANGE" = "yes" ]
then
	pfmt -l $label -s info -g $msgdb:19 "Diskrm for disk %s DONE at %s\n" "$driv_arg" "`date`"
else
	pfmt -l $label -s info -g $msgdb:20 "No entries for disk %s exist in /etc/vfstab\n" "$driv_arg"
fi
exit 0
