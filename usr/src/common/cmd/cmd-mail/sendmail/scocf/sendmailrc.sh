#ident "@(#)sendmailrc.sh	11.5"

# our start/stop sendmail script

cleanup() {
	rm -f /tmp/pid$$
	exit $1
}

umask 022

FILE=/etc/sendmail.pid

cd /etc/mail

ps -aef > /tmp/pid$$

case "$1" in
start)
	if grep "/usr/lib/sendmail -q" /tmp/pid$$ > /dev/null
	then
		cleanup 0
	fi
	rm -f $FILE ; touch $FILE
	# Check if network is there
	check=`netcfg -s | grep tcp`
	if [ "$check" = "" ]
	then
		echo "hosts files" > /etc/service.switch
	else
		rm -f /etc/service.switch
	fi
	# rebuild aliases file if needed
	/etc/mail/newaliases > /dev/null
	/usr/lib/sendmail -q1m -bd
	cleanup 0
	;;
stop)
	if [ -s /etc/sendmail.pid ]
	then
		read pid < $FILE
		kill $pid
		rm -f $FILE; touch $FILE
		sleep 2
	fi
	cleanup 0
	;;
*)
	dspmsg $MF_SENDMAILRC -s $MS_SENDMAILRC $USAGE "Usage: $0 start|stop\n" $0
	cleanup 0
	;;
esac
