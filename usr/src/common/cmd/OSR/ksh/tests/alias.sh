#ident	"@(#)OSRcmds:ksh/tests/alias.sh	1.1"
#	@(#) alias.sh 25.2 92/12/11 
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

Command=$0
integer Errors=0
alias foo='print hello'
if	[[ $(foo) != hello ]]
then	err_exit 'foo, where foo is alias for "print hello" failed'
fi
if	[[ $(foo world) != 'hello world' ]]
then	err_exit 'foo world, where foo is alias for "print hello" failed'
fi
alias foo='print hello '
alias bar=world
if	[[ $(foo bar) != 'hello world' ]]
then	err_exit 'foo bar, where foo is alias for "print hello " failed'
fi
if	[[ $(foo \bar) != 'hello bar' ]]
then	err_exit 'foo \bar, where foo is alias for "print hello " failed'
fi
alias bar='foo world'
if	[[ $(bar) != 'hello world' ]]
then	err_exit 'bar, where bar is alias for "foo world" failed'
fi
if	[[ $(alias bar) != 'bar=foo world' ]]
then	err_exit 'alias bar, where bar is alias for "foo world" failed'
fi
unalias foo  || err_exit  "unalias foo failed"
alias foo 2> /dev/null  && err_exit "alias for non-existent alias foo returns true"
exit $((Errors))
