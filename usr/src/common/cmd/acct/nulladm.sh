#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)acct:common/cmd/acct/nulladm.sh	1.5.1.3"
#ident "$Header$"
#	"nulladm name..."
#	"creates each named file mode 664"
#	"make sure owned by adm (in case created by root)"
for _file
do
	cp /dev/null $_file
	chmod 664 $_file
	chgrp adm $_file
	chown adm $_file
done
