#!/usr/bin/xksh
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fprof:common/fprof.sh	1.4"

function usage
{
	echo "Usage:
      fprof [log] . . .
      fprof -C[Logging=on|off,][StartState=on|off,][Accuracy=accurate|normal,][LogPrefix=pathname-for-prefix]
      fprof -s [-C[Logging=on|off,][StartState=on|off,][Accuracy=accurate|normal,][LogPrefix=pathname-for-prefix]] command
      fprof [-o|-O] [-m] log [log] . . ."
	exit 1
}

[ -n "$_SECOND_TIME_IN" ] && set -- $_FPROF_ARGS
test= compile= off= on= info= mark= config=
[ ! -r CCSLIB/fprof.cfg ] || . CCSLIB/fprof.cfg
_FprofLogging=on
_FprofAccuracy=normal
_FprofLogPrefix=/tmp/out
_FprofStartState=on
while getopts '?C:oOsimc' c
do
	case "$c" in
	C)
		config=on
		OIFS="$IFS"
		IFS=,
		set -A a $OPTARG
		IFS="$OIFS"
		for i in "${a[@]}"
		do
			case "$i" in
			Logging=*|Accuracy=*|LogPrefix=*|StartState=*)
				export "_Fprof$i"
				;;
			*)
				print -u2 Illegal configuration parameter $i
				usage
			esac
		done
		;;
	s)
		test=on
		;;
	c)
		[ -n "$test" ] && usage
		compile=on
		;;
	m)
		[ -n "$test" ] && usage
		mark=on
		;;
	o)
		[ -n "$test" ] && usage
		on=on
		;;
	i)
		[ -n "$test" ] && usage
		info=on
		;;
	O)
		[ -n "$test" ] && usage
		off=on
		;;
	\?)
		usage
		;;
	*)
		echo Illegal option: $c
		usage
	esac
done

let OPTIND=OPTIND-1
shift $OPTIND

if [ "$test" ]
then
	exec "$@"
fi

if [ "$config" ]
then
	call getuid || {
		print -u2 System-wide configuration requires root privilege
		usage
		exit 1
	}
	echo "Logging=$_FprofLogging\nAccuracy=$_FprofAccuracy\nStartState=$_FprofStartState\nLogPrefix=$_FprofLogPrefix" > CCSLIB/fprof.cfg
	exit 0
fi
libload CCSLIB/libfprof.so.1

nl='
'
call do_cmdload 2 '@char *[2]:{ "cmdload", "do_cmdload=cmdload" }'
cmdload open output output_until_mark bracket search rewind count close callers callees stats info compile

if [ "$compile$mark$on$off$info" ]
then
	[ -n "$1" ] || usage
	open "$@" || {
		echo Cannot open "$@"
		usage
	}
	[ -z "$compile" ] || compile
	[ -z "$mark" ] || call do_mark
	[ -z "$info" ] || info
	[ -z "$on" ] || call do_switch 1
	[ -z "$off" ] || call do_switch 0
elif [ -z "$_SECOND_TIME_IN" ]
then
	export _SECOND_TIME_IN=on
	export _OLDENV=$ENV
	export ENV=$(whence -p $0)
	export _FPROF_ARGS="$@"
	exec /usr/bin/xksh
else
	unset _SECOND_TIME_IN
	unset _FPROF_ARGS
	ENV=$_OLDENV
	unset _OLDENV
	[ -z "$ENV" ] || . $ENV
	PS1='fprof: '
	alias q=exit
	if [ -n "$1" ]
	then
		open "$@" || {
			echo Cannot open "$@"
			usage
		}
	fi
fi
