:
#	@(#) viperpci.sh 11.1 97/10/22
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

if [ -d /usr/X11R6.1 ]
then
        GRAF_DIR="/usr/X11R6.1/lib/grafinfo/"
        SCRIPTS_DIR="/usr/X11R6.1/lib/vidconf/scripts/"
else
        GRAF_DIR="/usr/lib/grafinfo/"
        SCRIPTS_DIR="/usr/lib/vidconf/scripts/"
fi

${SCRIPTS_DIR}pciinfo -q -d 0x9001 -v 0x100E
status="$?"
if [ $status = "1" ]
then
	echo "Cannot find a Diamond Viper PCI card in this computer!"
	cleanup 1
fi
base=`${SCRIPTS_DIR}pciinfo -d 0x9001 -v 0x100E -W 0x12`
slot=`${SCRIPTS_DIR}pciinfo -d 0x9001 -v 0x100E -B 0x15`
sedarg1="s/@MEM_BASE@/$base/g"
sedarg2="s/@PCI_SLOT@/$slot/g"
sed -e $sedarg1 -e $sedarg2 < ${GRAF_DIR}diamond/viperpci.tmpl \
	> ${GRAF_DIR}diamond/viperpci.xgi
cleanup 0
