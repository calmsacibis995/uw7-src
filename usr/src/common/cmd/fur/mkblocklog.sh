#!/usr/bin/ksh
#ident	"@(#)fur:common/cmd/fur/mkblocklog.sh	1.3"
set -e
while [ -n "$1" ]
do
	case "$1" in
	-p)
		shift
		BLOCKLOGPREFIX="$1"
		;;
	-*)
		OPTS="$1 $OPTS"
		;;
	*)
		if [ -z "$NBLOCKS" ]
		then
			NBLOCKS="$1"
		elif [ -z "$FILENAME" ]
		then
			FILENAME="$1"
		fi
	esac
	shift
done
i=0
flow=0
while (( (i=i+1) < ARGC ))
do
	eval ARG=\"\$ARGV$i\"
	case "$ARG" in
	-bflow)
		flow=1
		OPTS="$OPTS -DFLOW"
		;;
	-b)
		let j=i+1
		eval ARG=\"\$ARGV$j\"
		if [ "$ARG" = flow ]
		then
			OPTS="$OPTS -DFLOW"
			flow=1
		fi
		;;
	esac
done
if [ -z "$BLOCKLOGPREFIX" ]
then
	BLOCKLOGPREFIX="block.$FILENAME"
fi
typeset -L12 SHORT
FILEN=${FILENAME##*/}
FILEN=${FILEN%%.*}
SHORT=$FILEN
eval SFILEN=$SHORT
if (( NBLOCKS <= 0 ))
then
	/bin/as -o log.$FILEN.o - < /dev/null
	exit 0
fi
sed -e "s/BlockCount/${SFILEN}BlockCount/g" -e "s/blocklog/${SFILEN}blocklog/g" FURDIR/flowlog.s | m4 $OPTS | as -o fast.$$.o -
if (( flow ))
then
	sed -e "s/blockcount/flowcount/g" -e "s/blocklog/${SFILEN}blocklog/g" FURDIR/blocklog.c > log.$$.c
else
	sed -e "s/blocklog/${SFILEN}blocklog/g" FURDIR/blocklog.c > log.$$.c
fi
cc -O -IFURDIR $OPTS -DNBLOCKS=$NBLOCKS -DBlockSize="${SFILEN}BlockSize" -Dblocklog="${SFILEN}blocklog" -DBlockCount="${SFILEN}BlockCount" -DBLOCKLOGPREFIX=\"$BLOCKLOGPREFIX\" -c log.$$.c && ld -r -o log.$FILEN.o fast.$$.o log.$$.o && rm -f log.$$.c log.$$.o fast.$$.o
