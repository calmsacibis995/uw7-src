#!/sbin/sh
#ident	"@(#)rsk_image.sh	15.1"

#This function maps the OS partition type to the corresponding
#number used by fdisk command
find_os_num()
{
	OS_NUM=0
	case "$1" in

		"UNIX System" )
			OS_NUM=1
			;;
		"pre-5.0DOS" ) 
			OS_NUM=2
			;;
		"DOS" ) 
			OS_NUM=3
			;;
		"System" )
			OS_NUM=4
			;;
		"Other" )
			OS_NUM=5
			;;
		*)	;;
	esac
}

#This function creates the ${FD_INFO} file corresponding to $1
#FD_INFO  file will contain the partition info corresponding to $1

fdisk_cmds()
{
   RTDEV=`devattr $1 cdevice 2>/dev/null`

   FDSKOUT=/tmp/fd_out export FDSKOUT
   echo x | LC_ALL=C fdisk -L $RTDEV >${FDSKOUT} 
   RDEV=`echo $RTDEV | sed "s/..$//"`
   eval `grep NPART ${FDSKOUT}`
   eval `grep NUMMB ${FDSKOUT}`
   echo "$RTDEV $NPART $NUMMB" > ${FD_INFO}

   PART=1
   # Create a file to contain all the partitions info.
   while [ $PART -le $NPART ]
   do
	OS_LINE=`grep PART$PART ${FDSKOUT}`
	OS_STAT=`echo "$OS_LINE" | cut -f2`
	OS_TYPE=`echo "$OS_LINE" | cut -f3`
	OS_SIZE=`echo "$OS_LINE" | cut -f8 | cut -f1 -d'"'`
	find_os_num "$OS_TYPE"
	OS_ST="0"
	[ "$OS_STAT" = "Active" -a $OS_NUM -eq 1 ]  && OS_ST=1
	echo "$OS_NUM $OS_ST $OS_SIZE" >> ${FD_INFO}
	PART=`expr $PART \+ 1`
   done
   rm -f ${FDSKOUT}
}  # End of fdisk_cmds

#this function creates the $LAYOUT_FL to match the existing
#file systems in ${RDEV}s0 unix slice
#This file will be used to setup the corresponding disk

