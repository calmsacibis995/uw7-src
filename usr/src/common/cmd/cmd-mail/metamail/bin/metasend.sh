#
#	ident @(#) metasend.sh 11.2 97/11/03 
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
then METAMAIL_TMPDIR=/tmp
fi

MustDelete=0
batchmode=0
splitsize=100000

while test ! -z "$*"
do
	case $1 in
		-S) shift
		    if test -z "$*"
		    then
			dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_ERR_S \
			    "-S requires a following argument, the SPLIT threshhold\n" "-S"
			exit 1
		    fi
		    splitsize=$1
		    shift ;;

		-b) batchmode=1
		    shift ;;

		-c) shift
		    if test -z "$*"
		    then
			dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_ERR_C \
			    "-c requires a following argument, the CC address\n" "-c"
			exit 1
		    fi
		    cc=$1
		    shift ;;

		-s) shift
		    if test -z "$*"
		    then
			dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_ERR_SUB \
			    "-s requires a following argument, the SUBJECT\n" "-s"
			exit 1
		    fi
		    subject=$1
		    shift ;;

		-t) shift
		    if test -z "$*"
		    then
			dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_ERR_T \
			    "-t requires a following argument, the TO address\n" "-t"
			exit 1
		    fi
		    to=$1
		    shift ;;

		-e) shift
		    if test -z "$*"
		    then
			dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_ERR_E \
			    "-e requires a following argument, the ENCODING value\n" "-e"
			exit 1
		    fi
		    encode=$1
		    shift ;;

		-f) shift
		    if test -z "$*"
		    then
			dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_ERR_F \
			    "-f requires a following argument, the DATA FILE\n" "-f"
			exit 1
		    fi
		    datafile=$1
		    shift ;;

		-m) shift
		    if test -z "$*"
		    then
			dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_ERR_M \
			    "-m requires a following argument, the MIME CONTENT-TYPE\n" "-m"
			exit 1
		    fi
		    ctype=$1
		    shift ;;

		-z) MustDelete=1
		    shift ;;

		*) dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_ERR_OPTION \
			"UNRECOGNIZED METASEND OPTION: $1\n" "$1"
		   exit 1 ;;
	esac
done

if test $batchmode -eq 0
then
	if test -z "${to:-}"
	then
		dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_MSG_TO "To: "
		read to
	fi
	if test -z "${subject:-}"
	then
        	dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_MSG_SUBJECT \
			"Subject: "
		read subject
	fi
	if test -z "${cc:-}"
	then
		dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_MSG_CC "CC: "
		read cc
	fi
	if test -z "${ctype:-}"
	then
        	dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_MSG_CONTENT \
			"Content-type: "
		read ctype
	fi
	if test -z "${datafile:-}"
	then
		looping=1
		while test $looping -eq 1
		do
			dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_MSG_FILE \
				"Name of file containing $ctype data: " "$ctype"
			read datafile
			if test -r "$datafile"
			then
				looping=0
			else
				dspmsg $MF_METAMAIL -s $MS_METASEND \
					$METASEND_ERR_FILE \
					"The file $datafile does not exist.\n" \
					"$datafile"
			fi
		done
	fi

	if test -z "${encode:-}"
	then
		looping=1
		while test $looping -eq 1
		do
			dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_MSG_ENC \
				"Do you want to encode this data for sending through the mail?\n"
			dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_MSG_NO \
				"  1 -- No, it is already in 7 bit ASCII\n"
			dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_MSG_BASE64 \
				"  2 -- Yes, encode in base64 (most efficient)\n"
			dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_MSG_QP \
				"  3 -- Yes, encode in quoted-printable (less efficient, more readable)\n"
			dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_MSG_UUENCODE \
				"  4 -- Yes, encode it using uuencode (not standard, being phased out)\n"
			read encode
			looping=0
			case "$encode" in
				1) encodingprog=cat
				   encode=7bit ;;
				2) encodingprog="mimencode -b"
				   encode=base64 ;;
				3) encodingprog="mimencode -q"
				   encode=quoted-printable ;;
				4) encodingprog="uuencode $datafile"
				   encode=x-uue ;;
				*) dspmsg $MF_METAMAIL -s $MS_METASEND \
					$METASEND_ERR_ANSWER \
					"Unrecognized answer, please try again.\n"
				   looping=1 ;;
			esac
		done
	fi
else
	if test -z "${to:-}" \
		-o -z "${subject:-}" \
		-o -z "${ctype:-}" \
		-o -z "${datafile:-}"
	then
		dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_ERR_BATCH \
			"metasend: in batch mode, -t, -s, -f, and -m are all required\n" \
			"-t" "-s" "-f" "-m"
		exit 1
	fi

	if test ! -r "$datafile"
	then
		dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_ERR_DATAFILE \
			"metasend: The file $datafile does not exist\n" "$datafile"
		exit 1
	fi

	if test -z "${cc:-}"
	then
		cc=''
	fi

	if test -z "${encode:-}"
	then
		case "$ctype" in
			text*) encodingprog="mimencode -q"
			       encode=quoted-printable ;;
			*) encodingprog="mimencode -b"
			   encode=base64 ;;
		esac
	else
		case "$encode" in
			base64) encodingprog="mimencode -b" ;;
			x-uue) encodingprog="uuencode $datafile" ;;
			*) encodingprog="mimencode -q"
			   encode=quoted-printable ;;
		esac
	fi
fi

fname=$METAMAIL_TMPDIR/metasend.$$
echo "To: $to" > $fname
echo "Subject: $subject" >> $fname
echo "CC: $cc" >> $fname
echo "MIME-Version: 1.0" >> $fname
echo "Content-type: $ctype" >> $fname
echo "Content-Transfer-Encoding: $encode" >> $fname
echo "" >> $fname
$encodingprog < "$datafile" >> "$fname"
# Ensure last line has trailing carriage return
echo "" >> "$fname"

splitmail -s $splitsize -d $fname

if test $? -eq 0
then
	rm -f $fname
elif test "$MustDelete" -eq 1
then
	dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_ERR_DELIVERY \
		"Mail delivery failed\n"
	rm -f $fname
else
	dspmsg $MF_METAMAIL -s $MS_METASEND $METASEND_ERR_DRAFT \
		"Mail delivery failed, draft mail is in $fname\n" "$fname"
fi
