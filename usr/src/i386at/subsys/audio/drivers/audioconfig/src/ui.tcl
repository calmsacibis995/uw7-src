#==============================================================================
# Audio Configuration Manager
#------------------------------------------------------------------------------
# ui.tcl
#------------------------------------------------------------------------------
# @(#)	@(#)ui.tcl	7.2	11/12/97	17:50:07
#------------------------------------------------------------------------------
# User interface, binding to SCO Visual Tcl
#------------------------------------------------------------------------------
# Revision History:
# 1996-Nov-25, shawnm, one card only
# 1996-Oct-28, shawnm, add test button
# 1996-Oct-08, shawnm, main screen prototype
# 1996-Sep-27, shawnm, created from template
#==============================================================================

# Globals
set appvals(title) 	[IntlMsg TITLE]	;# UI title string
set appvals(vtApp)	{}		;# VtOpen return; generally not used 
set appvals(vtMain)	{}		;# widget string of main dialog
set appvals(addInProgress)	0
set appvals(examineInProgress)	0
set appvals(pendingUnit)	0
set appvals(pendingManufacturer)	{}
set appvals(pendingModel)		{}

# ToolBar
set appvals(toolBar) 		1		;# toolBar

#-------------------------------------------------------------------------------
# Generic UI Functions
#-------------------------------------------------------------------------------

#====================================================================== XXX ===
# UiWidgetGet
#   
# Mechanism for retrieving widget strings with a symbolic name under an 
# optional parent. Defaults to vtMain
#------------------------------------------------------------------------------
proc UiWidgetGet {widget {parent {}}} {
	global appvals
	if {[lempty $parent]} {
		set parent $appvals(vtMain)
	}
	return [VxGetVar $parent $widget]
}

#====================================================================== XXX ===
# UiDefaultFormSpacingSet
#   
# Every form gets the same spacing and margins: all 0
#------------------------------------------------------------------------------
proc UiDefaultFormSpacingSet {form} {
	VtSetValues $form \
		-horizontalSpacing 0 \
		-verticalSpacing 0 \
		-marginWidth 0 \
		-marginHeight 0 
}

#====================================================================== XXX ===
# UiDisplayErrorStackCB
#   
# Call back for OK button on error stack dialog. 
# Return control to original caller if arranged.
#------------------------------------------------------------------------------
proc UiDisplayErrorStackCB {callback cbs} {
	global appvals

	set dialog [keylget cbs dialog]
	VtDestroyDialog $dialog
	SaStatusBarClear [VxGetVar $appvals(vtMain) statusBar]

	if {![lempty $callback]} {
		eval $callback [list $cbs]
	}
}

#====================================================================== XXX ===
# UiDisplayErrorStacks
#   
# Error stack dialog
# o return control to caller if desired
#------------------------------------------------------------------------------
proc UiDisplayErrorStacks {caller stack {callback ""}} {
	global appvals

	SaDisplayErrorStacks $appvals(vtMain).$caller [list $stack] HELP \
		[list UiDisplayErrorStackCB $callback]
}

#====================================================================== XXX ===
# UiSetAppFocus
#   
# Sets proper widget focus depending on contents of main list 
#------------------------------------------------------------------------------
proc UiSetAppFocus {} {
	global appvals

	VtSetFocus [VxGetVar $appvals(vtMain) menuBar] 
	if {![lempty [UiMainListGetItems]]} {
		VtSetFocus [VxGetVar $appvals(vtMain) mainList]
	}
}


#-------------------------------------------------------------------------------
# UI CallBacks
#-------------------------------------------------------------------------------

#====================================================================== XXX ===
# UiCloseCB
#   
# Generally, the exit callback including Ui shutdown. 
# Two callbacks into the underlying application module, 1 before shutting
# the Ui (e.g. so that message/error/question dialogs are still possible,
# and 1 after Ui has been shutdown.
# Note, this is actually implemented in
#------------------------------------------------------------------------------
proc UiCloseCB {cbs} {
	# Call underlying non-ui module: close that may need additional ui
	CloseWithUi
	UiStop	UiCloseCB2
}

#====================================================================== XXX ===
# UiCloseCB2
#   
# Part 2 (see CloseCB) 
# Regained control from UiStop which may have required a detour to
# the event-driven loop to present an error dialog. We now resume
# procedural control and call back out of the Ui module.
#------------------------------------------------------------------------------
proc UiCloseCB2 {{cbs {}}} {
	# Call underlying non-ui module: close without Ui
	CloseAfterUi
}


