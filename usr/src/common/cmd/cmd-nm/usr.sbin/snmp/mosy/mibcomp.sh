#ident	"@(#)mibcomp.sh	1.2"
#ident	"$Header$"
:
#      @(#)mibcomp.sh	1.1 STREAMWare TCP/IP SVR4.2  source
:
#      @(#)mibcomp.sh	6.1 Lachman System V STREAMS TCP  source
#      SCCS IDENTIFICATION
#
# Copyrighted as an unpublished work.
# (c) Copyright 1992 INTERACTIVE Systems Corporation
# All rights reserved.
#
: run this script through /bin/sh
BIN="/usr/sbin"
sflag=
OUTPUT=
while getopts so: C; do
	case $C in
	s)
		sflag=-s
		;;
	o)
		OUTPUT=$OPTARG
		;;
	\?)
		echo "usage: $0: [-s] -o output-file mib-file..."
		exit 1
		;;
	esac
done
shift `expr $OPTIND - 1`
if [ $# = 0 -o X$OUTPUT = X ]; then
	echo "usage: $0: [-s] -o output-file mib-file..."
	exit 1
fi
FILES=$*
OFILES=
for i in $FILES; do
	curfile=`basename $i \.my`.defs
	OFILES=${OFILES}" "$curfile
	${BIN}/mosy $sflag -o $curfile $i
done
cat $OFILES > /tmp/mc$$
${BIN}/post_mosy -i /tmp/mc$$ -o $OUTPUT
rm -f /tmp/mc$$
