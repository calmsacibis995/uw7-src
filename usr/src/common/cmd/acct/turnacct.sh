#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)acct:common/cmd/acct/turnacct.sh	1.8.1.3"
#ident "$Header$"
#	"control process accounting (must be root)"
#	"turnacct on	makes sure it's on"
#	"turnacct off	turns it off"
#	"turnacct switch	switches pacct to pacct?, starts fresh one"
#	"/var/adm/pacct is always the current pacct file"
PATH=/usr/lib/acct:/usr/bin:/usr/sbin
cd /var/adm
case "$1"  in
on)
	if test ! -r pacct
	then
		nulladm pacct
	fi
	accton pacct
	_rc=$?
	;;
off)
	accton
	_rc=$?
	;;
switch)
	if test -r pacct
	then
		_i=1
		while test -r pacct${_i}
		do
			_i="`expr ${_i} + 1`"
		done
		mv pacct pacct${_i}
	fi
	nulladm pacct
	accton pacct
	_rc=$?
	if test ${_rc} -ne 0; then
		echo "accton failed"
		rm pacct
		mv pacct${_i} pacct
		exit ${_rc}
	fi
	;;
*)
	echo "Usage: turnacct on|off|switch"
	_rc=1
	;;
esac
exit ${_rc}
