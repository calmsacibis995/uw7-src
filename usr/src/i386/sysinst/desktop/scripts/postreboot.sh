#!/usr/bin/xksh
#ident	"@(#)postreboot.sh	15.1	98/03/04"

if [ "$RANDOM" = "$RANDOM" ]
then
	exec /usr/bin/xksh /etc/rc2.d/S02POSTINST 
fi

{
TERM=AT386-ie
export TERM
/bin/mkdir /tmp/log >/dev/null 2>&1
/usr/bin/winxksh /etc/inst/scripts/postreboot
} < /dev/vt01 > /dev/vt01