dskset_cmds ()
{
    LC_ALL=C prtvtoc -f $PRT_FOUT ${RDEV}s0
    LC_ALL=C prtvtoc ${RDEV}s0 > ${PRT_OUT}
    >${LAYOUT_FL}
    TOTL_FLs=`grep " 0	0x" < $PRT_FOUT | cut -f5`
    SWAP_FLs=0
    SWAP_SLICE=200
    grep SWAP $PRT_OUT >/dev/null
    [ $? -eq 0 ] && {
	SWAP_SLICE=`grep SWAP $PRT_OUT | cut -f1 -d":" | cut -f2 -d" "`
    	SWAP_FLs=`grep " ${SWAP_SLICE}	0x" $PRT_FOUT | cut -f5`
    }
    DUMP_SLICE=200
    grep DUMP $PRT_OUT  >/dev/null
    [ $? -eq 0 ] && 
	DUMP_SLICE=`grep DUMP $PRT_OUT | cut -f1 -d":" | cut -f2 -d" "`
    STAND_FLs=0
    STAND_SLICE=200
    grep STAND $PRT_OUT >/dev/null
    [ $? -eq 0 ] && {
	STAND_SLICE=`grep STAND $PRT_OUT | cut -f1 -d":" | cut -f2 -d" "`
    	STAND_FLs=`grep "${STAND_SLICE}	0x" $PRT_FOUT | cut -f5`
    }
    VOL_FLs=0
    VOL_SLICE=200
    grep VOLPRIVATE $PRT_OUT >/dev/null
    [ $? -eq 0 ] && {
	VOL_SLICE=`grep VOLPRIVATE $PRT_OUT | cut -f1 -d":" | cut -f2 -d" "`
    	VOL_FLs=`grep "${VOL_SLICE}	0x" $PRT_FOUT | cut -f5`
    }
    BOOT_FLs=`grep " 7	0x" < $PRT_FOUT | cut -f5`
    ALT_FLs=`grep " 8	0x" < $PRT_FOUT | cut -f5`
    TOTL_EXP_FLs=`expr $TOTL_FLs \- $SWAP_FLs \- $STAND_FLs \- $BOOT_FLs \- $ALT_FLs \- $VOL_FLs`

# go through entire 'prtvtoc' output (to be able to get extended [>16] slices)
    while read slice rest
    do
	[ $slice = "#SLICE" -o $slice = "0" -o $slice = "7" -o $slice = "8" ] && continue
	ii=$slice
#device node is slice number in hexadecimal
	ap=`echo $slice|awk '{ printf( "%x\n", $1) }'`
	if [ $slice -lt 10 ] 
	then
	    ii=" $slice"
	fi

	grep "^$ii	0x0	0x0	0	0" < $PRT_FOUT > /dev/null
	[ $? -eq  0 ] && continue
	AN=`basename $RDEV`
 	BDSK=/dev/dsk/${AN}s${ap}
	MNTPNT=`grep "^$BDSK[ 	]" /etc/vfstab | sort -u | cut -f3`
	[ -z "$MNTPNT" ] &&
	   MNTPNT=`mount | grep $BDSK | cut -f1 -d" "`
	[ -z "$MNTPNT" ] &&
	   case $slice in
 	      "1")   MNTPNT="/"
		     ;;
	      "$SWAP_SLICE")   MNTPNT="/dev/swap"
		     ;;
	      "$DUMP_SLICE")   MNTPNT="/dev/dump"
		     ;;
	      "$VOL_SLICE")  MNTPNT="/dev/volprivate"
		     ;;
	   esac 
	
	if [ $slice -eq $SWAP_SLICE -o $slice -eq $DUMP_SLICE -o $slice -eq $VOL_SLICE ] 
	then
		FStyp="-"
		BUFSZ="-"
	else
		FStyp=`grep "^$BDSK[ 	]" /etc/vfstab | sort -u | cut -f4`
		[ -z "$FStyp" -a $slice -eq 1 ] &&
		     FStyp=`grep "/dev/root" /etc/vfstab | cut -f4`
		[ -z "$FStyp" ] && continue
        	Binfo=`mkfs -m -F $FStyp  $BDSK 2>/dev/null | grep bsize 2>/dev/null`
		if [ $? -eq 0 ]
		then
        	    BUFSZ=`echo "$Binfo" | sed 's/.*bsize=//' | cut -f1 -d','`
		else
		    case "$FStyp" in  #set the default value for the Fstyp
		       "bfs"  )		BUFSZ=512;;
		       "s5" | "vxfs" )	BUFSZ=1024;;
		       *) 		BUFSZ=4096;;
		    esac
		fi
	fi
		
	if [ $slice -eq $SWAP_SLICE -o $slice -eq 7 -o $slice -eq 8 -o $slice -eq $STAND_SLICE -o $slice -eq $VOL_SLICE ] 
	then
		FLs=`grep "^$ii	0x" < $PRT_FOUT | cut -f5`
		[ -z "$FLs" ] && continue
		UN_TYP=M
		FLsiz=`expr $FLs \/ 2048`
		[ $FLsiz -eq 0 ] && {
			FLsiz=`expr $FLs \/ 2`
			UN_TYP=K
		}
	else
		FLs=`grep "^$ii	0x" < $PRT_FOUT | cut -f5`
		[ -z "$FLs" ] && continue
		UN_TYP=W
		FLsiz=`expr $FLs \* 100 \/ $TOTL_EXP_FLs`
	fi

	echo "$slice	$MNTPNT	$FStyp	$BUFSZ	${FLsiz}${UN_TYP}	1R" >> ${LAYOUT_FL}
    done <$PRT_FOUT
}

