#------------------------------------------------------------------------------
# @(#)uiProcessor.tcl	1.3
#-------------------------------------------------------------------------------
#====================================================================== XXX ===
# UiProcessorCfg
#------------------------------------------------------------------------------
proc UiProcessorCfg { parent } {
	global appvals

	set mainForm \
		[VtFormDialog $parent.createDaemonForm \
                -okCallback     UiProcessorCfgOKCB \
		-cancelCallback UiProcessorCfgCancelCB \
                -autoLock       "UiProcessorCfgOKCB UiProcessorCfgCancelCB" \
		-title 		[IntlLocalizeMsg SCO_PSRSETUPGUI_MSG_FORMTITLE \
				 ($appvals(managedhost))] \
		-ok \
		-cancel \
		-marginWidth 	1 \
		]
	
	set appvals(vtMain) $mainForm

	# Menu Options
	set menuBar [VtMenuBar $mainForm.menuBar ]
	set hostMenu [VtPulldown $menuBar.hostMenu \
		-label [IntlLocalizeMsg SCO_PSRSETUPGUI_MSG_MENU_HOSTS] \
		-mnemonic [IntlLocalizeMsg SCO_PSRSETUPGUI_MSG_MN_MENU_HOSTS] ]

	VtPushButton $hostMenu.openhost \
		-label [IntlLocalizeMsg SCO_PSRSETUPGUI_MSG_MENU_OPENHOST] \
		-mnemonic [IntlLocalizeMsg SCO_PSRSETUPGUI_MSG_MN_MENU_OPENHOST] \
		-callback SaOpenHostCB  \
		-autoLock SaOpenHostCB
	VtSeparator $hostMenu.s1
	VtPushButton $hostMenu.exit \
		-label [IntlLocalizeMsg SCO_PSRSETUPGUI_MSG_MENU_EXIT] \
		-mnemonic [IntlLocalizeMsg SCO_PSRSETUPGUI_MSG_MN_MENU_EXIT] \
		-callback UiProcessorCfgCancelCB  \
		-autoLock UiProcessorCfgCancelCB

        set actionsPD [VtPulldown $menuBar.actionsPD \
                -label [IntlLocalizeMsg SCO_PSRSETUPGUI_MSG_MENU_ACTION] \
		-mnemonic [IntlLocalizeMsg SCO_PSRSETUPGUI_MSG_MN_MENU_ACTION] ]
	VtPushButton $actionsPD.properties \
                -label [IntlLocalizeMsg SCO_PSRSETUPGUI_MSG_MENU_PROPS] \
		-mnemonic [IntlLocalizeMsg SCO_PSRSETUPGUI_MSG_MN_MENU_PROPS] \
                -callback "UiProcessorPropertiesCB" \
                -autoLock "UiProcessorPropertiesCB"

	VtPushButton $actionsPD.startstop \
                -label [IntlLocalizeMsg SCO_PSRSETUPGUI_MSG_MENU_START] \
		-mnemonic [IntlLocalizeMsg SCO_PSRSETUPGUI_MSG_MN_MENU_START] \
                -callback "UiProcessorStartStopCB" \
                -autoLock "UiProcessorStartStopCB"

	set formatlist {{STRING 50}}
	set CHARM_formatlist {{STRING 50}}

	# List of processors
	set drawnList [VtDrawnList $mainForm.drawnList \
		-formatList $formatlist \
                -CHARM_formatList $CHARM_formatlist \
		-columns 50 \
		-rows 6 \
		-rightSide FORM]
	
	VxSetVar $mainForm drawnList $drawnList

	UiProcessorRefresh $mainForm

	VtShow $mainForm
	VtUnLock
}

#====================================================================== XXX ===
# UiProcessorRefresh
#------------------------------------------------------------------------------
proc UiProcessorRefresh { psrDlg } {
	global appvals
	set drawnList [VxGetVar $psrDlg drawnList]
	VtDrawnListDeleteItem $drawnList -all

	# Call osa to get list 
	set result [osaGetAllProcessorInfo $appvals(managedhost)]
	if { [lindex $result 0] != 0 } {
		UiDisplayErrorMessage $parent [lindex $result 1]
		return
	}

	set recordlist [list]
	foreach i [lindex $result 1] {
		append tmpvar " [list [concat Processor $i]]"
		lappend recordlist $tmpvar
		unset tmpvar
	}

	VtDrawnListAddItem $drawnList \
		-recordList $recordlist
}

#====================================================================== XXX ===
# UiProcessorCfgOKCB
#------------------------------------------------------------------------------
proc UiProcessorCfgOKCB { cbs } {
	set parent [keylget cbs dialog]
	VtDestroy $parent
	VtUnLock
	UiStop
}

#====================================================================== XXX ===
# UiProcessorCfgCancelCB
#------------------------------------------------------------------------------
proc UiProcessorCfgCancelCB { cbs } {
	set parent [keylget cbs dialog]
	VtDestroy $parent
	VtUnLock
	UiStop
}

#====================================================================== XXX ===
# UiProcessorPropertiesCB
#------------------------------------------------------------------------------
proc UiProcessorPropertiesCB { cbs } {
	global appvals
	set parent [keylget cbs dialog]
	set psrno [UiGetSelectProcessorNo $parent]
	if [cequal $psrno ""] {
		# Nothing selected
		UiDisplayInfoMessage $parent \
			[IntlLocalizeMsg SCO_PSRSETUPGUI_MSG_SELECT_PROP]
	} else {
		# Call OSA
		set result [osaGetProcessorInfo $appvals(managedhost) $psrno]
		if { [lindex $result 0] != 0 } {
			# Report error
			UiDisplayErrorMessage $parent [lindex $result 1]
		} else {
			set info [VtInformationDialog $parent.info \
				-message [lindex $result 1]]
			VtShow $info
		}
	}
	VtUnLock
}

#====================================================================== XXX ===
# UiProcessorStartStopCB
#------------------------------------------------------------------------------
proc UiProcessorStartStopCB { cbs } {
	global appvals
	set parent [keylget cbs dialog]
	set psrno [UiGetSelectProcessorNo $parent]
	if [cequal $psrno ""] {
		UiDisplayInfoMessage $parent \
			[IntlLocalizeMsg SCO_PSRSETUPGUI_MSG_SELECT_STOP]
	} else {
		set result [osaStartStopProcessor $appvals(managedhost) $psrno]
		if { [lindex $result 0] != 0 } {
			# Report error
			UiDisplayErrorMessage $parent [lindex $result 1]
		} else {
			UiProcessorRefresh $parent
		}
	}
	VtUnLock
}

#====================================================================== XXX ===
# UiGetSelectProcessorNo
#------------------------------------------------------------------------------
proc UiGetSelectProcessorNo { psrDlg } {
	set drawnList [VxGetVar $psrDlg drawnList]
	set posList [VtDrawnListGetSelectedItem $drawnList -byPositionList]
	if { $posList != 0 } {
		set psrno [expr $posList - 1]
		return $psrno
	} else {
		return ""
	}
}

#====================================================================== XXX ===
# UiDisplayErrorMessage
#------------------------------------------------------------------------------
proc UiDisplayErrorMessage { parent msg } {
	set errMsgDlg [VtErrorDialog $parent.errMsgDlg \
		-message $msg ]
	VtShow $errMsgDlg
}

#====================================================================== XXX ===
# UiDisplayInfoMessage
#------------------------------------------------------------------------------
proc UiDisplayInfoMessage { parent msg } {
	set infoMsgDlg [VtInformationDialog $parent.errMsgDlg \
		-message $msg ]
	VtShow $infoMsgDlg
}
