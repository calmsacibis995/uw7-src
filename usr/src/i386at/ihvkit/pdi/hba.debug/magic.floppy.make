#ident	"@(#)ihvkit:pdi/hba.debug/magic.floppy.make	1.1"
#!/usr/bin/ksh

# Note: if MODDIR changes, then the path of the mod.d source file in the
#	magicproto file must be changed to match
MODDIR=/tmp/mod.d

# Routine to check to make sure that that the loadable module for the driver
# name passed in, is available.  If not, then the driver needs to be configured
# as loadable, instead of static

check_load () {
	MODULE=$1
	SYSFIL=/tmp/System.${MODULE}$$
	SYSDIR=/tmp/sys.$$
	/bin/rm -rf $SYSFIL 2> /dev/null
	/bin/rm -rf $SYSDIR 2> /dev/null
	if [ -r "/etc/conf/mod.d/$MODULE" ]
	then
		/bin/cp /etc/conf/mod.d/$MODULE $MODDIR
		return 0;
	fi
	print -n "Creating a required loadable module...\n"
	/bin/cp /etc/conf/sdevice.d/$MODULE $SYSFIL
	/bin/mkdir $SYSDIR
	/bin/cat $SYSFIL | /bin/sed 's/^$static/#$static/g' > $SYSDIR/System
	ODIR=`pwd`
	cd $SYSDIR
	/etc/conf/bin/idinstall -u $MODULE
	/etc/conf/bin/idbuild -M $MODULE
	/bin/cp /etc/conf/mod.d/$MODULE $MODDIR
	/bin/rm /etc/conf/mod.d/$MODULE
	/bin/cp $SYSFIL ./System
	/etc/conf/bin/idinstall -u $MODULE
	cd $ODIR
	/bin/rm -rf $SYSDIR 2> /dev/null
	/bin/rm $SYSFIL 2> /dev/null
	if [ "`/bin/grep "^$static" /etc/conf/sdevice.d/$MODULE`" ]
	then
		/bin/rm -rf /tmp/module$$ 2> /dev/null
		/bin/cat /etc/conf/mod_register | /bin/grep -v ${MODULE}:${MODULE} > /tmp/module$$
		/bin/cp /tmp/module$$ /etc/conf/mod_register
		/bin/rm /tmp/module$$
	fi
}

# Begin by asking which device to create magic floppy on
MEDIUM=""
print -n "Select a floppy device to create the magic floppy on.\n"
[ -n "$MEDIUM" ] ||
read "MEDIUM?Please enter diskette1 or diskette2 (default is diskette1): "
[ -n "$MEDIUM" ] || MEDIUM="diskette1"
case "$MEDIUM" in
	diskette1)
		;;
	diskette2)
		;;
	*)
		print -u2 ERROR: Must specify diskette1 or diskette2.
		exit 1
		;;
esac
export BLOCKS=$(devattr $MEDIUM capacity)
case $BLOCKS in
2844) # 3.5-inch diskette
	TRKSIZE=36
	;;
*)
	print -u2 ERROR -- diskette must be 1.44MB 3.5 inch
	exit 2
	;;
esac

# Create or relocate the necessary loadable modules
/bin/rm -rf $MODDIR 2> /dev/null
/bin/mkdir $MODDIR
print -n "Finding/creating the required loadable modules...\n"
check_load kdb
check_load kdb_util

# Create the block device name to format/mkfs
FDRIVE=$(devattr $MEDIUM fmtcmd)
FDRIVE=${FDRIVE##* }

# Allow the selection to possibly format the floppy

while :
do
	print -n "\007\nInsert a blank writeable floppy into $MEDIUM drive and press\n "
	print -n "\t<ENTER> to write a previously formatted floppy,\n\tF\tto format "
	print -n "and write floppy,\n\t   or\n\tq\tto quit without writing\nEnter "
	print -n "selection: "
	read a
	case "$a" in
		"")
			break
			;;
		F)
			/usr/sbin/format -i2 $FDRIVE || exit $?
			break
			;;
		q)
			exit 0
			;;
		*)
			print -u2 ERROR: Invalid response -- try again.
			;;
	esac
done

# Now, dump the s5 file system and files out to the floppy

/sbin/mkfs -Fs5 -b 512 $FDRIVE ./magicproto 2 $TRKSIZE
if [ "$?" -ne "0" ]
then
	print -n "mkfs on floppy failed!\nCreation of magic floppy unsuccessful.\n"
	exit $?
fi
/bin/rm -rf $MODDIR
print -n "Creation of magic floppy was successful.\n"
