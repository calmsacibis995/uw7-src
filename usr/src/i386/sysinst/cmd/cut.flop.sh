#!/usr/bin/ksh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997 Santa Cruz Operation, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident "@(#)cut.flop.sh	15.3	97/12/31"

# script to create boot floppy.

function ask_drive
{
	[ -n "$MEDIUM" ] ||
	read "MEDIUM? Please enter diskette1 or diskette2 (default is diskette1): "
	[ -n "$MEDIUM" ] || MEDIUM="diskette1"
	case "$MEDIUM" in
	diskette1)
		DRVLETTER=A
		;;
	diskette2)
		DRVLETTER=B
		;;
	*)
		print -u2 ERROR: Must specify diskette1 or diskette2.
		exit 1
		;;
	esac
	export BLOCKS=$(devattr $MEDIUM capacity)
	case $BLOCKS in
	2844) # 3.5-inch diskette

	   	read "TWO_FLOP? Do you want to cut two floppies Y/N (default is N): "
		
		[ -n "$TWO_FLOP" ] || NUM_FLOP="N"
		if [ "$TWO_FLOP" = "Y" -o "$TWO_FLOP" = "y" ]
		then
	            BLOCKS=2370
                fi	
		
		TRKSIZE=36
		if [ "${LANG}" = "ja" ]
		then
			FIRST='FIRST '
		else
			FIRST=''
		fi
		;;
	2370) # 5.25-inch diskette
		TRKSIZE=30
		FIRST='FIRST '
		;;
	*)
		print -u2 ERROR -- diskette must be either 1.44MB 3.5 inch
		print -u2 or 1.2MB 5.25 inch
		exit 2
		;;
	esac
	FDRIVE=$(devattr $MEDIUM fmtcmd)
	FDRIVE=${FDRIVE##* }
}

function select_boot
{
# bootmsgs localization not yet addressed
#       rm -f bootmsgs
#
#       BOOT=$PROTO/locale/$LANG/bootmsgs
#       [ -s "$BOOT" ] || BOOT=$PROTO/locale/C/bootmsgs
#	[ -s "$BOOT" ] || {
#		print -u2 ERROR: $BOOT does not exist.
#		exit 1
#	}
#       ln -s $BOOT bootmsgs
:


# not used with the new boot
#	FBOOT=$ROOT/$LCL_MACH/etc/fboot
#	[ -z "$special_flag" ] || FBOOT=$ROOT/$LCL_MACH/etc/cpqfboot
#	[ -s "$FBOOT" ] || {
#		print -u2 ERROR: $FBOOT does not exist.
#		exit 1
#	}
#	ln -s $FBOOT fboot
}

function stripem
{
	typeset i save_pwd=$PWD

	[ -d $2 ] || mkdir -p $2
	cd $1
	for i in *
	do
		rmwhite $i $2/$i
	done
	cd $save_pwd
}

function strip_comments
{
	. $PROTO/bin/rmwhite
	[ -d $PROTO/stage/winxksh ] || mkdir $PROTO/stage/winxksh
	rmwhite $ROOT/$MACH/usr/lib/winxksh/scr_init $PROTO/stage/winxksh/scr_init

	rmwhite $ROOT/$MACH/usr/lib/winxksh/winrc $PROTO/stage/winxksh/winrc
#	rmwhite $PROTO/winrc $PROTO/stage/winxksh/winrc

	stripem $ROOT/$MACH/etc/dcu.d/menus   $PROTO/stage/dcu.d/menus
	stripem $ROOT/$MACH/etc/dcu.d/scripts $PROTO/stage/dcu.d/scripts
	stripem $PROTO/desktop/menus          $PROTO/stage/desktop/menus
	stripem $PROTO/desktop/scripts        $PROTO/stage/desktop/scripts
	stripem $PROTO/desktop/ui_modules     $PROTO/stage/desktop/ui_modules
	stripem $PROTO/desktop/install_modules $PROTO/stage/desktop/install_modules
}

