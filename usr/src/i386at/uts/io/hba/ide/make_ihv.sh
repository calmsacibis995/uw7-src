#!/sbin/sh

#ident	"@(#)kern-i386at:io/hba/ide/make_ihv.sh	1.1"

#------------------------------------------------------------------------------
# Version 3.0 9/15/95 miked
# This shell script will produce an IHVHBA diskette for an HBA driver.
# It must be run from the root source tree (this current directory) in order
# for it to function, as it will run make to build the driver as well as
# run idbuild to build the loadable module.  It will make changes to the
# current sdevice.d file as it has to in order for the idbuild to work
#------------------------------------------------------------------------------
# the name of the driver
DRIVER="ide"
# the name of the driver the system will use for messages
DSNAME="Generic IDE/ATAPI/ESDI IHV HBA "
# the long name used by the kernel init messages
DLNAME="Generic IDE/ATAPI/ESDI HBA"
# the copyright
COPYRIGHT="Copyright (c) 1995 Novell"
# the vendor
VENDOR="Novell"
# pertinent things about the floppy
FD_TYPE=`devattr diskette1 bdevice`
DESC=`devattr diskette1 desc`
BDEV=`devattr diskette1 bdevice`
VOLUME=`devattr diskette1 volume`
MACH=i386at
PACKAGE=ide
CWD=`pwd`
#------------------------------------------------------------------------------
# this ends the variables you need to alter, if any.  Do not alter anything
# after this line

echo "DO NOT INTERRUPT FOR ANY REASON!"

#------------------------------------------------------------------------------

# build the driver, even if it isn't needed
echo
echo "Invoking make to build the driver"

#rm -rf /tmp/ide
mkdir -p /tmp/ide/${MACH}/etc/conf/pack.d/$DRIVER
mkdir -p /tmp/ide/${MACH}/etc/conf/cf.d
mkdir -p /tmp/ide/${MACH}/etc/conf/ftab.d
mkdir -p /tmp/ide/${MACH}/etc/conf/mtune.d
touch /tmp/ide/${MACH}/etc/conf/mtune.d/ide
make -f ide.mk IDINSTALL=/etc/conf/bin/idinstall DDEBUG= STATIC=static ROOT=/tmp/ide install
ed /tmp/ide/${MACH}/etc/conf/sdevice.d/$DRIVER <<EOF
1,\$s/^$DRIVER	N/$DRIVER	Y/
w
q
EOF

ROOT=/tmp/ide/${MACH} MACH= /etc/conf/bin/idbuild -M $DRIVER
mv /tmp/ide/${MACH}/etc/conf/mod.d/$DRIVER ./ide.cf
rm -rf /tmp/ide
mkdir /tmp/ide

# check to see of the floppy is already mounted
if mount | grep $BDEV >/dev/null
then
	echo
	echo "$DESC is already mounted!"
	echo "Please un-mount the floppy and restart the program"
	exit
fi

# check to see if the mnt directory is already mounted
if mount | grep "^/mnt " >/dev/null
then
	echo
	echo "The /mnt directory already has something mounted on it!"
	DMOUNT=`mount | grep "/mnt " | cut -f3 -d' '`
	echo "It looks like $DMOUNT is mounted."
	echo "Please un-mount $DMOUNT and restart the program."
	exit
fi

# mkfs floppy
#echo
#echo "Insert a floppy into $DESC and"
#echo -n "\tPRESS <ENTER> WHEN READY"
#read a
echo "Formatting the $VOLUME in $DESC"
eval `devattr diskette1 fmtcmd`
echo "Creating a filesystem on $DESC"
eval `devattr diskette1 mkfscmd`

# mount the floppy
echo "Mounting the floppy..."

if mount -F s5 $BDEV /mnt
then
	echo "$DESC mounted"
else
	echo "Floppy $FD_TYPE mount failed!"
	echo "Please restart the program"
	exit
fi

echo "Creating the needed directories..."
# make the base directories
rmdir /mnt/lost+found
mkdir -p /mnt/etc/conf/mod.d

# In the etc directory are the files load.name and loadmods
# the load.name file contains the name of the driver
echo -n "Creating etc/load.name "
echo "$DSNAME" >/mnt/etc/load.name
# the loadmods file contains information for loading the driver
echo "etc/loadmods"
cp ./ide.pkg/loadmods /mnt/etc/loadmods
cp ./ide.cf/$DRIVER /mnt/etc/conf/mod.d/$DRIVER

# unmount the floppy
echo "Unmount the floppy"
umount /mnt

echo "copying files to temp directory"
cp ./ide.cf/* /tmp/ide
cp ./ide.pkg/loadmods /tmp/ide
cp ./ide.pkg/postinstall.sh ./ide.pkg/postinstall
cp ./ide.h ./ide_ha.h ata/ata_ha.h mcesdi/mc_esdi.h /tmp/ide

echo "making the package on $DESC"
cd ./ide.pkg
pkgmk -o -d diskette1 -r / ide 2>/dev/null <<EOF

EOF

echo "The IHVHBA disk is now complete."
