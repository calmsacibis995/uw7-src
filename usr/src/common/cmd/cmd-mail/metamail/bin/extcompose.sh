#
#	ident @(#) extcompose.sh 11.2 97/11/03 
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
# This file Copyright (c) 1992 Z-Code Software Corp.
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

echo_n() {
    echo "$@\c"
}

:

if [ $# -lt 1 ]
then
    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_ERR_USAGE \
	"Usage:  $0 output-file-name\n" $0 1>&2
    exit 1
fi
OUTFNAME=$1

choosing=yes
while [ $choosing = yes ]
do
    echo ""
    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_WHERE \
	"Where is the external data that you want this mail message to reference?\n"
    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_LOCAL \
	"    1 -- In a local file\n"
    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_AFS \
	"    2 -- In an AFS file\n"
    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_ANON \
	"    3 -- In an anonymous FTP directory on the Internet\n"
    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_LOGIN \
	"    4 -- In an Internet FTP directory that requires a valid login\n"
    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_MAILSRVR \
	"    5 -- Under the control of a mail server that will send the data on request\n"
    echo ""
    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_NUMBER \
	"Please enter a number from 1 to 5: "
    read ans
    case "$ans" in
        1) accesstype=local-file ;;
	2) accesstype=afs ;;
	3) accesstype=anon-ftp ;;
	4) accesstype=ftp ;;
	5) accesstype=mail-server ;;
	* ) dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_ERR_CHOICE \
		"That is NOT one of your choices.\n" 1>&2; continue ;;
    esac

    case "$accesstype" in
        ftp | anon-ftp )
	    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_DOMAIN \
		"Enter the full Internet domain name of the FTP site: "
	    read site
	    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_DIR \
		"Enter the name of the directory containing the file (RETURN for top-level): "
	    read directory
	    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_NAME \
		"Enter the name of the file itself: "
	    read name
	    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_TRANSFER \
		"Enter the transfer mode (type 'image' for binary data, RETURN otherwise): "
	    read mode
	    if [ -n "$mode" ]
	    then mode=ascii
	    fi
	    echo "Content-type: message/external-body; access-type=$accesstype; name="\"$name\"\; > $OUTFNAME
	    echo_n "    site="\"$site\" >> $OUTFNAME
	    if [ -n "$directory" ]
	    then echo_n "; directory="\"$directory\">> $OUTFNAME
	    fi
	    echo_n "; mode="\"$mode\">> $OUTFNAME
	    echo "">> $OUTFNAME
	    choosing=no
	    ;;

	local-file | afs )
	    name=
	    while [ -z "$name" ]
	    do
	        dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_PATH \
		    "Enter the full path name for the file: "
		read name
		if [ ! -f "$name" ]
		then
		    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE \
			$EXTCOMPOSE_ERR_EXIST \
			"The file $name does not seem to exist.\n" "$name"
		    name=
		fi
	    done
	    echo "Content-type: message/external-body; access-type=$accesstype; name="\"$name\"> $OUTFNAME
	    choosing=no
	    ;;
	
	mail-server )
	    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_ADDR \
		"Enter the full email address for the mailserver: "
	    read server
	    echo "Content-type: message/external-body; access-type=$accesstype; server="\"$server\"> $OUTFNAME
	    choosing=no
	    ;;
	
	* )
	    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_ACCESS \
		"access type $accesstype not yet implemented\n" "$accesstype"
	    ;;
    esac
done

dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_CONTENT \
    "Please enter the MIME content-type for the externally referenced data: "
read ctype

choosing=yes
while [ $choosing = yes ]
do
    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_ENCODED \
	"Is this data already encoded for email transport?\n"
    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_NOT \
	"  1 -- No, it is not encoded\n"
    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_BASE64 \
	"  2 -- Yes, it is encoded in base64\n"
    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_QP \
	"  3 -- Yes, it is encoded in quoted-printable\n"
    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_UUENCODE \
	"  4 -- Yes, it is encoded using uuencode\n"
    read encode
    case "$encode" in
	1 ) cenc="" choosing=no ;;
	2 ) cenc="base64" choosing=no ;;
	3 ) cenc="quoted-printable" choosing=no ;;
	4 ) cenc="x-uue" choosing=no ;;
	* ) dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_ERR_CHOICE \
		"That is not one of your choices.\n" ;;
    esac
done

echo "" >> $OUTFNAME
echo "Content-type: " $ctype >> $OUTFNAME
if [ -n "$cenc" ]
then echo "Content-transfer-encoding: " $cenc >> $OUTFNAME
fi
echo "" >> $OUTFNAME
if [ "$accesstype" = "mail-server" ]
then
    dspmsg $MF_METAMAIL -s $MS_EXTCOMPOSE $EXTCOMPOSE_MSG_DATA \
	"Please enter all the data to be sent to the mailserver in the message body, \nending with ^D or your usual end-of-data character:\n"
    cat >> $OUTFNAME
fi