function prep_flop2
{
	typeset FONT

	(( BLOCKS == 2370 )) || [ "$LANG" = "ja" ] || return
	print "Making tree for SECOND boot floppy."
	rm -rf flop2root
	set -e
	mkdir flop2root
	cd flop2root
	mkdir -p \
                tmp \
                isl/ui_modules \
                isl/install_modules \
		etc/conf/hbamod.d \
		etc/conf/fsmod.d \
		etc/dcu.d/dculib \
		etc/dcu.d/locale/C/help \
		etc/dcu.d/locale/fr/help \
		etc/dcu.d/locale/de/help \
		etc/dcu.d/locale/es/help \
		etc/dcu.d/menus \
		etc/dcu.d/scripts \
		etc/inst/scripts \
		etc/inst/locale/C/menus/help \
		etc/inst/locale/de/menus/help \
		etc/inst/locale/fr/menus/help \
		etc/inst/locale/es/menus/help \
		etc/inst/locale/keyboards/code_sets \
		etc/scsi \
		sbin \
		usr/bin \
		usr/lib \
		usr/sbin \
		.extra.d

		for LANG in C fr de es
		do
	cp $PROTO/locale/$LANG/menus/help/nics.conf/config.z etc/inst/locale/${LANG}/menus/help/config.z
	cp $PROTO/locale/$LANG/menus/help/nicshlp.z etc/inst/locale/${LANG}/menus/help/nicshlp.z
	cp $ROOT/$MACH/etc/dcu.d/locale/$LANG/config etc/dcu.d/locale/$LANG
	cp $ROOT/$MACH/etc/dcu.d/locale/$LANG/txtstrings etc/dcu.d/locale/$LANG
	cp $PROTO/locale/$LANG/menus/help/locale_hcf.z etc/inst/locale/$LANG/menus/help
	cp $PROTO/locale/$LANG/menus/help/dcu.d/locale_hcf.z etc/dcu.d/locale/$LANG/help
		done
	cp $PROTO/stage/dcu.d/menus/drivers etc/dcu.d/menus/drivers
	cp $PROTO/stage/desktop/scripts/desktop.prep etc/inst/scripts/desktop.prep
	cp $PROTO/stage/desktop/scripts/netinst etc/inst/scripts/netinst
	cp $PROTO/stage/desktop/scripts/netinst_clean etc/inst/scripts/netinst_clean
	cp $ROOT/$MACH/usr/bin/ls usr/bin/ls
	cp $ROOT/$MACH/usr/bin/autodetect usr/bin/autodetect
	cp $ROOT/$MACH/usr/bin/uncompress usr/bin/uncompress
	cp $ROOT/$MACH/usr/lib/libresmgr.so usr/lib/libresmgr.so
	cp $ROOT/$MACH/usr/sbin/fdisk.boot usr/sbin/fdisk
	cp $ROOT/$MACH/usr/sbin/prtvtoc usr/sbin/prtvtoc
#	cp $ROOT/.$MACH/etc/conf/modnew.d/adsc etc/conf/hbamod.d/adsc
#	cp $ROOT/.$MACH/etc/conf/modnew.d/cpqsc  etc/conf/hbamod.d/cpqsc
#	cp $ROOT/.$MACH/etc/conf/modnew.d/dpt etc/conf/hbamod.d/dpt
#	cp $ROOT/.$MACH/etc/conf/modnew.d/ictha etc/conf/hbamod.d/ictha
#	cp $ROOT/.$MACH/etc/conf/modnew.d/ide etc/conf/hbamod.d/ide
	cp $ROOT/.$MACH/etc/conf/modnew.d/dosfs etc/conf/fsmod.d/dosfs
	cp $ROOT/.$MACH/etc/conf/modnew.d/s5 etc/conf/fsmod.d/s5
	cp $ROOT/$MACH/sbin/resmgr sbin/resmgr
	cp $ROOT/$MACH/usr/bin/sort usr/bin/sort
	cp $PROTO/stage/desktop/install_modules/inst isl/install_modules/inst

	cp $PROTO/stage/hbaflop.cpio etc/hbaflop.cpio
	cp $ROOT/usr/src/work/subsys/license/snakapi/isl sbin/licisl

# copy non-Latin-1 keyboards

	for FONT in 88592 88595 88597 88599
	do
		cp $PROTO/desktop/keyboards/code_sets/$FONT \
			etc/inst/locale/keyboards/code_sets
	done

# removed until DDI8 subsystem support works during ISL
## for ddi8 nic support
#	mkdir -p etc/conf/bin etc/conf/interface.d
#	cp $ROOT/.$MACH/etc/conf/bin/idmknodd etc/conf/bin/idmknodd
#	(
#		cd $ROOT/.$MACH/etc/conf/interface.d
#		ls * | cpio -dumpv $PROTO/stage/flop2root/etc/conf/interface.d
#	)

        typeset INSTALL_UI

        INSTALL_UI="check_pkg_sizes cans pla ad_flash date name cans.rc change_disk_ops change_slices check_media check_preserve dcu disk_config disk_ops disk_size  hba keyboard lang license media net_wrapper osmp owner partition rusure security slices password password.rc welcome disk.rc frametype.rc sets services ip_math.rc tcpconf ipxconf nisconf locale zone nics_config nics_detect nics_select"


       for i in $INSTALL_UI
       do
       cp $PROTO/stage/desktop/ui_modules/$i isl/ui_modules/$i
       done

       cp $PROTO/bin/encrypt_str isl/ui_modules

        typeset txts

        txts=" check_pkg_sizes.txtstrings ad_flash.txtstrings pla.txtstrings cans.txtstrings change_disk_ops.txtstrings change_slices.txtstrings check_media.txtstrings check_preserve.txtstrings date.txtstrings dcu.txtstrings disk_config.txtstrings disk_ops.txtstrings disk_related.txtstrings disk_size.txtstrings hba.txtstrings keyboard.txtstrings license.txtstrings media.txtstrings name.txtstrings osmp.txtstrings owner.txtstrings partition.txtstrings password.txtstrings rusure.txtstrings sets.txtstrings security.txtstrings slices.txtstrings welcome.txtstrings inst.txtstrings partition_sys.txtstrings pkginst.txtstrings net_wrapper.txtstrings locale.txtstrings help.txtstrings tcpconf.txtstrings ipxconf.txtstrings nisconf.txtstrings zone.txtstrings"


       for i in $txts
       do
		for bubu in C de fr es
		do
#      	cp $PROTO/locale/${LANG}/menus/$i etc/inst/locale/${LANG}/menus/$i
       			cp $PROTO/locale/${bubu}/menus/$i etc/inst/locale/${bubu}/menus/$i
			chmod u+w etc/inst/locale/${bubu}/menus/$i

		done
       done

	set +e
# (Jeremy) was:
#	LIST=$(find * -type f -print)
	LIST=$(find . -type f -print)
	BOOT2_FILES=$(find . -type f -print | wc -l)
	LANG_FILES=$(find . -type f -print | grep "/locale/C/" | wc -l)

	print $LIST | xargs chmod 555 
	print $LIST | xargs chgrp sys
	print $LIST | xargs chown root
	typeset OIFS=$IFS
	IFS=
	print -r $LIST | cpio -oLDV -H crc > /tmp/out1.$$
	IFS=$OIFS
	cd ..
	print Compressing image for SECOND boot floppy.
	bzip -s32k /tmp/out1.$$ > /tmp/out2.$$
	wrt -s /tmp/out2.$$ > flop2.image
	rm /tmp/out?.$$
}

