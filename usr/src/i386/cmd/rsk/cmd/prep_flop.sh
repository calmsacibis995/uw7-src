#!/sbin/sh
#ident	"@(#)prep_flop.sh	15.1"

 
		
set_mod_list()
{
	#Get the file system type for all filesystems identified in /etc/vfstab
	#(Except for the ones associated with the floppy devices)

	FSCKS=`cut -f4 /etc/vfstab | grep -v /dev/dsk/f  \
		| egrep 'bfs|s5|ufs|vxfs' | sort -u`

	FSMODS="$FSCKS"
	# ufs module depends on sfs module. So,....
	echo $FSMODS | grep ufs >/dev/null 2>&1
	[ $? -eq 0 ] && FSMODS="$FSMODS sfs"
	#Add dow module if ufs or sfs module is present
	echo $FSMODS | egrep 'ufs' > /dev/null 2>&1
	[ $? -eq 0 ] && FSMODS="$FSMODS dow"
	_DRVMODS="`/etc/scsi/pdiconfig | grep '[	]Y[	]' | cut -f1 | sort -u`"
	MOD_LIST="$FSMODS $_DRVMODS" 

}

add_terminfo_ie()
{
	ed -s $RAMPROTO >/dev/null <<-END
	/ANSI
	a
	AT386-ie  ---644 0 3 /usr/share/lib/terminfo/A/AT386-ie
	AT386-M-ie  ---644 0 3 /usr/share/lib/terminfo/A/AT386-M-ie
	.
	w
	q
	END
	return;
}

add_terminfo_mb()
{
	ed -s $RAMPROTO >/dev/null <<-END
	/ANSI
	a
	AT386-mb  ---644 0 3 /usr/share/lib/terminfo/A/AT386-mb
	AT386-M-mb  ---644 0 3 /usr/share/lib/terminfo/A/AT386-M-mb
	.
	w
	q
	END
	return;
}

add_terminfo()
{
	ed -s $RAMPROTO >/dev/null <<-END
	/ANSI
	a
	AT386  ---644 0 3 /usr/share/lib/terminfo/A/AT386
	AT386-M  ---644 0 3 /usr/share/lib/terminfo/A/AT386-M
	.
	w
	q
	END
	return;
}

add_keyboard()
{
	[ -s /usr/lib/keyboard/$KEYBOARD ] || {
	    echo "\n\t/usr/lib/keyboard/$KEYBOARD file is missing" >> ${DRF_LOG_FL}
	    return 
	}
	[ -s /usr/lib/mapchan/88591.dk ] || {
	    echo "\n\t/usr/lib/mapchan/88591.dk file is missing" >> ${DRF_LOG_FL}
	    return 
	}
	( cd  /usr/lib/rsk
	 ./keycomp /usr/lib/keyboard/$KEYBOARD $PROTO/kbmap 
	 ./dkcomp /usr/lib/mapchan/88591.dk $PROTO/dead_keys
	)
	ed -s $RAMPROTO >/dev/null <<-END
	/keyboards
	a
	$KEYBOARD   d--755 2 2 
	kbmap  ---644 0 3 $PROTO/kbmap
	dead_keys  ---644 0 3 $PROTO/dead_keys
	\$
	.
	w
	q
	END
	return
}

add_lc_ctype()
{
	[ -s /usr/lib/locale/$LANG/LC_CTYPE ] || {
	    echo "\n\t/usr/lib/locale/$LANG/LC_CTYPE file is missing" >> ${DRF_LOG_FL}
	    return 
	}
	ed -s $RAMPROTO >/dev/null <<-END
	/locale
	a
	$LANG   d--755 2 2 
	LC_CTYPE  ---644 0 3 /usr/lib/locale/$LANG/LC_CTYPE
	\$
	.
	w
	q
	END
	return
}

turnoff()
{
	cd $ROOT/$LCL_MACH/etc/conf/sdevice.d
	[ -d .save ] || mkdir .save
	mv * .save

	cd $ROOT/$LCL_MACH/etc/conf/mdevice.d
	[ -d .save ] || mkdir .save
	mv * .save
}

