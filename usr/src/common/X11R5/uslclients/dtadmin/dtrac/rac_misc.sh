#ident	"@(#)dtrac:rac_misc.sh	1.5"
# This file contains all the shell functions that will call the C functions.

# To trace the functions, export MYDEBUG=1 before invoking this script
if [ "$MYDEBUG" = "" ]
then
	alias TRACE=:
else
	alias TRACE="set -x"
fi

# These variables are for first time creation of the error/information dialogs.
CREATE_MBOX=False
CREATE_IBOX=False

# Allocate an array of action area buttons...
# typeset -A CONTROL_AREA "Button 1" "Button 2" ... "Button n"
set -A CONTROL_AREA
set -A CA_CB
set -A CA_WID

typeset -i Iterations=0

# This function is used to check the existence of /etc/hosts
# if not, popup error message
ChkEtcHosts()
{
	host=`uname -n`
	if [ -f /etc/hosts ]
	then 
		grep "^[^#].*$host" /etc/hosts
		if [ $? = 0 ]
		then
			return 0
		else
			return 1
		fi
	else
		return 1
	fi
}

# This function call the nslookup library function, lookupCB.
# $1 ==> pushbutton widget
# $2 ==> text widget
Lookup_CB()
{
	TRACE
	if ChkEtcHosts
	then	
		call handle_to_widget "push_but" $1
		PB=$RET
		call handle_to_widget "text" $2
		TXT=$RET
		call lookup $PB $TXT
	else
		Message_CB $TOPLEVEL "$($GETTXT $ERR_NoSetup)" 0 0
	fi
}

# This function is used to remove the Max/Min/Resize function on the MWM
# $1 ==> Shell widget
Rm_mwm_funcs()
{
	TRACE
	call handle_to_widget "top_level" $1
	TOP=$RET
	call r_decor $TOP
}

# This function is used to convert the handle to widget and call eventCB()
# three parameters 
# $1 ==> the widget for which the focus event is registered
# $2 ==> the status widget where the message is displayed
# $3 ==> the actual message that is to be displayed 
Focus_CB()
{
	TRACE
	call handle_to_widget "push_but" "$1"
	wid=$RET
	call handle_to_widget "push_but" "$2"
	stat=$RET
	call eventCB $wid $stat "$3"
}


# $1 ==> widget to be unmanaged
popdown_CB()
{
	umc  $1
}

# This function pops down the message diaglog and depends on the 2nd
# argument, travesal the focus to the appriate tab group.
# $1 ==> message diaglog
# $2 ==> widget that has the current focus on
pdmsg_CB()
{
	TRACE
	umc $1	
	if [ $Iterations -ne 0 ]
	then
		call handle_to_widget "pushbut" $2
		tmp=$RET	
		call setFocus $tmp $Iterations
	fi
}

# This function is used to create a error message box (only once) and
# display different messages.
# $1 ==> widget
# $2 ==> message
# $3 ==> number of iterations for the traversal
# $4 ==> widget that has the current focus on
Message_CB()
{
	TRACE
	echo "$1 $2 $3 $4"
	if [ $CREATE_MBOX = False ]
	then
		XmCreateErrorDialog messDialog $1 messDialog \
			autoUnmanage:False \
			dialogStyle:DIALOG_SYSTEM_MODAL \
			dialogTitle:"$($GETTXT $TXT_ErrorTitle)" \
			cancelLabelString:"$($GETTXT $LABEL_OK)"
		acb $messDialog cancelCallback "pdmsg_CB $messDialog $4"
		umc $messDialog_OK
		umc $messDialog_HELP
		CREATE_MBOX=true # avoid the if stmt after first time
	fi
	Iterations=$3
	sv $messDialog messageString:"$2"
	mc $messDialog
}

# This function is used to create a top level error message for the
# dtcopy.sh and dtaccess.sh, since they may be invoked without the
# toplevel shell (in unconfirm case).
# $1 ==> error message
Top_error()
{
	TRACE
	ai TOPLEVEL rac Rac \
		-title "$($GETTXT $TXT_ErrorTitle)" \
		"$@"
	sv $1 allowShellResize:True
	XmCreateMessageBox errmb $TOPLEVEL errmb \
		cancelLabelString:"`$GETTXT $LABEL_OK`" \
		dialogType:DIALOG_ERROR
	acb $errmb cancelCallback 'exit 0'
	umc $errmb_OK
	umc $errmb_HELP

	sv $errmb messageString:"$1"
	mc $errmb
	rw $TOPLEVEL
	ml
}

