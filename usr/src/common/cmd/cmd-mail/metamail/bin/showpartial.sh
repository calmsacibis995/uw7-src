#
#	ident @(#) showpartial.sh 11.2 97/11/03 
#
#############################################################################
#
#	Copyright (c) 1997 The Santa Cruz Operation, Inc.. 
#		All Rights Reserved. 
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF 
#		THE SANTA CRUZ OPERATION INC.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.
#
#############################################################################
#
# Copyright (c) 1991 Bell Communications Research, Inc. (Bellcore)
# 
# Permission to use, copy, modify, and distribute this material 
# for any purpose and without fee is hereby granted, provided 
# that the above copyright notice and this permission notice 
# appear in all copies, and that the name of Bellcore not be 
# used in advertising or publicity pertaining to this 
# material without the specific, prior written permission 
# of an authorized representative of Bellcore.  BELLCORE 
# MAKES NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY 
# OF THIS MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS", 
# WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES.
#
#############################################################################
#
# Conversion from C shell to Bourne shell by Z-Code Software Corp.
# Conversion Copyright (c) 1992 Z-Code Software Corp.
# Permission to use, copy, modify, and distribute this material
# for any purpose and without fee is hereby granted, provided
# that the above copyright notice and this permission notice
# appear in all copies, and that the name of Z-Code Software not
# be used in advertising or publicity pertaining to this
# material without the specific, prior written permission
# of an authorized representative of Z-Code.  Z-CODE SOFTWARE
# MAKES NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY
# OF THIS MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS",
# WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES.
#

echo_n()
{
    echo "$@\c"
}

:

if [ -z "$METAMAIL_TMPDIR" ]
then
	METAMAIL_TMPDIR=/tmp
fi

TREEROOT=$METAMAIL_TMPDIR/msg-parts-`who am i | awk '{print $1}' -`

if test -z "$3" -o ! -z "$5"
then
	dspmsg $MF_METAMAIL -s $MS_SHOWPARTIAL $SHOWPARTIAL_ERR_USAGE \
		"Usage: showpartial file id partnum totalnum\n"
	exit 1
fi

file=$1

# This next line is because message-id can contain weird chars
id=`echo $2 | tr -d \!\$\&\*\(\)\|\'\"\;\/\<\>\\ `

partnum=$3

if test -z "$4"
then
	totalnum=-1
else
	totalnum=$4
fi

if test ! -d $TREEROOT
then
	mkdir $TREEROOT
	if test $? -ne 0
	then
		dspmsg $MF_METAMAIL -s $MS_SHOWPARTIAL $SHOWPARTIAL_ERR_TREEROOT \
			"mkdir $TREEROOT failed\n" "$TREEROOT"
		exit 1
	fi
fi

if test ! -d ${TREEROOT}/$id
then
	mkdir ${TREEROOT}/$id
	if test $? -ne 0
	then
		dspmsg $MF_METAMAIL -s $MS_SHOWPARTIAL $SHOWPARTIAL_ERR_ID \
	        	"mkdir ${TREEROOT}/$id failed\n" "$TREEROOT" "$id"
		exit 1
	fi
fi

cp $file ${TREEROOT}/$id/$partnum
if test $? -ne 0
then
	dspmsg $MF_METAMAIL -s $MS_SHOWPARTIAL $SHOWPARTIAL_ERR_CP \
		"cp $file ${TREEROOT}/$id/$partnum failed\n" \
		"$file" "$TREEROOT" "$id" "$partnum"
	exit 1
fi

if test $totalnum -eq -1
then
	if test -r ${TREEROOT}/$id/CT
	then
		totalnum=`cat ${TREEROOT}/$id/CT`
	else
    		totalnum=-1
	fi
else
	echo $totalnum > ${TREEROOT}/$id/CT
fi

# Slightly bogus here -- the shell messes up the newlines in the headers
# if ($partnum == 1) then
#     echo $MM_HEADERS > ${TREEROOT}/$id/HDRS
# endif
found=0
ix=1
list=""
limit=$totalnum
if test $limit -eq -1
then
	limit=25
fi

while test "$ix" -le "$limit"
do
	if test -f ${TREEROOT}/$id/$ix
	then
		list="$list $ix"
		found=`expr $found + 1`
	fi
	ix=`expr $ix + 1`
done

if test "$found" = "$totalnum"
then
	cd ${TREEROOT}/$id
	cat $list > ${TREEROOT}/$id/FULL
	rm $list
	dspmsg $MF_METAMAIL -s $MS_SHOWPARTIAL $SHOWPARTIAL_MSG_READ \
		"All parts of this ${totalnum}-part message have now been read.\n" \
		"$totalnum"
	metamail -d  ${TREEROOT}/$id/FULL
	dspmsg $MF_METAMAIL -s $MS_SHOWPARTIAL $SHOWPARTIAL_MSG_WARN \
		"WARNING:  To save space, the full file is now being deleted.\nYou will have to read all $totalnum parts again to see the full message again.\n" \
		"$totalnum"
	rm ${TREEROOT}/$id/FULL
	rm ${TREEROOT}/$id/CT
	cd ${METAMAIL_TMPDIR}
	rmdir ${TREEROOT}/$id
	rmdir ${TREEROOT} > /dev/null 2>&1
else
	if test "$totalnum" -eq -1
	then
		dspmsg $MF_METAMAIL -s $MS_SHOWPARTIAL $SHOWPARTIAL_MSG_SEV \
			"So far you have only read $found of the several parts of this message.\n" \
			"$found"
	else
		dspmsg $MF_METAMAIL -s $MS_SHOWPARTIAL $SHOWPARTIAL_MSG_TOTAL \
			"So far you have only read $found of the $totalnum parts of this message.\n" \
			"$found" "$totalnum"
	fi
	dspmsg $MF_METAMAIL -s $MS_SHOWPARTIAL $SHOWPARTIAL_MSG_FULL \
		"When you have read them all, then you will see the message in full.\n"
fi
