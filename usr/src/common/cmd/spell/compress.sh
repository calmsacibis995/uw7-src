#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)spell:compress.sh	1.3.1.3"
#ident "$Header$"
#	compress - compress the spell program log

trap 'rm -f /usr/tmp/spellhist;exit' 1 2 3 15
echo "COMPRESSED `date`" > /usr/tmp/spellhist
grep -v ' ' /var/adm/spellhist | sort -fud >> /usr/tmp/spellhist
cp /usr/tmp/spellhist /var/adm
rm -f /usr/tmp/spellhist
