#!/usr/X/bin/wksh -motif
#ident	"@(#)dtrac:rac_prop.sh	1.12"

# Functionality: This script is called when desktop user selects 
#		the Properties item on the command menu.  User can 
#		fill out the information about remote access and 
#		file transfer properties.
#
# Purposes of the other files:
#
# 	- libdtnet.so has all the library functions that will be called 
#	  by this client. e.g nslookup. 
# 	- rac.msgs contains all the different kinds of messages used 
#	  in this client. The messages are in I18N format and can be 
#	  translated in other locales.
#	- misc.sh contains shell interfaces to the C functions.

# To trace this program, export MYDEBUG=1 before invoking this script.

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

#libload ${XWINHOME}/lib/libdtnet.so
`pkginfo -i inet > /dev/null 2>&1`
INET=$?

if [ $INET = 0 ]
then
	libload ${XWINHOME}/lib/libdtnet.so
else
	libload ${XWINHOME}/lib/libdtrac.so
fi

# When the desktop user presses OK button, all the data will be saved 
# into the datafile. Error checking is done on certain fields.  Any
# invalid entry will be flaged and the control is returned back to
# main loop.

ok_CB()
{
	TRACE
	gv $systext value:NSYSTEM_NAME

	# don't check the system name if it is blank, user can leave it blank
	echo $NSYSTEM_NAME | read tmp
	if [ "$tmp" != "" ]
	then
		if CheckHost "$tmp"
		then
			return
		fi
	fi

	SYSTEM_NAME=$tmp

	# Use a new name here, so the old valid value will not be 
	# corrupted and the reset function can restore the old value.
	gv $logintext value:NLOGIN_NAME
	echo $NLOGIN_NAME | read tmp
	if [ "$tmp" = "" ]
	then
		LOGIN_NAME=""
	else
		LOGIN_NAME=$NLOGIN_NAME
	fi

	gv $transtext value:NTRANSFER_FILE_TO
	echo $NTRANSFER_FILE_TO | read tmp
	if [ "$tmp" = "" ]
	then
		TRANSFER_FILE_TO=""
	else
		TRANSFER_FILE_TO=$NTRANSFER_FILE_TO
	fi
	
	if [ "$XFER_OPTION" = "both" ]
	then
		gv $uucp set:UUCP_VAL
		if [ $UUCP_VAL = true ]
		then
			TRANSFER_FILE_USING=UUCP
		else
			TRANSFER_FILE_USING=RCP
		fi
		gv $copytext value:COPY_VAL
	else
		if [ "$XFER_OPTION" = "UUCP" ]
		then
			TRANSFER_FILE_USING=UUCP
		else
			TRANSFER_FILE_USING=RCP
			gv $copytext value:COPY_VAL
		fi
	fi

	gv $connections set:CONN_VAL
	gv $ftp set:FTP_VAL

	# After getting all values and doing the error checking, 
	# update the database.

	echo \
	"SYSTEM_NAME=$SYSTEM_NAME
DUSER=$LOGIN_NAME
TRANSFER_FILE_TO=$TRANSFER_FILE_TO
TRANSFER_FILE_USING=$TRANSFER_FILE_USING
COPY_FILE_TO=$COPY_VAL
CONN_CONFIRM=$CONN_VAL
FTP_CONFIRM=$FTP_VAL
XFER_OPTION=$XFER_OPTION" > $NODE_DIR/$ICON_NAME

	exit 0	
}

cancel_CB()
{
	exit 0
}

# $1 ==> help button widget
help_CB()
{
	TRACE
	call handle_to_widget "push_but" $1
	call prop_help $RET
}

# Reset will set the values back to the last saved values.
reset_CB()
{
	TRACE
	echo "hello there"
	set_values
}

# Map and unmap the TRANSFER_FILE_TO fields
# $1 ==> chkbox widget 
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
# $3 ==> textField widget
rcp_CB()
{
	TRACE
	gv $1 set:value
	if [ $value = false ]
	then
		umc $2
		umc $3
	else
		mc $2
		mc $3
	fi
}

