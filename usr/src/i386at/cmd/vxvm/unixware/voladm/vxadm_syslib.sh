#!/sbin/sh -
# @(#)cmd.vxvm:unixware/voladm/vxadm_syslib.sh	1.6 11/13/97 17:14:22 - cmd.vxvm:unixware/voladm/vxadm_syslib.sh
#ident	"@(#)cmd.vxvm:unixware/voladm/vxadm_syslib.sh	1.6"

#set -x

# Copyright(C)1996 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
# UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
# LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
# IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
# OR DISCLOSURE.
# 
# THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
# TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
# OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
# EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
# 
#               RESTRICTED RIGHTS LEGEND
# USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
# SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
# (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
# COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
#               VERITAS SOFTWARE
# 1600 PLYMOUTH STREET, MOUNTAIN VIEW, CA 94043

#
# vxadm_syslib.sh -- library of routines that need to be ported
#		     for each platform
#

#
# For the 'add' operation check for any inuse devices. The user will
# decide whether these inuse partitions should be encapsulated.
#
inuse_check()
{
	mount_outfile=$tmpfile2

	mount > $mount_outfile

	inuse_list=
	not_inuse_list=

	shift $#	# Reset argument count to zero.
	set -- $dev_list
	while [ $# -gt 0 ]
	do
		device=$1
		daname=$2

# 		egrep -s /dev/dsk/$device $mount_outfile 2>/dev/null
		egrep -s  "/dev(/|/ap/)dsk/$device" $mount_outfile 2>/dev/null
		if [ $? -eq 0 ]
		then
			append_list inuse_list $device $daname
		else
			append_list not_inuse_list $device $daname
		fi
		shift $dev_num
	done

	if [ -n "$inuse_list" ]
	then
		if [ `list_count $inuse_list` -eq $dev_num ]
		then
			ewritemsg -f $disp_file -M vxvmshm:296 \
"One or more partitions of the following disk device are in use as
  a mounted file system. The disk cannot be initialized, but you can
  encapsulate the existing disk partitions as volumes.
  Output format: [Device_Name]"
		else
			ewritemsg -f $disp_file -M vxvmshm:297 \
"One or more partitions of the following disk devices are in use as
  a mounted file system. The disks cannot be initialized, but you can
  encapsulate the existing disk partitions as volumes.
  Output format: [Device_Name]"
		fi

		voladm_yorn_batch -h \
		-g $dev_num -p $dev_index -d $dev_index \
		-f $disp_file -M vxvmshm:0 -m vxvmshm:0 \
		"Encapsulate these devices?" Y \
		"Encapsulate this device?" y \
		inuse_list encap_list ignore_list

		if [ -n "$encap_list" ]
		then
			append_list ckinit_encap $encap_list
		fi
	fi

	# If any not-inuse disks were found, then set dev_list to
	# the not-inuse list and continue. Otherwise if no not-inuse
	# disks were found all other disks have been set-up for
	# encapsulation or have been ignored so return.
	if [ -n "$not_inuse_list" ]
	then
		dev_list="$not_inuse_list"
	fi
}
convert_special()
{

	ls -l ${1} > ${tmpfile1}.cs
	read x1 x2 x3 x4 maj min rest < ${tmpfile1}.cs
	maj=`expr "\$maj" : '\([0-9]*\),'`
	min=`expr $min + 512`
	res=`ls -l /dev/"${2}"/c*b*t*d*s* | grep "${maj},${min}[ 	]" \
						| awk '{ print $NF }'`
	if [ -z "$res" ]
	then
		min=`expr $min - 512`
		res=`ls -l /dev/vx/"${2}"/rootdg/* | grep "${maj},[ ]*${min}" \
						| awk '{ print $NF }'`
		#res=`vxprint -sF "%device" -e 'assoc.assoc="rootvol"'`
	fi	
	rm -f ${tmpfile1}.cs
	eval "${3}=$res"
}

getBootDiskInfo()
{
	res=
	BOOTSTAMP=
	eval `bootparam | grep BOOTSTAMP`

	if [ "${1}" = "dsk" ]
	then
		if [ $BOOTSTAMP = "" ]
		then
			res=/dev/dsk/c0b0t0d0s0
		else
			dskno=1
			while devattr -v disk$dskno bdevice cdevice stamp > /tmp/dskno$$ 2> /dev/null
			do 
				. /tmp/dskno$$
				if [ "$BOOTSTAMP" = "$stamp" ] 
				then
					rm -f /tmp/dskno$$
					echo $bdevice
					break
				fi
				rm -f /tmp/dskno$$
				dskno=`expr $dskno + 1`
			done
		fi
	else
		if [ $BOOTSTAMP = "" ]
		then
			res=/dev/rdsk/c0b0t0d0s0
		else
			dskno=1
			while devattr -v disk$dskno cdevice stamp > /tmp/dskno$$ 2> /dev/null
			do 
				. /tmp/dskno$$
				if [ "$BOOTSTAMP" = "$stamp" ]
				then
					rm -f /tmp/dskno$$
					echo $cdevice
					break
				fi
				rm -f /tmp/dskno$$
				dskno=`expr $dskno + 1`
			done
		fi
	fi
}

convert_special_uw()
{
	res=`getBootDiskInfo $2`
	if [ -z "$res" ]
	then
		res=`ls -l /dev/vx/"${2}"/rootdg/* | grep "${maj},[ ]*${min}" \
						| awk '{ print $NF }'`
		#res=`vxprint -sF "%device" -e 'assoc.assoc="rootvol"'`
	fi	
	rm -f ${tmpfile1}.cs
	eval "${3}=$res"
}
#
# get_mountdevice dir basevarname dgvarname
#
# Locate dir in the vfstab file and set the following variables as
# appropriate:
#
#	basevarname_bdev	- the block device for the file system
#	basevarname_rdev	- the raw device for the file system
#	basevarname_disk	- the standard form disk (cNbNtNdN), if the
#				  root file system is on a regular disk
#	basevarname_vol		- the volume containing the file system,
#				  if the file system is on a volume
#
# If the optional third arg is given, then volumes in disk groups other
# than rootdg are allowed, and the disk group name will be stuffed in
# the variable named in $dgvarname.
#
get_mountdevice()
{
	# On UnixWare read the /etc/vfstab to get mount information.
	# /dev/{r}root and /dev/{r}swap need more processing though.
	__bdev=
	__cdev=
	__disk=
	__vol=
	__dg=
	exec 9<&0 < $SYSROOT/etc/vfstab
	while read _special _fsckdev _mountp _fstype _fsckpass _automnt _mntopts
	do
		case ${_special} in
		'#'* | '')	#  Ignore comments, empty lines
				continue ;;
		'-')		#  Ignore no-action lines
				continue ;;
		esac

		if [ "${_mountp}" = "$1" ]
		then
			if [ "$_special" = "/dev/root" ] || \
			   [ "$_special" = "/dev/swap" ]
			then
				#echo "Special device : $_special" ;
				convert_special_uw $_special "dsk" special_dev
				_special=$special_dev
				convert_special_uw $_fsckdev "rdsk" special_dev
				_fsckdev=$special_dev
			fi
			__bdev=$_special
			__cdev=$_fsckdev
			dogi_path_to_slice ${__bdev} slc
			#echo "Slice: $slc"
			dogi_slice_to_device $slc __disk
			#echo "Disk: ${__disk}"
			#__disk=`expr "\${__bdev}" : '^/dev/dsk/\(.*\)s.\$'`
			__vol=`expr "\${__bdev}" : '^/dev/vx/dsk/\(.*\)'`
			case $__vol in
			*"/"*"/"*) __vol=;;
			*"/"*)	__dg="`expr "\$__vol" : '\(.*\)/.*'`"
				__vol="`expr "\$__vol" : '.*/\(.*\)'`";;
			*)	__dg=rootdg;;
			esac
			break
		fi
	done
	exec <&9 9<&-
	# rootdisk will  be empty if the root is defined as a volume in
	# the /etc/vfstab file.

	eval "${2}_bdev=$__bdev ${2}_cdev=$__cdev ${2}_disk=$__disk"
	if [ $# -eq 2 ] && [ "X$__dg" != Xrootdg ]
	then
		eval "${2}_vol="
	elif [ $# -eq 2 ]
	then
		eval "${2}_vol=$__vol"
	else
		eval "${2}_vol=$__vol ${3}=$__dg"
	fi
}

#
# set_bootdisk - set parameters for the root file system
#
set_rootdisk()
{
	get_mountdevice / root
	# for backward compatibility, set rootdisk
	rootdisk=$root_disk
	if [ -z "$rootdisk" ] && [ "$root_bdev" = "/dev/vx/dsk/rootdg/rootvol" ]
	then
		rootdisk=`vxprint -sF "%device" -e 'assoc.assoc="rootvol"' | \
				sed -e 's/\(.*\)s./\1/g' | sort | head -1`
	fi
}

#
# dev_to_disk - convert a DA or full slice reference to a disk reference
#
dev_to_disk()
{
	expr "$1" : '.*\(s[0-9a-f]*\)'
}

get_drv_list() 
{
	dir_entry="`ls -ld \$1`"
	path=`expr "$dir_entry" : ".*\/devices\(.*\)"`
	[ -n "$path" ] || {
		egettxt >&2 \
			'get_drv_list: Invalid device file: $1' vxvmshm:638 $1
		return 1
	}

	node=`basename $path`
	path=`dirname $path`

	while [ -n "$path" -a "X$node" != "X/" ]; do
		case $node in
			*@*)	drv=`expr $node : "\(.*\)@.*"`
				;;
			*)	drv=$node
				;;
		esac
		echo $drv
		node=`basename $path`
		path=`dirname $path`
	done
}