turnon()
{
	cd $ROOT/$LCL_MACH/etc/conf/sdevice.d/.save
	mv $* .. >/dev/null 2>&1
	cd ..

	for i in `grep -l '[	 ]N[	 ]' $* 2>/dev/null`
	do
	   # Check whether it is hardware module. If it is, leave
	   # it as it is.
	   A=`grep "^$i	" /etc/conf/mdevice.d/$i | cut -f3 | grep h`
	   if [ -z "$A" ]
	   then
		# it is not a hardware module
		ed -s $i <<-END
			g/[	 ]N[	 ]/s//	Y	/
			w
			q
		END
	   fi  
	done

	cd $ROOT/$LCL_MACH/etc/conf/mdevice.d/.save
	mv $* .. >/dev/null 2>&1
}

stub()
{
	cd $ROOT/$LCL_MACH/etc/conf/sdevice.d/.save
	mv $* .. >/dev/null 2>&1
	cd ..

	for i in `grep -l '[	 ]Y[	 ]' $* 2>/dev/null`
	do
		ed -s $i <<-END
			g/[	 ]Y[	 ]/s//	N	/
			w
			q
		END
	done
	cd $ROOT/$LCL_MACH/etc/conf/mdevice.d/.save
	mv $* .. >/dev/null 2>&1
}

make_static()
{
	cd $ROOT/$LCL_MACH/etc/conf/sdevice.d
	for i in $*
	do
		ed -s $i <<-END
			/\$version/a
			\$static
			.
			w
			q
		END
	done
}

tune()
{
	MACH=$LCL_MACH idtune -f PAGES_UNLOCK 50
	MACH=$LCL_MACH idtune -f FLCKREC 100
	MACH=$LCL_MACH idtune -f NPROC 100
	MACH=$LCL_MACH idtune -f MAXUP 30
	MACH=$LCL_MACH idtune -f NHBUF 32
	MACH=$LCL_MACH idtune -f FD_DOOR_SENSE 0
	MACH=$LCL_MACH idtune -f ARG_MAX 20480
	MACH=$LCL_MACH idtune -f MEMFS_MAXKMEM 4096000
	MACH=$LCL_MACH idtune -f PHYSTOKVMEM 1
}

make_space()
{
	cd $ROOT/$LCL_MACH/etc/conf/sdevice.d/.save
	for i in *
	do
		rm -rf $ROOT/$LCL_MACH/etc/conf/pack.d/$i
		rm -rf $ROOT/$LCL_MACH/etc/conf/mod.d/$i
		rm -rf $ROOT/$LCL_MACH/etc/conf/mtune.d/$i
		rm -rf $ROOT/$LCL_MACH/etc/conf/autotune.d/$i
	done
	cd ..
	rm -rf .save
	rm -rf $ROOT/$LCL_MACH/etc/conf/mdevice.d/.save
	rm -f $ROOT/$LCL_MACH/etc/conf/cf.d/stune*
}

mini_kernel()
{
	PATH=$ROOT/$LCL_MACH/etc/conf/bin:$PATH

	STATIC_LIST="ansi asyc atup ca ccnv cdfs char clone cmux confmgr cram 
	dcompat dma dosfs eisa elf fd fdbuf fifofs fpe fs gentty gvid iaf iasy 
	intmap intp io kd kdvm kernel kma ldterm mca mem memfs mm mod modksym 
	name namefs nullzero pci pit proc procfs pstart resmgr rtc sad 
	sc01 sd01 sdi specfs st01 sum svc sysclass ts udev util ws"

	if [ "$LANG" = "ja" ]
	then
		STATIC_LIST="$STATIC_LIST gsd fnt"
	fi

	DYNAMIC_LIST="$MOD_LIST"
	MOD_LIST=""
	STATIC_LIST="$STATIC_LIST $DYNAMIC_LIST"

	STUB_LIST="asyhp async audit coff dac event ipc log mac merge 
	nfs prf pse rand segdev sysdump tpath xnamfs xque kdb_util"
	[ -d /etc/conf/pack.d/rlogin ] && STUB_LIST="$STUB_LIST rlogin"

	turnoff
	turnon $STATIC_LIST 
	stub $STUB_LIST
	make_static $STATIC_LIST
	tune
	make_space
}

