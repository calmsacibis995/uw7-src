#==============================================================================
# Audio Configuration Manager
#------------------------------------------------------------------------------
# uitoolbar.tcl
#------------------------------------------------------------------------------
# @(#)uitoolbar.tcl	7.1	97/10/22
#------------------------------------------------------------------------------
# User Interface: ToolBar, binding to SCO Visual Tcl
#------------------------------------------------------------------------------
# Revision History:
# 1996-Sep-27, shawnm, created from template
# 1996-Oct-08, shawnm, main screen prototype
# 1996-Oct-28, shawnm, add test button
#==============================================================================

#-------------------------------------------------------------------------------
# ToolBar Module 
#
# This file implements the pieces required when using the SaToolbar package 
# in sysadm.tlib. If a toolBar is not used (e.g. appvals(toolBar) == 0), this
# file can be deleted and removed from the Makefile.
#
# The toolBar below uses two sample icons and a separator to illustrate the
# necessary pieces of a non-trivial toolBar. Both the openhost and refresh
# icons could realistically be used in a production app. Note, however, that
# application specific icons/actions are expected and need to be merged into
# the data structures below. To build the final toolBar, modify the following
# below (in routine UiBuildToolBar:
#
#	appvals(toolBarIcons)
#	appvals(toolBarCommands)
#	appvals(toolBarStandard)
#-------------------------------------------------------------------------------

# Globals
set appvals(toolBarIcons) {}		;# set of available icons
set appvals(toolBarCommands) {}		;# set of available command definitions
set appvals(toolBarStandard) {}		;# default toolBar (icon/command pairs)

#====================================================================== XXX ===
# UiToolBarStore
#   
# Store toolBar settings in preferences file
#------------------------------------------------------------------------------
proc UiToolBarStore {} {
	global appvals

	if {[VtInfo -charm] || ! $appvals(toolBar)} {
		return
	}

	SaToolbarGet visibility current frame
	SaToolbarStore 	$appvals(client) visibility
}


#====================================================================== XXX ===
# UiToolBarResensitizeCB
#   
# The toolBar has changed and new buttons may be present without proper
# sensitization.
#------------------------------------------------------------------------------
proc UiToolBarResensitizeCB {keys cbs} {
	# ToolBar has possibly been modified.
	# traverse key list and recompute and sensitize each button
	UiSensitizeFunctions
}

#====================================================================== XXX ===
# UiToolBarSetSensitive
#   
# Set the sensitivity of the toolBar buttons. "button" is the button
# tag vs. its actual widget string. State is boolean.
#------------------------------------------------------------------------------
proc UiToolBarSetSensitive {button state} {
	SaToolbarButtonSetSensitive $button $state
}

#====================================================================== XXX ===
# UiBuildToolBar
#   
# Builds the toolBar on the main form along with supporting data structures.
#------------------------------------------------------------------------------
proc UiBuildToolBar {form top} {
	global appvals

	set client $appvals(client)
	set frame IN
	set visibility 1

	# relavent icons
	set appvals(toolBarIcons) [list \
		[IntlMsg TB_OPENHOST] \
		[IntlMsg TB_ADD] \
		[IntlMsg TB_EXAMINE] \
		[IntlMsg TB_TEST] \
		[IntlMsg TB_REMOVE] \
		] 

	# set of toolBar command records: tag callback shorthelpstring
	set appvals(toolBarCommands) [list \
		[list openhost SaOpenHostCB \
			[IntlMsg OPENHOST_SH]] \
		[list add UiAddCB \
			[IntlMsg ADD_SH]] \
		[list examine UiExamineCB \
			[IntlMsg EXAMINE_SH]] \
		[list test UiTestCB \
			[IntlMsg TEST_SH]] \
		[list remove UiRemoveCB \
			[IntlMsg REMOVE_SH]] \
		]

	# build the standard default toolBar
	set standard {}
	lappend standard {0 0}
	lappend standard {{} S}
	lappend standard {1 1}
	lappend standard {2 2}
	lappend standard {3 3}
	lappend standard {{} S}
	lappend standard {4 4}
	set appvals(toolBarStandard) $standard 

	set current $appvals(toolBarStandard)
	SaToolbarLoad $client visibility
	set toolBar \
		[SaToolbar 	$form.toolBar $top \
				$appvals(toolBarIcons) \
				$appvals(toolBarCommands) \
				$appvals(toolBarStandard) \
				SaShortHelpCB \
				$visibility \
				$current \
				$frame \
				UiToolBarResensitizeCB \
				{} \
				]
	VxSetVar $appvals(vtMain) toolBar $toolBar
	return $toolBar
}
