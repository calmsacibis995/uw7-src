#===============================================================================
#
#	ident @(#) main.tcl 11.1 97/10/30 
#
#******************************************************************************
#
#	Copyright (C) 1993-1997 The Santa Cruz Operation, Inc.
#		All Rights Reserved.
#
#	The information in this file is provided for the exclusive use of
#	the licensees of The Santa Cruz Operation, Inc.  Such users have the
#	right to use, modify, and incorporate this code into other products
#	for purposes authorized by the license agreement provided they include
#	this notice and the associated copyright notice with any such product.
#	The information in this file is provided "AS IS" without warranty.
#
#******************************************************************************
#
# SCO Mail Administration Domain client:
#       "main" module
#
#
# Modification History
#
# M000, 20-Feb-97, andrean
#	- Created.
#
#==============================================================================

#------------------------------------------------------------------------------
# Fundamental Globals: appvals array
#
# To reduce global name space pollution, a handful of variables that are
# global for convenience are collected under one name, appvals, which is
# a tcl array. See other *.tcl files for possible additions to appvals.
#------------------------------------------------------------------------------
set appvals(script) [file tail [info script]]	;# argv[0]: UNIX command name 
set appvals(client) $appvals(script)		;# widget strings, preferences

# globals
set appvals(filename) {}			;# filename of channel table
set appvals(fileid) -1				;# backend file id of table
set appvals(hostname) localhost			;# name of host being managed

#-------------------------------------------------------------------------------
# Underlying Non-User Interface Modules
#-------------------------------------------------------------------------------

#====================================================================== XXX ===
# Exit
#   
# Terminate application with proper exit code. 
# Potential hooks for final processing and Cleanup. 
# Never returns.
#------------------------------------------------------------------------------
proc Exit {{code 0}} {
	# Final exit tasks
	exit $code
}

#====================================================================== XXX ===
# Usage
#   
# Display usage message on stderr
#------------------------------------------------------------------------------
proc Usage {} {
	global appvals

	set usage [IntlErr USAGE $appvals(script)]

	puts stderr $usage
}

#====================================================================== XXX ===
# ParseCommandLine
#   
# Deal with command line parameters.
# Uses vtcl (e.g. sysadm.tlib) getopt like getopt(C)
#------------------------------------------------------------------------------
proc ParseCommandLine {} {
	global argv argc optarg opterr optind appvals
@if test
	global TEST
	set TEST ""
@endif

	# scan for commandline flags
@if test
	set optString   "f:h:t:"
@else
	set optString	"f:h:"
@endif
	set argc [llength $argv]
	if {$argc == 0} {
		Usage; Exit 1
	}

	set opterr 0
	while {[set opt [getopt $argc $argv $optString]] != -1} {
		switch $opt {
			{f} {
				set appvals(filename) $optarg
			}

			{h} {
				set appvals(hostname) $optarg
			}
@if test
			{t} {
				set TEST $optarg
			}
@endif

			{?} {
				Usage; Exit 1
			}
		}
	}

	set optCount [expr "$argc - $optind"]

	if {$optCount > 0} {
		Usage; Exit 1
	}

	if {"$appvals(filename)" == ""} {
		Usage; Exit 1
	}
}

#====================================================================== XXX ===
# Main
# 
# Application entry point.
# o Process Commandline parameters
# o Initialize and start UI
# o Additional startup tasks
#------------------------------------------------------------------------------
proc Main {} {
	global appvals errorInfo

	# Collect command line options
	ParseCommandLine

	# Initialize UI, build main form, present initial view

	# Init a few globals first
	ChannelInit

	UiStart

	# Main UI is now presented and Locked
	# Time-consuming startup operations can now take place with
	# optional status messages 
	#
	# + collect data to populate main form widgets
	# + populate main form widgets with live data

	SaStatusBarSet \
		[VxGetVar $appvals(vtMain) statusBar] [IntlMsg INIT] 

	# Show/hide the toolbar, depending on screen policy
	if {! [VtInfo -charm]} {
		set cmd "set visibility \[SaScreenPolicyGet $appvals(client) toolbarVisibility\]"
		if {[ErrorCatch errStack 0 $cmd errMsg] != 0} {
			set visibility 1
		}
		set menuBar [VxGetVar $appvals(vtMain) menuBar]
		set optionMenu [VxGetVar $menuBar optionMenu]
		set toolBarToggle [VxGetVar $optionMenu toolBarToggle]
		set toolBar [VxGetVar $appvals(vtMain) toolBar]
		set toolBarFrame [VxGetVar $appvals(vtMain) toolBarFrame]
		VtSetValues $toolBarToggle -value $visibility
		UiToolBarSetVisibilityCB
        }


	# if file does not exist, let the user know
	if {![file exists $appvals(filename)]} {
		set main $appvals(vtMain)
		set dialog [VtInformationDialog $main.exist \
			-message [IntlMsg FILE_DOESNT_EXIST] \
			-ok]
		VtShow $dialog
	}

	# if file is a directory, show error and exit
	if {[file isdirectory $appvals(filename)]} {
		set main $appvals(vtMain)
		set dialog [VtErrorDialog $main.error \
			-message [IntlErr DIRECTORY $appvals(filename)] \
			-ok -okCallback UiCloseCB -wmCloseCallback UiCloseCB]
		VtShow $dialog
		VtUnLock
		VtMainLoop
		return
	}

	# Open the file and read in the data to back end structures
	if {[ErrorCatch errStack 0 "Map:Open $appvals(filename)" result]} {
		ErrorPush errStack 0 [IntlErrId NONSTDCMD] [list $errorInfo]
		ErrorPush errStack 0 [IntlErrId LOAD] $appvals(filename)
		VtUnLock
		UiDisplayErrorStacks Main $errStack UiCloseCB
	} else {
		set appvals(fileid) $result
	}

	# Refresh main form data
	UiRefreshCB

	# Select first item, if exists
	set mainList [VxGetVar $appvals(vtMain) mainList]
	if {![lempty [VtDrawnListGetItem $mainList -all]]} {
		VtDrawnListSelectItem $mainList -position 1
	}

	# update the count label
	UiUpdateCountLabel

	# Set initial focus (list or menu bar)
	UiSetAppFocus

	# Set sensitivity of all main screen ui selection devices
	UiSensitizeMainFunctions

	# Setup complete
	# Wait for user events

	SaStatusBarClear [VxGetVar $appvals(vtMain) statusBar]
	VtUnLock
	VtMainLoop
}

#====================================================================== XXX ===
# CloseWithUi
# 
# Perform shutdown actions where Ui may still be required/useful
# for message/error/question dialogs
#------------------------------------------------------------------------------
proc CloseWithUi {} {
	set saveConfirmDlg [UiBuildSaveConfirmDialog]
	VtShow $saveConfirmDlg
}

#====================================================================== XXX ===
# CloseAfterUi
# 
# Final close of application AFTER Ui has terminated
#------------------------------------------------------------------------------
proc CloseAfterUi {} {
	Exit
}

#====================================================================== XXX ===
# Refresh
# 
# Dummy function for refreshing the main form view
# For auto refresh and refresh, this is called by the Ui side callback
#------------------------------------------------------------------------------
proc Refresh {} {
	global appvals

	# Populate main form data here
	set data {}

	# TODO
	# Setup simple example using the "who" module

	# Except for the following line, Refresh should be generic
	# and usable as is.


	# load Ui
	set data [ChannelGetData]
	UiMainListSetItems $data
}

#------------------------------------------------------------------------------
# Start
#------------------------------------------------------------------------------
ErrorTopLevelCatch {Main} $appvals(script)
