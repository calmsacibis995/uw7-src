#ident	"@(#)OSRcmds:ksh/tests/scoextra.sh	1.1"
#	@(#) scoextra.sh 25.1 93/01/20 
#
#       Copyright (C) The Santa Cruz Operation, 1992-1993
#	Copyright (C) AT&T, 1984-1992
#               All Rights Reserved.
#       This Module contains Proprietary Information of
#       The Santa Cruz Operation, and should be treated as Confidential.
#
#	Modification history
#
#	created		22 Dec 92	scol!anthonys
#	- additional tests created within sco
#

function err_exit
{
	print -u2 -n "\t"
	print -u2 -r $Command: "$@"
	let Errors+=1
}

Command=$0
integer Errors=0

# Globbing problem

dirname=/tmp/testing$$
file=$dirname/organization
trap "rm -f $file; rmdir $dirname" EXIT
mkdir $dirname
> $file
for p in "organi[sz]ation" "o[rs]ganization" "organizati[ox]n" "organizatio[np]" "[no]rganization"; do
    if [[ `echo $dirname/$p` != "$file" ]]
    then err_exit "globbing $p didn't work"
    fi
done

# Pattern matching problem

case 1 in
    [0-9]) ;;
    *) err_exit "1 did not match [0-9] in case statement";;
esac

# Pattern matching problem

if	[[ 1 != [0-9] ]];
then	err_exit "1 did not match [0-9] in conditional expression"
fi

# This removes the a, indicating a match, but the # is left in the value

z='a#123'
if	[[ ${z#a#} != 123 ]]
then	err_exit "result of ${z#a#} was not 123"
fi

#echo "Using typeset -R to make a right-adjusted variable,
#and assigning it value 'x'."
#typeset -R a
#a=x
#echo "Value of var a (should be 'x'): a=$a"

# 'The value of ${file%%*([!x])} should be null, because the *([!x]) should
# match the largest trailing segment that does not contain an x, which in
# these cases is the entire value.

for value in lowercase foo; do
    file=$value
    if [[ ! -z ${file%%*([!x])} ]]
    then	err_exit '${file%%*([!x])}='${file%%*([!x])}
    fi
done
exit $((Errors))
