#!/sbin/sh
#ident	"@(#)drf:cmd/emergency_rec.sh	1.15.1.9"

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

#This function creates the ${FD_CMDS1} file corresponding to $1
#FD_CMDS1 file will contain the commands to be executed to create the
#unix and system partition if present and if it is first disk

fdisk_cmds()
{
   RTDEV=`devattr $1 cdevice 2>/dev/null`

   FDSKOUT=/tmp/fd_out export FDSKOUT
   echo x | LC_ALL=C fdisk -L $RTDEV >${FDSKOUT} 
   RDEV=`echo $RTDEV | sed "s/..$//"`
   eval `grep NPART ${FDSKOUT}`
   echo "fdisk -L \$1 >/tmp/fdisk_out <<-END" > ${FD_CMDS1}

   > ${PART_DD_INF}
   > ${FD_CMDS2}
   PART=1
   # Check for Unix and system partition and write the create the command
   # to create it in to FD_CMDS1 file and the boundary info to FD_CMDS2. 
   # Update the PART_DD_INF file to contain the info about system partition 
   # to be used by dd to backup/restore the system partition. 
   while [ $PART -le $NPART ]
   do
	OS_LINE=`grep PART$PART ${FDSKOUT}`
	OS_STAT=`echo "$OS_LINE" | cut -f2`
	OS_TYPE=`echo "$OS_LINE" | cut -f3`
	BEG_CYL=`echo "$OS_LINE" | cut -f4`
	END_CYL=`echo "$OS_LINE" | cut -f5`
	SIZE_CYL=`echo "$OS_LINE" | cut -f6`
	find_os_num "$OS_TYPE"
	OS_ST=""
	if [ "$OS_STAT" = "Active" -a $OS_NUM -eq 1 ] 
	then
		OS_ST=1
		PART_CMD="c $OS_NUM $BEG_CYL $SIZE_CYL $OS_ST"
		echo $PART_CMD >>${FD_CMDS1}
		PART_CMD="$BEG_CYL $END_CYL"
		echo $PART_CMD >> ${FD_CMDS2}
	else
		#Make a note if it system partition on first disk
		[ $OS_NUM -eq 4 -a $2 -eq 1 ] && {
		        Dd_dev="${RDEV}p${PART}"	
		        echo "$Dd_dev  p$PART"  > ${PART_DD_INF}
			PART_CMD="c $OS_NUM $BEG_CYL $SIZE_CYL $OS_ST"
			echo $PART_CMD >>${FD_CMDS1}
			PART_CMD="$BEG_CYL $END_CYL"
			echo $PART_CMD >> ${FD_CMDS2}
		}
	fi
	PART=`expr $PART \+ 1`
   done
   echo s >>${FD_CMDS1}
   echo END >>${FD_CMDS1}

}  # End of fdisk_cmds


#this function creates the $LAYOUT_FL to match the existing
#file systems in ${RDEV}s0 unix slice
#This file will be used to setup the corresponding disk