set_values()
{
	TRACE
	sv $systext value:$SYSTEM_NAME

	if [ "$LOGIN_NAME" = "" ]
	then
		# take the default user login
		LOGIN_NAME=$LOGNAME
	fi		
	sv $logintext value:$LOGIN_NAME

	sv $transtext value:"${TRANSFER_FILE_TO:-$LOGIN_NAME}"

	if [ "$XFER_OPTION" = "both" ]
	then
		if [ "$TRANSFER_FILE_USING" = "" ]
		then
			sv $uucp set:TRUE
			sv $rcp set:FALSE
			umc $copyfile
			umc $copytext
		else
			if [ $TRANSFER_FILE_USING = UUCP ]
			then
				sv $uucp set:TRUE
				sv $rcp set:FALSE
				umc $copyfile
				umc $copytext
			else
				sv $rcp set:TRUE
				sv $uucp set:FALSE
				mc $copyfile
				mc $copytext
			fi
		fi
		sv $copytext value:$COPY_FILE_TO
	else
		if [ "$XFER_OPTION" = "RCP" ]
		then
			sv $copytext value:$COPY_FILE_TO
		fi
	fi

	if [[ "$CONN_CONFIRM" = "" && "$FTP_CONFIRM" = "" ]]
	then
		sv $ftp set:TRUE
		# turn on the connection confirm all the time
		sv $connections set:TRUE
	else
		if [ "$CONN_CONFIRM" = "true" ]
		then
			sv $connections set:TRUE
		fi
		if [ "$FTP_CONFIRM" = "true" ]
		then
			sv $ftp set:TRUE
		fi
	fi
}

