#!/usr/X/bin/wksh -motif
#ident	"@(#)dtrac:rac_copy.sh	1.16"

# Functionality: This script is called when desktop user drag and drop a 
#		folder onto the friend node icon. The Access window will
#		be displayed only when the "Always Confirm" for File 
#		Transfers field is set.
#
# Purposes of the other files:
#
#       - libdtnet.so has all the library functions that will be called
#         by this client. e.g nslookup.
#       - rac.msgs contains all the different kinds of messages used
#         in this client. The messages are in I18N format and can be
#         translated in other locales.
#       - misc.sh contains all the shell interfaces to the C functions 
#	  and common shell functions called by others (e.g. dtprop.sh
#	  and dtcopy.sh.

# To debug the shell script, export MYDEBUG=1 before invoking this script
if [ "$MYDEBUG" = "" ]
then
	alias TRACE=:
else
	alias TRACE="set -x"
fi

XWINHOME=${XWINHOME:-/usr/X}
RACAPP=${RACAPP:-$XWINHOME/desktop/rac}

. ${XWINHOME}/lib/wksh/xmfuncs/xmkfuncs.sh
. ${RACAPP}/rac.msgs
. ${RACAPP}/rac_readin.sh
. ${RACAPP}/rac_misc.sh

`pkginfo -i inet > /dev/null 2>&1`
INET=$?

if [ $INET = 0 ]
then
	libload ${XWINHOME}/lib/libdtnet.so
else
	libload ${XWINHOME}/lib/libdtrac.so
fi

# These global variables are used to keep track of number of uucp/rcp jobs
# submitted.
typeset -i NUM_UUCP=1
typeset -i NUM_RCP=1
typeset -i USE_UUCP=1
typeset -i USE_RCP=2

# This function calls the rcp to transfer file(s) and performs error checking
# $1 ==> parent widget
# $2 ==> source
# $3 ==> destination
# $4 ==> keyword (shell or box) 
do_rcp()
{
	TRACE
	if [ ! -x /usr/bin/rcp ]
	then
		if [ "$4" = "shell" ]
		then
			Message_CB $1 \
				"$($GETTXT $ERR_CannotExecute) $($GETTXT $LABEL_RCP)" 0 0
		else
			Display_error $1 \
				"$($GETTXT $ERR_CannotExecute) $($GETTXT $LABEL_RCP)"
		fi
		return 1
	fi
	if /usr/bin/rcp -r $2 "$3" 2>/tmp/rcp$$_$NUM_RCP 
	then
		echo success
	else
		echo $?
		echo failed
	fi > /tmp/rcp_res$$_$NUM_RCP &
	processid=$!
	ato rcpid$$_$NUM_RCP 5000 "CheckResult $1 /tmp/rcp$$_$NUM_RCP /tmp/rcp_res$$_$NUM_RCP $4 rcp rcp$$_$NUM_RCP $processid rcpid$$_$NUM_RCP"
	Report_CB $TOPLEVEL "$($GETTXT $REP_RCP_WORKING)" \
		$4 nocontrol /tmp/rcp$$_$NUM_RCP /tmp/rcp_res$$_$NUM_RCP \
		rcp$$_$NUM_RCP $processid rcpid$$_$NUM_RCP
	NUM_RCP=$NUM_RCP+1
}