forceload_drv()
{
	file=$SYSROOT/etc/system
	cp $file $file.sav
	drv=$1
	grep "forceload: drv/$drv" $file 2>&1 > /dev/null
	if [ $? -eq 0 ] ; then
		return 0
	fi

	end_line=`grep -n vxvm_END $file`
	if [ -z "$end_line" ] ; then
	sed \
	-e '$ a\
* vxvm_START\
* vxvm_END' $file > $file.sav 
	cp $file.sav $file
	end_line=`grep -n vxvm_END $file`
	fi

	end_line=`expr "$end_line" : "\(.*\):.*"`
	end_line=`expr $end_line - 1`
	sed \
	-e ''$end_line' a\
forceload: 'drv/$drv'' $file > $file.sav
	cp $file.sav $file
}

#
# Routines to comment/uncomment "rootdev:" lines from /etc/system
#

# Common pattern variables
rpattern='^rootdev:'
block="*(vxvm rootability)*"
eblock='\*(vxvm rootability)\*'
cpattern="${eblock} rootdev:.*"

#
# Name:	comment_out_rootdev()
#
# Purpose: Any line matching $rpattern will be replaced by one line
# 	   composed of the matching line preceded by $block.
#
comment_out_rootdev()
{
	if [ $# -gt 1 ]
	then
		file=$1
	else
		file=$SYSROOT/etc/system
	fi

	# Return if no "rootdev" specs are found
	grep "${rpattern}" $file 2>&1 > /dev/null
	if [ $? -ne 0 ]; then
		return 0
	fi

	# Comment each one 
	sed -e "/${rpattern}/s,.*,${block} &," $file > $file.cor.save

	mv $file.cor.save $file

	return 0
}

#
# Name: restore_commented_rootdev()
#
# Purpose: Any line matching $cpattern will be replaced by one line
# 	   composed of the matching line with $block removed.  
#
restore_commented_rootdev()
{
	if [ $# -gt 1 ]
	then
		file=$1
	else
		file=$SYSROOT/etc/system
	fi

	# Return if no "rootdev" specs are found
	grep "${cpattern}" $file 2>&1 > /dev/null
	if [ $? -ne 0 ]; then
		return 0
	fi

	# Uncomment each one 
	sed -e "/${cpattern}/s,${eblock} \(.*\),\1," $file > $file.rcr.save

	mv $file.rcr.save $file

	return 0
}

#
# Set OS specific variables
#
set_OS_variables()
{
	VOL_FULL_SLICE=s0
	VOL_PRIV_SLICE=15
	VOL_PUB_SLICE=14
	VOL_GHOST_LEN=0
	VOL_PRIV_OFFSET=0
	EDVTOC=edvtoc
	PRTVTOC=prtvtoc
        VOLD=vxconfigd

	VOL_DISK_RAWPATH="/dev/rdsk"
	VOL_DISK_BLKPATH="/dev/dsk"
	VOL_AP_DISK_RAWPATH="/dev/ap/rdsk"
	VOL_AP_DISK_BLKPATH="/dev/ap/dsk"

	VOL_ROOTABLE_FILESYSTEMS="/ /usr /stand /var /home"
	VOL_KEEPER_FILESYSTEMS="/ /usr /stand /var /home"

	VOL_VFSTAB_PATH="/etc/vfstab"

	export VOL_FULL_SLICE VOL_PRIV_SLICE VOL_PUB_SLICE VOL_PRIV_OFFSET 
	export EDVTOC PRTVTOC VOLD
	export VOL_GHOST_LEN
	export VOL_DISK_RAWPATH VOL_DISK_BLKPATH
	export VOL_AP_DISK_RAWPATH VOL_AP_DISK_BLKPATH
	export VOL_ROOTABLE_FILESYSTEMS
	export VOL_VFSTAB_PATH
}

voladm_mounted_filesystems()
{
	fslist=`/sbin/mount | sed 's/ .*//'`
	if [ $# -gt 0 ]
	then
		eval "${1}=\"$fslist\""
	else
		echo $fslist
	fi
}

#
# voladm_mount_table - output (or copy to a file) the
# table of "filesystems to be moutned". On some unix systems,
# root (/) is not part of the vfstab, so it may need to
# be represented explicitly.

voladm_mount_table()
{
	if [ $# -gt 0 ]
	then
	      xcmd cp $SYSROOT/$VOL_VFSTAB_PATH ${1}
	else
	      cat $SYSROOT/$VOL_VFSTAB_PATH
	fi
}

voladm_disk_in_use()
{
	dogi_slice_blkpath $1 $diskpath
	mount | grep $diskpath  > /dev/null 2> /dev/null
	if [ $? = 0 ] ; then
		return 0
	else
		return 1
	fi	
}

#
# is_std_disk - test if a disk is standard format
#
is_std_disk()
{
	_isd_base="m\{0,1\}c[0-9][0-9]*b[0-9][0-9]*t[0-9][0-9]*d[0-9][0-9]*"
	_isd_slice=s[0-9a-f]*
	case $1 in
	"/dev/dsk/"*s*)	expr "$1" : "^/dev/dsk/$_isd_base$_isd_slice\$";;
	"/dev/rdsk/"*s*) expr "$1" : "^/dev/rdsk/$_isd_base$_isd_slice\$";;
	"/dev/ap/dsk/"*s*) expr "$1" : "^/dev/ap/dsk/$_isd_base$_isd_slice\$";;
	"/dev/ap/rdsk/"*s*) expr \
	 	"$1" : "^/dev/ap/rdsk/$_isd_base$_isd_slice\$";;
	*s*)		expr "$1" : "^$_isd_base$_isd_slice";;
	*)		expr "$1" : "^$_isd_base$_isd_slice";;
	esac
}

