#ident	"@(#)wksh:xksrc/gensym.sh	1.1"

#	Copyright (c) 1990, 1991 AT&T and UNIX System Laboratories, Inc. 
#	All Rights Reserved     

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T    
#	and UNIX System Laboratories, Inc.			
#	The copyright notice above does not evidence any       
#	actual or intended publication of such source code.    
#
#

if [ "$RANDOM" = "$RANDOM" ]
then
	outofdate() {
		newest=`ls -dt "$@" 2>/dev/null | sed 1q`
		test "$1" != "$newest"
	}
	base() {
		echo $1 | sed -e 's!^.*/\([^/]*\)$!\1!' -e 's/\.[^.]*$//' -e 's/_s$//'
	}
else
	outofdate() {
		typeset i
		tester=$1
		shift
		for i
		do
			if [ "$i" -nt "$tester" ]
			then
				return 0
			fi
		done
		return 1
	}
	base() {
		typeset j
		j=${1##*/}
		j=${j%.*}
		echo ${j%_s}
	}
fi
trunc() {
	case "$1" in
	???????????????*)
		echo $1 | cut -c1-14
		;;
	*)
		echo $1
	esac
}
saveargs() {
	if sameargs
	then
		return
	else
		rm -f $targbase.args
		echo "$args" > $targbase.args
	fi
}
sameargs() {
	if [ "$args" = "`cat $targbase.args 2>/dev/null`" ]
	then
		return 0
	else
		return 1
	fi
}
list() {
	PATH=:$PATH:../xksh
	NM=$NM ./listsyms $i | grep '^[A-Za-z0-9_]\{1,19\}$' | grep -v fptrap | sort -u
}
tmp=/tmp/X$$
found=
targ=$1
targbase=`base $targ`
shift
args="$*"
for i
do
	if [ ! -f "$i" ]
	then
		case "$i" in
		lib*)
			if [ -f "/lib/$i" ]
			then
				i=/lib/$i
			elif [ -f "/usr/lib/$i" ]
			then
				i=/usr/lib/$i
			else
				echo "Cannot find $i"
				exit 1
			fi
			;;
		*)
			echo "Cannot find $i"
			exit 1
		esac
	fi
	basei=`base $i`
	lisi=`trunc $basei.list`
	olisi=`trunc $basei.over`
	FILES="$FILES $lisi"
	rm -f "$tmp"
	if [ ! -f "$lisi" ] || outofdate "$lisi" "$i" "$olisi"
	then
		if [ -f $olisi ]
		then
			cp $olisi $tmp
		else
			list "$i" | sort -u > "$tmp"
		fi
		if [ -f "$lisi" ] && diff "$lisi" "$tmp" >/dev/null
		then
			:
		else
			echo "Regenerated $lisi"
			rm -f "$lisi"
			mv -f "$tmp" "$lisi"
			found=1
		fi
	fi
	if outofdate "$targ" "$lisi"
	then
		found=1
	fi
done
rm -f $tmp
if [ -z "$found" ] && [ -f "$targ" ] && sameargs
then
	exit 0
fi
saveargs
echo "Regenerated $targ"
rm -f $targ
exec >$targ
cat <<!
/*#include <sys/types.h>*/
/*#include <sys/stropts.h>*/
#define SYMS_ONLY
#include "xksh.h"
!
sort -m -u $FILES | sed 's/^\(.*\)$/extern unsigned long \1;/'
cat <<!

struct symarray Symarray[] = {
!
sort -m -u $FILES | sed 's/^\(.*\)$/	{ "\1", (unsigned long) \&\1 },/'
cat <<!
	{ 0, 0 }
};

int Symsize = sizeof(Symarray) / sizeof(struct symarray);
!
