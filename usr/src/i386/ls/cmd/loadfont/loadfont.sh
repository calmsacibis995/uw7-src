#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)loadfont.sh	1.6"


eval `defadm locale LANG 2> /dev/null`
eval `defadm coterm MBCONSOLE 2> /dev/null`

if [ $? = 0 -a "$MBCONSOLE" = "yes" ]
then
	case "$LANG" in
		ja*)
			/sbin/pcfont -f /etc/fonts/8x16rk.bdf -i 1 -c 2
			/sbin/pcfont -f /etc/fonts/jiskan16.bdf -i 2 -c 1
			;;
		ko*)
			/sbin/pcfont -f /etc/fonts/hanglm16.bdf -i 2 -c 1
		;;
	esac
	/sbin/pcfont
else
	eval `defadm cofont COFONT 2> /dev/null`
	if [ $? = 0  -a ! -z "$COFONT" -a -x /sbin/pcfont ]
	then
		/sbin/pcfont $COFONT
	fi
fi