chroot_to_vol()
{
	file=$SYSROOT/etc/system
	grep "vxvm_END" $file 1>&2 > /dev/null
	if [ $? -ne 0 ] ; then
		echo vxvm BLOCK NOT FOUND
		return 1
	fi

	# First, comment out any "old" rootdev specs
	comment_out_rootdev $file

	end_line=`grep -n vxvm_END $file`
	end_line=`expr "$end_line" : "\(.*\):.*"`
	end_line=`expr $end_line - 1`
	sed \
	-e ''$end_line' a\
rootdev:/pseudo/vxio@0:0\
set vxio:vol_rootdev_is_volume=1' $file > $file.sav
	mv $file.sav $file
	return 0
}

#
# This is used by some if the init.d scripts
#
set_pub_attr()
{

	#
	# Find the len and offset of the public region. 
	# If priv at start it is straightforward
	# else It should be the 
	# len of pub partition - len of priv partition - pub offset + priv
	# offset + priv offset because pub should enter the priv for the
	# ghost sd For an encapsulated root disk with priv carved out of swap,
	# the priv will be lying in the middle of pub. In this case return 
	# len of pub partition - pub offset
	#

	dogi_slice_rawpath $1 rawpath
	eval `vxparms`
	eval `vxtaginfo -s $rawpath $VOL_PUB_SLICE_TAG 2> /dev/null`
	pub_start=$PART_START
	pub_size=$PART_SIZE
	pub_end=`expr $pub_start + $pub_size`
	eval `vxtaginfo -s $rawpath $VOL_PRIV_SLICE_TAG 2> /dev/null`
	priv_start=$PART_START
	priv_size=$PART_SIZE
	eval `vxpartinfo -s $rawpath 2 2> /dev/null`
	full_start=$PART_START
	full_size=$PART_SIZE
	priv_end=`expr $priv_start + $priv_size`
	if [ $priv_start -eq 0 ] 
	then
		VOL_PUB_OFFSET=0
		VOL_PUB_LEN=$pub_size
	elif [ $priv_end -eq $full_size ] 
	then
		VOL_PUB_OFFSET=1
		if [ $pub_end -gt $priv_start ] ; then
			VOL_PUB_LEN=`expr $pub_size - $priv_size - \
			     $VOL_PUB_OFFSET + $VOL_PRIV_OFFSET`
		else
			VOL_PUB_LEN=`expr $pub_size - $VOL_PUB_OFFSET`
		fi
	else
		VOL_PUB_OFFSET=1
		VOL_PUB_LEN=`expr $pub_size - $VOL_PUB_OFFSET`
	fi
	export VOL_PUB_LEN VOL_PUB_OFFSET
}

