:
#
#	@(#)buildgrafinfo.sh	11.1	10/22/97	12:33:18
#	@(#) buildgrafinfo.sh 60.1 96/12/13 
#
#	Copyright (C) The Santa Cruz Operation, 1991-1997.
#	The Santa Cruz Operation, and should be treated as Confidential.
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#
# The information in this file is provided for the exclusive use of the
# licensees of The Santa Cruz Operation, Inc.  Such users have the right 
# to use, modify, and incorporate this code into other products for purposes 
# authorized by the license agreement provided they include this notice 
# and the associated copyright notice with any such product.  The 
# information in this file is provided "AS IS" without warranty.
#
if [ "$1" = "" ]
then
	echo "usage: $0 template < modes.list" 1>&2
	exit 1
fi
templ=$1
while read modenum width height depth vhz pitch
do
	extra=""
	case $height in
	    480)
		maxres=00;;
	    600)
		maxres=01;;
	    768)
		maxres=02;;
	    864)
		maxres=03;;
	    1024)
		maxres=03;;
	    1200)
		maxres=04;;
	    *)
		echo "$0: unknown vertical dimension $height!  Fix this." 1>&2
		exit 1;;
	esac
	case $vhz in
	    43i)
		extra='(interlaced)'
		bh=00
		ch=00;;
	    48i)
		extra='(interlaced)'
		bh=00
		ch=00;;
	    56)
		bh=00
		ch=00;;
	    60)
		bh=11
		ch=10;;
	    70)
		bh=21
		ch=20;;
	    71)
		bh=21
		ch=20;;
	    72)
		bh=32
		ch=20;;
	    75)
		bh=34
		ch=31;;
	    85)
		bh=45
		ch=40;;
	    *)
		echo "$0: unknown refresh rate $vhs!  Fix this." 1>&2
		exit 1;;
	esac
	case $modenum in
	    7B)
		extra="$extra BIOS 1.12 or later only";;
	esac
	case $depth in
	    8)
		colors=256;;
	    16)
		colors=64K;;
	    24)
		colors=16M;;
	    *)
		echo "$0: unknown depth $depth!  Fix this." 1>&2
		exit 1;;
	esac
	sed -e "s/@WIDTH@/$width/g" \
	    -e "s/@HEIGHT@/$height/g" \
	    -e "s/@DEPTH@/$depth/g" \
	    -e "s/@COLORS@/$colors/g" \
	    -e "s/@PITCH@/$pitch/g" \
	    -e "s/@VHZ@/$vhz/g" \
	    -e "s/@MAXRES@/$maxres/g" \
	    -e "s/@MODENUM@/$modenum/g" \
	    -e "s/@BH@/$bh/g" \
	    -e "s/@CH@/$ch/g" \
	    -e "s/@EXTRA@/$extra/" \
	    < $templ
	echo
done
