#!/sbin/sh
#ident	"@(#)putdev.sh	15.1"

# The /sbin/wsinit command calls putdev even if there is no device
# database (/etc/device.tab).  This stub keeps /sbin/wsinit happy.

exit 0
