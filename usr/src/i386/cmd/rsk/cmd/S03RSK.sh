#ident	"@(#)S03RSK.sh	15.1"

TERM=AT386 export TERM
SILENT_INSTALL=false export SILENT_INSTALL
REDIR="</dev/vt01 >/dev/vt01 2>/mnt/inet.errs" 
[ -s /unixware.dat ] && {
	cp /unixware.dat /tmp/unixware.dat
	SILENT_INSTALL=true 
	REDIR=""
}

[ -x /etc/inet/menu ] && /usr/bin/xksh /etc/inet/menu "$REDIR"

mv /etc/rc2.d/S03RSK /etc/rc2.d/.S03RSK
rm /unixware.dat /tmp/unixware.dat 2>/dev/null
exit 0
