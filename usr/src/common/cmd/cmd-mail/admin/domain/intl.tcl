#===============================================================================
#
#	ident @(#) intl.tcl 11.1 97/10/30 
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
# 	Convenience support for Internationalization 
#
#
# Modification History
#
# M000, 20-Feb-97, andrean
#	- Created.
#
#===============================================================================

#-------------------------------------------------------------------------------
# Fundamental Globals: appvals array
#
# To reduce global name space pollution, a handful of variables that are
# global for convenience are collected under one name, appvals, which is
# a tcl array. See other *.tcl files for possible additions to appvals.
#-------------------------------------------------------------------------------
set appvals(intlpre) SCO_MAIL_ADMIN_DOMAIN		;# i10n msg prefix

#====================================================================== XXX ===
# IntlMsg
#   
#------------------------------------------------------------------------------
proc IntlMsg {id {argList {}}} {
	global appvals
	return [IntlLocalizeMsg $appvals(intlpre)_MSG_$id $argList]
}

#====================================================================== XXX ===
# IntlErr
#   
#------------------------------------------------------------------------------
proc IntlErr {id {argList {}}} {
	global appvals
	return [IntlLocalizeMsg $appvals(intlpre)_ERR_$id $argList]
}

#====================================================================== XXX ===
# IntlErrId
#   
#------------------------------------------------------------------------------
proc IntlErrId {id} {
	global appvals
	return $appvals(intlpre)_ERR_$id
}