# This function calls uucp to transfer file(s) and performs error checking
# $1 ==> parent widget 
# $2 ==> receiver's name 
# $3 ==> keyword (shell or box) for creating toplevel shell with box or
#	 dialogshell
do_uucp()
{
	TRACE
	if [ ! -x /usr/bin/uucp ]
	then
		if [ "$3" = "shell" ]
		then
			Message_CB $1 \
				"$($GETTXT $ERR_CannotExecute) $($GETTXT $LABEL_UUCP)" 0 0
		else
			Display_error $1 \
				"$($GETTXT $ERR_CannotExecute) $($GETTXT $LABEL_UUCP)" 
		fi
		return 1
	fi
	# Need to get the machine's base name only, not full gualified name
	# ${SYSTEM_NAME%%\.*} will strip off everything after and including 
	# the '.'.
	base=${SYSTEM_NAME%%\.*}
	if /usr/bin/uuto -p -m $SOURCE $base!$2 2>/tmp/uuto$$_$NUM_UUCP
	then
		echo success
	else
		echo $?
		echo failed
	fi > /tmp/uuto_res$$_$NUM_UUCP &
	processid=$!
	ato uutoid$$_$NUM_UUCP 5000 "CheckResult $1 /tmp/uuto$$_$NUM_UUCP /tmp/uuto_res$$_$NUM_UUCP $3 uuto uuto$$_$NUM_UUCP $processid uutoid$$_$NUM_UUCP"
	Report_CB $TOPLEVEL "$($GETTXT $REP_UUTO_WORKING2)" \
		$3 nocontrol /tmp/uuto$$_$NUM_UUCP \
		/tmp/uuto_res$$_$NUM_UUCP uuto$$_$NUM_UUCP \
		$processid uutoid$$_$NUM_UUCP
	NUM_UUCP=$NUM_UUCP+1
}

# When the system name field has been modified, sensitize the send button

fsysnameMod_CB()
{
	gv $fsystext value:tmp_name
	echo $tmp_name | read tmp
	if [ "$tmp" = "" ]
	then
		sv ${CA_WID[0]} sensitive:False
	else
		sv ${CA_WID[0]} sensitive:True
	fi
}

# When the RAC user presses Send button, the chosen method (uucp or rcp)
# is used to transfer file(s).

fsend_CB()
{
	TRACE
	gv $fsystext value:NSYSTEM_NAME
	if CheckHost "$NSYSTEM_NAME"
	then
            	return
	else
		SYSTEM_NAME=$NSYSTEM_NAME
	fi

	gv $rcvtext value:NRCV_LOGIN
	echo $NRCV_LOGIN | read tmp
	if [ "$tmp" = "" ]
	then
		Message_CB $TOPLEVEL "$($GETTXT $ERR_NoLoginName)" \
			3 ${CA_WID[0]}
		return
	else
		RCV_LOGIN=$NRCV_LOGIN
	fi

	if [ "$XFER_OPTION" = "both" ]
	then
		gv $fuucp set:FUUCP
	else
		if [ "$XFER_OPTION" = "UUCP" ]
		then
			FUUCP=true
		else
			FUUCP=false
		fi
	fi

	if [ $FUUCP = true ]
	then
		do_uucp $TOPLEVEL $RCV_LOGIN shell
	else
		gv $fcopytext value:COPY_FILE_TO
		echo $COPY_FILE_TO | read tmp
		if [ "$tmp" = "" ]
		then
			# default to $HOME directory
			DESTSRC="$SYSTEM_NAME:"
		else
			DESTSRC="$SYSTEM_NAME:$COPY_FILE_TO"
		fi
		do_rcp $TOPLEVEL "$SOURCE" $DESTSRC shell
	fi
}

# When Save button is pressed, all the data will be saved into the datafile.

