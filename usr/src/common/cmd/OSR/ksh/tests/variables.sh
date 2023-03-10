#ident	"@(#)OSRcmds:ksh/tests/variables.sh	1.1"
#	@(#) variables.sh 25.2 92/12/11 
#
#	Copyright (C) The Santa Cruz Operation, 1990-1992
#	Copyright (C) AT&T, 1984-1992
#		All Rights Reserved.
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#
function err_exit
{
	print -u2 -n "\t"
	print -u2 -r $Command: "$@"
	let Errors+=1
}

integer Errors=0
Command=$0
# RANDOM
if	(( RANDOM==RANDOM || $RANDOM==$RANDOM ))
then	err_exit RAMDOM variable not working
fi
# SECONDS
sleep 3
if	(( SECONDS < 2 ))
then	err_exit SECONDS variable not working
fi
# _
set abc def
if	[[ $_ != def ]]
then	err_exit _ variable not working
fi
# ERRNO
set abc def
rm -f foobar#
ERRNO=
2> /dev/null < foobar#
if	(( ERRNO == 0 ))
then	err_exit ERRNO variable not working
fi
# PWD
if	[[ !  $PWD -ef . ]]
then	err_exit PWD variable not working
fi
# PPID
if	[[ $($SHELL -c 'echo $PPID')  != $$ ]]
then	err_exit PPID variable not working
fi
# OLDPWD
old=$PWD
cd /
if	[[ $OLDPWD != $old ]]
then	err_exit OLDPWD variable not working
fi
cd $d || err_exit cd failed
# REPLY
read <<-!
	foobar
	!
if	[[ $REPLY != foobar ]]
then	err_exit REPLY variable not working
fi
# LINENO
LINENO=10
#
#  These lines intentionally left blank
#
if	(( LINENO != 13))
then	err_exit LINENO variable not working
fi
exit $((Errors))