dskset_cmds ()
{
    AN=`basename $RDEV`
    ODM_FILES=/etc/vx/reconfig.d/disk.d/$AN
    if [ "$VM_RUNNING" = "Yes" -a -s $ODM_FILES/vtoc ]
    then
	LC_ALL=C prtvtoc -f $ODM_VTOC ${RDEV}s0
	echo "${RDEV}sf  sf" > $ODM_VOLINFO	
	cp $ODM_FILES/vtoc $PRT_FOUT
    else
	LC_ALL=C prtvtoc -f $PRT_FOUT ${RDEV}s0
    fi
    LC_ALL=C prtvtoc ${RDEV}s0 > ${PRT_OUT}
    SWAP_SLICE=200
    grep SWAP $PRT_OUT >/dev/null
    [ $? -eq 0 ] && 
	SWAP_SLICE=`grep SWAP $PRT_OUT | cut -f1 -d":" | cut -f2 -d" "`
    DUMP_SLICE=201
    grep DUMP $PRT_OUT >/dev/null
    [ $? -eq 0 ] && 
	DUMP_SLICE=`grep DUMP $PRT_OUT | cut -f1 -d":" | cut -f2 -d" "`
    STAND_SLICE=202
    grep STAND $PRT_OUT >/dev/null
    [ $? -eq 0 ] && 
	STAND_SLICE=`grep STAND $PRT_OUT | cut -f1 -d":" | cut -f2 -d" "`
    VOL_PRIV_SLICE=203
    grep VOLPRIVATE $PRT_OUT >/dev/null
    [ $? -eq 0 ] && 
	VOL_PRIV_SLICE=`grep VOLPRIVATE $PRT_OUT | cut -f1 -d":" | cut -f2 -d" "`
    VOL_PUB_SLICE=203
    grep VOLPUBLIC $PRT_OUT >/dev/null
    [ $? -eq 0 ] && 
	VOL_PUB_SLICE=`grep VOLPUBLIC $PRT_OUT | cut -f1 -d":" | cut -f2 -d" "`
    >${LAYOUT_FL}
    while read slice rest
    do
	[ $slice = "#SLICE" -o $slice = "0" -o $slice = "7" -o $slice = "8" ] && continue
	ii=$slice
	ap=`echo $slice|awk '{ printf( "%x\n", $1) }'`
	if [ $slice -lt 10 ] 
	then
	    ii=" $slice"
	fi

	grep "^$ii	0x0	0x0	0	0" $PRT_FOUT > /dev/null
	[ $? -eq  0 ] && continue	#If the slice is all 0s, skip it

	grep "^$ii" $PRT_FOUT > /dev/null
	[ $? -ne 0 ] && continue	#If the slice is not configured, skip it

 	BDSK=/dev/dsk/${AN}s${ap}
	MNTPNT=`grep "^$BDSK[ 	]" $VFSTAB | sort -u | awk '{print $3}'`
	[ -z "$MNTPNT" ] &&
		case $slice in
		      "1")	MNTPNT="/"
				if [ $dd_fs -eq 1 ]
				then
					echo "/dev/rdsk/${AN}s1 $BDSK" > $WRK_DIR/dd_list
				fi
				;;
		      "10")	MNTPNT="/stand"
				if [ $dd_fs -eq 1 ]
				then
					echo "/dev/rdsk/$(AN}sa $BDISK" >> $WRK_DIR/dd_list
				fi
				;;
		      "$SWAP_SLICE")         MNTPNT="/dev/swap"
					     ;;
		      "$DUMP_SLICE")         MNTPNT="/dev/dump"
					     ;;
		      "$VOL_PRIV_SLICE")     MNTPNT="/dev/volprivate"
					     ;;
		      "$VOL_PUB_SLICE")      MNTPNT="/dev/volpublic"
					     ;;
		esac 

	[ "$MNTPNT" = "" ] && {
		pfmt -l UX:drf -s Warning -g drf:34 "Cannot determine mountpoint for device $BDSK"
	}
	
	
	if [ $slice -eq $SWAP_SLICE -o $slice -eq $DUMP_SLICE -o $slice -eq $VOL_PRIV_SLICE -o $slice -eq $VOL_PUB_SLICE ] 
	then
		FStyp="-"
		BUFSZ="-"
	else
		FStyp=`grep "^$BDSK[ 	]" $VFSTAB | awk '{print $4}'`
		[ -z "$FStyp" -a $slice -eq 1 ] &&
		     FStyp=`grep "/dev/root" $VFSTAB | awk '{print $4}'`
		[ -z "$FStyp" -a $slice -eq 10 ] &&
		     FStyp=`grep "/dev/stand" $VFSTAB | awk '{print $4}'`
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

	UN_TYP=K
	if [ $slice -eq $VOL_PUB_SLICE ] 
	then
	   FLs=200
	   UN_TYP=W
	else
	    FLs=`grep "^$ii	0x" $PRT_FOUT | cut -f5`
	fi
	[ -z "$FLs" ] && continue
	FLsiz=`expr $FLs \/ 2`

	if grep -q "^${MNTPNT}$" /tmp/mnt_points 2>/dev/null
	then
		pfmt -l UX:drf -s error -g drf:35 "Duplicate mountpoint $MNTPNT detected with device $BDSK"
		cleanup
	else
		echo $MNTPNT >>/tmp/mnt_points
	fi
	echo "$slice	$MNTPNT	$FStyp	$BUFSZ	${FLsiz}${UN_TYP}	1R" >> ${LAYOUT_FL}
    done <$PRT_FOUT
}