#
# THESE ARE NOT USED
#

vol_add_slice()
{
	#
	# If device specified as cCbBtTdD then  add s0
	#
	VOL_FULL_DISK=$1

	if vxcheckda $1
	then
		VOL_FULL_DISK=$VOL_FULL_DISK$VOL_FULL_SLICE
	fi
	export VOL_FULL_DISK
}

pipe_escape()
{
	echo $1 | sed -e 's/\|/\\|/g'
}
#
# add_cntrlr_expression -- add the given controller name to the
# expression to egrep out controller names. The second argument should
# be the variable name of the expression to be used.
#
add_cntrlr_expression()
{
	cntrlr=$1

	eval "expression=\${${2}}"

	if [ -z "$expression" ]
	then
		expression="${cntrlr}b"
	else
		expression="${expression}|${cntrlr}b"
	fi

	expression=`pipe_escape $expression`

	eval "${2}=$expression"
}

#
# add_device_expression -- add the given disk name to the
# expression to egrep out disk access names. The second argument should
# be the variable name of the expression to be used.
#
add_device_expression()
{
	dogi_whole_slice $1 device

	eval "expression=\${${2}}"

	if [ -z "$expression" ]
	then
		expression="${device}"
	else
		expression="${expression}|${device}"
	fi

	expression=`pipe_escape $expression`
	eval "${2}=$expression"
}

#
# extr_list_info -- extract information from a long listing of a device file.
#
extr_list_info()
{
	listing=$1
	diskname=$2

	cinfo=
	cntrlr=`expr "$diskname" : "\(c[0-9][0-9]*\)b[0-9][0-9]*t[0-9][0-9]*d[0-9][0-9]*${VOL_FULL_SLICE}"`

	eval "${3}=${cinfo}"
	eval "${4}=${cntrlr}"
}

extr_plninfo()
{
	info=$1
	ssa_id=`expr "$info" : '.*/SUNW,pln@[ab]\([0-9,a-fA-F]*\).*'`
	[ -n "$ssa_id" ] || return 1
	eval "${2}=${ssa_id}"
}

tchar="-"
twirl()
{
	/bin/echo "$tchar\b\c"
	case $tchar in
		"-") tchar="/";;
		"/") tchar="|";;
		"|") tchar="\\\\";;
		"\\\\") tchar="-";;
	esac
}