create_boot_passwd ()
{
	echo "rootfs=memfs" > $PROTO/boot
	BOOTMSG=`pfmt -l UX:rsk_437 -s NOSTD -g rsk:27 "Booting from the Replicated System Kit Installation Floppy..." 2>&1`
	echo "BOOTMSG=$BOOTMSG" >> $PROTO/boot

	sed -e "/^MIP=/d"        \
	    -e "/^SIP=/d"        \
	    -e "/^BOOTMSG=/d"    \
	    -e "/^AUTOBOOT=/d"    \
	    -e "/^KERNEL=/d"     \
	    -e "/^DISK=/d"       \
	    -e "/^SLOWBOOT=/d"   \
	    -e "/^RESMGR=/d"     \
	    -e "/^TIMEOUT=/d"    \
	    -e "/^rootfs=/d"     \
	    -e "/^rootdev=/d"  /stand/boot >> $PROTO/boot

	sed 's/ttcompat//g' /etc/ap/chan.ap > $PROTO/chan.ap.flop
	awk -F: ' NF > 0 { if ( $3 < 100 )
			print $0 } ' /etc/passwd > $PROTO/passwd
	awk -F: ' NF > 0 { if ( $3 < 100 )
			print $0 } ' /etc/group > $PROTO/group
}
conframdfs ()
{
	PATH=$PROTO/bin:$TOOLS/usr/ccs/bin:$PATH export PATH
	EDSYM="/usr/lib/rsk/edsym"

	BASE=$ROOT/$LCL_MACH
	SOURCE_KERNEL=$BASE/stand/unix.$medium
	DEST_KERNEL=$BASE/stand/unix.rsk

	[ ! -s $SOURCE_KERNEL ] && {
		echo "$SOURCE_KERNEL does not exist." >> ${DRF_LOG_FL}
		return 1
	}
	cp $SOURCE_KERNEL $DEST_KERNEL 

	MEMSZ=`memsize`
	sed -e "s/LANG=XXX/LANG=$LANG/" \
	    -e "s/LC_CTYPE=XXX/LC_CTYPE=$LANG/" \
	    -e "s/KEYBOARD=XXX/KEYBOARD=$KEYBOARD/" \
	    -e "s/MEMSZ=XXX/MEMSZ=$MEMSZ/" \
		/usr/lib/rsk/rsk_inst.gen \
		> $PROTO/rsk_inst


	LCL_TEMP=/tmp/ramd$$ export LCL_TEMP
	ROOTFS=$LCL_TEMP/ramdisk.fs
	mkdir $LCL_TEMP
	if [ -s /usr/lib/rsk/locale/$LANG/txtstr ] 
	then
		DLANG=$LANG
	else
		DLANG=C
	fi

	#Uncomment all locale-specific lines
	#Delete all other comment lines
	sed \
		-e "s,^#$LANG,," \
		-e "s,^#.*,," \
		-e "s,\$LANG,$LANG," \
		-e "s,\$DLANG,$DLANG," \
		-e "s,\$PROTO,$PROTO," \
		-e "s,\$ROOT,$ROOT," \
		-e "s,\$MACH,$MACH," \
		-e "s,\$RESMGR,$RESMGR," \
		$RAMPROTO \
		> $LCL_TEMP/ramdproto
	> $ROOTFS

	create_boot_passwd

#setup for NWS (if installed)
	>/tmp/nws_sys_path
	$NWS_CONFIG && echo $NWS_SYS_PATH >/tmp/nws_sys_path

	echo "\nMaking file system image.\n" >> ${DRF_LOG_FL}
	for i in $FSCKS
	do
		mkdir -p $PROTO/stage/etc/fs/$i 2>/dev/null
		cp /usr/lib/rsk/fscks/$i.mkfs $PROTO/stage/etc/fs/$i/mkfs
		cp /etc/fs/$i/labelit $PROTO/stage/etc/fs/$i/labelit 2>/dev/null
	done
	(cd $PROTO/stage;find etc/fs -print|cpio -oc>$PROTO/mkfs.cpio)
	ls /dev/dsk/c*b*t*d*s* /dev/dsk/c*b*t*d*p* /dev/rdsk/c*b*t*d*s* /dev/rdsk/c*b*t*d*p* /dev/rmt/*tape*| cpio -oc >$PROTO/devs.cpio

#
# This file needs to be '0' length if $NODE is not set, so the 'echo'
# line cannot quote the variable name, and has to have the \\c appended
#
	echo $NODE \\c >$PROTO/nodename

	echo "$SERNO" >$PROTO/serno
#
# This file needs to exist even if there's no networking configured
# because it's put on the boot floppy filesystem in the prototype file
#
	>$PROTO/unixware.dat

	$INET && {
#Get current inet parameters before setting params for replicated system
		inet_getvals >/tmp/unixware.dat
		cp /etc/inet/hosts /etc/inet/Ohosts
		mv /etc/nodename /etc/Onodename
		setuname -n $NODE
		ROOT=/ /etc/inet/menu
		pfmt -l UX:rsk -s NOSTD -g rsk:21 "Processing continues...\n" 2>&1
		inet_getvals >$PROTO/unixware.dat
#Set inet parameters back
		mv /etc/Onodename /etc/nodename
		mv /etc/inet/Ohosts /etc/inet/hosts
		setuname -n `cat /etc/nodename`
		SILENT_INSTALL=true ROOT=/ /etc/inet/menu
		rm /tmp/unixware.dat
	}
	/usr/lib/rsk/sbfmkfs $MEMFS_META $MEMFS_FS $LCL_TEMP/ramdproto 2>$LCL_TEMP/mkfs_log
	if [ -s $LCL_TEMP/mkfs_log ]
	then
		echo  "mkfs of root filesystem failed" >> ${DRF_LOG_FL}
        	cat $LCL_TEMP/mkfs_log >> ${DRF_LOG_FL}
        	return 1
	fi
	cp $MEMFS_FS $MEMFS_META $BASE/stand 

	rm $PROTO/chan.ap.flop $PROTO/passwd $PROTO/group
	echo "\nStripping symbol table from $DEST_KERNEL" >> ${DRF_LOG_FL}
	$STRIP $DEST_KERNEL
	echo "\nEmptying .comment section of $DEST_KERNEL" >> ${DRF_LOG_FL}
	$MCS -d  $DEST_KERNEL
	rm -rf $LCL_TEMP
	return 0
}

build_kernel()
{
	[ -f $ROOT/$LCL_MACH/stand/unix.tape ] && $SkipKernBuild && return 0
	echo "\nCreating a temporary kernel build tree in $ROOT/$LCL_MACH" >> ${DRF_LOG_FL}
	rm -f $ROOT/$LCL_MACH/stand/unix.rsk*	# Informs conframdfs that a new
												# unix is being built.
	mkdir $FLOP_TMP_DIR
	[ -d $ROOT/$LCL_MACH/stand ] || mkdir -p $ROOT/$LCL_MACH/stand
	[ -d $ROOT/$LCL_MACH/etc/conf/mod.d ] || mkdir -p $ROOT/$LCL_MACH/etc/conf/mod.d
	[ -d $ROOT/$LCL_MACH/etc/conf/modnew.d ] || mkdir -p $ROOT/$LCL_MACH/etc/conf/modnew.d
	(
	cd /
	find etc/conf/*.d etc/initprog -print |
		egrep -v 'unix$|\.o$|mod\.d|modnew\.d' |
		cpio -pdumV $ROOT/$LCL_MACH >/dev/null 2>&1
	find etc/conf/pack.d \( -name Driver.o -o -name Driver_atup.o -o -name Modstub.o \) -print |
		cpio -pdumV $ROOT/$LCL_MACH >/dev/null 2>&1
	chmod 0644 $ROOT/$LCL_MACH/etc/initprog/?ip $ROOT/$LCL_MACH/etc/initprog/dcmp
	)
	[ -h $ROOT/$LCL_MACH/etc/conf/bin ] ||
		ln -s /etc/conf/bin $ROOT/$LCL_MACH/etc/conf/bin

	echo "\nReconfiguring files under $ROOT/$LCL_MACH/etc/conf" >> ${DRF_LOG_FL}

	( mini_kernel )
	MOD_LIST=""  ###TEMP ###
	echo "\nBuilding MOD list" >> ${DRF_LOG_FL}
	(
	cd $ROOT/$LCL_MACH/etc/conf/sdevice.d
	for i in $MOD_LIST
	do
		grep '^\$static' $i >/dev/null 2>&1
		if [ $? -eq 0 ] 
		then
			ed -s $i > /dev/null <<-END
				/^\$static
				d
				w
				q
			END
		fi
	done

	MOPTS=`for i in $MOD_LIST
		do
			echo "-M $i \c"
		done`

	echo "\nDoing idbuild -M to create loadable modules." >> ${DRF_LOG_FL}
	echo "The loadable modules option : $MOPTS" >> ${DRF_LOG_FL}

	MACH=$LCL_MACH /etc/conf/bin/idtype atup

	IDBLD_ERRS=/tmp/rsk_$$_moderr
	MACH=$LCL_MACH /etc/conf/bin/idbuild \
	     -I/usr/include ${MOPTS} -# > ${IDBLD_ERRS} 2>&1 || {
		  echo "The idbuild of ${MOPTS} failed with error = $? " >> ${DRF_LOG_FL}
		  echo "Check the ${IDBLD_ERRS} file for the idbuild failure reasons. " >> ${DRF_LOG_FL}
		  return 1
		}

	rm -f ${IDBLD_ERRS}
	mv $ROOT/$LCL_MACH/etc/conf/mod.d/* $ROOT/$LCL_MACH/etc/conf/modnew.d 2>/dev/null
	cd $ROOT/$LCL_MACH/etc/conf/modnew.d
	echo "\nExamining symbol tables of various loadable modules." >> ${DRF_LOG_FL}
	for i in $MOD_LIST
	do
		[ -f $i ] || {
			echo " Cannot find $ROOT/$LCL_MACH/etc/conf/modnew.d/$i" >> ${DRF_LOG_FL}
			return 1
		}
		$NM $i | grep UNDEF | sed -e 's/.*|//' > $FLOP_TMP_DIR/${i}list
	done
	) || return 1
	sed -e '/#/D' -e '/^$/D' /usr/lib/rsk/staticlist  > $FLOP_TMP_DIR/staticlist
	(
	for i in $MOD_LIST static
	do
		cat $FLOP_TMP_DIR/${i}list
	done
	) | LC_ALL=C sort -u > $FLOP_TMP_DIR/symlist

	IDBLD_ERRS=/tmp/rsk_$$_kernerr
	echo "\nBuilding the mini-kernel." >> ${DRF_LOG_FL}
	MACH=$LCL_MACH /etc/conf/bin/idbuild ${SYMS} \
		-I/usr/include  -# \
		-O $ROOT/$LCL_MACH/stand/unix.$medium > ${IDBLD_ERRS} 2>&1 || {
		    echo "The mini-kernel build failed with error = $? " >> ${DRF_LOG_FL}
		    echo "Check the ${IDBLD_ERRS} file for the mini-kernel failure reasons. " >> ${DRF_LOG_FL}
		    return 1
		 }

	rm -f ${IDBLD_ERRS}
	[ -d $PROTO/stage ] || mkdir $PROTO/stage
	rm -rf $FLOP_TMP_DIR
	return 0
}

f_pstamp()
{
	if [ -w $1 ]
	then
		$STRIP $1 > /dev/null 2>&1
		$MCS -d $1 > /dev/null 2>&1
	else
		echo  " Cannot write file $1, not mcs'ing" >> ${DRF_LOG_FL}
	fi
}

strip_em()
{
	echo "\nStripping various files." >> ${DRF_LOG_FL}
	for name in mip sip dcmp
	do
		f_pstamp $ROOT/$LCL_MACH/etc/initprog/$name
	done
	return 0
}


image_make1()
{
	set -e

	COMPRESS=/usr/lib/rsk/bzip

	${COMPRESS} -s28k ${ROOT}/${LCL_MACH}/stand/${MEMFS_META} > ${MEMFS_META}

	${COMPRESS} -s28k ${ROOT}/${LCL_MACH}/stand/${MEMFS_FS} > ${MEMFS_FS}

	${COMPRESS} -s28k ${ROOT}/${LCL_MACH}/etc/initprog/mip > mip

	${COMPRESS} -s28k ${ROOT}/${LCL_MACH}/etc/initprog/sip > sip 

	${COMPRESS} -s28k ${ROOT}/${LCL_MACH}/stand/${UNIX_FILE} > ${UNIX_FILE}

	[ -s ${ROOT}/${LCL_MACH}/etc/initprog/${LOGO_IMG} ] &&
	${COMPRESS} -s28k ${ROOT}/${LCL_MACH}/etc/initprog/${LOGO_IMG} > ${LOGO_IMG}
	cp ${ROOT}/${LCL_MACH}/etc/initprog/dcmp dcmp 

	${COMPRESS} -s28k /stand/${RESMGR} > resmgr

	set +e
	return 0
}

image_make2 ()
{
	rm -rf $PROTO/stage/.extra.d
	mkdir -p $PROTO/stage/.extra.d/etc/fs
	for i in $FSCKS
	do
	    mkdir $PROTO/stage/.extra.d/etc/fs/$i
	    ln -s /usr/lib/rsk/fscks/$i.mkfs $PROTO/stage/.extra.d/etc/fs/$i/mkfs
	    [ "$i" = "bfs" ] && continue
	    ln -s /etc/fs/$i/labelit $PROTO/stage/.extra.d/etc/fs/$i/labelit
	done

	mkdir -p $PROTO/stage/.extra.d/etc/conf/mod.d
	for i in $FSMODS
	do
	    $STRIP  $ROOT/$LCL_MACH/etc/conf/modnew.d/$i
	    $MCS -d $ROOT/$LCL_MACH/etc/conf/modnew.d/$i
	    ln -s $ROOT/$LCL_MACH/etc/conf/modnew.d/$i $PROTO/stage/.extra.d/etc/conf/mod.d/$i
	done
	return 0
}

cut_rsk()
{
	LOGO_IMG=logo.img export LOGO_IMG

	UNIX_FILE=unix.rsk export UNIX_FILE

	cd $PROTO/stage
	echo "${FDRIVE}\t${BLKCYLS}\t${BLKS}" > $PROTO/stage/drive_info
	rm -f unix
	image_make1  || return $?
	ln $UNIX_FILE unix
	cp $PROTO/boot boot
	[ -s ${LOGO_IMG} ] || LOGO_IMG=

	/usr/lib/rsk/sbfpack /etc/fboot dcmp mip sip boot $LOGO_IMG ${MEMFS_META} ${MEMFS_FS} \
		unix resmgr /dev/rdsk/${FDRIVE}t  >> ${DRF_LOG_FL} || return $?

	return $ret_val
}

#main()
medium=tape
PATH=:/usr/lib/rsk:$PATH export PATH
RAMPROTO="$PROTO/rskram.proto" export RAMPROTO
cp /usr/lib/rsk/rskram.proto $RAMPROTO
FLOP_TMP_DIR=/tmp/flop.$$ export FLOP_TMP_DIR
MEMFS_FS=memfs.fs
MEMFS_META=memfs.meta
NM=/usr/ccs/bin/nm
DUMP=/usr/ccs/bin/dump
STRIP=/usr/ccs/bin/strip 
MCS=/usr/bin/mcs 
export NM DUMP STRIP MCS
export MOD_LIST _DRVMODS LCL_TEMP 
SYMS="-l $FLOP_TMP_DIR/symlist" export SYMS
eval `defadm locale LANG 2>/dev/null`
eval `defadm keyboard KEYBOARD 2>/dev/null` 
[ "$KEYBOARD" = "NONE" ] && KEYBOARD=""
LANG=${LANG:-C}  
#Remove the following line when RSK supports non-C locales
LANG=C
export LANG KEYBOARD 

echo "\n\t The log messages from prep_flop " > ${DRF_LOG_FL}

if [ "$LANG" = "ja" ]
then
	add_terminfo_mb    # add multi-byte terminfo to ram proto file
elif [ "$LANG" = "C" -a -z "$KEYBOARD" ] ||
        [ "$KEYBOARD" = "AX" ] || [ "$KEYBOARD" = "A01" ]
then
	add_terminfo      # add AT386 terminfo entries to ram proto file
else
	add_terminfo_ie   # add AT386-ie terminfo entries ro ram proto file
fi
[ -z "$KEYBOARD" ] || add_keyboard   # add keyboard info to ram proto file
[ "$LANG" != "C" ] && add_lc_ctype   # Add the LC_CTYPE file to ram proto file

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
		pfmt -l UX:rsk -s NOSTD -g rsk:1 "\n\tReplicated System Kit boot floppy creation terminated.\n\n"
		exit 1
	}
	grep -q $NWS_SYS_PATH /etc/vfstab && {
		mount | grep $NWS_SYS_PATH >/dev/null || mount $NWS_SYS_PATH || {
			pfmt -l UX:rsk -s NOSTD -g rsk:32 "\n\tCannot mount SYS volume of NetWare server.\n"
			pfmt -l UX:rsk -s NOSTD -g rsk:1 "\n\tReplicated System Kit boot floppy creation terminated.\n\n"
			exit 1
		}
	}
}
echo "exit 0" > $PROTO/putdev
echo "/dev/vt /dev/kd/kd	3" > $PROTO/work.1

RESMGR= export RESMGR
eval `grep "^RESMGR=" /stand/boot`
[ -z "$RESMGR" ] && RESMGR=resmgr
set_mod_list
NO_SECOND_FLP=0 export NO_SECOND_FLP
error=1
build_kernel && strip_em && conframdfs && cut_rsk && error=0

[ "$error" = "0" -a "$NO_SECOND_FLP" = "1" ] && error=2
rm -f $PROTO/rskram.proto $PROTO/putdev $PROTO/work.1 $PROTO/rsk_inst
echo "\nprep_flop: Done." >> ${DRF_LOG_FL}
$KeepKernFiles || rm -rf $LCL_TEMP $FLOP_TMP_DIR 

exit $error
