#------------------------------------------------------------------------------
# @(#)main.tcl	1.2
#-------------------------------------------------------------------------------
set appvals(title) [IntlLocalizeMsg SCO_PSRSETUPGUI_MSG_DLGTITLE]
set appvals(script) [file tail [info script]] ;# argv[0]: UNIX command name
set appvals(client) $appvals(script)          ;# widget strings, preferences
set appvals(version) 1.0
set appvals(openhost) 1  
set appvals(vtMain) ""

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
	if $appvals(openhost) {
		set usage [IntlLocalizeMsg SCO_PSRSETUPGUI_MSG_USAGE1]
	} else {
		set usage [IntlLocalizeMsg SCO_PSRSETUPGUI_MSG_USAGE2]
	}
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
		switch $opt -- {
			{?} {
				Usage; Exit 1
			}
		}
	}

	# process commandline arguments
	set optCount [expr "$argc - $optind"]

	# take managed host name as a command line argument
	if {[expr $optCount >= 1]} {
	    set appvals(managedhost) [lindex $argv $optind]
	    incr optCount -1
	    incr optind 1
	} 
	set appvals(groupList) {}
	set appvals(errorPrompt) {}

	if ([expr $optCount >= 2]) {
		set appvals(groupList) [lindex $argv $optind]
                incr optind 1
		set appvals(errorPrompt) [lindex $argv $optind]
                incr optind 1

                incr optCount -2
        }


	if {$optCount != 0} {
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

	# Start the user interface
	if { [catch UiStart catrchres ] } {
		echo Problem Starting GUI
		echo $catrchres
		VtUnLock
		if { ! [cequal $appvals(vtMain) ""] } {
			VtDestroy $appvals(vtMain)
		}
	}
}


#====================================================================== XXX ===
# UiStart
#------------------------------------------------------------------------------
proc UiStart {} {
	global appvals
	set vtApp [VtOpen $appvals(client) ]
	VtSetAppValues $vtApp \
		-versionString "$appvals(title) $appvals(version)" \
		-errorCallback {SaUnexpectedErrorCB {}}
	UiProcessorCfg $vtApp
	VtMainLoop
}

#====================================================================== XXX ===
# UiStop
#------------------------------------------------------------------------------
proc UiStop {} {
	global appvals
	VtClose
}

#------------------------------------------------------------------------------
# Start
#------------------------------------------------------------------------------
ErrorTopLevelCatch {Main} $appvals(script)