#====================================================================== XXX ===
# UiSensitizeFunctions
#   
# Set the sensitivity of all pulldown menu items 
# Called before initial main screen is unlocked, and any time an action
# changes state such that item sensitivity must be recomputed globally
#------------------------------------------------------------------------------
proc UiSensitizeFunctions {} {
	# visit all appropriate menu items:
	# 	VxGetVar
	#	VtSetsensitive
	global appvals

	set mainList [VxGetVar $appvals(vtMain) mainList]
	if {$appvals(deviceCount) == 0} {
		set selected 0
	} else {
		set selected [llength [VtDrawnListGetSelectedItem $mainList]]
	}

	set menuBar [VxGetVar $appvals(vtMain) menuBar]
	set add [VxGetVar $menuBar add]
	if {$selected == 0} {
		VtSetSensitive $add 1
		UiToolBarSetSensitive add 1
	} else {
		VtSetSensitive $add 0
		UiToolBarSetSensitive add 0
	}
	set examine [VxGetVar $menuBar examine]
	if {$selected == 0} {
		VtSetSensitive $examine 0
		UiToolBarSetSensitive examine 0
	} else {
		VtSetSensitive $examine 1
		UiToolBarSetSensitive examine 1
	}
	set remove [VxGetVar $menuBar remove]
	if {$selected == 0} {
		VtSetSensitive $remove 0
		UiToolBarSetSensitive remove 0
	} else {
		VtSetSensitive $remove 1
		UiToolBarSetSensitive remove 1
	}
	set test [VxGetVar $menuBar test]
	if {$selected == 0} {
		VtSetSensitive $test 0
		UiToolBarSetSensitive test 0
	} else {
		VtSetSensitive $test 1
		UiToolBarSetSensitive test 1
	}

	if {$appvals(deviceCount) > 0} {
		VtSetSensitive $mainList 1
	} else {
		VtSetSensitive $mainList 0
	}
}


#-------------------------------------------------------------------------------
# Basic UI Entry Points: start, stop, loop
#-------------------------------------------------------------------------------

#====================================================================== XXX ===
# UiStart
#   
# Build/Startup the Ui:
# o build main form
# o pulldown menus
# o main form widgets
# o statusBar
# Main form is presented in a locked state and control returns to the 
# underlying application for additional startup tasks. Control eventually
# returns to Ui loop for user events via VtMainLoop
#------------------------------------------------------------------------------
proc UiStart {} {
	global appvals


	# Open/connect to widget server
	set vtApp [VtOpen $appvals(client) [IntlMsg HELPBOOK]]
	set appvals(vtApp) $vtApp
	VtSetAppValues $vtApp \
                -versionString "$appvals(title) $appvals(version)" \
                -errorCallback {SaUnexpectedErrorCB {}}

	set title \
	"$appvals(title) [IntlMsg ON] [SaHostExtractSystemName $appvals(managedhost)]"

	# Create main dialog
	set vtMain [VtFormDialog $vtApp.main \
			-title $title \
			-wmShadowThickness 0 \
			-wmCloseCallback UiCloseCB] 
	set appvals(vtMain) $vtMain
	VxSetVar $appvals(vtMain) vtMain $vtMain
	UiDefaultFormSpacingSet $vtMain
	SaCharmSetMaxFormDimensions $vtMain

	# Build pulldown menus
	set menuBar [UiBuildMenus $vtMain]
	VxSetVar $appvals(vtMain) menuBar $menuBar

	# Build the toolBar
	if {! [VtInfo -charm]} {
		if {$appvals(toolBar)} {
			set toolBar [UiBuildToolBar $vtMain $menuBar]
		}
	}

	# Main form label
	set mainlabel [VtLabel $vtMain.mainlabel \
		-label "[IntlMsg MAIN_LABEL $appvals(managedhost)]"]
	VxSetVar $appvals(vtMain) mainlabel $mainlabel

	# Pass main form and widget just above for attachments
	set mainList [UiBuildMainList $vtMain $mainlabel]

	# Build the status bar
	# Standard SCOadmin status bar pixmap
	set logofile {}
	# Full pathname for non-SCOadmin status bar pixmap 
	# set logofile "?"

	if {[lempty $logofile]} {
		set statusBar [SaStatusBar $vtMain.statusBar 1]
	} else {
		set statusBar [SaStatusBar $vtMain.statusBar 1 $logofile]
	}
	VxSetVar $appvals(vtMain) statusBar $statusBar
	# Connect main screen widget bottom to statusBar
	VtSetValues $mainList -bottomSide $statusBar

	# Display the main form and lock it
	VtShow $vtMain
	VtLock
}

#====================================================================== XXX ===
# UiStop
#   
# Shut down the Ui. We may have to interrupt the procedural flow
# from the caller to present error dialogs to the user. This requires
# returning to the event-driven Ui loop. To return control to the caller,
# UiStop utilizes UiStop2 (e.g. part 2) and a callback to return control
# to the caller.
#------------------------------------------------------------------------------
proc UiStop {cb} {
	global appvals

	set vtMain $appvals(vtMain)
	set client $appvals(client)

	# Store modified preferences

	# short help
	set cmd [list SaShortHelpStore $client]
	if {[ErrorCatch errorStack 0 $cmd dummy] != 0 } {
		lappend errorStacks $errorStack
	}

	# any problems?
	if [info exists errorStacks] {
		# present error dialog and enter event-driven Ui loop.
		# Re-gain procedural control via UiStop2 (part 2) and
		# eventuall, the caller's cb
		SaDisplayErrorStacks $vtMain.UIStop $errorStacks {} \
			[list UiStop2 $cb]
		VtUnLock
		VtMainLoop
	} else {
		# stop vtcl
		VtUnLock
		VtClose
		# return control to the caller
		eval $cb
	}
}

#====================================================================== XXX ===
# UiStop2
#   
# Part 2 (see UiStop).
# Shutdown the UI for sure: no more possible dialogs, no more event-driven
# control flow. Shutdown and return control to caller of UiStop.
#------------------------------------------------------------------------------
proc UiStop2 {cb {cbs {}}} {
	VtUnLock
	VtClose
	# return control to the caller
	eval $cb
}