#
# controllers_on_system -- list all controllers on the system
#
controllers_on_system()
{
	ctl_temp=
	mctl_temp=

	tfile=${TMPDIR:-/tmp}/ctrltmp.$$

	if [ -d "$VOL_AP_DISK_RAWPATH" ]
	then
		>$tmpfile3
		# Look for AP metadevice controller ids. This will
		# be used to eliminate the underlying base controllers
		# later.
		xcmd cd $VOL_AP_DISK_RAWPATH
		ls | \
		egrep "mc[0-9][0-9]*t[0-9][0-9]*d[0-9][0-9]*${VOL_FULL_SLICE}" | \
		 while read mdisk
		 do
			 twirl >&2
			 #echo "mtemp: $mcntl_temp"
			 mcntr_info=`ls -l $mdisk`
			 case $mcntr_info in
				 l*" -> "*)	: ;;
				 *)		continue;;
			 esac

			 extr_list_info "$mcntr_info" $mdisk mcntr_info mcntr
			 list_member $mcntr $mcntl_temp && continue
			 append_list mcntl_temp $mcntr


			 if [ -f $CNTRL_EXCLUDE_FILE ]
			 then
				 if fgrep -x "$mcntr" $CNTRL_EXCLUDE_FILE \
				  >/dev/null 2>&1
				 then
					 cntr="$cntr" \
					  egettxt "<excluding $cntr>" \
					  vxvmshm:87 >&2
					 continue
				 fi
			 fi

			 echo "$mcntr $mcntr_info"

			 # Find the base controller that it subsumes and put
			 # it on a list
			 disk=`expr "$mdisk" : \
			  "m\(c[0-9][0-9]*t[0-9][0-9]*d[0-9][0-9]*${VOL_FULL_SLICE}\)"`
			 cinfo=`ls -l $VOL_DISK_RAWPATH/$disk`
			 extr_list_info "$cinfo" $disk cntr_info cntr
			 if extr_plninfo $cntr_info ssa_id
			 then
				 echo $ssa_id >> $tfile
			 fi
			 list_member $cntr $cntr_covered && continue
			 append_list cntr_covered $cntr
			 append_list cntr_covinfo $cntr_info
		 done
	fi

	# echo "covered:      $cntr_covered"
	# echo "covered info: $cntr_covinfo"
	ssalist=
	xcmd cd $VOL_DISK_RAWPATH
	ls | egrep "c[0-9][0-9]*b[0-9][0-9]*t[0-9][0-9]*d[0-9][0-9]*${VOL_FULL_SLICE}" | \
	 while read disk
	 do
		 twirl >&2
		 cntr_info=`ls -l $disk`
#Solaris#		 case $cntr_info in
#Solaris#			 l*" -> "*)	: ;;
#Solaris#			 *)		continue;;
#Solaris#		 esac
#Solaris#
#Solaris#		 extr_list_info "$cntr_info" $disk cntr_info cntr
		 $PRTVTOC -f /dev/null ./$disk 2> /dev/null
		 if [ $? -ne 0 ]; then
			 continue
		 fi
		 extr_list_info "$cntr_info" $disk cntr_info cntr
		 list_member $cntr $ctl_temp && continue
		 append_list ctl_temp $cntr

		 # Skip it if it's subsumed
		 if list_member $cntr $cntr_covered
		 then
			 cntr="$cntr" \
			  egettxt "<controller $cntr covered by metadevice>" \
			  vxvmshm:85 >&2
			 continue
		 fi

		 # See if there's an SSA alternate
		 if extr_plninfo $cntr_info ssa_id
		 then
			 grep -- $ssa_id $tfile >/dev/null 2>&1
			 if [ $? -eq 0 ]
			 then
				 cntr="$cntr" \
				  egettxt \
				  "<controller $cntr covered by metadevice>" \
				  vxvmshm:85 >&2
				 continue
			 fi
			 if list_member $ssa_id ssalist
			 then
				 cntr="$cntr" \
				  egettxt \
				  "<controller $cntr is an alternate path to SSA #$ssa_id>" \
				  vxvmshm:86 >&2
				 continue
			 fi
		 fi

		 if [ -f $CNTRL_EXCLUDE_FILE ]
		 then
			 if fgrep -x "$cntr" $VOL_CONFIG_DIR/cntrls.exclude \
			  > /dev/null 2>&1
			 then
				 cntr="$cntr" egettxt "<excluding $cntr>" vxvmshm:87 >&2
				 continue
			 fi
		 fi
		 echo "$cntr $cntr_info"
	 done
}

#
# disks_on_controller -- given a controller number, list all disks
# that we can find on that controller to stdout.

disks_on_controller()
{
	cntrlr=$1

	(cd $VOL_DISK_RAWPATH;
	ls;
	if [ -d "$VOL_AP_DISK_RAWPATH" ]
	then
		cd $VOL_AP_DISK_RAWPATH;
		ls
	fi) | egrep "m?${1}b[0-9][0-9]*t[0-9][0-9]*d[0-9][0-9]*${VOL_FULL_SLICE}"
}