# This function creates the properties window. 
crt_connprop() 
{
	TRACE
	typeset -i len
	# calcuate the maximun width for the labels
	GetMaxWidth $TOPLEVEL
	
	sv $TOPLEVEL allowShellResize:True
	cmw formp formp form $TOPLEVEL
	cmw PRC prc rowColumn $formp orientation:vertical 
	cmw form1 form1 form $PRC

	cmw syslabel syslabel label $form1 \
		labelString:"$($GETTXT $LABEL_SystemName)"
	sv $syslabel width:$maxWidth alignment:ALIGNMENT_END \
		leftAttachment:ATTACH_FORM topAttachment:ATTACH_FORM \
		bottomAttachment:ATTACH_FORM
	cmw systext systext textField $form1
	sv $systext leftAttachment:ATTACH_WIDGET leftWidget:$syslabel \
		topAttachment:ATTACH_FORM bottomAttachment:ATTACH_FORM

	cmw lookup lookup pushButton $form1 \
		labelString:"$($GETTXT $LABEL_lookup)"
	sv $lookup leftAttachment:ATTACH_WIDGET leftWidget:$systext \
		topAttachment:ATTACH_FORM bottomAttachment:ATTACH_FORM \
		activateCallback:"Lookup_CB $lookup $systext"

	if [ $INET = 0 ]
	then
		:
	else
		sv $lookup sensitive:False
	fi

	cmw form2 form2 form $PRC
	cmw loginlabel loginlabel label $form2 \
		labelString:"$($GETTXT $LABEL_LoginToSysAs)"
	sv $loginlabel width:$maxWidth alignment:ALIGNMENT_END \
		leftAttachment:ATTACH_FORM topAttachment:ATTACH_FORM \
		bottomAttachment:ATTACH_FORM
	cmw logintext logintext textField $form2 
	sv $logintext leftAttachment:ATTACH_WIDGET leftWidget:$loginlabel \
		topAttachment:ATTACH_FORM bottomAttachment:ATTACH_FORM
	
	cmw form4 form4 form $PRC
	cmw translabel translabel label $form4 \
		labelString:"$($GETTXT $LABEL_XferFilesTo)"
	sv $translabel width:$maxWidth alignment:ALIGNMENT_END \
		leftAttachment:ATTACH_FORM topAttachment:ATTACH_FORM \
		bottomAttachment:ATTACH_FORM
	cmw transtext transtext textField $form4
	sv $transtext leftAttachment:ATTACH_WIDGET leftWidget:$translabel \
		topAttachment:ATTACH_FORM bottomAttachment:ATTACH_FORM

	# other options
	cmw form5 form5 form $PRC
	cmw chkbox chkbox toggleButton $form5 \
		labelString:"$($GETTXT $LABEL_ShowOtherOpt)"
	sv $chkbox leftAttachment:ATTACH_FORM topAttachment:ATTACH_FORM \
		bottomAttachment:ATTACH_FORM

	# another rowColumn - pmiddle - for the Options part
	cmw pmiddle pmiddle rowColumn $PRC orientation:vertical
	cmw form6 form6 form $pmiddle
	cmw using using label $form6 \
		labelString:"$($GETTXT $LABEL_XferFilesU)"
	sv $using width:$maxWidth leftAttachment:ATTACH_FORM \
		alignment:ALIGNMENT_END topAttachment:ATTACH_FORM \
		bottomAttachment:ATTACH_FORM

	if [ "$XFER_OPTION" = "both" ]
	then
		XmCreateRadioBox uradio $form6 uradio orientation:HORIZONTAL
		sv $uradio leftAttachment:ATTACH_WIDGET leftWidget:$using \
			topAttachment:ATTACH_FORM bottomAttachment:ATTACH_FORM
		cmw uucp uucp toggleButton $uradio set:true \
			labelString:"$($GETTXT $LABEL_UUCP)"
		cmw rcp rcp toggleButton $uradio set:false \
			labelString:"$($GETTXT $LABEL_RCP)"
		mc $uradio
		
		cmw form7 form7 form $pmiddle
		cmw copyfile copyfile label $form7 \
			labelString:"$($GETTXT $LABEL_CopyFilesTo)"
		sv $copyfile alignment:ALIGNMENT_END width:$maxWidth \
			leftAttachment:ATTACH_FORM \
			topAttachment:ATTACH_FORM bottomAttachment:ATTACH_FORM
		cmw copytext copytext textField $form7
		sv $copytext leftAttachment:ATTACH_WIDGET leftWidget:$copyfile \
			topAttachment:ATTACH_FORM bottomAttachment:ATTACH_FORM
		sv $rcp valueChangedCallback:'rcp_CB $rcp $copyfile $copytext'
	else
		# Only one of them exists, just create a label for it.
		cmw xferu xferu label $form6
		sv $xferu alignment:ALIGNMENT_BEGINNING \
			leftAttachment:ATTACH_WIDGET leftWidget:$using \
			topAttachment:ATTACH_FORM bottomAttachment:ATTACH_FORM
	
		if [ "$XFER_OPTION" = "UUCP" ]
		then
			sv $xferu labelString:"$($GETTXT $LABEL_UUCP)"
		else
			sv $xferu labelString:"$($GETTXT $LABEL_RCP)"
			cmw form7 form7 form $pmiddle
			cmw copyfile copyfile label $form7 \
				labelString:"$($GETTXT $LABEL_CopyFilesTo)"
			sv $copyfile alignment:ALIGNMENT_END width:$maxWidth \
				leftAttachment:ATTACH_FORM \
				topAttachment:ATTACH_FORM bottomAttachment:ATTACH_FORM
			cmw copytext copytext textField $form7
			sv $copytext leftAttachment:ATTACH_WIDGET \
				leftWidget:$copyfile \
				topAttachment:ATTACH_FORM \
				bottomAttachment:ATTACH_FORM
		fi
	fi

	cmw form8 form8 form $pmiddle
	cmw confirmlabel confirmlabel label $form8 \
		labelString:"$($GETTXT $LABEL_AlwaysConfirm)"
	sv $confirmlabel width:$maxWidth leftAttachment:ATTACH_FORM \
		alignment:ALIGNMENT_END topAttachment:ATTACH_FORM \
		bottomAttachment:ATTACH_FORM
	addrows $form8 confirm
	sv $confirm leftAttachment:ATTACH_WIDGET leftWidget:$confirmlabel \
		topAttachment:ATTACH_FORM bottomAttachment:ATTACH_FORM
	cmw connections connections toggleButton $confirm \
		labelString:"$($GETTXT $LABEL_Connections)"
	cmw ftp ftp toggleButton $confirm \
		labelString:"$($GETTXT $LABEL_FileXfers)"


	sv $chkbox set:False valueChangedCallback:'chkbox_CB $chkbox $pmiddle'

	XmCreateSeparator -m SEP1 $PRC "seperator"
	# control area - add the reset button
	cmw butrc butrc rowColumn $PRC orientation:horizontal
	CONTROL_AREA[0]="$LABEL_OK"
	CONTROL_AREA[1]="$LABEL_Reset"
	CONTROL_AREA[2]="$LABEL_Cancel"
	CONTROL_AREA[3]="$LABEL_Help"
	CA_CB[0]=ok_CB
	CA_CB[1]=reset_CB
	CA_CB[2]=cancel_CB
	CA_CB[3]="help_CB $PRC"
	call handle_to_widget "form" $form2
	call getlen $RET
	tmp=$RET
	l=$(printf '%d' $tmp)	
	CrtActionArea $butrc $l

	acb $PRC helpCallback 'help_CB $PRC'
	sv $formp defaultButton:${CA_WID[0]}
	sv $formp cancelButton:${CA_WID[2]}

	#sv $form1 initialFoucs:$systext
	#move the initial focus to the textField
	#call handle_to_widget "pushbut" $lookup
	#tmp=$RET
	#call setLeftFocus $tmp

	XmCreateSeparator -m SEP2 $PRC "seperator"
	cmw status status label $PRC labelString:" "
	set_values

	# change the status line to display different messages 
	Focus_CB $systext $status "$($GETTXT $TXT_EnterSystemName)"
	Focus_CB $logintext $status "$($GETTXT $TXT_EnterLoginName)"
	Focus_CB $transtext $status "$($GETTXT $TXT_XferFileTo2)"
	Focus_CB $chkbox $status "$($GETTXT $TXT_OtherOption2)"
	if [ "$XFER_OPTION" = "both" ]
	then
		Focus_CB $uucp $status "$($GETTXT $TXT_Uucp2)"
		Focus_CB $rcp $status "$($GETTXT $TXT_Rcp)"
		Focus_CB $copytext $status "$($GETTXT $TXT_CopyFileTo2)"
	else
		if [ "$XFER_OPTION" = "RCP" ]
		then
			Focus_CB $copytext $status "$($GETTXT $TXT_CopyFileTo2)"
		fi
	fi
	Focus_CB $connections $status "$($GETTXT $TXT_Connections2)"
	Focus_CB $ftp $status "$($GETTXT $TXT_FileXfer2)"

	gv $chkbox set:chkbox_state
	if [ $chkbox_state = false ]
	then
		umc $pmiddle
	fi

	if [ "$XFER_OPTION" = "both" ]
	then
		gv $rcp set:rcp_state
		if [ $rcp_state = false ]
		then
			umc $copyfile
			umc $copytext
		fi 
	fi
}

