#ident	"@(#)asknodename.sh	15.1"

function validnode
{
	if [ "${#1}" -lt 3 ]
	then
		errmsg "$SHORTNAME"
		return 1
	fi
	case "$1" in
	*([A-Za-z0-9_-]))
		;;
	*)
		errmsg "$BADCHARNAME"
		return 1
	esac
	wclose $WCURRENT
	return 0
}

function asknodename
{
	typeset RIGHT BELOW 
#	if [ "$1" = "center" ]
#	then
		RIGHT=1
		BELOW=1
#	else
#		RIGHT=0
#		BELOW=0
#	fi
	place_window 2+${#System_Name}+2+35+2 3 -current 'footer "$NODEPROMPT"' -right $RIGHT -below $BELOW -fg $COMBO2_FG -bg $COMBO2_BG -title "$INITIAL_NODE_TITLE"
	typeset i wid=$CURWIN
	if [ -z "$NODEFID" ]
	then
		open_form -exit msg
		NODEFID=$FID
		add_field -help 'helpwin initsysname' -entry msgnoerr -exit 'validnode "$NODE"' -ilen 35 -p "$System_Name" -px 2 -py 1 -ix 2+${#System_Name}+2 -iy 1 "NODE"
	fi
	run_form $NODEFID
}