#Set the variables for different output files for disk 1 and invoke fdisk_cmds
#and dskset_cmds function
disk1_setup ()
{
	rm -rf $WRK_DIR
	mkdir -p $WRK_DIR
	LAYOUT_FL=$WRK_DIR/lay_out_1 export LAYOUT_FL
	PRT_FOUT=$WRK_DIR/prt_f_11 export PRT_FOUT
	PRT_OUT=$WRK_DIR/prt_f_12 export PRT_OUT
	PART_DD_INF=$WRK_DIR/part_dd_1 export PART_DD_INF
	FD_CMDS1=$WRK_DIR/fd_cmds_11 export FD_CMDS1
	FD_CMDS2=$WRK_DIR/fd_cmds_12 export FD_CMDS2
	[ "$VM_RUNNING" = "Yes" ] && {
		ODM_VTOC=$WRK_DIR/odm_vtoc_1 export ODM_VTOC
		ODM_VOLINFO=$WRK_DIR/odm_volinfo_1 export ODM_VOLINFO
	}
	fdisk_cmds disk1 1
	dskset_cmds 1
	rm -f $PRT_FOUT $PRT_OUT
}

#Set the variables for different output files for disk 2 and invoke fdisk_cmds
#and dskset_cmds function
disk2_setup ()
{
	LAYOUT_FL=$WRK_DIR/lay_out_2 export LAYOUT_FL
	PRT_FOUT=$WRK_DIR/prt_f_21 export PRT_FOUT
	PRT_OUT=$WRK_DIR/prt_f_22 export PRT_FOUT
	PART_DD_INF=$WRK_DIR/part_dd_2 export PART_DD_INF
	FD_CMDS1=$WRK_DIR/fd_cmds_21 export FD_CMDS1
	FD_CMDS2=$WRK_DIR/fd_cmds_22 export FD_CMDS2
	[ "$VM_RUNNING" = "Yes" ] && {
		ODM_VTOC=$WRK_DIR/odm_vtoc_2 export ODM_VTOC
		ODM_VOLINFO=$WRK_DIR/odm_volinfo_2 export ODM_VOLINFO
	}
	fdisk_cmds disk2 2
	dskset_cmds 2
	rm -f $PRT_FOUT $PRT_OUT $PART_DD_INF 
}

check_disk2 ()
{
	devattr disk2 1>/dev/null 2>&1
	[ $? -ne 0 ] && return
	RT2=`devattr disk2 addcmd | cut -f 2 -d" "`
	for i in /usr /home /home2 /var $NWS_SYS_PATH
	do
	   spl=`grep "[	 ]$i[	 ]" $VFSTAB  | grep -v "^#" | awk '{print $1}'`
	   if [ $? -eq 0 ] 
	   then
		echo $spl | grep $RT2 >/dev/null 2>&1
		[ $? -eq 0 ] && {
		    disk2_setup
		    return
	  	}
	   fi
	done
	return
}

dd_copy_disk()
{
	pfmt -l UX:drf -s NOSTD -g drf:26 "\n\tCopying the primary disk to tape, please wait ...\n"
	cd /tmp
	rm -rf $WRK_DIR
	mkdir -p $WRK_DIR
	tapecntl -t $TAPEDEV
	tapecntl -w $TAPEDEV
	tapecntl -f 512 $TAPEDEV
	OUTDEV=`devattr $MEDIUM norewind`
	DV=`devattr disk1 addcmd | cut -f2 -d" "`
	echo "Entire Disk" > $WRK_DIR/entire_dsk
	find $CPIO_DIR -depth -print | cpio -ocdu -O $OUTDEV 2>/dev/null 1>&2
	
	INDEV=/dev/rdsk/${DV}p0
	dd if=$INDEV of=$OUTDEV bs=64k 2>&1 >/dev/null
	if [ $? -eq 0 ]
	then
	    pfmt -l UX:drf -s NOSTD -g drf:12 "\n\tCreation of the Emergency Recovery tape was successful.\n\n"
	else
	    pfmt -l UX:drf -s NOSTD -g drf:25 "\n\tCreation of the Emergency Recovery tape was NOT successful.\n\n"
	fi
	return 
}

