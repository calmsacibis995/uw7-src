#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)initpkg:i386/cmd/initpkg/vfstab.sh	1.1.7.2"
echo "#special          fsckdev          mountp   fstype fsckpass automnt mntopts
/dev/root	/dev/root	/	s5	1	yes	-
">vfstab