fsaves_CB()
{
	TRACE
	gv $fsystext value:NSYSTEM_NAME
	echo $NSYSTEM_NAME | read tmp
	if [ "$tmp" != "" ]
	then
		if CheckHost "$tmp"
		then
			return
		fi
	fi

	SYSTEM_NAME=$tmp

	gv $rcvtext value:NLOGIN_NAME
	echo $NLOGIN_NAME | read tmp
	if [ "$tmp" = "" ]
	then
		LOGIN_NAME=""
	else
		LOGIN_NAME=$NLOGIN_NAME
	fi 

	if [ "$XFER_OPTION" = "both" ]
	then
		gv $fuucp set:uucp_val
		if [ $uucp_val = true ]
		then
			TRANSFER_FILE_USING=UUCP
		else
			TRANSFER_FILE_USING=RCP
		fi
		gv $fcopytext value:COPY_FILE_TO
	else
		if [ "$XFER_OPTION" = "UUCP" ]
		then
			TRANSFER_FILE_USING=UUCP
		else
			TRANSFER_FILE_USING=RCP
			gv $fcopytext value:COPY_FILE_TO
		fi	
	fi
		
	echo \
"SYSTEM_NAME=$SYSTEM_NAME
DUSER=$LOGIN_NAME
TRANSFER_FILE_TO=$TRANSFER_FILE_TO
TRANSFER_FILE_USING=$TRANSFER_FILE_USING
COPY_FILE_TO=$COPY_FILE_TO
CONN_CONFIRM=$CONN_CONFIRM
FTP_CONFIRM=$FTP_CONFIRM
XFER_OPTION=$XFER_OPTION" > $NODE_DIR/$ICON_NAME

	sv $fstatus labelString:"$($GETTXT $TXT_SaveSetting)"
}

fhelp_CB()
{
	TRACE
	call handle_to_widget "push_but" $1
	call copy_help $RET
}

fcancel_CB()
{
	exit 0
}

freset_CB()
{
	TRACE
	set_values
}

set_values() 
{
	TRACE
	if [ "$SYSTEM_NAME" = "" ]
	then
		sv ${CA_WID[0]} sensitive:False
	else
		sv ${CA_WID[0]} sensitive:True
	fi
	sv $fsystext value:$SYSTEM_NAME
	if [ "$LOGIN_NAME" = "" ]
	then
		LOGIN_NAME=$LOGNAME
	fi
	sv $rcvtext value:"${TRANSFER_FILE_TO:-$LOGIN_NAME}"

	if [ "$XFER_OPTION" = "both" ]
	then

		if [ "$TRANSFER_FILE_USING" = "" ]
		then
			sv $fuucp set:true
			sv $frcp set:false
			umc $fcopyfile
			umc $fcopytext
		else
			if [ $TRANSFER_FILE_USING = UUCP ]
			then
				sv $fuucp set:true
				sv $frcp set:false
				umc $fcopyfile
				umc $fcopytext
			else
				sv $frcp set:true
				sv $fuucp set:false
				mc $fcopyfile
				mc $fcopytext
			fi
			sv $fcopytext value:$COPY_FILE_TO
		fi
	else
		if [ "$XFER_OPTION" = "RCP" ]
		then
			sv $fcopytext value:$COPY_FILE_TO
		fi
	fi
		
}

# $2 ==> chkbox widget
# $3 ==> manager's widget to map/unmap
chkbox_CB()
{
	TRACE
	gv $1 set:value
	if [ $value = false ]
	then
		umc $2
	else
		mc $2
	fi
}

# $1 ==> rcp widget 
# $2 ==> label widget 
# $3 ==> textfield widget
frcp_CB()
{
	TRACE
	gv $1 set:value
	if [ $value = false ]
	then
		umc $2
		umc $3
		mc $fform2
	else
		umc $fform2
		mc $2
		mc $3
		#sv $3 value:$COPY_FILE_TO
	fi
}

fuucp_CB()
{
	TRACE
	gv $1 set:value
	if [ $value = false ]
	then
		umc $fform2
	else
		mc $fform2
	fi
}

