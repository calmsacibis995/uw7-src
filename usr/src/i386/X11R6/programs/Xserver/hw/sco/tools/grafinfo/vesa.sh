#!/bin/sh
#
#	@(#) vesa.sh 11.1 97/10/29 
#
# Copyright (C) 1997 The Santa Cruz Operation, Inc.
#
# The information in this file is provided for the exclusive use of the
# licensees of The Santa Cruz Operation, Inc.  Such users have the right 
# to use, modify, and incorporate this code into other products for purposes 
# authorized by the license agreement provided they include this notice 
# and the associated copyright notice with any such product.  The 
# information in this file is provided "AS IS" without warranty.
#
# Generate new vesa/svga.xgi when Video Configuration Manager starts up.
#

LIBDIR=/usr/X11R6.1/lib
GRAFDIR=${LIBDIR}/grafinfo
GRAFINFO=vesa/svga.xgi
AOFDIR=${LIBDIR}/vidconf/AOF
AOFBIN=${AOFDIR}/bin

rm -f ${GRAFDIR}/vesa/svga.xgi
[ -d ${GRAFDIR}/vesa ] || mkdir ${GRAFDIR}/vesa

[ -x $AOFBIN/grafmkxgi ] && \
  ${AOFBIN}/grafmkxgi -v VESA -m SVGA > ${GRAFDIR}/${GRAFINFO} 2> /dev/null

if [ $? != 0 ]
then
	rm -f ${GRAFDIR}/vesa/svga.xgi
	exit 1
fi
exit 0

