#******************************************************************************
# @(#)objcalls.tcl	1.2
#------------------------------------------------------------------------------
# Contains all the direct OSA calls
#==============================================================================

#====================================================================== INT ===
# GetInstanceList
#
# Arguments:
#       names and hostname
#
# Returns:
#       reformatted list.
#
#------------------------------------------------------------------------------
proc GetInstanceList { names host} {

    set instanceList {}
    if { $host == "localhost" } {
        foreach name $names {
            set instance [list $name]
            lappend instanceList $instance
        }
    } else {
        foreach name $names {
            set instance [list [list systemId $host] [list $name]]
            lappend instanceList $instance
        }
    }
    return $instanceList
}


#====================================================================== INT ===
# osaPsrSetupOSACall
#------------------------------------------------------------------------------
proc osaPsrSetupOSACall { host actionType args  } {
	set class [list sco psrsetup]
        set instance [GetInstanceList NULL $host]
        set command [list ObjectAction $class $instance $actionType $args]
        set result [SaMakeObjectCall $command]
        set result [lindex $result 0]
                if { [BmipResponseErrorIsPresent result] } {
                set errorStack [BmipResponseErrorStack result]
                ErrorThrow errorStack
                return {}
                }
        set retVal [BmipResponseActionInfo result]
        return $retVal
}


#====================================================================== INT ===
# osaGetAllProcessorInfo
#------------------------------------------------------------------------------
proc osaGetAllProcessorInfo { host } {
	return [osaPsrSetupOSACall $host GetAllProcessorInfo {}]
}

#====================================================================== INT ===
# osaStartStopProcessor
#------------------------------------------------------------------------------
proc osaStartStopProcessor { host psrno } {
	return [osaPsrSetupOSACall $host StartStopProcessor $psrno]
}

#====================================================================== INT ===
# osaGetProcessorInfo
#------------------------------------------------------------------------------
proc osaGetProcessorInfo { host psrno } {
	return [osaPsrSetupOSACall $host GetProcessorInfo $psrno]
}