cpio_copy_disk()
{
	pfmt -l UX:drf -s NOSTD -g drf:11 "\n\tCopying the hard disk(s) to tape, please wait ...\n"
	cd /tmp
	tapecntl -t $TAPEDEV
	tapecntl -w $TAPEDEV
	tapecntl -f 512 $TAPEDEV
	OUTDEV=`devattr $MEDIUM norewind`
	cp /etc/boot $WRK_DIR
	cp /usr/sbin/disksetup $WRK_DIR
	[ -s $WRK_DIR/odm_volinfo_1 -o -s $WRK_DIR/odm_volinfo_2 ] && {
		rm -rf /etc/vx/.drf
		mkdir -p /etc/vx/.drf
		echo "#!/sbin/sh" > /etc/vx/.drf/priv_slc_copy
		echo "#!/sbin/sh" > /etc/vx/.drf/S90DRF
	}
	[ -s $WRK_DIR/odm_volinfo_1 ] && {
		read dd_to_cp dd_jnk < $WRK_DIR/odm_volinfo_1
		dd if=$dd_to_cp of=/etc/vx/.drf/Drf_Vol_1 bs=512 >/dev/null 2>&1
		echo "[ -s /etc/vx/.drf/Drf_Vol_1 ] && " >>/etc/vx/.drf/priv_slc_copy
		echo "	dd if=/etc/vx/.drf/Drf_Vol_1 of=\$1 bs=512 >/dev/null 2>&1" >> /etc/vx/.drf/priv_slc_copy
		chmod 0755 /etc/vx/.drf/priv_slc_copy
		echo "[ -s /etc/vx/.drf/Drf_Vol_1 ] && " >> /etc/vx/.drf/S90DRF
		echo "	/sbin/mv /etc/vx/.drf/Drf_Vol_1 /etc/vx/.drf/S_Drf_Vol_1 >/dev/null 2>&1 " >> /etc/vx/.drf/S90DRF

	}
	[ -s $WRK_DIR/odm_volinfo_2 ] && {
		read dd_to_cp  dd_jnk < $WRK_DIR/odm_volinfo_2
		dd if=$dd_to_cp of=/etc/vx/.drf/Drf_Vol_2 bs=512 >/dev/null 2>&1
		echo "[ -s /etc/vx/.drf/Drf_Vol_2 ] &&" >> /etc/vx/.drf/priv_slc_copy
		echo "	dd if=/etc/vx/.drf/Drf_Vol_2 of=\$2 bs=512 >/dev/null 2>&1" >>/etc/vx/.drf/priv_slc_copy
		chmod 0755 /etc/vx/.drf/priv_slc_copy
		echo "[ -s /etc/vx/.drf/Drf_Vol_2 ] &&" >>/etc/vx/.drf/S90DRF
		echo "	/sbin/mv /etc/vx/.drf/Drf_Vol_2 /etc/vx/.drf/S_Drf_Vol_2 >/dev/null 2>&1 " >> /etc/vx/.drf/S90DRF
	}
	[ -s $WRK_DIR/odm_volinfo_1 -o -s $WRK_DIR/odm_volinfo_2 ] && {
		echo "/usr/bin/ed -s /etc/inittab >/dev/null <<-END" >>/etc/vx/.drf/S90DRF
		echo "/priv_slc_copy/d" >>/etc/vx/.drf/S90DRF
		echo "w\nq\nEND" >>/etc/vx/.drf/S90DRF
		echo "/sbin/rm /etc/rc2.d/S90DRF " >>/etc/vx/.drf/S90DRF
	}
	rm -f $WRK_DIR/odm_volinfo_1 $WRK_DIR/odm_volinfo_2
	find $CPIO_DIR -depth -print | cpio -ocdu -O $OUTDEV 2>/dev/null 1>&2
	sleep 30	#workaround for ST01 returns too fast bug
	[ -s $WRK_DIR/part_dd_1 ] && {
		read dd_to_cp dd_size < $WRK_DIR/part_dd_1
		dd if=$dd_to_cp of=$OUTDEV bs=512 >/dev/null 2>&1
	}
	CONT_MSG=`pfmt -l UX:drf -s NOSTD -g drf:23 "\n\tRemove the tape from the tape drive.\n\tInsert the next tape and press <ENTER>." 2>&1`
#
#backup SYS volume as a separate cpio archive on the front of
#the tape, because we'll want to read it in again later
#
	$NWS_CONFIG && {
		cd /
		find ./$NWS_SYS_PATH -depth -print | cpio -ocdu -M "$CONT_MSG" -O $OUTDEV >/dev/null
		ret=$?
		if [ $ret -ne 0 -a $ret -ne 2 ]
		then
			pfmt -l UX:drf -s NOSTD -g drf:25 "\n\tCreation of the Emergency Recovery tape was NOT successful.\n\n"
		fi
	}

	cd /
	if [ $dd_fs -eq 0 ]
	then
	    # using cpio
	    if [ $NWS_FILTER = "No" ]
	    then
		find . -depth -print | cpio -ocdu -M "$CONT_MSG" -O $OUTDEV > /dev/null
		ret=$?
	    else
		NWS_SYS_PATH=`echo $NWS_SYS_PATH | sed "s&^\(.*[^/]\)/*&\1&"`
		find . -depth -print | awk -v filter="$NWS_SYS_PATH" '{ if ($0 !~ "^\." filter "/.*") { print $0 }}' | cpio -ocdu -M "$CONT_MSG" -O $OUTDEV >/dev/null
		ret=$?
	    fi
	    $NWS_CONFIG && {
		sleep 10
		stop_NWS_server
		$NWS_MOUNTED && {
			umount $NWS_SYS_PATH 2>/dev/null
			NWS_MOUNTED=false
		}
	    }
	else
	    # dd each file-system
	    $NWS_CONFIG && {
		sleep 10
		stop_NWS_server
		$NWS_MOUNTED && {
			umount $NWS_SYS_PATH 2>/dev/null
			NWS_MOUNTED=false
		}
	    }
	    ret=0
	    while read cdevice bdevice
	    do
		dd if=$cdevice of=$OUTDEV bs=64k 2>&1 1>/dev/null
		ret=$?
		[ $ret -ne 0 ] && break
	    done < $WRK_DIR/dd_list

	    # Ensure we get an error message
	    [ $ret -ne 0 ] && ret=1
	fi

	if [ $ret -eq 0 -o $ret -eq 2 ]
	then
	    pfmt -l UX:drf -s NOSTD -g drf:12 "\n\tCreation of the Emergency Recovery tape was successful.\n\n"
	else
	    pfmt -l UX:drf -s NOSTD -g drf:25 "\n\tCreation of the Emergency Recovery tape was NOT successful.\n\n"
	fi
	return 
 }

