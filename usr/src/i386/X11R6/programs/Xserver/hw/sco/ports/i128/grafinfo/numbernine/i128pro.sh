:
#
# @(#) i128pro.sh 11.1 97/10/22
#
# Copyright (C) 1992-1996 The Santa Cruz Operation, Inc.
#
# The information in this file is provided for the exclusive use of the
# licensees of The Santa Cruz Operation, Inc.  Such users have the right 
# to use, modify, and incorporate this code into other products for 
# purposes authorized by the license agreement provided they include
# this notice and the associated copyright notice with any such 
# product.  The information in this file is provided "AS IS" without 
# warranty.
#

ADAPTOR_NAME="#9 Imagine 128 Professional"
VENDOR_ID=0x105D        # Number Nine
DEVICE_ID=0x2309        # I-128
INSTANCE=0

if [ -d /usr/X11R6.1 ]
then
        TEMPLATE=/usr/X11R6.1/lib/grafinfo/numbernine/i128pro.tmpl
        GRAFINFO=/usr/X11R6.1/lib/grafinfo/numbernine/i128pro.xgi
        PATH=/usr/X11R6.1/lib/vidconf/scripts:$PATH
else
        TEMPLATE=/usr/lib/grafinfo/numbernine/i128pro.tmpl
        GRAFINFO=/usr/lib/grafinfo/numbernine/i128pro.xgi
        PATH=/usr/lib/vidconf/scripts:$PATH
fi

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

pciinfo -q -d $DEVICE_ID -v $VENDOR_ID -n $INSTANCE 2> /dev/null

if [ $? != "0" ]
then
        echo "Cannot find $ADAPTOR_NAME!"
        cleanup 1
fi

RBASE=`pciinfo -d $DEVICE_ID -v $VENDOR_ID -n $INSTANCE -W 0x22`0000
MW0=`pciinfo -d $DEVICE_ID -v $VENDOR_ID -n $INSTANCE -W 0x12`0000
MW1=`pciinfo -d $DEVICE_ID -v $VENDOR_ID -n $INSTANCE -W 0x16`0000
XY0=`pciinfo -d $DEVICE_ID -v $VENDOR_ID -n $INSTANCE -W 0x1A`0000
IO=`pciinfo -d $DEVICE_ID -v $VENDOR_ID -n $INSTANCE -B 0x25`00

if expr `xtod $MW0` = 0 > /dev/null 2>&1
then
        echo "Unable to configure $ADAPTOR_NAME"
        echo "Invalid base memory address: $MW0"
        cleanup 1
else
        sedarg1="s/@MW0_BASE@/$MW0/g"
        sedarg2="s/@MW1_BASE@/$MW1/g"
        sedarg3="s/@XY0_BASE@/$XY0/g"
        sedarg4="s/@RBASE_X_BASE@/$RBASE/g"
        sedarg5="s/@IO_BASE@/$IO/g"

        sed -e $sedarg1 -e $sedarg2 -e $sedarg3 \
            -e $sedarg4 -e $sedarg5 < $TEMPLATE > $GRAFINFO

        cleanup 0
fi
