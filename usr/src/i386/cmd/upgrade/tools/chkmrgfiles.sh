#ident	"@(#)chkmrgfiles.sh	15.1"
#ident	"$Header$"

### main ()
#  script to display the screen:
#  
#  Do you want to auto merge config files or not
#
#  The arg is the name of the pkg
#  This script is invoked from  the request script of a pkg

SBINPKGINST=/usr/sbin/pkginst

. $SBINPKGINST/updebug

[ "$UPDEBUG" = "YES" ] && set -x

NAME="$1"

Set_LANG
Chk_Color_Console
export TERM

#
#  Now invoke the menu program with everything we just extracted.
#

unset RETURN_VALUE

[ "$UPDEBUG" = "YES" ] && goany && set +x

menu -f ${UPGRADE_MSGS}/mergefiles.3 -o /tmp/response.$$ </dev/tty

[ "$UPDEBUG" = "YES" ] && set -x

. /tmp/response.$$
rm -f /tmp/response.$$
	
#	RETURN_VALUE=1 for Yes. Merge files
#	RETURN_VALUE=2 for No. Do not auto merge files.

rc=`expr $RETURN_VALUE - 1`
unset RETURN_VALUE

[ "$UPDEBUG" = "YES" ] && goany

exit  $rc