#Set the variables for different output files for each disk and 
#invoke fdisk_cmds and dskset_cmds function
check_disks ()
{
	rm -rf $WRK_DIR
	mkdir -p $WRK_DIR
	PRT_FOUT=${WRK_DIR}/prt_fout export PRT_FOUT
	PRT_OUT=${WRK_DIR}/prt_out export PRT_OUT
	NUM=1
	while devattr disk${NUM} 1>/dev/null 2>&1
	do
		LAYOUT_FL=${WRK_DIR}/lay_out_$NUM export LAYOUT_FL
		FD_INFO=${WRK_DIR}/fd_info_$NUM export FD_INFO
		fdisk_cmds disk$NUM 
		dskset_cmds
		NUM=`expr $NUM + 1`
	done
	rm -f $PRT_FOUT $PRT_OUT
	return
}

cpio_copy_disk()
{
	pfmt -l UX:rsk -s NOSTD -g rsk:11 "\n\tCopying the system to tape, please wait ...\n"
	cd /tmp
	tapecntl -t $TAPEDEV
	tapecntl -w $TAPEDEV
	tapecntl -f 512 $TAPEDEV
	OUTDEV=`devattr $MEDIUM norewind`
	cp /etc/boot $WRK_DIR
	cp /usr/sbin/disksetup $WRK_DIR
	find $CPIO_DIR -depth -print | cpio -ocdu -O $OUTDEV 2>/dev/null 1>&2
	sleep 30        #workaround for ST01 returns too fast bug
	CONT_MSG=`pfmt -l UX:rsk -s NOSTD -g rsk:23 "\n\tRemove the tape from the tape drive.\n\tInsert the next tape and press <ENTER>." 2>&1`

#
#backup SYS volume as a separate cpio archive on the front of
#the tape, because we'll want to read it in again later
#
	$NWS_CONFIG && {
		cd /
		find ./$NWS_SYS_PATH -depth -print | cpio -ocdu -M "$CONT_MSG" -O $OUTDEV >/dev/null
		sleep 10
		stop_NWS_server
		umount $NWS_SYS_PATH 2>/dev/null
	}

	cd /
	find . -depth -print | cpio -ocdu -M "$CONT_MSG" -O $OUTDEV >/dev/null
	if [ $? -eq 0 -o $? -eq 2 ]
	then
	    pfmt -l UX:rsk -s NOSTD -g rsk:12 "\n\tCreation of the Replicated System Kit installation tape was successful.\n\n"
	else
	    pfmt -l UX:rsk -s NOSTD -g rsk:25 "\n\tCreation of the Replicated System Kit installation tape was NOT successful.\n\n"
	fi
	return 
 }

# Display message and exit
cleanup()
{
	trap '' 1 2 15

	echo
	pfmt -l UX:rsk -s error -g rsk:13 "Replicated System Kit installation tape creation aborted.\n\n"
	exit 1
}

Usage()
{
   echo
   pfmt -l UX:rsk -s error -g rsk:14 "Usage: rsk_image ctape1|ctape2\n\n"
   exit 1
}


# Make sure that the system is in maintenance mode.
check_run_level()
{
   set `LC_ALL=C who -r`
   if [ "$3" != "S" -a "$3" != "s" -a "$3" != "1" ] 
   then
	echo
	pfmt -l UX:rsk -s error -g rsk:15 " You must be in maintenance mode to create the Replicated\n\t\tSystem Kit installation tape.\n\n"
	exit 1
   fi
}

mount_usrfs()
{
	[ -f /usr/lib/libc.so.1 ] && return 
	mount /usr  >/dev/null 2>&1
	[ $? -eq 0 ] && return
	while read spl fskdev mountp fstyp fsckpass atomnt mntpts macceil
	do
	    [ "$mountp" != "/usr" ] && continue
	    fsck -F $fstyp  -y $fskdev  >/dev/null 2>&1
	    mount -F $fstyp $spl $mountp >/dev/null 2>71
	    [ $? -eq 0 ] && return
	    pfmt -l UX:rsk -s error -g rsk:24 " Not able to mount /usr file system.\n\t\tAborting the RSK installation tape creation.\n\n"
	    exit 1
	done < /etc/vfstab
	pfmt -l UX:rsk -s error -g rsk:24 " Not able to mount /usr file system.\n\t\tAborting the RSK installation tape creation.\n\n"
	exit 1
}