# This function is called when the toplevel shell is used to report
# error message and it is never releaized.
# $1 ==> parent widget 
# $2 ==> error message
Display_error()
{
	TRACE
	XmCreateMessageBox errmb $1 errmb \
		cancelLabelString:"`$GETTXT $LABEL_OK`" \
		dialogType:DIALOG_ERROR
	sv $1 allowShellResize:True
	acb $errmb cancelCallback 'exit 0'
	umc $errmb_OK
	umc $errmb_HELP

	sv $errmb messageString:"$2"
	#rm_mwm_funcs $1
	mc $errmb
	rw $1
	ml
}

# This function is used to call the C function to verify the system host
# name.
# $1 ==> system name
CheckHost()
{
	TRACE
	#get rid of the white space
        echo $1 | read tmp
        if [ "$tmp" = "" ]
        then
                Message_CB $TOPLEVEL "$($GETTXT $ERR_NoSystemName)" \
			2 ${CA_WID[0]}
                return 0
        else
                # do the actual verification on the host name
		
		# system name may be a numeric value and we need to
		# cast it as char * explicitly. '!' is a delimiter
		if call checkHostName "@char *:!$1!"
                then
			/sbin/tfadmin rac_verify.sh $1
			if [ $? = 0 ]
			then
				# vaild hosts name
				return 1
			fi
			Message_CB $TOPLEVEL \
				"$1 $($GETTXT $ERR_InvalidSystemName2)" \
				2 ${CA_WID[0]}
			return 0
		else
			# valid host name found
			return 1
                fi
        fi
	return 1
}

# This function is used to kill the background process and remove the
# timeout.
# $1 ==> parent widget
# $2 ==> background process id
# $3 ==> key word (shell or box)
# $4 ==> error file
# $5 ==> result file
# $6 ==> timeout id
killpid()
{
	TRACE
	kill -9 -$2
	eval timeoutid=$"$6"
	rto $timeoutid
	cleanup $1 $3 $4 $5
}

# This function is used to popdown/exit the shell. Before that, all the
# tmp files needed to be removed.
# $1 ===> parent widget (toplevel shell with a message box or dialog shell)
# $2 ===> key word (shell or box) 
# $3 ===> error file 
# $4 ===> result file
cleanup()
{
	TRACE
	if [ -a $3 ]
	then
		rm -rf $3
	fi
	if [ -a $4 ]
	then
		rm -rf $4
	fi
	if [ $2 = "shell" ]
	then
		popdown_CB $1
	else
		exit
	fi
}

# This function creates the message box based on the key word (shell, box).
# It also umapps the separator and the control area for the "Working...."
# messages so user cannot delete the message until the status message
# is displayed.
# There are 7 parameters:
# $1 ==> parent 
# $2 ==> messages 
# $3 ==> keyword for creating box or dialogshell 
# $4 ==> control or no control area  (unmap the separator and OK button)
# $5 ==> error file  (pass to the ok callback function)
# $6 ==> result file (pass to the ok callback function)
# $7 ==> unique identifier for the job (make up of rcp$$_$NUM_RCP)
# $8 ==> background process id (for killing the background process)
# $9 ==> addtimeout id (for remove timeout)
Report_CB()
{
	TRACE
	eval rp=$"$7"
	if [ "$rp" = "" ]
	then
		if [ $3 = "shell" ]
		then
			XmCreateInformationDialog $7 $1 $7 \
				autoUnmanage:False \
				dialogTitle:"$($GETTXT $TXT_InfoTitle)"
		else
			XmCreateMessageBox $7 $1 $7 \
				autoUnmanage:False \
				dialogType:DIALOG_INFORMATION
		fi
		#acb $rp okCallback "cleanup $rp $3 $5 $6"
		eval rp=$"$7"
		eval rp_HELP=$"$7"_HELP
		umc $rp_HELP
	fi
	if [ $4 = "nocontrol" ]
	then
		eval rp_OK=$"$7"_OK
		umc $rp_OK
		eval rp_CAN=$"$7"_CAN
		sv $rp_CAN labelString:"STOP"
		mc $rp_CAN
		acb $rp cancelCallback "killpid $rp $8 $3 $5 $6 $9"
	else
		eval rp_OK=$"$7"_OK
		mc $rp_OK
		eval rp_CAN=$"$7"_CAN
		umc $rp_CAN
		acb $rp okCallback "cleanup $rp $3 $5 $6"
	fi
	sv $rp messageString:"$2"
	mc $rp
}

