#ident	"@(#)OSRcmds:ksh/tests/builtins.sh	1.1"
#	@(#) builtins.sh 25.2 92/12/11 
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

# test shell builtin commands
Command=$0
integer Errors=0
: ${foo=bar} || err_exit ": failed"
[[ $foo = bar ]] || err_exit ": side effects failed"
set -- -x foobar
[[ $# = 2 && $1 = -x && $2 = foobar ]] || err_exit "set -- -x foobar failed"
getopts :x: foo || err_exit "getopts :x: returns false"
[[ $foo = x && $OPTARG = foobar ]] || err_exit "getopts :x: failed"
false ${foo=bar} &&  err_exit "false failed"
read <<!
hello world
!
[[ $REPLY = 'hello world' ]] || err_exit "read builtin failed"
exit $((Errors))