# Display message and exit
cleanup()
{
	trap '' 1 2 15

	$NWS_CONFIG && {
		stop_NWS_server
	}
	$NWS_MOUNTED && {
		umount $NWS_SYS_PATH 2>/dev/null
		NWS_MOUNTED=false
	}

	echo
	rm /tmp/mnt_points 2>/dev/null
	pfmt -l UX:drf -s error -g drf:13 "Emergency Recovery tape creation aborted.\n\n"
	exit 1
}

Usage()
{
   echo
   pfmt -l UX:drf -s error -g drf:14 "Usage: emergency_rec [-e] ctape1|ctape2\n\n"
   exit 1
}


# Make sure that the system is in maintenance mode.
check_run_level()
{
   set `LC_ALL=C who -r`
   if [ "$3" != "S" -a "$3" != "s" -a "$3" != "1" ] 
   then
	echo
	pfmt -l UX:drf -s error -g drf:15 " You must be in maintenance mode to create the Emergency\n\t\tRecovery tape.\n\n"
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
	    mount -F $fstyp $spl $mountp >/dev/null 2>&1
	    [ $? -eq 0 ] && return
	    pfmt -l UX:drf -s error -g drf:24 " Not able to mount /usr file systems.\n\t\tAborting the emergency tape creation.\n\n"
	    exit 1
	done < /etc/vfstab
	pfmt -l UX:drf -s error -g drf:24 " Not able to mount /usr file systems.\n\t\tAborting the emergency tape creation.\n\n"
	exit 1
}

