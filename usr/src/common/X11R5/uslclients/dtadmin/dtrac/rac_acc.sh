#!/usr/X/bin/wksh -motif
#ident	"@(#)dtrac:rac_acc.sh	1.15"

# Functionality: This script is called when desktop user double-click 
#		the friend node icon on the desktop folder.  The user 
#		will be connected to the remote system either thru 
#		graphical TFP or rlogin. The access window will only
#		be displayed whent the "Always confirm" for the Connections
#		is set.
#
# Purposes of the other files:
#
#       - libdtnet.so has all the library functions that will be called
#         by this client. e.g nslookup.
#       - rac.msgs contains all the different kinds of messages used
#         in this client. The messages are in I18N format and can be
#         translated in other locales.
#       - misc.sh contains shell interfaces to the C functions.

# To trace this shell script, export MYDEBUG=1 before invoking this script
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

# This function calls xterm to invoke the rlogin.
# $1 ==> parent widget
# $2 ==> system name
# $3 ==> key word for creating box or dialogshell
do_login()
{
	TRACE
	if [ ! -x ${XWINHOME}/bin/xterm ]
	then
		if [ $3 = "shell" ]
		then
			Message_CB $1 \
				"$($GETTXT $ERR_CannotExecute) $($GETTXT $LABEL_XTERM)" 0 0
		else
			Display_error $1 \
				"$($GETTXT $ERR_CannotExecute) $($GETTXT $LABEL_XTERM)"
		fi
		return 1
	fi
	if [ "$XFER_OPTION" = "UUCP" ]
	then
		# use cu instead of rlogin
		if [ ! -x /usr/bin/cu ]
		then
			if [ $3 = "shell" ]
			then
				Message_CB $1 \
					"$($GETTXT $ERR_CannotExecute) $($GETTXT $LABEL_CU)" 0 0
			else
				Display_error $1 \
					"$($GETTXT $ERR_CannotExecute) $($GETTXT $LABEL_CU)"
			fi
			return 1
		else
			${XWINHOME}/bin/xterm -T $2 -E /usr/bin/cu $2 &
		fi
	else
		if [ ! -x /usr/bin/rlogin ]
		then
			# BUG - if rlogin is not available, we should try cu
			if [ $3 = "shell" ]
			then
				Message_CB $1 \
					"$($GETTXT $ERR_CannotExecute) $($GETTXT $LABEL_RLOGIN)" 0 0
			else
				Display_error $1 \
					"$($GETTXT $ERR_CannotExecute) $($GETTXT $LABEL_RLOGIN)"
			fi
			return 1
		else
			${XWINHOME}/bin/xterm -T $2 -E /usr/bin/rlogin $2 -l $LOGIN_NAME &
		fi
	fi
	sleep 2
	exit 0
}

# If the system name field has been modified, sensitize the connect button

sysnameMod_CB()
{
	gv $asystext value:tmp_name
	echo $tmp_name | read tmp
	if [ "$tmp" = "" ]
	then
		sv ${CA_WID[0]} sensitive:False
	else
		sv ${CA_WID[0]} sensitive:True
	fi
}

# When the Connect button is pressed, the user will be connected to remote
# system thru the chosen method (e.g  Graphical FTP or rlogin).

aconnect_CB()
{
	TRACE
	gv $asystext value:NSYSTEM_NAME
	if CheckHost "$NSYSTEM_NAME"
	then
		return
	else
		SYSTEM_NAME=$NSYSTEM_NAME
	fi

	gv $alogintext value:NLOGIN_NAME
	echo $NLOGIN_NAME | read tmp
	if [ "$tmp" = "" ]
	then
		Message_CB $TOPLEVEL "$($GETTXT $ERR_NoLoginName)" \
			3 ${CA_WID[0]}
		return
	else
		LOGIN_NAME=$NLOGIN_NAME
	fi
	do_login $TOPLEVEL $SYSTEM_NAME shell

	#ACANCEL_CB
}

ahelp_CB()
{
	TRACE
	call handle_to_widget "push_but" $1
	call access_help $RET
}

acancel_CB()
{
	exit 0
}

asaves_CB()
{
	TRACE
	gv $asystext value:NSYSTEM_NAME

	# don't check the system name if it is blank, user can leave it blank
	echo $NSYSTEM_NAME | read tmp
	if [ "$tmp" != "" ]
	then
		if CheckHost "$tmp"
		then
			return
		else
			SYSTEM_NAME=$tmp
		fi
	else
		SYSTEM_NAME=$tmp
	fi
	gv $alogintext value:NLOGIN_NAME
	echo $NLOGIN_NAME | read tmp
	if [ "$tmp" = "" ]
	then
		LOGIN_NAME=""
	else
		LOGIN_NAME=$NLOGIN_NAME
	fi
	# save the values back to the data file
	echo \
	"SYSTEM_NAME=$SYSTEM_NAME
DUSER=$LOGIN_NAME
TRANSFER_FILE_TO=$TRANSFER_FILE_TO
TRANSFER_FILE_USING=$TRANSFER_FILE_USING
COPY_FILE_TO=$COPY_FILE_TO
CONN_CONFIRM=$CONN_CONFIRM
FTP_CONFIRM=$FTP_CONFIRM
XFER_OPTION=$XFER_OPTION" > $NODE_DIR/$ICON_NAME

	sv $astatus labelString:"$($GETTXT $TXT_SaveSetting)"	
}

areset_CB()
{
	set_values
}

set_values() {
	TRACE

	if [ "$SYSTEM_NAME" = "" ]
	then
		sv ${CA_WID[0]} sensitive:False
	else	
		sv ${CA_WID[0]} sensitive:True
	fi
	sv $asystext value:$SYSTEM_NAME
	if [ "$LOGIN_NAME" = "" ]
	then
		LOGIN_NAME=$LOGNAME
	fi
	sv $alogintext value:$LOGIN_NAME
}