mount_flsystms ()
{

    RT1=`devattr disk1 addcmd | cut -f2 -d" "`

    devattr disk2 1>/dev/null 2>&1 && 
	RT2=`devattr disk2 addcmd | cut -f2 -d" "`

    Not_mnted=""
    echo 0   > /tmp/$$_rsk_abc
    while read special fsckdev mountp fstyp fsckpass automnt mountpts macceil
    do
	#skip the comment line
	fst_chr=`echo $special | cut -c1`
	[ "$fst_chr" = "#" ] && continue
	#skip if it / or /stand or /proc or /dev/fd
	[ "$mountp" = "/" -o "$mountp" = "/stand" -o "$mountp" = "/proc" -o "$mountp" = "/dev/fd" -o "$mountp" = "/usr" ] && continue
	#check if it is already mounted, this should not happen
	grep "[	]$mountp[	]" /etc/mnttab >/dev/null 2>&1
	[ $? -eq 0 ] && continue
	echo "$special" | grep $RT1 >/dev/null 2>&1
	if [ $? -eq 0 ] 
	then
	    mount -F$fstyp $special $mountp >/dev/null 2>&1
	    if [ $? -ne 0 ]
	    then
		fsck -F$fstyp -y $fsckdev >/dev/null 2>&1
		mount -F $fstyp $special $mountp >/dev/null 2>&1
		[ $? -ne 0 ] && {
			Not_mnted="$Not_mnted $mountp"
    			echo 1 $Not_mnted > /tmp/$$_rsk_abc
		}
	    fi
	else
	    [ "$mountp" = "/home" -o "$mountp" = "/home2" ] || continue
	    [ -z "$RT2" ] && continue
	    echo "$special" | grep $RT2 >/dev/null 2>&1
	    if [ $? -eq 0 ]
	    then
		mount -F $fstyp $special $mountp >/dev/null 2>&1
            	if [ $? -ne 0 ]
            	then
                	fsck -F$fstyp -y $fsckdev >/dev/null 2>&1
                	mount -F $fstyp $special $mountp >/dev/null 2>&1
                	[ $? -ne 0 ] && {
				Not_mnted="$Not_mnted $mountp"
    				echo 1 $Not_mnted > /tmp/$$_rsk_abc
			}
	    	fi
	    fi
	fi
    done < /etc/vfstab
    read err Not_mnted < /tmp/$$_rsk_abc
    rm -f /tmp/$$_rsk_abc
    if [ $err -eq 1 ]
    then
	pfmt -l UX:rsk -s error -g rsk:16 " Not able to mount %s file systems.\n\t\tAborting tape creation.\n\n" "$Not_mnted"
	exit 1
    fi
}

tape_ready ()
{
	while pfmt -l UX:rsk -s NOSTD -g rsk:17 "\n\tPlace a tape in %s and press <ENTER> or enter [q/Q] to abort : " $MEDIUM
	do
		read inp
		if [ "$inp" = "Q" -o "$inp" = "q" ] 
		then
			pfmt -l UX:rsk -s NOSTD -g rsk:18 "\n\tReplicated System Kit installation tape creation terminated.\n\n"
			exit 1
		fi
		ls /sbin/emergency_rec | cpio -ocdu -G STDIO -O $TAPEDEV 2>/dev/null
		[ $? = 0 ] && break
		echo
		pfmt -l UX:rsk -s error -g rsk:19 " Not able to write into the tape. Check the tape for damage\n\t\tor maybe it is write protected.\n"
	done
}
stop_NWS_server ()
{
	sh /etc/init.d/nws stop

	while nwserverstatus >/dev/null 2>&1 #wait until server is down
	do
		sleep 5
	done

	sh /etc/init.d/nw stop
	nwcm -q -s login=on	#reset NetWare server for full access mode
	nwcm -q -s nwum=$NWUM #set it back
	nwcm -q -s nws_start_at_boot=$NWSBOOT #set it back
	nwcm -q -s ipx_start_at_boot=$IPXBOOT #set it back
}


#main

trap 'cleanup 1' 1 2 15

mount_usrfs

while getopts '\?' c
do
	case $c in
		\?)	Usage
			;;
		*)	Usage
			;;
	esac
