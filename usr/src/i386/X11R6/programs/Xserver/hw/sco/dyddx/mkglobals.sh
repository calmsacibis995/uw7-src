:
#	@(#)mkglobals.sh	6.2	12/17/95	22:32:12
#
#	Copyright (C) The Santa Cruz Operation, 1992-1993.
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

#
# Directory order doesn't matter anymore
#
DIRLIST="../../lib/font cfb cfb16 cfb32 dix \
hw/sco/dyddx hw/sco/grafinfo hw/sco/mit hw/sco/mit/libs hw/sco/nfb \
hw/sco/ports/gen mfb mi os"
EXTRAFILE="names.extra"
REMOVEFILE="names.remove"

ADDLIST="$EXTRAFILE"
for i in $DIRLIST
do
	ADDLIST="$ADDLIST $i/names.`basename $i`"
done

DELLIST="$REMOVEFILE"

#	Move to ./Xserver
#
cd ../../..
rm -f g_addnam g_delnam g_names

sed -n '/XCOMM/!{;s/[ 	].*$//;p;}' $ADDLIST | sort | uniq > g_addnam
sed -n '/XCOMM/!{;s/[ 	].*$//;p;}' $DELLIST | sort | uniq > g_delnam
comm -23 g_addnam g_delnam > g_names

grep -v "_Insight_" g_names > g_names.0
mv -f g_names.0 g_names

rm -f hw/sco/dyddx/globals.c
{
echo '#include "dyddx.h"'
sed 's/.*/DYNA_EXTERN(&);/' g_names
echo 'symbolDef globalSymbols[] = {'
sed 's/.*/{"&", &},/' g_names
echo '{0, 0}};'
echo 'int globalSymbolCount =' `wc -l < g_names` ';'
} > hw/sco/dyddx/globals.c

rm -f g_addnam g_delnam g_names
