#ident	"@(#)applysid.sh	15.1"

function loadsid
{
[ -n "$SH_VERBOSE" ] && set -x
integer t
typeset mntsts sidname
typeset SID_ERROR=0

place_window $SID_REINSERTCols $SID_REINSERTLines+5 -fg $WHITE -bg $BLUE
wprintf $CURWIN "$SID_REINSERT"
t=${#SID_DISKETTE}/2
wgotoxy $CURWIN $SID_REINSERTCols/2-$t  $SID_REINSERTLines+1
wprintf $CURWIN "$SID_DISKETTE\n"
footer $SID_FOOTER
call getkey
wclose $CURWIN

eval footer "\"$SID_LOAD_FOOTER\""

while :
do
	/sbin/mount -Fs5 -r /dev/dsk/f0t /install 2>/dev/null || /sbin/mount -Fs5 -r /dev/dsk/f1t /install 2>/dev/null
	mntsts="$?"

	if [ "$mntsts" = "0" ] 
	then
		if [ -s /install/signature ]
		then
			grep -v '^$' /install/signature > /tmp/sidname
			read sidname < /tmp/sidname
			if [ "$SID_NAME" = "$sidname" ]
			then
				SID_ERROR=0
				. /install/sbin/sid
			else
				SID_ERROR=2
			fi
		else
			SID_ERROR=3
		fi
		/sbin/umount /install	 2>/dev/null
	else
		SID_ERROR=1
	fi

	case $SID_ERROR
	in
	0 ) 	footer ""	#clear footer on exit
		return ;;
	1 ) # cannot install sid diskette...try again.
		display -w "$SID_EMSG1" -bg $ERROR_BG -fg $ERROR_FG
		input_handler;;
	2) # inserted wrong sid diskette.
		display -w "$SID_EMSG2" -bg $ERROR_BG -fg $ERROR_FG
		input_handler;;
	3) # diskette is not a sid diskette.
		display -w "$SID_EMSG3" -bg $ERROR_BG -fg $ERROR_FG
		input_handler;;
	esac
done
}

$SID_MODE && loadsid 