# This function is used to check the result of the unix command.
# If the command is not completed, invoke another call to itself to
# do the checking.
# There are 6 parameters:
# $1 ==> parent widget 
# $2 ==> error file 
# $3 ==> result file
# $4 ==> key word for TOPLEVEL with box or dialogshell
# $5 ==> key word for which unix command (uuto or rcp)
# $6 ==> unique identifier for the job
# $7 ==> background process id
# $8 ==> addtimeout id
CheckResult()
{
	TRACE
	if [ -s $3 ]
	then
		while read line
		do
			echo $line
			if [ "$line" = "success" ]
			then
				if [ "$5" = "uuto" ]
				then
					Report_CB $1 "$($GETTXT $REP_UUTO_SUCCESS)" \
						$4 control $2 $3 $6 $7 $8
				else
					Report_CB $1 "$($GETTXT $REP_RCP_SUCCESS)" \
						$4 control $2 $3 $6 $7 $8
				fi
				return
			else
				if [ "$5" = "uuto" ]
				then
					Report_CB $1 "$($GETTXT $REP_UUTO_FAIL)" \
						$4 control $2 $3 $6 $7 $8
				else
					Report_CB $1 "$($GETTXT $REP_RCP_FAIL)" \
						$4 control $2 $3 $6 $7 $8
				fi
				return
			fi
		done < $3	
	else
		ato $8 5000 "CheckResult $1 $2 $3 $4 $5 $6 $7 $8"
	fi
}


# $1 ==> rowColum widget for the buttons
# $2 ==> len of the biggest widget
CrtActionArea() 
{
	TRACE
	typeset -i i=0
	typeset -i fraction
	typeset -i tightness
	typeset -i num_actions
	typeset -i maxlen=0 
	typeset -i num_but needlen t
	typeset -i cur_pos=0
	typeset -i left_pos right_pos

	#let fraction=$tightness*${#CONTROL_AREA[*]}-1
	fraction=$2-1
	echo $fraction

	M_ROWCOL=$1

	crtform ACTION_AREA $M_ROWCOL "form" \
		fractionBase:$fraction

	num_but=${#CONTROL_AREA[*]}
	# calculate the max len for the buttons
	while [ $i -lt $num_but ]
	do
		crtpushb b $1 "pushbutton" \
			labelString:"$($GETTXT ${CONTROL_AREA[$i]})"
		gv $b width:len
		if [ $len -gt $maxlen ]
		then
			maxlen=$len
		fi
		dw $b
		i=$i+1
	done

	i=0

	# calculate the tighness
	needlen=$maxlen*$num_but

	if [ $needlen -gt $2 ]
	then
		sv $ACTION_AREA fractionBase:$needlen
		tightness=0
	else
		let tightness=$2-$needlen
		let t=$num_but-1
		let tightness=$tightness/$t
	fi	

	while [ $i -lt ${#CONTROL_AREA[*]} ]
	do
		crtpushb WIDGET $ACTION_AREA "pushbutton" \
			labelString:"$($GETTXT ${CONTROL_AREA[$i]})"
		let num_actions=${#CONTROL_AREA[*]}-1
		if [ $i -eq 0 ]
		then
			leftattach=XmATTACH_FORM
			showasdefault=True
			left_pos=$cur_pos
		else
			leftattach=XmATTACH_POSITION
			showasdefault=False
			left_pos=$cur_pos+$tightness
		fi
		if [ $i -ne $num_actions ]
		then
			rightattach=XmATTACH_POSITION
			right_pos=$left_pos+$maxlen
			cur_pos=$right_pos
		else
			rightattach=XmATTACH_FORM
		fi
		sv $WIDGET leftAttachment:$leftattach \
			leftPosition:$left_pos \
			topAttachment:XmATTACH_FORM \
			bottomAttachment:XmATTACH_FORM \
			rightAttachment:$rightattach \
			rightPosition:$right_pos \
			defaultButtonShadowThickness:1 \
			showAsDefault:$showasdefault
		acb $WIDGET activateCallback "${CA_CB[$i]}"
		CA_WID[$i]=$WIDGET
		let i=i+1
	done

	mc $ACTION_AREA
	mc $M_ROWCOL
}
