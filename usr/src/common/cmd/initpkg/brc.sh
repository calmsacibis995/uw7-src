#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)initpkg:common/cmd/initpkg/brc.sh	1.7.11.2"
#ident "$Header$"

if [ -f /etc/dfs/sharetab ]
then
	/usr/bin/mv /etc/dfs/sharetab /etc/dfs/osharetab
fi
if [ ! -d /etc/dfs ]
then /usr/bin/mkdir /etc/dfs
fi
>/etc/dfs/sharetab
# chmod 644 /etc/dfs/sharetab