# This function creates the file transfer window.
crt_ftp() 
{
	TRACE
	typeset -i len
	GetMaxWidth $TOPLEVEL
	cmw formc fromc form $TOPLEVEL
	cmw frc frc rowColumn $formc
	cmw fform1 fform1 form $frc

	cmw fsysname fsysname label $fform1 \
		labelString:"$($GETTXT $LABEL_SystemName)"
	sv $fsysname width:$maxWidth alignment:ALIGNMENT_END \
		leftAttachment:ATTACH_FORM topAttachment:ATTACH_FORM \
		bottomAttachment:ATTACH_FORM
	cmw fsystext fsystext textField $fform1
	sv $fsystext leftAttachment:ATTACH_WIDGET leftWidget:$fsysname \
		topAttachment:ATTACH_FORM bottomAttachment:ATTACH_FORM
	acb $fsystext valueChangedCallback 'fsysnameMod_CB'
	cmw flookup flookup pushButton $fform1 \
		labelString:"$($GETTXT $LABEL_lookup)"
	sv $flookup leftAttachment:ATTACH_WIDGET leftWidget:$fsystext \
		topAttachment:ATTACH_FORM bottomAttachment:ATTACH_FORM \
		activateCallback:'Lookup_CB $flookup $fsystext'

	if [ $INET = 0 ]
	then
		:
	else
		sv $flookup sensitive:False
	fi

	cmw fform2 fform2 form $frc
	cmw recieve recieve label $fform2 \
		labelString:"$($GETTXT $LABEL_UserRcvFiles)"
	sv $recieve width:$maxWidth alignment:ALIGNMENT_END \
		leftAttachment:ATTACH_FORM topAttachment:ATTACH_FORM \
		bottomAttachment:ATTACH_FORM
	cmw rcvtext rcvtext textField $fform2
	sv $rcvtext leftAttachment:ATTACH_WIDGET leftWidget:$recieve \
		topAttachment:ATTACH_FORM bottomAttachment:ATTACH_FORM

	cmw fform3 fform3 form $frc
	cmw filesnd filesnd label $fform3 \
		labelString:"$($GETTXT $LABEL_FileSnd)"
	sv $filesnd width:$maxWidth alignment:ALIGNMENT_END \
		leftAttachment:ATTACH_FORM topAttachment:ATTACH_FORM \
		bottomAttachment:ATTACH_FORM
	echo $SOURCE | read f1 f2 tmp
	if [ "$f2" = "" ]
	then
		# ${f1##*/} will just get the basename of the path
		fn="${f1##*/}"
	else
		fn="${f1##*/}, ${f2##*/}"
	fi
	if [ "$tmp" != "" ]
	then
		fn="$fn, ..."
	fi
	cmw filename filename label $fform3 labelString:"$fn"
	sv $filename alignment:ALIGNMENT_BEGINNING \
		leftAttachment:ATTACH_WIDGET \
		leftWidget:$filesnd topAttachment:ATTACH_FORM \
		bottomAttachment:ATTACH_FORM

	cmw fform4 fform4 form $frc
	cmw fchkbox fchkbox toggleButton $fform4 \
		labelString:"$($GETTXT $LABEL_ShowOtherOpt)"
	sv $fchkbox leftAttachment:ATTACH_FORM topAttachment:ATTACH_FORM \
		bottomAttachment:ATTACH_FORM

	addcols $frc fmiddle

	cmw fform5 fform5 form $fmiddle
	cmw fusing fusing label $fform5 \
		labelString:"$($GETTXT $LABEL_XferFilesU)"
	sv $fusing width:$maxWidth leftAttachment:ATTACH_FORM \
		alignment:ALIGNMENT_END topAttachment:ATTACH_FORM \
		bottomAttachment:ATTACH_FORM

	if [ "$XFER_OPTION" = "both" ]
	then
		XmCreateRadioBox fradio $fform5 fradio orientation:HORIZONTAL
		sv $fradio leftAttachment:ATTACH_WIDGET leftWidget:$fusing \
			topAttachment:ATTACH_FORM bottomAttachment:ATTACH_FORM \
			radioBehavior:true isHomogenious:true
		cmw fuucp fuucp toggleButton $fradio set:true \
			indicatorType:ONE_OF_MANY visibleWhenOff:true \
			labelString:"$($GETTXT $LABEL_UUCP)"
		sv $fuucp valueChangedCallback:'fuucp_CB $fuucp'
		cmw frcp frcp toggleButton $fradio \
			indicatorType:ONE_OF_MANY visibleWhenOff:true \
			labelString:"$($GETTXT $LABEL_RCP)"
		mc $fradio
		
		cmw fform6 fform6 form $fmiddle
		cmw fcopyfile fcopyfile label $fform6 \
			labelString:"$($GETTXT $LABEL_CopyFilesTo)"
		sv $fcopyfile alignment:ALIGNMENT_END width:$maxWidth \
			leftAttachment:ATTACH_FORM \
			topAttachment:ATTACH_FORM bottomAttachment:ATTACH_FORM

		cmw fcopytext fcopytext textField $fform6
		sv $fcopytext leftAttachment:ATTACH_WIDGET \
			leftWidget:$fcopyfile \
			topAttachment:ATTACH_FORM bottomAttachment:ATTACH_FORM
		sv $frcp valueChangedCallback:'frcp_CB $frcp $fcopyfile $fcopytext'
	else
		# Only one of them exists, just create label for it.
		cmw fxferu fxferu label $fform5
		sv $fxferu alignment:ALIGNMENT_BEGINNING \
			leftAttachment:ATTACH_WIDGET leftWidget:$fusing \
			topAttachment:ATTACH_FORM bottomAttachment:ATTACH_FORM
		if [ "$XFER_OPTION" = "UUCP" ]
		then
			sv $fxferu labelString:"$($GETTXT $LABEL_UUCP)"
		else
			sv $fxferu labelString:"$($GETTXT $LABEL_RCP)"
			cmw fform6 fform6 form $fmiddle
			cmw fcopyfile fcopyfile label $fform6 \
				labelString:"$($GETTXT $LABEL_CopyFilesTo)"
			sv $fcopyfile alignment:ALIGNMENT_END width:$maxWidth \
				leftAttachment:ATTACH_FORM \
				topAttachment:ATTACH_FORM \
				bottomAttachment:ATTACH_FORM

			cmw fcopytext fcopytext textField $fform6
			sv $fcopytext leftAttachment:ATTACH_WIDGET \
				leftWidget:$fcopyfile \
				topAttachment:ATTACH_FORM \
				bottomAttachment:ATTACH_FORM
		fi	
	fi

	sv $fchkbox set:False valueChangedCallback:'chkbox_CB $fchkbox $fmiddle'

	XmCreateSeparator -m FSEP1 $frc "fseparator"
	cmw fbutrc fbutrc rowColumn $frc orientation:horizontal
	CONTROL_AREA[0]="$LABEL_Send"
	CONTROL_AREA[1]="$LABEL_SaveS"
	CONTROL_AREA[2]="$LABEL_Reset"
	CONTROL_AREA[3]="$LABEL_Cancel"
	CONTROL_AREA[4]="$LABEL_Help"
	CA_CB[0]=fsend_CB
	CA_CB[1]=fsaves_CB
	CA_CB[2]=freset_CB
	CA_CB[3]=fcancel_CB
	CA_CB[4]="fhelp_CB $frc"
	call handle_to_widget "fform" $fform2
	call getlen $RET
	tmp=$RET
	l=$(printf '%d' $tmp)
	CrtActionArea $fbutrc $l
	
	sv ${CA_WID[0]} sensitive:False
	sv $formc defaultButton:${CA_WID[0]}
	sv $formc cancelButton:${CA_WID[3]}
	#sv $formc initFocus:$fsystext

	acb $frc helpCallback 'fhelp_CB $frc'

	XmCreateSeparator -m FSEP2 $frc "fseparator"
	cmw fstatus fstatus label $frc labelString:" "
	set_values

	# change the status line to different messages
	Focus_CB $fsystext $fstatus "$($GETTXT $TXT_XferSystemName)"
	Focus_CB $rcvtext $fstatus "$($GETTXT $TXT_XferFileTo2)"
	Focus_CB $fchkbox $fstatus "$($GETTXT $TXT_OtherOption2)"
	if [ "$XFER_OPTION" = "both" ]
	then
		Focus_CB $fuucp $fstatus "$($GETTXT $TXT_Uucp2)"
		Focus_CB $frcp $fstatus "$($GETTXT $TXT_Rcp)"
		Focus_CB $fcopytext $fstatus "$($GETTXT $TXT_CopyFileTo2)"
	else
		if [ "$XFER_OPTION" = "RCP" ]
		then
			Focus_CB $fcopytext $fstatus "$($GETTXT $TXT_CopyFileTo2)"
		fi
	fi

	gv $fchkbox set:fchkbox_state
	if [ $fchkbox_state = false ]
	then
		umc $fmiddle
	fi

	if [ "$XFER_OPTION" = "both" ]
	then
		gv $frcp set:frcp_state
		if [ $frcp_state = false ]
		then
			umc $fcopyfile
			umc $fcopytext
		fi
	fi
}

