#!/bin/sh
#	@(#) 86c765.sh 11.1 97/10/22 
set_trap()  {
	trap 'echo "Interrupted! Giving up..."; cleanup 1' 1 2 3 15
}

unset_trap()  {
	trap '' 1 2 3 15
}
 
cleanup() {
	trap '' 1 2 3 15
	exit $1
}

set_trap

if [ -d /usr/X11R6.1 ]
then
        GRAF_DIR="/usr/X11R6.1/lib/grafinfo/"
        SCRIPTS_DIR="/usr/X11R6.1/lib/vidconf/scripts/"
else
        GRAF_DIR="/usr/lib/grafinfo/"
        SCRIPTS_DIR="/usr/lib/vidconf/scripts/"
fi

DEVICE_ID=0x8811
VENDOR_ID=0x5333
${SCRIPTS_DIR}pciinfo -q -d $DEVICE_ID -v $VENDOR_ID > /dev/null 2>&1
if [ $? -ne 0 ]
then
	echo "Cannot find a S3 Trio64V+ (86c765) PCI card in this computer!"
	cleanup 1
fi
base=`${SCRIPTS_DIR}pciinfo -d $DEVICE_ID -v $VENDOR_ID -W 0x12`
HI=`${SCRIPTS_DIR}pciinfo -d $DEVICE_ID -v $VENDOR_ID -B 0x13 \
	| sed -e s/0[xX]//`
LO=`${SCRIPTS_DIR}pciinfo -d $DEVICE_ID -v $VENDOR_ID -B 0x12 \
	| sed -e s/0[xX]//`

sedarg1="s/@MEMBASE@/${base}0000/g"
sedarg2="s/@HI@/${HI}/g"
sedarg3="s/@LO@/${LO}/g"

sed -e $sedarg1 -e $sedarg2 -e $sedarg3 \
	< ${GRAF_DIR}s3/86c765.tmpl \
	> ${GRAF_DIR}s3/86c765.xgi
cleanup 0
