#!/sbin/sh
#ident	"@(#)dtrac:rac_verify.sh	1.2.1.2"

/usr/bin/uuname | /usr/bin/grep "^$1\$"  >/dev/null 2>&1