#
# expand_device_wildcard -- expand a disk specification into all
# matching devices. This is used in voladm_disk_device_list. It honors
# controller/device exclusion patterns.
#
expand_device_wildcard()
{
	list_var=$1
	eval "disklist=\${${list_var}}"
	excl_disks=$2
	excl_cntrlr=$3
	disk_excluded_var=$4
	eval "disk_excl=\${$disk_excluded_var}"
	cntrlr_excluded_var=$5
	eval "cntrlr_excl=\${$cntrlr_excluded_var}"
	shift 5

	if list_member all $@
	then
		if [ $# -gt 1 ]
		then
			ewritemsg -M vxvmshm:83 \
			 "'all' should appear alone, enter ?? for help"
			return 1
		fi
		if list_member allc $@
		then
			ewritemsg -M vxvmshm:254 "Input not recognized: \"allc\""
			return 1
		elif list_member allm $@
		then
			ewritemsg -M vxvmshm:255 "Input not recognized: \"allm\""
			return 1
		fi
		set -- allc allm
	fi

	while [ $# -gt 0 ]
	do
		spec=$1
		pattern=

		# Verify that it's valid
		slen=`strlen $spec`
		ldir=
		case "$spec" in
			c[0-9]*b[0-9]*t[0-9]*d[0-9]*)
			    mlen=`expr $spec : \
			     'c[0-9][0-9]*b[0-9][0-9]*t[0-9][0-9]*d[0-9][0-9]*'`
			    [ $mlen -eq $slen ] && pattern="$spec"
			    ldir=$VOL_DISK_RAWPATH
			    ;;

			c[0-9]*b[0-9]*t[0-9]*)
			    mlen=`expr $spec : \
			     'c[0-9][0-9]*b[0-9][0-9]*t[0-9][0-9]*'`
			    [ $mlen -eq $slen ] && pattern="${spec}d*"
			    ldir=$VOL_DISK_RAWPATH
			    ;;

			c[0-9]*b[0-9]*)
			    mlen=`expr $spec : \
			     'c[0-9][0-9]*b[0-9][0-9]*'`
			    [ $mlen -eq $slen ] && pattern="${spec}t*"
			    ldir=$VOL_DISK_RAWPATH
			    ;;

			c[0-9]*)
			    mlen=`expr $spec : \
			     'c[0-9][0-9]*'`
			    [ $mlen -eq $slen ] && pattern="${spec}b*"
			    ldir=$VOL_DISK_RAWPATH
			    ;;

			allc)
			    pattern="c*"
			    ldir=$VOL_DISK_RAWPATH
			    ;;

			mc[0-9]*t[0-9]*d[0-9]*)
			    mlen=`expr $spec : \
			     'mc[0-9][0-9]*t[0-9][0-9]*d[0-9][0-9]*'`
			    [ $mlen -eq $slen ] && pattern="$spec"
			    ldir=$VOL_AP_DISK_RAWPATH
			    ;;

			mc[0-9]*t[0-9]*)
			    mlen=`expr $spec : \
			     'mc[0-9][0-9]*t[0-9][0-9]*'`
			    [ $mlen -eq $slen ] && pattern="${spec}d*"
			    ldir=$VOL_AP_DISK_RAWPATH
			    ;;

			mc[0-9]*)
			    mlen=`expr $spec : \
			     'mc[0-9][0-9]*'`
			    [ $mlen -eq $slen ] && pattern="${spec}t*"
			    ldir=$VOL_AP_DISK_RAWPATH
			    ;;

			allm)
			    pattern="mc*"
			    ldir=$VOL_AP_DISK_RAWPATH
			    ;;

			*) pattern=
			   ldir=
			   ;;
		esac

		if [ -z "$pattern" ]
		then
			ewritemsg -M vxvmshm:253 \
"Input not recognized: \"$spec\""
			return 1
		fi

		push_list disklist \
		 `ls -1 ${ldir}/${pattern}${VOL_FULL_SLICE} 2>/dev/null | \
			tee $_vdskd_rawmatch |
			if [ -n "$excl_cntrlr" ]
			then
				egrep -v -e "$excl_cntrlr"
			else
				cat
			fi |
			tee $_vdskd_cntrlmatch |
			if [ -n "$excl_disks" ]
			then
				egrep -v -e "$excl_disks"
			else
				cat
			fi |
			tee $_vdskd_diskmatch |
			sed "s%${ldir}/\(.*\)${VOL_FULL_SLICE}$%\1%"`

		# Add excluded controllers and disks to the lists
		append_list $cntrlr_excluded_var \
		 `comm -3 $_vdskd_rawmatch $_vdskd_cntrlmatch`
		append_list $disk_excluded_var \
		 `comm -3 $_vdskd_cntrlmatch $_vdskd_diskmatch`

		shift
	done

	eval "$list_var=\"$disklist\""
	return 0
}

#
# These are the dogi_* routines used by the rest of the administrative
# scripts. These should mask out any system-specific DA-naming or
# disk-related dependencies.
#

#
# dogi_name_is_device -- returns true if the (single) argument is
# in the form of a device name.
#
dogi_name_is_device()
{
	#
	# return true if the name is a valid device name
	#
	name=$1
	case "$name" in
		mc*)
		     name_len=`strlen $name`;
		     match_len=`expr $name : \
		      'mc[0-9][0-9]*t[0-9][0-9]*d[0-9][0-9]*'`
		     [ "$name_len" -eq "$match_len" ]
		     ;;
		c*)
		    name_len=`strlen $name`;
		    match_len=`expr $name : \
		     'c[0-9][0-9]*b[0-9][0-9]*t[0-9][0-9]*d[0-9][0-9]*'`
		    [ "$name_len" -eq "$match_len" ]
		    ;;
		*) false;;
	esac
}

#
# dogi_name_is_slice -- returns true if the (single) argument is in
# the form of a slice name.
#
dogi_name_is_slice()
{
	#
	# return true if the name looks like a slice of a disk
	#
	name=$1
	case "$name" in
		mc*)
		     name_len=`strlen $name`;
		     match_len=`expr $name : \
		      'mc[0-9][0-9]*t[0-9][0-9]*d[0-9][0-9]*s[0-9][0-9]*'`
		     [ "$name_len" -eq "$match_len" ]
		     ;;
		c*)
		    name_len=`strlen $name`;
		    match_len=`expr $name : \
		     'c[0-9][0-9]*b[0-9][0-9]*t[0-9][0-9]*d[0-9][0-9]*s[0-9a-f]*'`
		    [ "$name_len" -eq "$match_len" ]
		    ;;
		*) false;;
	esac
}

