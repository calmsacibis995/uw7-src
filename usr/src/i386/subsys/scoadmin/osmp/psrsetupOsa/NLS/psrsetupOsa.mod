# @(#)psrsetupOsa.mod	1.2
#------------------------------------------------------------------------------
# Module IDs are named in the form:
#
#    vendor_module
#
# Where vendor is a trademarked vendor mnemonic.  It may not contain any `_'
# characters. Module is a module mnemonic and may contain `_'.  This string
# must form a valid C identifier.  Upper-case is preferred, as these are used
# as constants in the code.
#
#------------------------------------------------------------------------------
#
# Data lines in this file are in the form:
#
#   moduleid   messagecat   messageset
#
#     o Module id is the module as described about.
#     o Messagecat is the name of the message catalog. It should follow the
#       SCO message catalog location and naming conventions.  More that one
#       module may share a message catalog.
#     o Messageset is the numeric message set number within the message
#       catalog for this module.
#
# Blank lines are ignored.  A comment line is a line whose first non-white
# space characeter is `#'.
#------------------------------------------------------------------------------

SCO_PSRSETUPOSA		psrsetupOSA.cat@sa 1
