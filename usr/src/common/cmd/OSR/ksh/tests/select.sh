#ident	"@(#)OSRcmds:ksh/tests/select.sh	1.1"
#	@(#) select.sh 25.2 92/12/11 
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
trap "rm -f /tmp/Sh$$*" EXIT
PS3='ABC '

cat > /tmp/Sh$$.1 <<\!
1) foo
2) bar
3) bam
!
print -n "$PS3" >> /tmp/Sh$$.1

select i in foo bar bam
do	case $i in
	foo)	break;;
	*)	err_exit "select 1 not working"
		break;;
	esac
done 2> /tmp/Sh$$.2 <<!
1
!
cmp /tmp/Sh$$.[12] || err_exit "select output error"

unset i
select i in foo bar bam
do	case $i in
	foo)	err_exit "select foo not working" 2>&3
		break;;
	*)	if	[[ $REPLY != foo ]]
		then	err_exit "select REPLY not correct" 2>&3
		fi
		( set -u; : $i ) || err_exit "select: i not set to null" 2>&3
		break;;
	esac
done  3>&2 2> /tmp/Sh$$.2 <<!
foo
!
exit $((Errors))
