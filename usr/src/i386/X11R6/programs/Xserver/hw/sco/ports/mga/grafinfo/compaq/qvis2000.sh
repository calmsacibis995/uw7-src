:
#
# @(#) qvis2000.sh 11.1 97/10/22
#
# Copyright (C) 1995 The Santa Cruz Operation, Inc.
# Copyright (C) 1995 Compaq Computer Corp.
#
# The information in this file is provided for the exclusive use of the
# licensees of The Santa Cruz Operation, Inc.  Such users have the right
# to use, modify, and incorporate this code into other products for purposes
# authorized by the license agreement provided they include this notice
# and the associated copyright notice with any such product.  The
# information in this file is provided "AS IS" without warranty.
#
#

# Configure Compaq QVision 2000 PCI memory address

PATH=/bin

if [ -d /usr/X11R6.1 ]
then
        GRAFDIR=/usr/X11R6.1/lib/grafinfo/compaq
        PCIINFO=/usr/X11R6.1/lib/vidconf/scripts/pciinfo
else
        GRAFDIR=/usr/lib/grafinfo/compaq
        PCIINFO=/usr/lib/vidconf/scripts/pciinfo
fi

create_xgi()
{
sed -e "s/@MEM_BASE@/$base/g" < $GRAFDIR/qvis2000.tmpl > $GRAFDIR/qvis2000.xgi
exit 0
}

base=`$PCIINFO -d 0x518 -v 0x102B -D 0x10`  # MGA Atlas
if [ $? -eq 0 ]; then
  create_xgi
fi

base=`$PCIINFO -d 0xD10 -v 0x102B -D 0x10`  # MGA Athena
if [ $? -eq 0 ]; then
  create_xgi
fi

echo "Cannot find Compaq QVision 2000 in computer!"
exit 1