#
# dogi_slice_num -- echo the slice part of the complete slice name
#
#	If optional second argument is provided, will set a variable by
#	that name to the slice portion of the first argument
#
dogi_slice_num()
{
	name=$1
	case "$name" in
		mc*)
		     s=`expr $name : \
		      'mc[0-9][0-9]*t[0-9][0-9]*d[0-9][0-9]*\(s[0-9][0-9]*\)'`
		     ;;
		c*)
		    s=`expr $name : \
		     'c[0-9][0-9]*b[0-9][0-9]*t[0-9][0-9]*d[0-9][0-9]*\(s[0-9a-f]*\)'`
		    ;;
		*)
		   s="";;
	esac
	if [ $# -gt 1 ]
	then
		eval "${2}=$s"
	else
		echo $s
	fi
}

#
# dogi_is_whole_slice -- return true if the slice names the special "whole"
# slice for the disk
#
dogi_is_whole_slice()
{
	if not dogi_name_is_slice "$1"
	then
		return 1
	fi

	dogi_slice_num $1 slice_name
	[ "$slice_name" = "${VOL_FULL_SLICE}" ]
}

#
# dogi_device_slice -- create a slice name out of the given device
# name and slice
dogi_device_slice()
{
	eval "${3}=${1}s${2}"
}

#
# dogi_slice_to_device -- given a slice name, return the device it names
#
#
dogi_slice_to_device()
{
	name=$1
	if not dogi_name_is_slice $name
	then
		return 1
	fi

	case "$name" in
		mc*)
		     d=`expr $name : \
		      '\(mc[0-9][0-9]*t[0-9][0-9]*d[0-9][0-9]*\)s[0-9][0-9]*'`
		     ;;
		c*)
		    d=`expr $name : \
		     '\(c[0-9][0-9]*b[0-9][0-9]*t[0-9][0-9]*d[0-9][0-9]*\)s[0-9a-f]*'`
		    ;;
		*) d="";;
	esac

	if [ $# -gt 1 ]
	then
		eval "${2}=$d"
	else
		echo $d
	fi
}

#
# dogi_name_to_device -- return the device name, given a name
#
dogi_name_to_device()
{
	name=$1
	if dogi_name_is_device $name
	then
		d=$name
	else
		dogi_slice_to_device $name d
	fi

	if [ $# -gt 1 ]
	then
		eval "${2}=$d"
	else
		echo $d
	fi
}

#
# dogi_slice_from_path -- given a disk pathname, strip off everything
# but the slice name.
#
dogi_path_to_slice()
{
	case $1 in
		"/dev/dsk/"*s*) d=`expr "$1" : '^/dev/dsk/\(.*s.\)\$'`;;
		"/dev/rdsk/"*s*) d=`expr "$1" : '^/dev/rdsk/\(.*s.\)\$'`;;
		"/dev/ap/dsk/"*s*)
			d=`expr "$1" : '^/dev/ap/dsk/\(.*s.\)\$'`;;
		"/dev/ap/rdsk/"*s*)
			d=`expr "$1" : '^/dev/ap/rdsk/\(.*s.\)\$'`;;
	esac

	if [ $# -gt 1 ]
	then
		eval "${2}=$d"
	else
		echo $d
	fi
}

#
# dogi_whole_slice -- given a device, return the slice name of the
# "whole slice"
#
# This is not necessarily a real DOGI interface function; it's a shortcut
# for solaris since the whole path is the device manipulation interface (i.e.,
# for prtvtoc, disk ioctls, etc.)
#
dogi_whole_slice()
{
	name=$1
	if not dogi_name_is_device $name
	then
		return 1
	fi

	p="${name}${VOL_FULL_SLICE}"
	if [ $# -gt 1 ]
	then
		eval "${2}=${p}"
	else
		echo $p
	fi
}

#
# dogi_device_rawpath -- return the path to the character node for the device
#

dogi_device_rawpath()
{
	if not dogi_name_is_device $1
	then
		return 1
	fi
	
	dogi_whole_slice "$1" w || return 1
	dogi_slice_rawpath $w pth

	if [ $# -gt 1 ]
	then
		eval "${2}=${pth}";
	else
		echo "$pth";
	fi
}


dogi_slice_rawpath()
{
	name=$1

	if not dogi_name_is_slice $name
	then
		return 1
	fi

	case $name in
		mc*)
		     p="${VOL_AP_DISK_RAWPATH}/${name}";;
		c*)
		    p="${VOL_DISK_RAWPATH}/${name}";;
		*) return 1;;
	esac

	if [ $# -gt 1 ]
	then
		eval "${2}=${p}";
	else
		echo "$p";
	fi
}

#
# dogi_device_blkpath -- return the path to the block node for the device
#
dogi_device_blkpath()
{
	if not dogi_name_is_device $1
	then
		return 1
	fi
	
	dogi_whole_slice "$1" w || return 1
	dogi_slice_blkpath $w pth
	if [ $# -gt 1 ]
	then
		eval "${2}=${pth}"
	else
		echo "$pth"
	fi
}


dogi_slice_blkpath()
{
	name=$1

	if not dogi_name_is_slice $name
	then
		return 1
	fi

	case $name in
		mc*)
		     p="${VOL_AP_DISK_BLKPATH}/${name}";;
		c*)
		    p="${VOL_DISK_BLKPATH}/${name}";;
		*) return 1;;
	esac

	if [ $# -gt 1 ]
	then
		eval "${2}=${p}";
	else
		echo "$p";
	fi
}

