#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)acct:common/cmd/acct/dodisk.sh	1.13.2.8"
#ident "$Header$"
# 'perform disk accounting'
PATH=:/usr/lib/acct:/usr/bin:/usr/sbin
export PATH
_dir=/var/adm
_pickup=acct/nite

set -- `getopt o $*`
if [ $? -ne 0 ]
then
	echo "Usage: $0 [ -o ] [ filesystem ... ]"
	exit 1
fi
for i in $*; do
	case $i in
	-o)	SLOW=1; shift;;
	--)	shift; break;;
	esac
done

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Some systems may not use /etc/vfstab to store mount information #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
DEVLIST=/etc/vfstab
format="special dev mnt fstype fsckpass automnt mntflags"
if [ ! -f "$DEVLIST"  -a  "$SLOW" = "" ]
then
	echo "$DEVLIST does not exist: Use -o option"
	echo "Usage: $0 [ -o ] [ filesystem ... ]"
	exit 2
fi

cd ${_dir}

if [ "$SLOW" = "" ]
then
	if [ $# -lt 1 ]
	then
	# Do disk accting for file systems in DEVLIST
		while :
		do
			if read $format
		       	then
				if [ "$fsckpass" = "-" ]
				then
					continue
				fi
				if [ "$special" = "" ]
				then
					continue
				fi
				if [ "$special" = "-" ]
				then
					continue
				fi
				if [ "$dev" = "-" ]
				then
					continue
				fi
				if [ `expr $special : '\(.\)'` = \# ]
		       		then
		               		continue
		        	fi
				if [ "$fstype" = s5 ]
				then
					diskusg $dev > `basename $dev`.dtmp &
					continue
				fi
				if [ "$fstype" = bfs ]
				then
					bfsdiskusg $dev > `basename $dev`.dtmp &
					continue
				fi
				if [ "$fstype" = ufs ]
				then
					ufsdiskusg $dev > `basename $dev`.dtmp &
					continue
				fi
				if [ "$fstype" = sfs ]
				then
					sfsdiskusg $dev > `basename $dev`.dtmp &
					continue
				fi
				if [ "$fstype" = vxfs ]
				then
					vxdiskusg $dev > `basename $dev`.dtmp &
					continue
				fi
				# rfs and nfs fstypes not done 
				if [ "$fstype" = rfs ]
				then
					continue
				fi
				if [ "$fstype" = nfs ]
				then
					continue
				fi
				# fs is none of the above so use slow method
				if [ -d "$mnt" ]
				then
					find $mnt -print | acctdusg > `basename $dev`.dtmp &
				fi
			else
				wait
				break
			fi
		done < $DEVLIST
		ls *.dtmp >/dev/null 2>&1
		if [ $? = 0 ]
		then
			cat *.dtmp | diskusg -s > dtmp
			rm -f *.dtmp
		else 
			echo "dodisk: No appropriate filesystems"
		fi
	else
	# do only those file systems in arg list
		args="$*"
		for i in $args; do
			fstype=`fstyp $i`
			# sort args by fstype
			if [ "$fstype" = s5 ]
			then
				s5="$s5 $i"
				continue
			fi
			if [ "$fstype" = bfs ]
			then
				bfs="$bfs $i"
				continue
			fi
			if [ "$fstype" = sfs ]
			then
				sfs="$sfs $i"
				continue
			fi
			if [ "$fstype" = ufs ]
			then
				ufs="$ufs $i"
				continue
			fi
			if [ "$fstype" = vxfs ]
			then
				vxfs="$vxfs $i"
				continue
			fi
			# don't do rfs and nfs
			if [ "$fstype" = rfs ]
			then
				continue
			fi
			if [ "$fstype" = nfs ]
			then
				continue
			fi
			
			# all other file system types are done via acctdusg
			mnt=`df -n $i 2>/dev/null | cut -f 1 -d " "`
			if [ -n "$mnt" ]
			then
				other="$other $mnt"
			else
				# df only gets mount name for /dev/dsk
				dirname=`dirname $i`
				if [ "$dirname" = "/dev/rdsk" ]
				then
					basename=`basename $i`
					mnt=`df -n "/dev/dsk/"${basename} 2>/dev/null | cut -f 1 -d " "`
				fi
				if [ -n "$mnt" ]
				then
					other="$other $mnt"
				else
					echo "dodisk: $i not done.  Bad name or not mounted."
				fi
			fi
		done
		if [ -n "$s5" ]
		then
			diskusg $s5 > dtmp1 &
		fi
		if [ -n "$bfs" ]
		then
			bfsdiskusg $bfs > dtmp2 &
		fi
		if [ -n "$sfs" ]
		then
			sfsdiskusg $sfs > dtmp3 &
		fi
		if [ -n "$ufs" ]
		then
			ufsdiskusg $ufs > dtmp4 &
		fi
		if [ -n "$vxfs" ]
		then
			vxdiskusg $vxfs > dtmp5 &
		fi
		if [ -n "$other" ]
		then
			find $other -print | acctdusg > dtmp6 &
		fi
		wait
		cat dtmp[1-6] | diskusg -s > dtmp
		rm dtmp[1-6]
	fi
else
	if [ $# -lt 1 ]
	then
		args="/"
	else
		args="$*"
	fi
	for i in $args; do
		if [ ! -d $i ]
		then
			echo "$0: $i is not a directory -- ignored"
		else
			dir="$i $dir"
		fi
	done
	if [ "$dir" = "" ]
	then
		echo "$0: No data"
		> dtmp
	else
		find $dir -print | acctdusg > dtmp
	fi
fi

sort +0n +1 dtmp | acctdisk > ${_pickup}/disktacct 2>dodiskerr
if [ -s dodiskerr ]
then
	cat dodiskerr > /dev/conslog
	mail adm root <dodiskerr
	rm -f dodiskerr
fi
chmod 644 ${_pickup}/disktacct
chown adm ${_pickup}/disktacct
