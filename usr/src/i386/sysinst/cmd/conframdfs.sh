#!/usr/bin/ksh

#ident  "@(#)conframdfs.sh	15.1	98/03/04"


PATH=$PROTO/bin:$TOOLS/usr/ccs/bin:$PATH: export PATH
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

LCL_MACH=.$MACH
BASE=$ROOT/$LCL_MACH
SOURCE_KERNEL=$BASE/stand/unix.nostrip
DEST_KERNEL=$BASE/stand/unix

# Begin main processing

for LANG in C de es fr
do

print "\nWorking in locale/$LANG/menus directory."
cd $PROTO
[ -d locale/$LANG/menus/help ] || {
	print -u2 ERROR -- locale/$LANG/menus/help directory does not exist.
	exit 2
}
(cd locale/$LANG/menus/help
 make -f help.mk all
 ls *.hcf | ksh ${PROTO}/desktop/buildscripts/cpioout > \
 locale_hcf.z || {
	print -u2 ERROR -- could not create locale_hcf.z.
	exit 2
    }
) 
(cd locale/$LANG/menus/help
 [ -d dcu.d ] || mkdir dcu.d
cp $ROOT/$MACH/etc/dcu.d/locale/$LANG/help/* dcu.d
cd dcu.d
 ls *.hcf | ksh ${PROTO}/desktop/buildscripts/cpioout > \
 locale_hcf.z || {
	print -u2 ERROR -- could not create dcu.d/locale_hcf.z.
	exit 2
    }
) 
#(cd locale/$LANG/menus; $ROOT/$MACH/usr/lib/winxksh/compile config.sh)

(
	PATH=$PROTO/bin:$PROTO/cmd:$PATH export PATH
	NICLOC="${ROOT}/${MACH}/etc/inst/locale/${LANG}/menus/nics"
	rm -rf $PROTO/locale/$LANG/menus/help/nics.d &&
	    mkdir -p $PROTO/locale/$LANG/menus/help/nics.d
	cp ${NICLOC}/help/*hcf \
	    $PROTO/locale/$LANG/menus/help/nics.d || {
		print -u2 ERROR -- copy failed from nics help directory.
		exit 2
	    }
	( cd ${NICLOC}/supported_nics/help/
	  cp *hcf $PROTO/locale/$LANG/menus/help/nics.d ) || {
		print -u2 ERROR -- copy failed from supported_nics directory.
		exit 2
	    }
	cd $PROTO/locale/$LANG/menus/help/nics.d
	find . -print | ksh ${PROTO}/desktop/buildscripts/cpioout > \
	    $PROTO/locale/$LANG/menus/help/nicshlp.z || {
		print -u2 ERROR -- could not create nicshlp.z.
		exit 2
	    }
) || exit $?

(
	PATH=$PROTO/bin:$PROTO/cmd:$PATH export PATH
	NICLOC="${ROOT}/${MACH}/etc/inst/locale/${LANG}/menus/nics"
	rm -rf $PROTO/locale/$LANG/menus/help/nics.conf/config &&
	    mkdir -p $PROTO/locale/$LANG/menus/help/nics.conf/config
	( cd ${NICLOC}/config/
	    cp * $PROTO/locale/$LANG/menus/help/nics.conf/config ) || {
		print -u2 ERROR -- copy failed from nics config directory.
		exit 2
	    }
	cd $PROTO/locale/$LANG/menus/help/nics.conf
	#
	# Remove the dc21* config file since it is not on the net install
	# floppies
	#
	rm -rf ./config/dc21*
	find config -print | ksh ${PROTO}/desktop/buildscripts/cpioout > \
	    $PROTO/locale/$LANG/menus/help/nics.conf/config.z || {
		print -u2 ERROR -- could not create config.z.
		exit 2
	    }
) || exit $?

done
[ ! -s $SOURCE_KERNEL ] && {
	print -u2 ERROR -- $SOURCE_KERNEL does not exist.
	exit 1
}
print "\nCopying $SOURCE_KERNEL into\n$DEST_KERNEL."
cp $SOURCE_KERNEL $DEST_KERNEL &

# loadmods not used; all mods now on HBA floppy.  (JTB, 4/7/97)
#
#if $special_flag
#then
#	sed -e '/:ide:/d' $BASE/stand/loadmods > $PROTO/stage/loadmods
#else
#	cp $BASE/stand/loadmods $PROTO/stage/loadmods
#fi

pick.set $setflag -l $LANG || exit $?

RAMPROTO="desktop/files/ramdfs.proto"
LCL_TEMP=/tmp/ramd$$ export LCL_TEMP
MEMFS_META="$LCL_TEMP/memfs.meta" export MEMFS_META
MEMFS_FS="$LCL_TEMP/memfs.fs" export MEMFS_FS

trap "rm -rf $LCL_TEMP; exit" 1 2 3 15
mkdir $LCL_TEMP

#
# Japanese requires two floppies, regardless of the floppy size
#
if (( BLOCKS == 2844 )) && [ "$LANG" != "ja" ]
then
	# uncomment lines for files on 3.5-inch floppy
	FLOP2="-e s,^#flop2,,"
else
	# leave lines commented out for 5.25-inch floppy
	FLOP2=""
fi
	
SMARTCMD='-e \,desktop/menus/smart,d'
$special_flag && [ -s $PROTO/desktop/menus/smart ] && SMARTCMD=""

# Compaq specific - lines in randfs.proto beginning with #mv2cd indicate
# files that will appear on the CD instead of on the boot floppy image
MV2CD="-e s,^#mv2cd,,"
$mv2cd_flag && MV2CD=""

#Uncomment all locale-specific lines, if any
#Delete all other comment lines
# Note: $MV2CD must be executed by sed before $FLOP2 is executed
sed \
	-e "s,^#$LANG,," \
	$MV2CD \
	$FLOP2 \
	-e '/^#/d' \
	$SMARTCMD \
	-e "s,\$ROOT,$ROOT," \
	-e "s,\$MACH,$MACH," \
	-e "s,\$WORK,$WORK," \
	-e "s,\$LANG,$LANG," \
	$RAMPROTO \
	> $LCL_TEMP/ramdproto

> $MEMFS_FS
> $MEMFS_META
print "\nMaking file system images.\n"
sbfmkfs $MEMFS_META $MEMFS_FS $LCL_TEMP/ramdproto 2>$LCL_TEMP/mkfs.log
if [ -s $LCL_TEMP/mkfs.log ]
then
	print -u2 "\nERROR -- mkfs of ramdisk filesystem failed."
	cat $LCL_TEMP/mkfs.log >&2
	print -u2 "\nErrors are logged in $LCL_TEMP/mkfs.log"
	rm -f $MEMFS_FS $MEMFS_META
	exit 1
fi
cp $MEMFS_FS $MEMFS_META $BASE/stand 

wait # wait for cp into $DEST_KERNEL to finish
print "\nStripping symbol table from $DEST_KERNEL."
$STRIP -x $DEST_KERNEL
print "\nEmptying .comment section of $DEST_KERNEL."
$MCS -d $DEST_KERNEL
rm -rf $LCL_TEMP
exit 0
