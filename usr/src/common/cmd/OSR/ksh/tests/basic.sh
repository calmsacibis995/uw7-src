#ident	"@(#)OSRcmds:ksh/tests/basic.sh	1.1"
#	@(#) basic.sh 25.2 92/12/11 
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

# test basic file operations like redirection, pipes, file expansion
Command=$0
integer Errors=0
umask u=rwx,go=rx || err_exit "umask u=rws,go=rx failed"
mkdir  /tmp/ksh$$ || err_exit "mkdir /tmp/ksh$$ failed" 
cd /tmp/ksh$$ || err_exit "cd /tmp/ksh$$ failed"
date > dat1 || err_exit "date > dat1 failed"
test -r dat1 || err_exit "dat1 is not readable"
x=dat1
cat <$x > dat2 || err_exit "cat < $x > dat2 failed"
cat dat1 dat2 | cat  | cat | cat > dat3 || err_exit "cat pipe failed"
cat > dat4 <<!
$(date)
!
cat dat1 dat2 | cat  | cat | cat > dat5 &
wait $!
set -- dat*
if	(( $# != 5 ))
then	err_exit "dat* matches only $# files"
fi
cd ~- || err_exit "cd back failed"
rm -r /tmp/ksh$$ || err_exit "rm -r /tmp/ksh$$ failed"
bar=foo
eval foo=\$bar
if	[[ $foo != foo ]]
then	err_exit 'eval foo=\$bar not working'
fi
bar='foo=foo\ bar'
eval $bar
if	[[ $foo != 'foo bar' ]]
then	err_exit 'eval foo=\$bar, with bar="foo\ bar" not working'
fi
cd /tmp
cd ../../dev || err_exit "cd ../../dev failed"
if	[[ $PWD != /dev ]]
then	err_exit 'cd ../../dev is not /dev'
fi
( sleep 2; cat <<!
foobar
!
) | cat > /tmp/foobar$$ &
wait $!
foobar=$( < /tmp/foobar$$) 
if	[[ $foobar != foobar ]]
then	err_exit "$foobar is not foobar"
fi
rm -f /tmp/foobar$$
exit $((Errors))