mount_flsystms ()
{

    RT1=`devattr disk1 addcmd | cut -f2 -d" "`

    devattr disk2 1>/dev/null 2>&1 && 
	RT2=`devattr disk2 addcmd | cut -f2 -d" "`

    Not_mnted=""
    echo 0   > /tmp/$$_drf_abc
    while read special fsckdev mountp fstyp fsckpass automnt mountpts macceil
    do
	#skip the comment line
	fst_chr=`echo $special | cut -c1`
	[ "$fst_chr" = "#" ] && continue
	#skip if it / or /stand or /proc or /dev/fd
	[ "$mountp" = "/" -o "$mountp" = "/proc" -o "$mountp" = "/dev/fd" -o "$mountp" = "/usr" ] && continue
	[ "$mountp" = "/usr" -a $dd_fs -eq 0 ] && continue
	[ "$mountp" = "/stand" -a $dd_fs -eq 0 ] && continue

	echo "$special" | grep $RT1 >/dev/null 2>&1
	rt=$?
	[ $rt -ne 0 -a "$VM_RUNNING" = "Yes" ] && {
		vspl=`grep "[	 ]$mountp[	 ]" $VFSTAB  | awk '{print $1}'`
		echo "$vspl" | grep $RT1 >/dev/null 2>&1
		rt=$?
	}
	if [ $rt -eq 0 ] 
	then
	    if [ $dd_fs -eq 1 ]
	    then
		[ "$fsckdev" != "-" ] &&
			echo "$fsckdev $special" >> $WRK_DIR/dd_list
		continue
	    fi

	    # check if it is already mounted
	    awk -v mntpt="$mountp" '{ if ($2 == mntpt) exit 1 }' < /etc/mnttab
	    [ $? -eq 1 ] && continue

	    mount -F$fstyp $special $mountp >/dev/null 2>&1
	    if [ $? -ne 0 ]
	    then
		fsck -F$fstyp -y $fsckdev >/dev/null 2>&1
		mount -F $fstyp $special $mountp >/dev/null 2>&1
		[ $? -ne 0 ] && {
			Not_mnted="$Not_mnted $mountp"
    			echo 1 $Not_mnted > /tmp/$$_drf_abc
		}
	    fi
	else
	    [ "$mountp" = "/home" -o "$mountp" = "/home2" -o "$mountp" = "/var" ] || continue
	    [ -z "$RT2" ] && continue
	    echo "$special" | grep $RT2 >/dev/null 2>&1
	    rt=$?
	    [ $rt -ne 0 -a "$VM_RUNNING" = "Yes" ] && {
		vspl=`grep "[ 	]$mountp[	 ]" $VFSTAB  | awk '{print $1}'`
		echo "$vspl" | grep $RT2 >/dev/null 2>&1
		rt=$?
	    }
	    if [ $rt -eq 0 ]
	    then
		if [ $dd_fs -eq 1 ]
		then
		    [ "$fsckdev" != "-" ] &&
			echo "$fsckdev $special" >> $WRK_DIR/dd_list
		    continue
		fi

		# check if it is already mounted
		awk -v mntpt="$mntp" '{ if ($2 == mntpt) exit 1 }' < /etc/mnttab

		mount -F $fstyp $special $mountp >/dev/null 2>&1
            	if [ $? -ne 0 ]
            	then
                	fsck -F$fstyp -y $fsckdev >/dev/null 2>&1
                	mount -F $fstyp $special $mountp >/dev/null 2>&1
                	[ $? -ne 0 ] && {
				Not_mnted="$Not_mnted $mountp"
    				echo 1 $Not_mnted > /tmp/$$_drf_abc
			}
	    	fi
	    fi
	fi
    done < /etc/vfstab
    read err Not_mnted < /tmp/$$_drf_abc
    rm -f /tmp/$$_drf_abc
    if [ $err -eq 1 ]
    then
	pfmt -l UX:drf -s error -g drf:16 " Not able to mount %s file systems.\n\t\tAborting the emergency tape creation.\n\n" "$Not_mnted"
	exit 1
    fi
}

tape_ready ()
{
	while pfmt -l UX:drf -s NOSTD -g drf:17 "\n\tPlace a tape in %s and press <ENTER> or enter [q/Q] to abort : " $MEDIUM
	do
		read inp
		if [ "$inp" = "Q" -o "$inp" = "q" ] 
		then
			pfmt -l UX:drf -s NOSTD -g drf:18 "\n\tEmergency Recovery tape creation terminated.\n\n"
			exit 1
		fi
		tapecntl -w $TAPEDEV >/dev/null 2>&1
		ls /sbin/emergency_rec | cpio -ocdu -G STDIO -O $TAPEDEV 2>/dev/null
		[ $? = 0 ] && break
		echo
		pfmt -l UX:drf -s error -g drf:19 " Not able to write into the tape. Check the tape for damage\n\t\tor may be it is write protected.\n"
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

check_run_level

# cd to /tmp, to insure it does not get unmounted, because we need it

cd /tmp
umountall 2>&1 | grep -v " /tmp "

NWS_CONFIG=false
NWS_MOUNTED=false
NWS_FILTER="No"
trap 'cleanup 1' 1 2 15
rm /tmp/mnt_points 2>/dev/null


mount_usrfs

ENTire=NO
dd_fs=0
while getopts 'e\?' c
do
	case $c in
		e)	ENTire=YES
			;;
		d)	dd_fs=1
			;;
		\?)	Usage
			;;
		*)	Usage
			;;
	esac