done

shift `expr $OPTIND - 1`

[ $# -eq 1 ] || Usage

MEDIUM=$1
devattr ${MEDIUM} 1>/dev/null 2>&1 ||
	{ echo
	  pfmt -l UX:rsk -s error -g rsk:20 "Device %s is not present in /etc/device.tab.\n" $MEDIUM;
	  echo
	  exit 1;
	}

check_run_level

TAPEDEV=`devattr $MEDIUM cdevice`
WRK_DIR=/tmp/.extra.d/Rsk_Rec export WRK_DIR  #Need to have some fixed name
CPIO_DIR=.extra.d/Rsk_Rec export CPIO_DIR  #Need to have some fixed name

tape_ready

#mount_flsystms
NWS_CONFIG=false
NWS_SYS_PATH=""
[ -f /var/sadm/pkg/nwsrvr/pkginfo ] && {
	NWS_CONFIG=true
	while read num vol type path rest
	do
		[ "$vol" != "SYS" ] && continue
		echo $path >/tmp/sys_path
		break
	done </etc/netware/voltab

	NWS_SYS_PATH=`cat /tmp/sys_path`

	[ "$NWS_SYS_PATH" = "" ] && {
		pfmt -l UX:rsk -s NOSTD -g rsk:31 "\n\tCannot locate SYS volume of NetWare server.\n"
		pfmt -l UX:rsk -s NOSTD -g rsk:13 "\n\tReplicated System Kit installation tape creation terminated.\n\n"
		exit 1
	}
	grep -q $NWS_SYS_PATH /etc/vfstab  && {
		mount | grep -q $NWS_SYS_PATH || mount $NWS_SYS_PATH || {
			pfmt -l UX:rsk -s NOSTD -g rsk:32 "\n\tCannot mount SYS volume of NetWare server.\n"
			pfmt -l UX:rsk -s NOSTD -g rsk:13 "\n\tReplicated System Kit installation tape creation terminated.\n\n"
			exit 1
		}
	}
	eval `/usr/sbin/nwcm -v nwum -v nws_start_at_boot -v ipx_start_at_boot`
	NWUM=$nwum
	NWSBOOT=$nws_start_at_boot
	IPXBOOT=$ipx_start_at_boot
	nwcm -q -s nwum=off #Allows NWS to run without in.snmpd
	nwcm -q -s nws_start_at_boot=on #In case they had turned it off
	nwcm -q -s ipx_start_at_boot=on #In case they had turned it off

	sh /etc/init.d/nw start	#start NPSD daemon
	nwcm -q -s login=on	#Needed to allow server to come up
	sh /etc/init.d/nws start	#start NetWare Server

	count=0
	until nwserverstatus >/tmp/status 2>&1	#nwserverstatus returns 0 (true)
	do					#when the NetWare server is up
		rc=$?
		count=`expr $count + 1`
		[ $count -gt 25 ] && {
			pfmt -l UX:drf -s NOSTD -g rsk:33 "\n\tCannot start NetWare Server.\n"
			cat /tmp/status
			pfmt -l UX:drf -s NOSTD -g drf:13 "\n\tReplicated System Kit installation tape creation terminated.\n\n"
			exit 1
		}
		[ $rc = 254 ] && {
			pfmt -l UX:rsk -s NOSTD -g rsk:33 "\n\tCannot start NetWare server.\n"
			cat /tmp/status
			pfmt -l UX:rsk -s NOSTD -g rsk:13 "\n\tReplicated System Kit installation tape creation terminated.\n\n"
			exit 1
		}

		[ $rc = 255 ] && {
			pfmt -l UX:rsk -s NOSTD -g rsk:33 "\n\tCannot start NetWare server.\n"
			cat /tmp/status
			pfmt -l UX:rsk -s NOSTD -g rsk:13 "\n\tReplicated System Kit installation tape creation terminated.\n\n"
			exit 1
		}
		sleep 5
	done

	nwcm -q -s login=off	#set NetWare server for limited access mode
	ndsbackup -o /usr/lib/rsk/nds.backup	#backup NDS just in case
}

check_disks

cpio_copy_disk

exit 0

