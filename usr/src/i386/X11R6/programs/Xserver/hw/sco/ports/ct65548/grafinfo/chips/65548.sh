:
#	@(#)65548.sh	11.1	10/22/97	12:33:38
#	@(#) 65548.sh 58.1 96/10/09 
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

${PCIINFO} -q -d 0x00DC -v 0x102C > /dev/null 2>&1

if [ $? -ne 1 ]
then
	base=`${PCIINFO} -d 0x00DC -v 0x102C -W 0x12`
else
	base="0x700"
fi

TEMPLATE=${USRLIBDIR}/grafinfo/chips/65548.tmpl
XGIFILE=${USRLIBDIR}/grafinfo/chips/65548.xgi

sedarg1="s/@MEM_BASE@/$base/g"
sed -e $sedarg1 < ${TEMPLATE} > ${XGIFILE}
cleanup 0
