#ident	"@(#)dtrac:rac_readin.sh	1.4"
# This file contains the Readin() for parsing the data file and initialize
# global variables for the rest of the program.

# To trace this function, export MYDEBUG=1 before invoking this script
if [ "$MYDEBUG" = "" ]
then
	alias TRACE=:
else
	alias TRACE="set -x"
fi

set -A labelArray
typeset -i maxWidth=0

NODE_DIR=$HOME/.node
FIRST_TIME=FALSE
ICON_NAME=""
SYSTEM_NAME=""
LOGIN_NAME=""
TRANSER_FILE_TO=""
TRNASER_FILE_USING=""
COPY_FILE_TO=""
ALWAYS_CONFIRM=""
CONN_CONFIRM=""
FTP_CONFIRM=""
# for dialup setup, XFER_OPTION should set to UUCP only
XFER_OPTION=both

Readin() 
{
	TRACE
	OIFS="$IFS"
	# IFS includes =, space, tab, and newline .
	IFS="= 	
	"

	cat $NODE_DIR/$1 | \
	while read name value
	do
		case "$name" in
		SYSTEM_NAME)
			SYSTEM_NAME="$value";;
		DUSER)
			# old friend node format and it should be overwritten
			# by the new format - LOGIN_NAME
			LOGIN_NAME="$value";;
		LOGIN_NAME)
			LOGIN_NAME="$value";;
		TRANSFER_FILE_TO)
			TRANSFER_FILE_TO="$value";;	
		TRANSFER_FILE_USING)
			TRANSFER_FILE_USING="$value";;
		DPATH)
			# old friend node format and it should be overwritten
			# by the new format - COPY_FILE_TO
			COPY_FILE_TO="$value";;
		COPY_FILE_TO)
			COPY_FILE_TO="$value";;
		CONN_CONFIRM)
			CONN_CONFIRM="$value";;
		FTP_CONFIRM)
			FTP_CONFIRM="$value";;
		XFER_OPTION)
			XFER_OPTION="$value";;
		esac

	done
	IFS="$OIFS"
	return 0
}

# This function calcuate the maximun width for the labels, so we can
# align the label/text widgets correctly in the form.
# $1 ==> parent shell
GetMaxWidth()
{
	TRACE
	labelArray[1]="$($GETTXT $LABEL_SystemName)"
	labelArray[2]="$($GETTXT $LABEL_LoginToSysAs)"
	labelArray[3]="$($GETTXT $LABEL_DisplayConnAs)"
	labelArray[4]="$($GETTXT $LABEL_XferFilesTo)"
	labelArray[5]="$($GETTXT $LABEL_XferFilesU)"
	labelArray[6]="$($GETTXT $LABEL_AlwaysConfirm)"
	labelArray[7]="$($GETTXT $LABEL_UserRcvFiles)"
	labelArray[8]="$($GETTXT $LABEL_FileSnd)"

	for i in 1 2 3 4 5 6 7 8
	do
		cmw l l label $1 labelString:"${labelArray[$i]}"
		gv $l width:len
		if [ $len -gt $maxWidth ]
		then
			maxWidth=$len
		fi
		dw $l	
	done
	maxWidth=$maxWidth+1
}
