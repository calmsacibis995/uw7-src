#!/bin/sh
#	@(#)m64pci.sh	11.2	11/26/97	14:44:59
#
set_trap()  {
	trap 'echo "/usr/X11R6.1/lib/vidconf/scripts/m64pci: Interrupted! Giving up..."; cleanup 1' 1 2 3 15
}

unset_trap()  {
	trap '' 1 2 3 15
}
 
cleanup() {
	trap '' 1 2 3 15
	exit $1
}

TEMPLATE=/usr/X11R6.1/lib/grafinfo/ati/m64pci.tmpl
XGIFILE=/usr/X11R6.1/lib/grafinfo/ati/m64pci.xgi
PCIINFO=/usr/X11R6.1/lib/vidconf/scripts/pciinfo

set_trap
$PCIINFO -q -v 0x1002
status="$?"
if [ $status = "1" ]
then
	echo "Cannot find a ATI mach64 PCI card in this computer!"
	cleanup 1
fi
base=`$PCIINFO -v 0x1002 -W 0x12`
sedarg1="s/@MEMBASE@/${base}0000/g"
sed -e $sedarg1 < $TEMPLATE > $XGIFILE
cleanup 0