done

shift `expr $OPTIND - 1`

[ $# -eq 1 ] || Usage

if [ "$ENTire" = "YES" -a dd_fs -eq 1 ]
then
	Usage
fi

MEDIUM=$1
devattr ${MEDIUM} 1>/dev/null 2>&1 ||
	{ echo
	  pfmt -l UX:drf -s error -g drf:20 "Device %s is not present in /etc/device.tab.\n" $MEDIUM;
	  echo
	  exit 1;
	}

if [ "$ENTire" != "YES" ]
then
	mount /tmp 2>/dev/null
fi

TAPEDEV=`devattr $MEDIUM cdevice`
WRK_DIR=/tmp/.extra.d/Drf_Rec export WRK_DIR  #Need to have some fixed name
CPIO_DIR=.extra.d/Drf_Rec export CPIO_DIR  #Need to have some fixed name

tape_ready

VFSTAB=/etc/vfstab export VFSTAB
VM_RUNNING="No" export VM_RUNNING

grep standvol /etc/vfstab >/dev/null 2>&1  #Check for odm
[ $? -eq -0 -o $? -eq 3 ] && {
	VM_RUNNING=Yes
	/usr/lib/drf/odm_vfs /tmp/odm_vfstab /dev/null /dev/null /dev/null /dev/null
	VFSTAB=/tmp/odm_vfstab
}

NWS_SYS_PATH=""

if [ "$ENTire" != "YES" ]
then

[ -f /var/sadm/pkg/nwsrvr/pkginfo -o -f /var/sadm/pkg/nwsrvrJ/pkginfo ] && {
	while read num vol type path rest
	do
		[ "$vol" != "SYS" ] && continue
		echo $path >/tmp/sys_path
		break
	done </etc/netware/voltab

	NWS_SYS_PATH=`cat /tmp/sys_path`

	[ "$NWS_SYS_PATH" = "" ] && {
		pfmt -l UX:drf -s NOSTD -g drf:31 "\n\tCannot locate SYS volume of NetWare Server.\n"
		pfmt -l UX:drf -s NOSTD -g drf:18 "\n\tEmergency Recovery tape creation terminated.\n\n"
		exit 1
	}
	NWS_FILTER="Yes"

	awk -v mnt_pt="$NWS_SYS_PATH" '{ if ($3==mnt_pt) exit_val=1 } END { exit exit_val }' </etc/vfstab || {
		# "SYS is a separate file-system
		mount | grep -q $NWS_SYS_PATH || mount $NWS_SYS_PATH || {
			pfmt -l UX:drf -s NOSTD -g drf:32 "\n\tCannot mount SYS volume of NetWare Server.\n"
			pfmt -l UX:drf -s NOSTD -g drf:18 "\n\tEmergency Recovery tape creation terminated.\n\n"
			exit 1
		}
		NWS_MOUNTED=true
	}
	NWS_CONFIG=true
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
		[ $count -gt 50 ] && {
			pfmt -l UX:drf -s NOSTD -g drf:33 "\n\tCannot start NetWare Server.\n"
			cat /tmp/status
			pfmt -l UX:drf -s NOSTD -g drf:18 "\n\tEmergency Recovery tape creation terminated.\n\n"
			exit 1
		}
		[ $rc = 254 ] && {
			pfmt -l UX:drf -s NOSTD -g drf:33 "\n\tCannot start NetWare Server.\n"
			cat /tmp/status
			pfmt -l UX:drf -s NOSTD -g drf:18 "\n\tEmergency Recovery tape creation terminated.\n\n"
			exit 1
		}

		[ $rc = 255 ] && {
			pfmt -l UX:drf -s NOSTD -g drf:33 "\n\tCannot start NetWare Server.\n"
			cat /tmp/status
			pfmt -l UX:drf -s NOSTD -g drf:18 "\n\tEmergency Recovery tape creation terminated.\n\n"
			exit 1
		}
		sleep 5
	done

	nwcm -q -s login=off	#set NetWare server for limited access mode
	ndsbackup -o /usr/lib/drf/nds.backup	#backup NDS just in case
}
fi

if [ "$ENTire" = "YES" ]
then
	dd_copy_disk
else
	disk1_setup

	mount_flsystms

	check_disk2

	cpio_copy_disk
fi

exit 0

