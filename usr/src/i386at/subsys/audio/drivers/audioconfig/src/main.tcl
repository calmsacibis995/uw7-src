#==============================================================================
# Audio Configuration Manager
#------------------------------------------------------------------------------
# main.tcl
#------------------------------------------------------------------------------
# @(#)main.tcl	5.2	97/07/14
#------------------------------------------------------------------------------
# The main module for audioconfig
#------------------------------------------------------------------------------
# Revision History:
# 1996-Oct-08, shawnm, main screen prototype
# 1996-Sep-27, shawnm, modified from template
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
set appvals(localhost) {}			;# where we are running 
set appvals(managedhost) {}			;# which host to manage 

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

	set usage "Usage: $appvals(script) \[host\]"
	puts stderr $usage
}

#====================================================================== XXX ===
# ParseCommandLine
#   
# Deal with command line parameters.
# Uses vtcl (e.g. sysadm.tlib) getopt like getopt(C)
#------------------------------------------------------------------------------
proc ParseCommandLine {} {
	global argv argc opterr optind appvals

	# scan for commandline flags
	set optString	""
	set argc [llength $argv]
	set opterr 0
	while {[set opt [getopt $argc $argv $optString]] != -1} {
		switch $opt {
			{?} {
				Usage; Exit 1
			}
		}
	}

	# process commandline arguments
	set optCount [expr "$argc - $optind"]

	# take managed host name as a command line argument
	if {$optCount} {
		set appvals(managedhost) [lindex $argv $optind]
		incr optCount -1
	}

	if {$optCount > 0} {
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
	global appvals

	# Where are we?
	set appvals(localhost)	[SaHostGetLocalName]
	set appvals(managedhost) $appvals(localhost)

	# Collect command line options
	ParseCommandLine

	# Initialize UI, build main form, present initial view

	UiStart

	# Main UI is now presented and Locked
	# Time-consuming startup operations can now take place with
	# optional status messages 
	#
	# Examples:
	# + check user authorizations
	# + collect data to populate main form widgets
	# + populate main form widgets with live data

	SaStatusBarSet [VxGetVar $appvals(vtMain) statusBar] [IntlMsg INIT] 

	# Authorizations
	set authorized [AuthQuery]

	# Canonical error dialog and exit if not authorized
	if {!$authorized} {
		set dialog [VxGetVar $appvals(vtMain) vtMain].noAuths
		SaDisplayNoAuths dialog $appvals(title) UiCloseCB $appvals(managedhost)
		VtUnLock
		VtMainLoop
		return
	}

	# Get main form data
	set templistdata [GetInstalledCardList]
	set mainlistdata [ConvertConfigDataToMainListData $templistdata]
	set appvals(deviceCount) [llength $mainlistdata]
	UiMainListSetItems $mainlistdata

	# Check for any new PnP cards (Moved to uiadd.tcl *** GC ***)
	# set newcardlist [GetNewCardList]
	# if we have any then we can either prompt the user with a dialog
	# or go into the Examine Dialog

	# Set initial focus (list or menu bar)
	UiSetAppFocus

	# Set sensitivity of all main screen ui selection devices
	UiSensitizeFunctions

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
}

#====================================================================== XXX ===
# CloseAfterUi
# 
# Final close of application AFTER Ui has terminated
#------------------------------------------------------------------------------
proc CloseAfterUi {} {
	Exit
}


#------------------------------------------------------------------------------
# Start
#------------------------------------------------------------------------------
# cmdtrace on [open /tmp/tcl.trace w]
ErrorTopLevelCatch {Main} $appvals(script)
