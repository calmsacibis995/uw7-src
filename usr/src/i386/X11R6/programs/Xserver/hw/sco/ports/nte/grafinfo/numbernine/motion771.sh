:
#	@(#) motion771.sh 11.1 97/10/22

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
/usr/X11R6.1/lib/vidconf/scripts/pciinfo -q -d 0x88F0 -v 0x5333 2> /dev/null
pcistatus="$?"

if [ $pcistatus = "1" ]
then
        echo "Cannot find #9FX Motion 771!"
	cleanup 1
fi

base=`/usr/X11R6.1/lib/vidconf/scripts/pciinfo -d 0x88F0 -v 0x5333 -W 0x12`
sedarg1="s/@MEM_BASE@/$base/g"

sed -e $sedarg1 \
        < /usr/X11R6.1/lib/grafinfo/numbernine/motion771.tmpl \
	> /usr/X11R6.1/lib/grafinfo/numbernine/motion771.xgi

cleanup 0