function get_answer
{
	while :
	do
		print -n "\007\nInsert $1 floppy into $MEDIUM drive and press\n\t<ENTER> "
		print -n "to write floppy,\n\tF\tto format and write floppy,\n\ts\tto "
		print -n "skip, \n\td\tto change output device, or\n\tq\tto quit: "
		read a
		case "$a" in
		"")
			return 0
			;;
		F)
			/usr/sbin/format -i$2 $FDRIVE || exit $?
			return 0
			;;
		s)
			return 1
			;;
		d)
			echo
			echo -n "Enter new device to write to :"
			read FDRIVE
			return 0
			;;
		q)
			exit 0
			;;
		*)
			print -u2 ERROR: Invalid response -- try again.
			;;
		esac
	done
}

function cut_flop1
{
	get_answer "${FIRST}boot" 1 || return

	# Create kdb.rc.  Contents can be a single space.
	echo " " > kdb.rc

	cmd="$MKSMALLFS -Bfdboot -x2 -Gstage2.fdinst \
		dcmp.blm stage3.blm platform.blm $LOGO \
		boot bootmsgs help.txt kdb.rc \
		unix resmgr memfs.meta memfs.fs $FDRIVE"
	echo $cmd
	$cmd || exit $?

	print "Done with ${FIRST}boot floppy.\007"
}