# Main function starts here
TRACE
ai TOPLEVEL rac Rac \
	-title "$($GETTXT $TXT_PropTitle)" \
	"$@"

# take out all the arguments that known by X and toolkit, leave 
# those known by this application.

#set "${ARG[@]}" 

if [ $# != 2 ]
then
	echo "$($GETTXT $ERR_PropUsage)"
	Display_error $TOPLEVEL "$($GETTXT $ERR_PropUsage)"
	exit 1
fi

while getopts r: opt
do
	case $opt in
	r)	ICON_NAME=$OPTARG;;
	\?)	
		echo "$($GETTXT $ERR_PropUsage)"
		Display_error $TOPLEVEL "$($GETTXT $ERR_PropUsage)"
		exit 1
		;;
	esac
done

# temporary directory, in the future, this should reside on the current dir.
NODE_DIR=$HOME/.node
if [ -f $NODE_DIR/$ICON_NAME ]
then
	# read the data file in and set up the variables

	Readin $ICON_NAME
	# remove the mwm functions - Size, Minimize and Maximize
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
			
		# create the property sheet
		crt_connprop 
		rw $TOPLEVEL
		ml
	fi

else
	MSG=$($GETTXT $ERR_NoFile2)
	MSG=`printf "$MSG" $NODE_DIR/$ICON_NAME $ICON_NAME`
	Display_error $TOPLEVEL "$MSG"
fi