# This function creates the access window.
crt_access() 
{
	TRACE
	typeset -i len
	# calculate the maximun width for the labels
	GetMaxWidth $TOPLEVEL	

	cmw forma forma form $TOPLEVEL
	cmw arc arc rowColumn $forma orientation:vertical
	sv $TOPLEVEL allowShellResize:True
	cmw aform1 aform1 form $arc

	# System Name field: label, text and a push button
	cmw sysname sysname label $aform1 \
		labelString:"$($GETTXT $LABEL_SystemName)"
	sv $sysname width:$maxWidth alignment:ALIGNMENT_END \
		leftAttachment:ATTACH_FORM topAttachment:ATTACH_FORM \
		bottomAttachment:ATTACH_FORM
	cmw asystext asystext textField $aform1
	sv $asystext leftAttachment:ATTACH_WIDGET leftWidget:$sysname \
		topAttachment:ATTACH_FORM bottomAttachment:ATTACH_FORM
	acb $asystext valueChangedCallback 'sysnameMod_CB'
	cmw alookup alookup pushButton $aform1 \
		labelString:"$($GETTXT $LABEL_lookup)"
	sv $alookup leftAttachment:ATTACH_WIDGET leftWidget:$asystext \
		topAttachment:ATTACH_FORM bottomAttachment:ATTACH_FORM \
		activateCallback:'Lookup_CB $alookup $asystext'

	if [ $INET = 0 ]
	then
		:
	else
		sv $alookup sensitive:False
	fi

	# Login field: label and text
	cmw aform2 aform2 form $arc
	cmw aloginlabel aloginlabel label $aform2 \
		labelString:"$($GETTXT $LABEL_LoginToSysAs)"
	sv $aloginlabel width:$maxWidth alignment:ALIGNMENT_END \
		leftAttachment:ATTACH_FORM topAttachment:ATTACH_FORM \
		bottomAttachment:ATTACH_FORM
	cmw alogintext alogintext textField $aform2
	sv $alogintext leftAttachment:ATTACH_WIDGET leftWidget:$aloginlabel \
		topAttachment:ATTACH_FORM bottomAttachment:ATTACH_FORM

	XmCreateSeparator -m ASEP1 $arc "aseperator"
	cmw abutrc abutrc rowColumn $arc orientation:horizontal
	CONTROL_AREA[0]="$LABEL_Connect2"
	CONTROL_AREA[1]="$LABEL_SaveS"
	CONTROL_AREA[2]="$LABEL_Reset"
	CONTROL_AREA[3]="$LABEL_Cancel"
	CONTROL_AREA[4]="$LABEL_Help"
	CA_CB[0]=aconnect_CB
	CA_CB[1]=asaves_CB
	CA_CB[2]=areset_CB
	CA_CB[3]=acancel_CB
	CA_CB[4]="ahelp_CB $arc"
	
	call handle_to_widget "aform" $aform2
	call getlen $RET
	tmp=$RET
	l=$(printf '%d' $tmp)
	CrtActionArea $abutrc $l

	sv ${CA_WID[0]} sensitive:False
	sv $forma defaultButton:${CA_WID[0]}
	sv $forma cancelButton:${CA_WID[3]}
	#sv $forma initialFocus:$asystext

	acb $arc helpCallback 'ahelp_CB $arc'

	XmCreateSeparator -m ASEP2 $arc "aseperator"
	cmw astatus astatus label $arc labelString:" "

	set_values
	
	# change the status line to display different messages
	Focus_CB $asystext $astatus "$($GETTXT $TXT_EnterSystemName)"
	Focus_CB $alogintext $astatus "$($GETTXT $TXT_EnterLoginName)"
}

# Main function starts here
TRACE

ai TOPLEVEL rac_acc Rac_acc \
	-title "$($GETTXT $TXT_AccTitle2)" \
	"$@"

# Take out all the X/tookit known arguments, leave the rest to this 
# application

#set "${ARG[@]}"

if [ $# -lt 1 ]
then
	echo "$($GETTXT $ERR_AccessUsage)"
	Display_error $TOPLEVEL "$($GETTXT $ERR_AccessUsage)"
	exit 1
fi

while getopts fr: opt
do
	case $opt in
	r)	ICON_NAME=$OPTARG;;
	\?) 	echo "$($GETTXT $ERR_AccessUsage)"
		Display_error $TOPLEVEL "$($GETTXT $ERR_AccessUsage)"
		exit 1;;
	esac
done
		
# Temporay directory, will be removed since the file is in current 
# folder
NODE_DIR=$HOME/.node
if [ ! -f $NODE_DIR/$ICON_NAME ]
then
	MSG=$($GETTXT $ERR_NoFile2)
	MSG=`printf "$MSG" $NODE_DIR/$ICON_NAME $ICON_NAME`
	Display_error $TOPLEVEL "$MSG"
else
	Readin $ICON_NAME

	if [[ "$SYSTEM_NAME" = "" || "$CONN_CONFIRM" != "false" ]]
	then
		Rm_mwm_funcs $TOPLEVEL
		crt_access
		rw $TOPLEVEL
		ml
	else
		# use the information getting from readin.sh

		if CheckHost "$SYSTEM_NAME"
		then
			Display_error $TOPLEVEL "$($GETTXT $ERR_NoSystemName)"
		fi

		echo $LOGIN_NAME | read tmp
		if [ "$tmp" = "" ]
		then
			# should selently retrieve the user's login
			LOGIN_NAME=$LOGNAME
		fi
		do_login $TOPLEVEL $SYSTEM_NAME box
	fi
fi