#
# dogi_name_daname -- return an appropriate DA record name for the
# given device name
#
dogi_name_daname()
{
	name=$1

	if not dogi_name_is_device $name
	then
		return 1
	fi

	dogi_whole_slice "$name" daname || return 1

	if [ $# -gt 1 ]
	then
		eval "${2}=${daname}"
	else
		echo "$daname"
	fi
}

#
# dogi_root_device -- return true if the named device is the boot disk
#
dogi_root_device()
{
	dname=$1
	if not dogi_name_is_device $dname
	then
		return 1
	fi
	
	set_rootdisk
	[ "$dname" = "$root_disk" ]
}

#
# dogi_name_is_cntrlr -- returns true if the argument is in the form
# of a controller name.
#
dogi_name_is_cntrlr()
{
	name=$1
	case "$name" in
		mc*)
		     name_len=`strlen $name`;
		     match_len=`expr $name : \
		      'mc[0-9][0-9]*'`
		     [ "$name_len" -eq "$match_len" ]
		     ;;
		c*)
		    name_len=`strlen $name`;
		    match_len=`expr $name : \
		     'c[0-9][0-9]*'`
		    [ "$name_len" -eq "$match_len" ]
		    ;;
		*) false;;
	esac
}

#
# dogi_darecs_on_device -- list any DA records that are defined on
# the named device. The form of the list is "<da> <tm> <type>". The
# list is put into the variable named by the second argument. If any
# are found, return true.
#
dogi_darecs_on_device()
{
	name=$1

	> $tmpfile3
	vxdisk list | grep "${name}s" > $tmpfile4

	if [ -s $tmpfile4 ]
	then
		xlist=`cat $tmpfile4`
		rm $tmpfile4
	fi

	if [ $# -gt 1 ]
	then
		eval "${2}=\"${xlist}\""
	else
		echo "$xlist"
	fi
	[ `list_count $xlist` -ne 0 ]
}

#
# dogi_valid_vtoc -- return false if the disk does not have a valid
# table of contents.
#
dogi_valid_vtoc()
{
	name=$1
	if dogi_name_is_slice $name
	then
		dogi_slice_to_device $name name
	elif not dogi_name_is_device $name
	then
		return 1
	fi

	dogi_device_rawpath $name diskpath

	$PRTVTOC -f /dev/null $diskpath >/dev/null 2>&1
}

#
# dogi_is_boot_device -- return true if the named device or disk media 
#                      record is the boot disk
#                      disk names are in the cNbNtNdN form
#
dogi_is_boot_device()
{
      dname=$1
      get_mountdevice / root

      #
      # If the aegument is a real disk device then check 
      # to see if it matches the root_disk var set by the
      # get_mountdevice call
      #
      if dogi_name_is_device $dname
      then
              [ "$dname" = "$root_disk" ] && return 1
              return 0
      fi

      #
      # The argument must be a dm name
      #
      dm=$dname

      dmlist=`vxprint -F %dm -e as.as=\"$root_vol\" | sort -u`

      for bootdm in $dmlist
      do
              [ "$bootdm" = "$dm" ] && return 1
      done

      return 0
}

#
# dogi_get_prompath -- For systems that support setting their eeproms,
# determine correct path independent of disk name spaces.  Given a
# slice name for the new root device return the appropriate block path
# name.  
#
dogi_get_prompath()
{
	name=$1

	if not dogi_name_is_slice $name
	then
		return 1
	fi

	# 
	# Return the block path for slices that are NOT from AP
	# metadisks.  
	#
	# Otherwise, here's the deal with AP metadisk devices:  What
	# the eeprom needs is the raw disk path name of the active
	# alternate path.
	#
	# EXACT METHOD: Get alternate path group for metadisk and 
	# then determine which of the two is the active alternate.
	# 
	# GUESS METHOD: AP metadisks map to at most two alternate 
	# paths and the naming is coordinated:  e.g., mc3t4d0 refers to
	# alternate path c3t4d0 and cXt4d0 where "X" is unknown at this
	# point. So, just strip the 'm' off and you have at least one
	# alternate path.
	#

	case $name in
		mc*)
			#
			# EXACT METHOD -- not implemented
			#
			# dogi_get_ap $name ap1 ap2
			# One of these two should be the primary ap
			# if dogi_is_ap_primary $ap1
			# then
			# ap_primary=$ap1
			# else if dogi_is_ap_primary $ap2
			# then
			# ap_primary=$ap2
			# else
			# return 1        
			# fi

			#
			# GUESS METHOD
			#
			ap_primary=`expr $name : '^m\(.*\)'`

			# Get the block node for this primary ap
			dogi_slice_blkpath $ap_primary blkpath
			;;
		c*)
			dogi_slice_blkpath $name blkpath
			;;
		*)
			return 1
			;;
	esac

	# Return information to caller
	if [ $# -gt 1 ]
	then
		eval "${2}=${blkpath}"
	else
		echo "$blkpath"
	fi
}
