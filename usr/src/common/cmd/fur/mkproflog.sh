#!/usr/bin/ksh
#ident	"@(#)fur:common/cmd/fur/mkproflog.sh	1.1"
while [ -n "$1" ]
do
	case "$1" in
	-p)
		shift
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
		else
			NFUNCS="$1"
		fi
	esac
	shift
done
FILEN=${FILENAME##*/}
FILEN=${FILEN%%.*}
cc -O -IFURDIR -DProfCount=${FILEN}ProfCount $OPTS -DNFUNCS=$NFUNCS -c FURDIR/proflog.c && mv proflog.o prof.$FILEN.o
