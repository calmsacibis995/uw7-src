:
#	@(#) viperpro.sh 11.1 97/10/22

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
        GRAF_DIR=/usr/X11R6.1/lib/grafinfo/
        SCRIPTS_DIR=/usr/X11R6.1/lib/vidconf/scripts/
else
        GRAF_DIR=/usr/lib/grafinfo/
        SCRIPTS_DIR=/usr/lib/vidconf/scripts/
fi

${SCRIPTS_DIR}pciinfo -q -d 0x9100 -v 0x100E 2> /dev/null
pcistatus="$?"

if [ $pcistatus = "0" ]
then
    base=`${SCRIPTS_DIR}pciinfo -d 0x9100 -v 0x100E -W 0x12 | head -1`
else
    base=`${SCRIPTS_DIR}p9100vlbmem`
    vlbstatus="$?"
    if [ $vlbstatus = "1" ]
    then
	echo "Cannot find Diamond Viper PRO!"
	cleanup 1
    fi
fi

sedarg1="s/@MEM_BASE@/$base/g"

sed -e $sedarg1 \
        < ${GRAF_DIR}diamond/viperpro.tmpl \
	> ${GRAF_DIR}diamond/viperpro.xgi

cleanup 0
