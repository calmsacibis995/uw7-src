#!/usr/bin/ksh

#ident	"@(#)drf:cmd/conframdfs.sh	1.1"

# set -x

PATH=$PROTO/bin:$TOOLS/usr/ccs/bin:$PATH: export PATH
PFX=""
EDSYM="bin/edsym"
NM="${PFX}nm"
DUMP="${PFX}dump"
STRIP="${PFX}strip"
UNIXSYMS="${PFX}unixsyms"
MCS="${PFX}mcs"

setflag=-u		#default is UnixWare set
LANG=C
special_flag=false
mv2cd_flag=false
while getopts ul:s c
do
	case $c in
		u)
			# make UnixWare floppy
			setflag=-u
			;;
		l)
			LANG=$OPTARG
			;;
		s)
			special_flag=true
			mv2cd_flag=true
			;;
		\?)
			print -u2 "Usage: $0 [-u] [-l locale]"
			# The -s option is intentionally not listed here.
			exit 1
			;;
		*)
			print -u2 Internal error during getopts.
			exit 2
			;;
	esac
done

BASE=$ROOT/$LCL_MACH
SOURCE_KERNEL=$BASE/stand/unix.nostrip
DEST_KERNEL=$BASE/stand/unix

# Begin main processing
cd $PROTO
[ -d locale/$LANG/menus/help ] || mkdir -p locale/$LANG/menus/help
cp /usr/lib/drf/locale/$LANG/locale_hcf.z locale/$LANG/menus/help
(cd locale/$LANG/menus/help
 [ -d dcu.d ] || mkdir dcu.d
 cp /etc/dcu.d/locale/$LANG/help/* dcu.d
 cd dcu.d
 ls *.hcf | ksh /usr/lib/drf/cpioout > \
 locale_hcf.z || {
	print -u2 ERROR -- could not create dcu.d/locale_hcf.z.
	exit 2
    }
) 

[ ! -s $SOURCE_KERNEL ] && {
	print -u2 ERROR -- $SOURCE_KERNEL does not exist.
	exit 1
}
print "\nCopying $SOURCE_KERNEL into\n$DEST_KERNEL."
cp $SOURCE_KERNEL $DEST_KERNEL

cp $BASE/stand/loadmods $PROTO/stage/loadmods

# Create the bootmsg file
echo "BOOTMSG1=Booting from the Emergency Recovery Floppy ..." > $PROTO/bootmsgs
echo "BOOTMSG2=Bootstrap Command Processor\\" >> $PROTO/bootmsgs
echo "Ready for boot commands... [? for help]\\" >> $PROTO/bootmsgs
echo "" >> $PROTO/bootmsgs
echo "TITLE=SCO UnixWare, based on SCO UNIX System V Release 5" >> $PROTO/bootmsgs
echo "COPYRIGHT=Copyright 1997 SCO, Inc.  All Rights Reserved.\\" >> $PROTO/bootmsgs
echo "U.S. Pat. No. 5,349,642" >> $PROTO/bootmsgs
echo "AUTOMSG=Automatic Boot Procedure" >> $PROTO/bootmsgs
echo "REBOOTMSG=Press any key to reboot..." >> $PROTO/bootmsgs
echo "STARTUPMSG=The system is coming up.  Please wait." >> $PROTO/bootmsgs

# Create the boot file
echo "ROOTFS=memfs" > $PROTO/boot
echo "FILES=resmgr,memfs.meta,memfs.fs,kdb.rc" >> $PROTO/boot
# echo "INITSTATE=1" >> $PROTO/boot
grep -i "^DISABLE_CACHE=" /stand/boot >> $PROTO/boot
grep -i "^DISABLE_PGE=" /stand/boot >> $PROTO/boot
grep -i "^ENABLE_4GB_MEM=" /stand/boot >> $PROTO/boot
grep -i "^IGNORE_MACHINE_CHECK=" /stand/boot >> $PROTO/boot
grep -i "^LUNSEARCH=" /stand/boot >> $PROTO/boot
grep -i "^MEMADJUST=" /stand/boot >> $PROTO/boot
grep -i "^PCISCAN=" /stand/boot >> $PROTO/boot
grep -i "^PDI_TIMEOUT=" /stand/boot >> $PROTO/boot
grep -i "^PMAPLIMIT=" /stand/boot >> $PROTO/boot

# Copy boot help file
cp /stand/help.txt $PROTO

# Create the passwd/group files, with all ids less than 100
awk -F: ' NF > 0 { if ( $3 < 100 )
		print $0 } ' /etc/passwd > $PROTO/passwd
awk -F: ' NF > 0 { if ( $3 < 100 )
		print $0 } ' /etc/group > $PROTO/group

# Create chan.ap.flop
sed 's/ttcompat//g' /etc/ap/chan.ap > $PROTO/chan.ap.flop

#RAMPROTO="$ROOT/.$LCL_MACH/ramdfs.proto"
LCL_TEMP=$PROTO/ramd$$ export LCL_TEMP
MEMFS_META="$LCL_TEMP/memfs.meta" export MEMFS_META
MEMFS_FS="$LCL_TEMP/memfs.fs" export MEMFS_FS

trap "[ ! -f $LCL_TEMP/mkfs.log ] && { rm -rf $LCL_TEMP}; exit 1" 1 2 3 15
mkdir -p $LCL_TEMP

# Uncomment all locale-specific lines, if any
# Delete all other comment lines
sed \
	-e '/^#/d' \
	-e "s,\$ROOT,$ROOT," \
	-e "s,\$MACH,$MACH," \
	-e "s,\$WORK,$WORK," \
	-e "s,\$LANG,$LANG," \
	-e "s,\$PROTO,$PROTO," \
	$RAMPROTOF \
	> $LCL_TEMP/ramdproto

> $MEMFS_FS
> $MEMFS_META

cp $LCL_TEMP/ramdproto /tmp/ramdproto.file

	#setup for NWS (if installed)
	>$PROTO/nws_sys_path
	$NWS_CONFIG && echo $NWS_SYS_PATH >$PROTO/nws_sys_path

print "\nMaking file system images.\n"
ls /dev/dsk/c*b*t*d*s* /dev/rdsk/c*b*t*d*s* 2>/dev/null | cpio -oc > $PROTO/devs.cpio
/usr/lib/drf/sbfmkfs $MEMFS_META $MEMFS_FS $LCL_TEMP/ramdproto 2>$LCL_TEMP/mkfs.log
if [ -s $LCL_TEMP/mkfs.log ]
then
	print -u2 "\nERROR -- mkfs of ramdisk filesystem failed."
	cat $LCL_TEMP/mkfs.log >&2
	print -u2 "\nErrors are logged in $LCL_TEMP/mkfs.log"
	rm -f $MEMFS_FS $MEMFS_META
	exit 1
fi
cp $MEMFS_FS $MEMFS_META $PROTO/$LCL_MACH/stand

# If this isn't a KDB Bootable DRF floppy, strip the symbol table
# if [ -z "$DRF_KDB" ]
# then
#	print "\nStripping symbol table from $DEST_KERNEL."
#	$STRIP $DEST_KERNEL
#fi
print "\nEmptying .comment section of $DEST_KERNEL."
$MCS -d $DEST_KERNEL
rm -rf $LCL_TEMP
exit 0
