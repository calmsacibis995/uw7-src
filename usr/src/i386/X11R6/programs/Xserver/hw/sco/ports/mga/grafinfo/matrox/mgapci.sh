:
#
# @(#) mgapci.sh 11.1 97/10/22
#
# Copyright (C) 1994-1995 The Santa Cruz Operation, Inc.
#
# The information in this file is provided for the exclusive use of the
# licensees of The Santa Cruz Operation, Inc.  Such users have the right
# to use, modify, and incorporate this code into other products for purposes
# authorized by the license agreement provided they include this notice
# and the associated copyright notice with any such product.  The
# information in this file is provided "AS IS" without warranty.
#
#

ADAPTOR_NAME="Matrox MGA"
VENDOR_ID=0x102B
DEVICE_ID=0x0518
PREFIX=mgapci_
MODEL=MGAPCI_
CLASS=MGAPCI_

if [ -d /usr/X11R6.1 ]
then
        GRAF_TMPL=/usr/X11R6.1/lib/grafinfo/matrox/mgapci.tmpl
        GRAF_DIR=/usr/X11R6.1/lib/grafinfo/matrox
        AOF_DIR=/usr/X11R6.1/lib/vidconf/AOF/ID/matrox
        AOF_TMPL=/usr/X11R6.1/lib/vidconf/AOF/TMPL/matrox/mgapci
        PATH=/usr/X11R6.1/lib/vidconf/scripts:$PATH
else
        GRAF_TMPL=/usr/lib/grafinfo/matrox/mgapci.tmpl
        GRAF_DIR=/usr/lib/grafinfo/matrox
        AOF_DIR=/usr/lib/vidconf/AOF/ID/matrox
        AOF_TMPL=/usr/lib/vidconf/AOF/TMPL/matrox/mgapci
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

set_trap

[ -f ${GRAF_TMPL} ] || cleanup 0
[ -d ${AOF_DIR} ] || mkdir -p ${AOF_DIR}

rm -f ${GRAF_DIR}/${PREFIX}[0-9]*.xgi
rm -f ${AOF_DIR}/${PREFIX}[0-9]*

n=0
while pciinfo -q -d $DEVICE_ID -v $VENDOR_ID -n $n && test $n -lt 10
do
	base=`pciinfo -d $DEVICE_ID -v $VENDOR_ID -n $n -D 0x10`

        if expr `xtod $base` != 0 > /dev/null 2>&1
        then
                ID=`expr $n + 1`

                # Create grafinfo
                arg1="s/@MEM_BASE@/$base/g"
                arg2="s/@MODEL@/${MODEL}${n}/g"
                arg3="s/@CLASS@/${CLASS}${n}/g"
                if test $n -eq 0
                then
                        arg4="s/@CARD@//g"
                else
                        arg4="s/@CARD@/\(#${ID}\)/g"                        
                fi
                sed -e $arg1 -e $arg2 -e $arg3 -e $arg4 \
                        < ${GRAF_TMPL} > ${GRAF_DIR}/${PREFIX}${n}.xgi

                # Create AOF
                if [ -f ${AOF_TMPL} ]
                then
                        if test $n -eq 0
                        then
                                arg1="s/@CARD@//g"
                        else
                                arg1="s/@CARD@/\(#${ID}\)/g"                        
                        fi
                        arg2="s/@ID@/0${n}/g"
                        sed -e $arg1 -e $arg2 \
                                < ${AOF_TMPL} > ${AOF_DIR}/${PREFIX}${n}
                fi
        fi
        n=`expr $n + 1`
done

cleanup 0

