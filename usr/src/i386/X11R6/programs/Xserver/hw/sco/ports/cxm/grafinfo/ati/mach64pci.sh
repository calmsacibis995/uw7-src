#!/bin/sh
#
#	@(#)mach64pci.sh	11.2	12/4/97	15:43:15
#	@(#)mach64pci.sh	12.3	3/22/96	11:03:43
#
#
USRLIBDIR=/usr/X11R6.1/lib
#
set_trap()  {
	trap 'echo "${USRLIBDIR}/vidconf/scripts/mach64pci: Interrupted! Giving up..."; cleanup 1' 1 2 3 15
}

unset_trap()  {
	trap '' 1 2 3 15
}
 
cleanup() {
	trap '' 1 2 3 15
	exit $1
}

TEMPLATE=${USRLIBDIR}/grafinfo/ati/mach64pci.tmpl
XGIFILE=${USRLIBDIR}/grafinfo/ati/mach64pci.xgi
PCIINFO=${USRLIBDIR}/vidconf/scripts/pciinfo

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

