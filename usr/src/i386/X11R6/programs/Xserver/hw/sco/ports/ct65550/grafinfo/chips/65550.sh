:
#	@(#)65550.sh	11.1	10/22/97	12:34:30
#	@(#) 65550.sh 59.1 96/11/04 "
#
USRLIBDIR=/usr/X/lib
PCIINFO=${USRLIBDIR}/vidconf/scripts/pciinfo
#
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

set_trap

${PCIINFO} -q -d 0x00E0 -v 0x102C > /dev/null 2>&1

if [ $? -ne 1 ]
then
	base=`${PCIINFO} -d 0x00E0 -v 0x102C -W 0x12`
else
	${PCIINFO} -q -d 0x00E4 -v 0x102C > /dev/null 2>&1

	if [ $? -ne 1 ]
	then
		base=`${PCIINFO} -d 0x00E4 -v 0x102C -W 0x12`
	else
		base="0x700"
	fi
fi

TEMPLATE=${USRLIBDIR}/grafinfo/chips/65550.tmpl
XGIFILE=${USRLIBDIR}/grafinfo/chips/65550.xgi

sedarg1="s/@MEM_BASE@/$base/g"
sed -e $sedarg1 < ${TEMPLATE} > ${XGIFILE}
cleanup 0
