#!/bin/sh

# $XFree86: xc/programs/Xserver/hw/xfree86/accel/et4000w32/confw32.sh,v 3.1 1995/01/28 15:49:52 dawes Exp $
#
# This script generates w32Conf.c
#
# usage: confw32.sh driver1 driver2 ...
#
# $XConsortium: confw32.sh /main/3 1995/11/12 16:20:44 kaleb $

VGACONF=./w32Conf.c

cat > $VGACONF <<EOF
/*
 * This file is generated automatically -- DO NOT EDIT
 */

#include "xf86.h"
#include "vga.h"

extern vgaVideoChipRec
EOF
Args="`echo $* | tr '[a-z]' '[A-Z]'`"
set - $Args
while [ $# -gt 1 ]; do
  echo "        $1," >> $VGACONF
  shift
done
echo "        $1;" >> $VGACONF
cat >> $VGACONF <<EOF

vgaVideoChipPtr W32Drivers[] =
{
EOF
for i in $Args; do
  echo "        &$i," >> $VGACONF
done
echo "        NULL" >> $VGACONF
echo "};" >> $VGACONF
