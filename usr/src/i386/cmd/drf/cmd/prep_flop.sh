#!/usr/bin/ksh

#ident	"@(#)drf:cmd/prep_flop.sh	1.30.2.3"

# This script will create the mini-kernel, build special boot-floppy only
# commands, and prep the executables included on the boot floppy.  The shell
# script cut_flop is then used to create the boot floppy.

############# Initialization and Command-line Parsing #############

cd $PROTO

STRIP=/usr/ccs/bin/strip
MCS=/usr/ccs/bin/mcs
NM=/usr/ccs/bin/nm

PATH=:/usr/lib/drf:/usr/ccs/bin:$PATH export PATH
RAMPROTOF="$PROTO/ramdfs.proto" export RAMPROTOF
FLOP_TMP_DIR=$PROTO/flop.$$ export FLOP_TMP_DIR

# IMPORTANT -- DO NOT CHANGE THE ORDER OF THE MODULES IN _DRVMODS!
#_DRVMODS="dpt cpqsc adsc ictha ide" export _DRVMODS
_DRVMODS=" " export _DRVMODS
FSMODS="" export FSMODS

CMD=$0

kernel=true
kflag=true
# We don't use unixsyms any more for both non/with kdb kernel.
#		SYMS="-l $FLOP_TMP_DIR/symlist"
SYMS=""
if [ "$DRF_KDB" ]
then
	KDB=kdb
else
	KDB=nokdb
fi

############# Function defintions #############

