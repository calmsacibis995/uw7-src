:
#
#	@(#) vision330.sh 11.3 97/11/26 
#
# Copyright (C) 1996 The Santa Cruz Operation, Inc.
#
# The information in this file is provided for the exclusive use of the
# licensees of The Santa Cruz Operation, Inc.  Such users have the right 
# to use, modify, and incorporate this code into other products for 
# purposes authorized by the license agreement provided they include
# this notice and the associated copyright notice with any such 
# product.  The information in this file is provided "AS IS" without 
# warranty.
#

ADAPTOR_NAME="#9 FX Vision 330"
VENDOR_ID=0x5333
DEVICE_ID=0x8811
INSTANCE=0
TEMPLATE=/usr/X11R6.1/lib/grafinfo/numbernine/vision330.tmpl
GRAFINFO=/usr/X11R6.1/lib/grafinfo/numbernine/vision330.xgi
PATH=/usr/X11R6.1/lib/vidconf/scripts:$PATH

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

press_key() {
	echo "\nPress [Enter] to continue.\c">&2
	read x
}

xtod() {
	x=`echo $1 | sed -e 's/0[xX]//'`
(cat << EOF
	ibase=16
	$x
EOF
) | bc
}


#
# main()
#
set_trap

pciinfo -q -d $DEVICE_ID -v $VENDOR_ID 2> /dev/null

if [ $? -eq "0" ]
then
        HI=`pciinfo -d $DEVICE_ID -v $VENDOR_ID -B 0x13 |
                sed -e s/0[xX]//`
        LO=`pciinfo -d $DEVICE_ID -v $VENDOR_ID -B 0x12 |
                sed -e s/0[xX]//`
        BASE=`pciinfo -d $DEVICE_ID -v $VENDOR_ID -W 0x12`
else
	# no vision330 detected
	cleanup 1
fi

if expr `xtod ${BASE}` = 0 > /dev/null 2>&1
then
        echo "Unable to configure $ADAPTOR_NAME"
        echo "Invalid base memory address: $BASE"
        cleanup 1
else
        sedarg1="s/@MEM_BASE@/${BASE}/g"
        sedarg2="s/@HI@/0x${HI}/g"
        sedarg3="s/@LO@/0x${LO}/g"

        sed -e $sedarg1 -e $sedarg2 -e $sedarg3 < $TEMPLATE > $GRAFINFO

        cleanup 0
fi
