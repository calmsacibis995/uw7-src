#ident  "@(#)exit_codes.ksh	15.2	98/01/18"

#
# Define variable names for the exit codes. The sequencer, the map file,
# the ui modules, and the install modules need to agree on these values.
# This file will be "dotted in" to a korn shell script.
#
# The range of return codes is -128 to 127. We want to use obscure values,
# so we can detect abnormal exits. For example, if 0 were used, it would
# not be possible to detect a shell script bug where a module was falling
# through to the end, since that would result in a exit code of 0.
# 
# There must be a variable called "PREV", this is used by the sequencer.
#
NEXT=112
PREV=113
HELP=114
NICS=115
CHOOSE=116
IPX=117
SPX=117
TCP=118
DIE=119
NET=120
PRESERVE=121
PARTITION=122
SLICE=123
OPTIONS=124

#
# If you add return codes, make sure you update these to the minimum and
# maximum values used. This is used for error checking; any return code
# outside this range is considered an error.
#
MIN_EXIT_CODE=112
MAX_EXIT_CODE=124


function ii_exit
{
	integer i=0


	if [ -n "$CURWIN" ] && [ -n "$MSGWID" ] && [ -n "$HEADWID" ] \
	   && [ -n "$FOOTWID" ]
	then
		while (( i <= CURWIN ))
		do
			if (( i != MSGWID )) && (( i != HEADWID )) \
				&& (( i != FOOTWID ))
			then
				wclose $i
			fi
			(( i += 1 ))
		done
	fi

	case $1 in
		NEXT)		exit $NEXT;;
		PREV)		exit $PREV;;
		HELP)		exit $HELP;;
		NICS)		exit $NICS;;
		CHOOSE)		exit $CHOOSE;;
		IPX)		exit $IPX;;
		SPX)		exit $SPX;;
		TCP)		exit $TCP;;
		DIE)		exit $DIE;;
		NET)		exit $NET;;
		PRESERVE)	exit $PRESERVE;;
		PARTITION)	exit $PARTITION;;
		SLICE)		exit $SLICE;;
		OPTIONS)	exit $OPTIONS;;
		*)		echo "ii_exit error: $1 not defined."
				exit 3;;
	esac
}


