#==============================================================================
# Audio Configuration Manager
#------------------------------------------------------------------------------
# intl.tcl
#------------------------------------------------------------------------------
# @(#)intl.tcl	7.1	97/10/22
#------------------------------------------------------------------------------
# Convenience support for internationalization 
#------------------------------------------------------------------------------
# Revision History:
# 1996-Sep-27, shawnm, created from template
#==============================================================================

# Globals
set appvals(intlpre) SCO_AUDIOCONFIG			;# i10n msg prefix

#====================================================================== XXX ===
# IntlMsg
#------------------------------------------------------------------------------
proc IntlMsg {id {argList {}}} {
	global appvals
	return [IntlLocalizeMsg $appvals(intlpre)_MSG_$id $argList]
}


#====================================================================== XXX ===
# IntlErr
#------------------------------------------------------------------------------
proc IntlErr {id {argList {}}} {
	global appvals
	return [IntlLocalizeMsg $appvals(intlpre)_ERR_$id $argList]
}


#====================================================================== XXX ===
# IntlErrId
#------------------------------------------------------------------------------
proc IntlErrId {id} {
	global appvals
	return $appvals(intlpre)_ERR_$id
}

