#!/sbin/sh
#	copyright	"%c%"
#ident	"@(#)postcheckfdb:postcfdb.sh	1.1"

[ -d /usr/X/lib/locale ] || exit 0
[ -d /usr/X/lib/classdb ] || exit 0
[ -f /usr/X/lib/classdb/dtadmin ] || exit 0
grep "^INCLUDE" /usr/X/lib/classdb/dtadmin >/tmp/i.list.$$
cd /usr/X/lib/locale
for i in [a-z][a-z]
do
	[ -d $i ] || continue
	[ $i = C ] && continue
	[ -d $i/classdb ] || continue
	[ -f $i/classdb/dtadmin ] || continue
	ed $i/classdb/dtadmin <<! 2>/dev/null >/dev/null
g/^INCLUDE/d
r /tmp/i.list.$$
w
q
!
done
rm /tmp/i.list.$$
exit 0