# Main function starts here
TRACE
ai TOPLEVEL rac_ftp Rac_ftp \
	-title "$($GETTXT $TXT_FxferTitle)" \
	"$@"

#set "${ARG[@]}"
sv $TOPLEVEL allowShellResize:True

if [ $# -lt 3 ]
then
	echo "less than 3, $#, $1,$2,$3" > /tmp/out
	echo "$($GETTXT $ERR_CopyUsage)"
	Display_error $TOPLEVEL "$($GETTXT $ERR_CopyUsage)"
	exit 1
fi

while getopts r: opt
do
	case $opt in
	r)	ICON_NAME=$OPTARG;;
	\?)	echo "$($GETTXT $ERR_CopyUsage)"
		Display_error $TOPLEVEL "$($GETTXT $ERR_CopyUsage)"
		exit 1;;
	esac
done

shift 2

SOURCE="$@"

NODE_DIR=$HOME/.node
if [ ! -f $NODE_DIR/$ICON_NAME ]
then
	MSG=$($GETTXT $ERR_NoFile2)
	MSG=`printf "$MSG" $NODE_DIR/$ICON_NAME $ICON_NAME`
	Display_error $TOPLEVEL "$MSG"
else
	Readin $ICON_NAME

	if [[ "$SYSTEM_NAME" = "" || "$FTP_CONFIRM" != "false" ]]
	then
		Rm_mwm_funcs $TOPLEVEL
		# Before creating the properties sheet, check if the UUCP
		# and RCP are available or not.
		if [[ ! -x /usr/bin/rcp && ! -x /usr/bin/uuto ]]
		then
			Display_error $TOPLEVEL "$($GETTXT $ERR_NoNetWork2)"
		else
			if [ ! -x /usr/bin/rcp ]
			then
				XFER_OPTION=UUCP
			fi
			if [ ! -x /usr/bin/uuto ]
			then
				XFER_OPTION=RCP
			fi
		fi
		crt_ftp
		rw $TOPLEVEL
		ml
	else
		# use the information getting from readin.sh
		if CheckHost "$SYSTEM_NAME"
		then
			Display_error $TOPLEVEL "$($GETTXT $ERR_NoSystemName)"
		fi

		if [ "$TRANSFER_FILE_USING" = "UUCP" ]
		then
			echo $TRANSFER_FILE_TO | read tmp
			if [ "$tmp" = "" ]
			then
				# silently retreive the user's login
				TRANSFER_FILE_TO=$LOGNAME
				
			fi
			do_uucp $TOPLEVEL $TRANSFER_FILE_TO box
			rw $TOPLEVEL
			ml
		else
			echo $COPY_FILE_TO | read tmp
			if [ "$tmp" = "" ]
			then
				DESTSRC="$SYSTEM_NAME"
			else
				DESTSRC="$SYSTEM_NAME:$COPY_FILE_TO"
			fi
			do_rcp $TOPLEVEL "$SOURCE" $DESTSRC box
			rw $TOPLEVEL
			ml
		fi
	fi
fi
