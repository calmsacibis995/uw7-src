#!/usr/bin/ksh
#ident	"@(#)drf:cmd/cpioout.sh	1.1"

FILE=/tmp/cpioout.$$
cpio -oLD -H crc > ${FILE}
bzip -s32k ${FILE} > ${FILE}.z
wrt -s ${FILE}.z
rm ${FILE} ${FILE}.z
