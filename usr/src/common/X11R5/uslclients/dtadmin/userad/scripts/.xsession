#!/bin/sh

#ident	"@(#)dtadmin:userad/scripts/.xsession	1.4"
#copyright	"%c%"

XDM_LOGIN=yes export XDM_LOGIN
. /etc/profile < /dev/null
. $HOME/.profile < /dev/null 
. $HOME/.olsetup