#
#  Floppy two is a little unusual. The first track is a small cpio archive,
# and the rest of the disk is a weird format. It might be cpio. It is created
# by a cpio, then bzip-ed, then wrt is used. Mayby it puts a cpio header
# back on the archive? The first archive is a disk label, the second is the
# files that didn't fit on boot1.
#  Anyway, the device /dev/dsk/f0 does not include the first track. The
# device /dev/dsk/f0t does include the first track. This function used to
# write to both. Now it creates an image of the disk, and does one write.
# This lets us cut floppies over the network.
#
function cut_flop2
{
	integer count
	(( BLOCKS == 2370 )) || [ "$LANG" = "ja" ] || return
	get_answer "SECOND boot" 2 || return
	set -- $(ls -l flop2.image)
	(( count = $5 / 512 ))
	. ${ROOT}/${MACH}/var/sadm/dist/rel_fullname
	echo "Creating SECOND boot floppy image..."
	#
	# Create one track's worth of zeros
	#
	rm -rf /tmp/zeros.$$ /tmp/cpio.$$ /tmp/both.$$ 
	rm -rf /tmp/track1.$$ /tmp/disk2.$$
	/usr/bin/dd if=/dev/zero of=/tmp/zeros.$$ bs=512 count=$TRKSIZE
	#
	# Create the cpio header
	#
	rm -f /tmp/flop.label
	if [ -f /tmp/flop.label ]
	then
		echo "Error: cannot remove /tmp/flop.label"
		exit 1
	fi
	echo  "${REL_FULLNAME} Boot Floppy 2" >/tmp/flop.label
	echo  "$BOOT2_FILES" >> /tmp/flop.label
	echo  "$LANG_FILES" >> /tmp/flop.label
	ls /tmp/flop.label | cpio -oc -O /tmp/cpio.$$ || exit $?
	chmod uog+w /tmp/flop.label
	#
	# Merge them
	#
	cat /tmp/cpio.$$ /tmp/zeros.$$ > /tmp/both.$$
	#
	# Extract one track's worth
	#
	/usr/bin/dd if=/tmp/both.$$ of=/tmp/track1.$$ bs=512 count=$TRKSIZE || exit $?
	#
	# Add on the rest of the disk
	#
	cat /tmp/track1.$$ flop2.image > /tmp/disk2.$$
	#
	# Dump to the disk
	#
	print Writing to SECOND boot floppy...
	/usr/bin/dd if=/tmp/disk2.$$ of=$FDRIVE bs=${TRKSIZE}b || exit $?
	print "Wrote $count blocks to SECOND boot floppy."
	print "Done with SECOND boot floppy.\007"
	rm -rf /tmp/zeros.$$ /tmp/cpio.$$ /tmp/both.$$ 
	rm -rf /tmp/track1.$$ /tmp/disk2.$$
}

function cut_magic
{
	get_answer "magic" 2 || return
	grep -v "^#" $PROTO/desktop/files/magic.proto |
		sed \
			-e "s,\$ROOT,$ROOT," \
			-e "s,\$MACH,$MACH," \
			-e "s,\$WORK,$WORK," \
			> $FLOPPROTO
	> $MAGICIMAGE
	/sbin/mkfs -Fs5 -b 512 $MAGICIMAGE $FLOPPROTO 2 $TRKSIZE || exit $?
	/usr/bin/dd if=$MAGICIMAGE of=$FDRIVE bs=${TRKSIZE}b || exit $?
	print "Done with magic floppy.\007"
}