build_kernel()
{
	trap "rm -rf $FLOP_TMP_DIR; exit 1" 1 2 3 15
	mkdir -p $FLOP_TMP_DIR

	echo "\nCreating a kernel build tree in $ROOT/$LCL_MACH" >> ${DRF_LOG_FL}
	rm -f $ROOT/$LCL_MACH/stand/unix.nostrip
	[ -d $ROOT/$LCL_MACH/stand ] || mkdir -p $ROOT/$LCL_MACH/stand
	[ -d $ROOT/$LCL_MACH/etc/conf/mod.d ] ||
		mkdir -p $ROOT/$LCL_MACH/etc/conf/mod.d
	[ -d $ROOT/$LCL_MACH/etc/conf/modnew.d ] ||
		mkdir -p $ROOT/$LCL_MACH/etc/conf/modnew.d
	(
		cd /
			find etc/conf/*.d -print |
			egrep -v 'unix$|\.o$|mod\.d|modnew\.d' |
			cpio -pdum $ROOT/$LCL_MACH
		find etc/conf/pack.d \( -name Driver.o -o -name Modstub.o -o -name Driver_atup.o -o -name stubs.o \) -print |
			cpio -pdum $ROOT/$LCL_MACH
	)
	cp /usr/lib/drf/fdboot $PROTO
	cp /usr/lib/drf/stage2.fdinst $PROTO
	cp /stand/dcmp.blm $PROTO
	cp /stand/stage3.blm $PROTO
	cp /stand/platform.blm $PROTO
	cp /stand/logo.img $PROTO 2>/dev/null
	[ -h $ROOT/$LCL_MACH/etc/conf/bin ] ||
		ln -s /etc/conf/bin $ROOT/$LCL_MACH/etc/conf/bin

	echo "\nReconfiguring files under $ROOT/$LCL_MACH/etc/conf" >> ${DRF_LOG_FL}
	# mini_kernel echoes the list of loadable modules to stdout.
	MACH=$LCL_MACH /usr/lib/drf/mini_kernel $KDB > $PROTO/mini.$$
	rc=$?
	if [ $rc -ne 0 ]
	then
		cat $PROTO/mini.$$ >> ${DRF_LOG_FL}
		rm -f $PROTO/mini.$$
		return 1
	fi

	MODLIST=`cat $PROTO/mini.$$`
	rm -f $PROTO/mini.$$
	
	echo "\nCreating loadable modules." >> ${DRF_LOG_FL}
	(
	cd $ROOT/$LCL_MACH/etc/conf/sdevice.d
	for i in $MODLIST
	do
		grep '^\$static' $i > /dev/null 2>&1
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

	MODARGS=`for i in $MODLIST
		do
			echo "-M $i \c"
		done`
	echo "\nDoing idbuild -M to create loadable modules." >> ${DRF_LOG_FL}
	echo "The loadable modules option : $MODARGS" >> ${DRF_LOG_FL}
	MACH=$LCL_MACH $ROOT/$LCL_MACH/etc/conf/bin/idtype atup

	IDBLD_ERRS=/tmp/drf_$$_moderr
	MACH=$LCL_MACH $ROOT/$LCL_MACH/etc/conf/bin/idbuild \
		-I/usr/include $MODARGS -# > ${IDBLD_ERRS} 2>&1
	rc=$?
	if [ $rc -ne 0 ]
	then
		echo "The idbuild of ${MODARGS} failed with error = $rc " >> ${DRF_LOG_FL}
		echo "Check the ${IDBLD_ERRS} file for the idbuild failure reasons." >> ${DRF_LOG_FL}
		exit 1
	fi

	rm -rf ${IDBLD_ERRS}

	mv $ROOT/$LCL_MACH/etc/conf/mod.d/* $ROOT/$LCL_MACH/etc/conf/modnew.d 2> /dev/null

	echo "\nExamining symbol tables of various loadable modules."
	(
	cd $ROOT/$LCL_MACH/etc/conf/modnew.d
	for i in $MODLIST
	do
		[ -f $i ] || {
			echo ERROR -- Cannot find $ROOT/$LCL_MACH/etc/conf/modnew.d/$i >&2
			exit 1
		}
		${NM} $i | grep UNDEF | sed -e 's/.*|//' > $FLOP_TMP_DIR/${i}list
	done
	)
	) || return 1
	sed -e '/#/D' -e '/^$/D' /usr/lib/drf/staticlist > $FLOP_TMP_DIR/staticlist
	{
	for i in $MODLIST static
	do
		cat $FLOP_TMP_DIR/${i}list
	done
	} | LC_ALL=C sort -u > $FLOP_TMP_DIR/symlist

	IDBLD_ERRS=/tmp/drf_$$_kernerr
	echo "\nBuilding the mini-kernel." >> ${DRF_LOG_FL}

	echo "\nSaving symlist ..."
	cp $FLOP_TMP_DIR/symlist $PROTO/symlist
#
# -c flag added to override the various mdep.c checks in idmkunix,
# e.g. don't do dma channel conflict checking, memory address overlap
# checking, etc.
#
	echo "$ROOT/$LCL_MACH/etc/conf/bin/idbuild " >> ${DRF_LOG_FL}

	MACH=$LCL_MACH $ROOT/$LCL_MACH/etc/conf/bin/idbuild -K -x \
		-I/usr/include \
		-O $ROOT/$LCL_MACH/stand/unix.nostrip > ${IDBLD_ERRS} 2>&1
	rc=$?
	if [ $rc -ne 0 ]
	then
			echo "The mini-kernel build failed with error = $rc " >> ${DRF_LOG_FL}
			echo "Check the ${IDBLD_ERRS} file for the mini-kernel failure reasons. " >> ${DRF_LOG_FL}
			return 1
	fi

	rm -f ${IDBLD_ERRS}
	[ -d $PROTO/stage ] || mkdir $PROTO/stage
	rm -rf $FLOP_TMP_DIR

	# collect PDI driver entries from mod_register file or /etc/loadmods
	> $ROOT/$LCL_MACH/stand/loadmods
	for i in $_DRVMODS
	do
		grep $i $ROOT/$LCL_MACH/etc/conf/cf.d/mod_register \
			>> $ROOT/$LCL_MACH/stand/loadmods
		if [ $? -ne 0 ]
		then
			grep $i /etc/loadmods >> $ROOT/$LCL_MACH/stand/loadmods
		fi
	done
	return $rc
}

# Just strip the comment section using mcs(1).

f_pstamp()
{
	if [ -w $1 ]
	then
		# There is a problem with strip, so don't use it
		# ${STRIP} $1 > /dev/null 2>&1
		${MCS} -d $1 > /dev/null 2>&1
	else
		echo "WARNING: Cannot write file $1, not mcs'ing"
	fi
}

strip_em()
{
	echo "\nStripping various files."
	cd $PROTO
	export SLBASE
	SLBASE=$PROTO/bin
	SLBASE=$ROOT/.$MACH
	for name in $ROOT/$LCL_MACH/etc/conf/modnew.d/*
	do
		f_pstamp $name
	done
}

# after putting the undefined DLM NIC symbols into .unixsyms in case that 
# driver is chosen for a netinstall we need to disable the driver so that a 
# later cut.flop->cut.flop.mk will not include the NICS when building the
# resmgr file (since idconfupdate -s would still find the nics as enabled from
# first call to mini_kernel)
turnoffnics()
{
	NICLIST=`MACH=$LCL_MACH mini_kernel nics` || exit $?
	for nic in $NICLIST
	do

		awk 'BEGIN	{
					OFS="\t";
				}
			NF==11	{
					$2="N";
					print $0
					next
				}
			NF==12	{
					$2="N";
					print $0
					next
				}
				{ print $0 }' \
		$ROOT/$LCL_MACH/etc/conf/sdevice.d/$nic > \
		$ROOT/$LCL_MACH/etc/conf/sdevice.d/.$nic && \
		mv $ROOT/$LCL_MACH/etc/conf/sdevice.d/.$nic \
		   $ROOT/$LCL_MACH/etc/conf/sdevice.d/$nic
	done
}

add_terminfo()
{
	ed -s $RAMPROTOF > /dev/null <<-END
		/ANSI
		a
		AT386	---644 0 3 /usr/share/lib/terminfo/A/AT386
		AT386-M	---644 0 3 /usr/share/lib/terminfo/A/AT386-M
		.
		w
		q
	END
	return
}

add_terminfo_ie()
{
	ed -s $RAMPROTOF > /dev/null <<-END
		/ANSI
		a
		AT386-ie	---644 0 3 /usr/share/lib/terminfo/A/AT386-ie
		AT386-M-ie	---644 0 3 /usr/share/lib/terminfo/A/AT386-M-ie
		.
		w
		q
	END
	return
}

add_terminfo_mb()
{
	ed -s $RAMPROTOF > /dev/null <<-END
		/ANSI
		a
		AT386-mb	---644 0 3 /usr/share/lib/terminfo/A/AT386-mb
		AT386-M-mb	---644 0 3 /usr/share/lib/terminfo/A/AT386-M-mb
		.
		w
		q
	END
	return
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
	( cd /usr/lib/drf
		./keycomp /usr/lib/keyboard/$KEYBOARD $PROTO/kbmap
		./dkcomp /usr/lib/mapchan/88591.dk $PROTO/dead_keys
	)
	# Check to see if it is a dir/file string instead of just a file string
	FIL="`echo $KEYBOARD | cut -d/ -f1`"
	DIR="`echo $KEYBOARD | cut -d/ -f2`"
	if [ "$DIR" != "$KEYBOARD" ]
	then
		ed -s $RAMPROTOF > /dev/null <<-END
			/keyboards
			a
			$FIL	d--755 2 2
				$DIR	d--755 2 2
				kbmap	---644 0 3 $PROTO/kbmap
				dead_keys	---644 0 3 $PROTO/dead_keys
				\$
			\$
			.
			w
			q
		END
	else
		ed -s $RAMPROTOF > /dev/null <<-END
			/keyboards
			a
			$KEYBOARD	d--755 2 2
			kbmap	---644 0 3 $PROTO/kbmap
			dead_keys	---644 0 3 $PROTO/dead_keys
			\$
			.
			w
			q
		END
	fi
	return
}

add_lc_ctype()
{
	[ -s /usr/lib/locale/$LANG/LC_CTYPE ] || {
		echo "\n\t/usr/lib/locale/$LANG/LC_CTYPE file is missing" >> ${DRF_LOG_FL}
		return
	}
	ed -s $RAMPROTOF > /dev/null <<-END
		/locale	d
		a
		$LANG	d--755 2 2
		LC_CTYPE	---644 0 3 /usr/lib/locale/$LANG/LC_CTYPE
		\$
		.
		w
		q
	END
	return
}

addodm_vfstab()
{
	ed -s $RAMPROTOF > /dev/null <<-END
		/loadmods
		a
		odm_vfstab	---644 0 3 $PROTO/vfstab
		old_vtoc_1	---644 0 3 $PROTO/old_vtoc_1
		new_vtoc_1	---644 0 3 $PROTO/new_vtoc_1
		.
		w
		q
	END
	return
}

addodm_vtoc_2()
{
	ed -s $RAMPROTOF > /dev/null <<-END
		/loadmods
		a
		old_vtoc_2	---644 0 3 $PROTO/old_vtoc_2
		new_vtoc_2	---644 0 3 $PROTO/new_vtoc_2
		.
		w
		q
	END
	return
}

set_mod_list()
{
	# Get the fs type for root, /stand, /usr, /home, /var, and /home2

	FSCKS="`print \`egrep -v '^\#' /etc/vfstab | egrep '[	 ]/[	 ]|[	 ]/stand[	 ]|[	 ]/usr[	 ]|[	 ]/home[	 ]|[	 ]/var[	] |[	 ]/home2[	 ]' | cut -f4 | egrep 'bfs|s5|ufs|vxfs' | sort -u\``"

	FSMODS=`echo "$FSCKS"`
	# ufs module depends on sfs module. So,...
	echo $FSMODS | grep ufs > /dev/null 2>&1
	[ $? -eq 0 ] && FSMODS="$FSMODS sfs"
	#Add dow module if sfs module is present
	echo "$FSMODS" | egrep 'sfs' > /dev/null 2>&1
	[ $? -eq 0 ] && FSMODS="$FSMODS dow"
	_DRVMODS="`print \`/etc/scsi/pdiconfig | grep '[	]Y[	]' | cut -f1 | sort -u\``"
	MODLIST="$FSMODS $_DRVMODS"
	export MODLIST
}

#main()
{
	VM_RUNNING=No export VM_RUNNING
	# Get and set current LANG and KEYBOARD
	LANG=${LANG:-NoLang}
	if [ "$LANG" == "NoLang" -o ! -d /usr/lib/drf/locale/$LANG ]
	then
		eval `defadm locale LANG 2>/dev/null`
	fi
	LANG=${LANG:-C}
	LANGS="`echo $LANG | cut -c1,2`"
	# Force use of system default keyboard, since restore will use the
	#	system keyboard
	eval `defadm keyboard KEYBOARD 2>/dev/null`
	[ "$KEYBOARD" = "NONE" ] && KEYBOARD=""
	KEYBOARDS="`echo $KEYBOARD | cut -d/ -f2`"
	export LANG LANGS KEYBOARD KEYBOARDS

	echo "\n\t The log messages from $0 " > ${DRF_LOG_FL}
	echo "\t\tLANG=$LANG, LANGS=$LANGS, KEYBOARD=$KEYBOARD, KEYBOARDS=$KEYBOARDS" >> ${DRF_LOG_FL}

	cp /usr/lib/drf/drfram.proto $RAMPROTOF

	if [ "$LANGS" = "ja" ]
	then
		add_terminfo_mb	# add multi-byte terminfo to ram proto file
	elif [ "$LANGS" = "C" -a -z "$KEYBOARD" ] ||
		[ "$KEYBOARDS" = "AX" ] || [ "$KEYBOARDS" = "A01" ]
	then
		add_terminfo	# add AT386 terminfo entries to ram proto file
	else
		add_terminfo_ie	# add AT386 terminfo entries to ram proto file
	fi
	[ -z "$KEYBOARD" ] || add_keyboard # add keyboard info to ram proto file
	[ "$LANG" != "C" ] && add_lc_ctype # add LC_CTYPE file to ram proto file

	grep standvol /etc/vfstab >/dev/null 2>&1	# check for odm
	[ $? -eq 0 ] && {
		VM_RUNNING=Yes
		/usr/lib/drf/odm_vfs $PROTO/vfstab $PROTO/old_vtoc_1 \
			$PROTO/new_vtoc_1 $PROTO/old_vtoc_2 $PROTO/new_vtoc_2
		addodm_vfstab
		[ -s $PROTO/old_vtoc_2 ] && addodm_vtoc_2
	}

	NWS_CONFIG=false
	NWS_SYS_PATH=""
	[ -f /var/sadm/pkg/nwsrvr/pkginfo -o -f /var/sadm/pkg/nwsrvrJ/pkginfo ] && {
		NWS_CONFIG=true
		while read num vol type path rest
		do
			[ "$vol" != "SYS" ] && continue
			echo $path >$PROTO/syspath
			break
		done < /etc/netware/voltab

		NWS_SYS_PATH=`cat $PROTO/syspath`

		[ "$NWS_SYS_PATH" = "" ] && {
			pfmt -l UX:drf -s NOSTD -g drf:31 "\n\tCannot locate SYS volume of NetWare Server.\n"
			pfmt -l UX:drf -s NOSTD -g drf:30 "\n\tEmergency Recovery floppy creation terminated.\n\n"
			exit 1
		}
		awk -v mnt_pt="$NWS_SYS_PATH" '{ if ($3==mnt_pt) exit_val=1 } END { exit exit_val }' </etc/vfstab || {
			mount | grep $NWS_SYS_PATH > /dev/null || mount $NWS_SYS_PATH ||
			pfmt -l UX:drf -s NOSTD -g drf:32 "\n\tCannot mount SYS volume of NetWare Server.\n"
			pfmt -l UX:drf -s NOSTD -g drf:30 "\n\tEmergency Recovery floppy creation terminated.\n\n"
			exit 1
		}
	}
	echo "exit 0" > $PROTO/putdev
	echo "/dev/vt /dev/kd/kd	3" > $PROTO/work.1
	RESMGR= export RESMGR
	eval `echo "RESMGR=\c"; grep -i "^files=" /stand/boot | cut -d'=' -f2-`
	[ -z "$RESMGR" ] && RESMGR=resmgr

	set_mod_list

	build_kernel
	[ $? -ne 0 ] && exit 1
	[ -s $ROOT/$LCL_MACH/stand/unix.nostrip ] || {
		echo ERROR -- You must use the -k or -K flag at least once. >&2
		exit 1
	}
	strip_em
	[ -d $PROTO/bin ] || mkdir -p $PROTO/bin
	[ -d $PROTO/etc/conf/hbamod.d ] || mkdir -p $PROTO/etc/conf/hbamod.d
	[ -d $PROTO/etc/conf/mod.d ] || mkdir -p $PROTO/etc/conf/mod.d
	[ -d $PROTO/stand ] || mkdir -p $PROTO/stand
	cp /usr/lib/drf/rmwhite $PROTO/bin
	cp /usr/lib/drf/checkwhite $PROTO/bin
	for mod in $_DRVMODS
	do
		mv $ROOT/$LCL_MACH/etc/conf/modnew.d/$mod $PROTO/etc/conf/hbamod.d
	done
	mv $ROOT/$LCL_MACH/etc/conf/modnew.d/* $PROTO/etc/conf/mod.d
	for i in $FSMODS
	do
		[ -d $PROTO/etc/fs/$i -o "$i" = "dow" ] || mkdir -p $PROTO/etc/fs/$i
		[ -r /etc/fs/$i/fsck ] && cp /etc/fs/$i/fsck $PROTO/etc/fs/$i/fsck
		[ -r /etc/fs/$i/mkfs ] && cp /etc/fs/$i/mkfs $PROTO/etc/fs/$i/mkfs
		[ -r /etc/fs/$i/labelit ] && cp /etc/fs/$i/labelit $PROTO/etc/fs/$i/labelit
	done
	cd /usr/lib/drf
	cut_flop || exit $?
	echo "\n$0: Done."
        exit 0
}
