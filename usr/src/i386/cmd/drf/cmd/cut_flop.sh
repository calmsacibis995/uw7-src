#!/usr/bin/ksh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997 Santa Cruz Operation, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)drf:cmd/cut_flop.sh	1.2"

# script to create boot floppy.

function ask_drive
{
	export BLOCKS=$(devattr $MEDIUM capacity)
	case $BLOCKS in
	2844) # 3.5-inch diskette
		# We're cutting 2 floppies, so step down the blocks
		BLOCKS=2370
		TRKSIZE=36
		if [ "${LANGS}" = "ja" ]
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
	rmwhite /usr/lib/winxksh/scr_init $PROTO/stage/winxksh/scr_init

	rmwhite /usr/lib/winxksh/winrc $PROTO/stage/winxksh/winrc

	stripem /etc/dcu.d/menus   $PROTO/stage/dcu.d/menus
	stripem /etc/dcu.d/scripts $PROTO/stage/dcu.d/scripts
}

function prep_flop2
{
	(( BLOCKS == 2370 )) || [ "$LANGS" = "ja" ] || return
	print "Making tree for SECOND boot floppy."
	flop2root=$PROTO/stage
	set -e
	mkdir flop2root 2>/dev/null
	cd flop2root
	mkdir -p \
		../tmp \
                tmp \
                isl/ui_modules \
                isl/install_modules \
		etc/conf/mod.d \
		etc/dcu.d/dculib \
		etc/dcu.d/locale/${LANG} \
		etc/dcu.d/menus \
		etc/dcu.d/scripts \
		etc/inst/scripts \
		etc/inst/locale/${LANG}/menus/ \
		etc/inst/locale/${LANG}/menus/help \
		etc/scsi \
		sbin \
		usr/bin \
		usr/sbin \
		usr/lib \
		.extra.d/sbin \
		.extra.d/usr/bin \
		.extra.d/etc/conf/hbamod.d \
		.extra.d/etc/conf/mod.d \
		.extra.d/etc/conf/bin
	sed \
		-e '/^#/d' \
		-e "s,\$ROOT,$ROOT," \
		-e "s,\$MACH,$MACH," \
		-e "s,\$WORK,$WORK," \
		-e "s,\$LANG,$LANG," \
		-e "s,\$PROTO,$PROTO," \
		/usr/lib/drf/disk2.files \
		> $PROTO/disk2.files

	while read srcfile destfile
	do
		cp $srcfile $destfile
	done < $PROTO/disk2.files

	# If the below not done, vi will fail
	ln usr/lib/libcrypt.so usr/lib/libcrypt.so.1

	for i in $FSMODS
	do
		[ -d .extra.d/etc/fs/$i -o "$i" = "dow" ] || mkdir -p .extra.d/etc/fs/$i
		[ -d .extra.d/etc/fs/$i ] && cp $PROTO/etc/fs/$i/* .extra.d/etc/fs/$i
	done 

	set +e
	LIST=$(find . -type f -print)
	print $LIST | xargs chmod 555 
	print $LIST | xargs chown root:sys
	typeset OIFS=$IFS
	IFS=
	print -r $LIST | cpio -oLDV -H crc > $PROTO/out1.$$
	IFS=$OIFS
	cd ..
	print Compressing image for SECOND boot floppy.
	bzip -s32k $PROTO/out1.$$ > $PROTO/out2.$$
	wrt -s $PROTO/out2.$$ > flop2.image
	rm $PROTO/out?.$$
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

function image_make1
{
	set -e
	COMPRESS=/usr/lib/drf/bzip
	${COMPRESS} -s28k $PROTO/$LCL_MACH/stand/unix > unix
	${COMPRESS} -s28k $PROTO/$LCL_MACH/stand/memfs.meta > memfs.meta
	${COMPRESS} -s28k $PROTO/$LCL_MACH/stand/memfs.fs > memfs.fs
	cp /stand/resmgr resmgr
}

function cut_flop1
{
	image_make1
	# Create kdb.rc.  By default, just exit, so boot is uninterrupted.
	echo "q" > kdb.rc

	echo "Creating/writing FIRST boot floppy image..."
	cmd="$MKSMALLFS -Bfdboot -x2 -Gstage2.fdinst \
		dcmp.blm stage3.blm platform.blm \
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
	(( BLOCKS == 2370 )) || [ "$LANGS" = "ja" ] || return
	get_answer "SECOND boot" 2 || return
	set -- $(ls -l flop2.image)
	(( count = $5 / 512 ))
	echo "Creating SECOND boot floppy image..."
	#
	# Create one track's worth of zeros
	#
	rm -rf $PROTO/tmp/zeros.$$ $PROTO/tmp/cpio.$$ $PROTO/tmp/both.$$ 
	rm -rf $PROTO/tmp/track1.$$ $PROTO/tmp/disk2.$$
	/usr/bin/dd if=/dev/zero of=$PROTO/tmp/zeros.$$ bs=512 count=$TRKSIZE
	#
	# Create the cpio header
	#
	rm -f $PROTO/tmp/flop.label
	if [ -f $PROTO/tmp/flop.label ]
	then
		echo "Error: cannot remove $PROTO/tmp/flop.label"
		exit 1
	fi
	echo  "${REL_FULLNAME} Boot Floppy 2" >$PROTO/tmp/flop.label
	cdir=`pwd`
	cd $PROTO
	ls tmp/flop.label | cpio -oc -O $PROTO/tmp/cpio.$$ || exit $?
	cd $cdir
	unset cdir
	chmod uog+w $PROTO/tmp/flop.label
	#
	# Merge them
	#
	cat $PROTO/tmp/cpio.$$ $PROTO/tmp/zeros.$$ > $PROTO/tmp/both.$$
	#
	# Extract one track's worth
	#
	/usr/bin/dd if=$PROTO/tmp/both.$$ of=$PROTO/tmp/track1.$$ bs=512 count=$TRKSIZE || exit $?
	#
	# Add on the rest of the disk
	#
	cat $PROTO/tmp/track1.$$ flop2.image > $PROTO/tmp/disk2.$$
	#
	# Dump to the disk
	#
	print Writing to SECOND boot floppy...
	/usr/bin/dd if=$PROTO/tmp/disk2.$$ of=$FDRIVE bs=${TRKSIZE}b || exit $?
	print "Wrote $count blocks to SECOND boot floppy."
	print "Done with SECOND boot floppy.\007"
	rm -rf $PROTO/tmp/zeros.$$ $PROTO/tmp/cpio.$$ $PROTO/tmp/both.$$ 
	rm -rf $PROTO/tmp/track1.$$ $PROTO/tmp/disk2.$$
}

#main()

trap "exit" 0 1 2 3 15
PATH=:/usr/lib/drf:$PATH export PATH
REL_FULLNAME="UnixWare7"

LOGO=logo.img
#LOGO=""
MKSMALLFS=/usr/lib/drf/mksmallfs

setflag=-u		#default is UnixWare set
unset special_flag
nflag=0
quick=true

cd $PROTO
ask_drive

if [ "$LANGS" != "ja" ]
then
	/usr/bin/egrep '^PC850.*88591.*d' \
		/usr/lib/iconv/iconv_data > $PROTO/iconv_data
else
	/usr/bin/egrep '^sjis.*eucJP' \
		/usr/lib/iconv/iconv_data > $PROTO/iconv_data
fi

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

MEMSZ=`/sbin/memsize`
sed -e "s%LANG=XXX%LANG=$LANG%" \
    -e "s%LANGS=XXX%LANGS=$LANGS%" \
    -e "s%LC_CTYPE=XXX%LC_CTYPE=$LANG%" \
    -e "s%KEYBOARD=XXX%KEYBOARD=$KEYBOARD%" \
    -e "s%KEYBOARDS=XXX%KEYBOARDS=$KEYBOARDS%" \
    -e "s%VM_RUNNING=XXX%VM_RUNNING=$VM_RUNNING%" \
    -e "s%MEMSZ=XXX%MEMSZ=$MEMSZ%" \
    -e "s%DRF_DEBUG=XXX%DRF_DEBUG=$DRF_DEBUG%" \
    -e "s%REL_FULLNAME=XXX%REL_FULLNAME=$REL_FULLNAME%" \
	/usr/lib/drf/drf_inst.gen > $PROTO/drf_inst

conframdfs $special_flag $setflag -l $LANG || exit $?
prep_flop2
cd $PROTO
cut_flop1
cut_flop2