function cut_lang_flop
{
        get_answer "LANGUAGE SUPPLEMENT" 1 || return
        rm -rf /tmp/etc
        rm -rf /tmp/image

        echo "Creating lang directory structure"
        mkdir -p /tmp/etc/dcu.d/locale
        mkdir -p /tmp/etc/inst/locale
        mkdir -p /tmp/image
        cd /tmp/etc/dcu.d/locale
        cp -r $ROOT/$MACH/etc/dcu.d/locale/* .
        cd /tmp/etc/inst/locale
        cp -r $PROTO/locale/* .

        cd /tmp

        for lang_hah_type in C fr de es fr it ja
        do
        dcu=etc/dcu.d/locale/$lang_hah_type
        menu=etc/inst/locale/$lang_hah_type/menus

        [ -d $dcu ] && ls >>/tmp/image/lang.2 2>>/tmp/image/lang.2 \
                $dcu/txtstrings  $dcu/config

        [ -f $menu/help/dcu.d/locale_hcf.z ] && {
                 cp  $menu/help/dcu.d/locale_hcf.z $dcu/help/locale_hcf.z
                ls >>/tmp/image/lang.2 2>>/tmp/image/lang.2 \
                $dcu/help/locale_hcf.z
       }

        [ -f $menu/help/locale_hcf.z ] &&
                ls >>/tmp/image/lang.2 2>>/tmp/image/lang.2 \
                $menu/help/locale_hcf.z

        [ -f $menu/help/nics.conf/config.z ] &&
                ls >>/tmp/image/lang.2 2>>/tmp/image/lang.2 \
                $menu/help/nics.conf/config.z

        [ -f $menu/help/nics.conf/nicshlp.z ] &&
                ls >>/tmp/image/lang.2 2>>/tmp/image/lang.2 \
                $menu/help/nicshlp.z

        [ -f $menu/cans.txtstrings ] && ls >>/tmp/image/lang.2 2>>/tmp/image/lang.2 \
                $menu/*.txtstrings

        done

        cat /tmp/image/lang.2 | cpio -ocud > /tmp/image/cpio.lang
        echo "Writing files to ${FDRIVE}"
        /usr/bin/dd if=/tmp/image/cpio.lang of=$FDRIVE bs=${TRKSIZE}b || exit $?

        return $?
        rm -rf /tmp/etc
        rm -rf /tmp/image
        print "Done with LANGUAGE SUPPLEMENT  floppy.\007"
}

function make_terminfo {
	TERMINFO_ORIG=$ROOT/$MACH/usr/share/lib/terminfo
	TERMINFO=$PROTO/stage
	export TERMINFO

	if [ ! -d $TERMINFO/A ]
	then
		mkdir -p $TERMINFO/A
	fi

	for tfile in ANSI AT386 AT386-M-ie AT386-M-mb AT386-ie AT386-mb
	do
		cp $TERMINFO_ORIG/A/$tfile $TERMINFO/A/$tfile
		suff=${tfile#*-}

		if [ "$tfile" = "ANSI" ]
		then
			newfile="ISL-ANSI"
		elif [ "$tfile" = "AT386" ]
		then
			newfile="ISL"
		else
			newfile="ISL-$suff"
		fi
		(
			echo "$newfile," ; \
			/usr/bin/infocmp $tfile | sed \
			-e "s/setab=[^,]*,/setab=\\\E[=%p1%dG,/g" \
			-e "s/setaf=[^,]*,/setaf=\\\E[=%p1%dF,/g" \
			-e "s/clear=\([^,]*\),/clear=\1\\\E[=0E\\\E[0m,/" \
			-e "s/colors\#[^,]*,/colors\#16,/"  \
			-e "s/pairs\#[^,]*,/pairs\#256,/" \
			| grep '^[ 	]'
		) > terminfo.src
		tic
	done
	export TERMINFO=/usr/share/lib/terminfo
}
	

#main()

make_terminfo

FLOPPROTO=/tmp/flopproto$$
MAGICIMAGE=/tmp/magic$$
trap "rm -f $FLOPPROTO $MAGICIMAGE; exit" 0 1 2 3 15
PATH=$PROTO/bin:$PATH export PATH

LOGO=logo.img
MKSMALLFS=$ROOT/$MACH/etc/.boot/mksmallfs

setflag=-u		#default is UnixWare set
LANG=C
unset special_flag
nflag=0
quick=false

while getopts Lqul:sn c
do
	case $c in
		u)
			# make UnixWare floppy (default)
			;;
		l)
			LANG=$OPTARG
			;;
		s)	
			special_flag=-s
			;;
		L)
                        LOGO=
                        ;;

		q)
			quick=true
			;;
		\?)
			print -u2 "Usage: $0 [-uL] [-l locale] [diskette1|diskette2]"
			print -u2 "\t-u cuts a boot floppy for UnixWare set."
			print -u2 "\t-l specifies the locale for the boot floppy."
	       	        print -u2 "\t-L leaves off the logo to save space."

			exit 1
			# The -s and -q options are intentionally not listed here.
			;;
		*)
			print -u2 Internal error during getopts.
			exit 2
			;;
	esac
done
(( VAL = OPTIND - 1 ))
shift $VAL

MEDIUM=$1
if [ "$PROTO" = "" ]
then
	if [ "$2" = "" ]
	then
		read "PROTO?PROTO is not set. Enter the path for PROTO: "
	else
		PROTO=$2
	fi
fi
if [ "$ROOT" = "" ]
then
	if [ "$3" = "" ]
	then
		read "ROOT?ROOT is not set. Enter the path for ROOT: "
	else
		ROOT=$3
	fi
fi
if [ "$MACH" = "" ]
then
	if [ "$4" = "" ]
	then
		read "MACH?MACH is not set. Enter the path for MACH: "
	else
		MACH=$4
	fi
fi
export LCL_MACH=.$MACH

cd $PROTO/stage
ask_drive
$quick || {
	select_boot

	# Construct the screen "Loading system modules" and the 
	# choose-language menu.  These are only used for BACK_END_MANUAL
	# cd-rom installations.
	>lang.items
	>lang.msgs
	>lang.footers
	>smartmsg1

	case $LANG in
	C|de|es|fr|it)	
		# Use lines for C and FIGS translations
		for i in C de es fr it
		do
			DIR=$PROTO/locale/$i
			[ -f $DIR/smartmsg1 ] && /usr/bin/cat $DIR/smartmsg1 >> smartmsg1 
			[ -f $DIR/lang.msgs -a -f $DIR/lang.footers ] && {
				echo `basename $DIR` >> lang.items
				cat $DIR/lang.msgs >> lang.msgs
				cat $DIR/lang.footers >> lang.footers
			}
		done
		;;
	*)
		# Default is message from only C locale. 
		/usr/bin/cat $PROTO/locale/C/smartmsg1 > smartmsg1
		;;
	esac	

	/usr/bin/egrep '(^PC850.*88591.*d$)|(^sjis.*eucJP)' \
		$ROOT/$MACH/var/opt/ls/iconv_data.ls > iconv_data

	#
	# We need the mfpd drvmap file on the boot floppy, but its
	# verify routine is NOT available to be run from the boot
	# floppy.  So the user doesn't get a "failed to verify"
	# message, we'll indicate NO verify routine during install.
	#

	ed -s $ROOT/.$MACH/etc/conf/drvmap.d/mfpd <<-EOF
	g/Y|Y/s//Y|N/
	w
	q
	EOF

	strip_comments
	cd $PROTO
	rm -f stage/desktop/menus/initrc
	for i in `cat $PROTO/desktop/menus/initrc.gen`
	do
		cat $i >> $PROTO/stage/desktop/menus/initrc
	done
	rm -f stage/desktop/scripts/tools
	for i in `cat $PROTO/desktop/scripts/tools.gen`
	do
		cat $i >> $PROTO/stage/desktop/scripts/tools
	done

	cd $PROTO/stage

	conframdfs $special_flag $setflag -l $LANG || exit $?
	make -f $PROTO/desktop/cut.flop.mk || exit $?
	prep_flop2
}
cut_flop1
cut_flop2
cut_magic
cut_lang_flop
