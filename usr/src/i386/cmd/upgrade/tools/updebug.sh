#ident	"@(#)updebug.sh	15.1"
#ident	"$Header$"

user_id () {
	unset UPDEBUG_UID
	UPDEBUG_UID=`id | sed 's/(.*$//' | cut -d= -f2`
	export UPDEBUG_UID
}
goany()
{
   (	# run in a subshell so we can turn off the set -x
	set +x
	[ "$1" ] && echo "$1"
	pfmt -s nostd -g uxupgrade:6 "Hit <CR> to continue OR "s" to get shell OR [0-9] to exit "
	read ANS
	case $ANS in
		[0-9])	exit $ANS ;;
		s)	/sbin/sh ;;
	esac

	#
	# <$in_goany added so goany will work in a "while read VAR" loop
	# otherwise the read will grab half the input and we never stop.
	#
	# The >$out_goany was added becasue -q option to pkgadd redirects
	# stdout to /dev/null.  If we broke out to a shell in that case, we
	# got NO output from commands.
	#

	) <$in_goany >$out_goany
}

Chk_Color_Console () {

   [ "$UPDEBUG" = "YES" ] && echo "In Chk_Color_Console" && set -x
   [ "$TERM" ] && return

   TERM=AT386-M
   /usr/sbin/adpt_type >/dev/null
   DISPLAY=$?
   case $DISPLAY in
      0)  TERM=ANSI ;; #non-intergal console
      1|4)          ;; #1=MONO 4=VGA_MONO
      2|5|9|10)   #2=CGA 5=VGA_? 9=EGA 10=unknown controller
         TERM=AT386 
	 ;;
      3) #VGA_COLOR
         TERM=AT386   ;;
   esac
   [ "$UPDEBUG" = "YES" ] && goany "Exit Chk_Color_Console"
}

Set_LANG () {
	#
	#  Make sure LANG environment variable is set.  If it's not set
	#  coming in to this script, then default to the C-locale.
	#
	[ "${LANG}" ] || LANG="C"
	export LANG

	MENU_DIR=$LOCALE_DIR/${LANG}/menus
	#
	#  If no ${LANG} directory, fall back on the C-locale.
	#
	[ -d "$MENU_DIR" ] || MENU_DIR=$LOCALE_DIR/C/menus

	UPGRADE_MSGS=$LOCALE_DIR/${LANG}/menus/upgrade

	#
	#  If no ${LANG} directory, fall back on the C-locale.
	#
	[ -d "$UPGRADE_MSGS" ] || UPGRADE_MSGS=$LOCALE_DIR/C/menus/upgrade

	#
	#  If the menu_colors.sh for the current ${LANG} does not exist,
	#  default to the C-locale directory.
	#
	
	MENU_COLOR=${LOCALE_DIR}/${LANG}/menus/menu_colors.sh
	[ -f ${MENU_COLOR} ] || MENU_COLOR=${LOCALE_DIR}/C/menus/menu_colors.sh
	. ${MENU_COLOR}

	[ "$UPDEBUG" = "YES" ] && goany "Exit Set_LANG"
}

# ## main
#	updebug defines the following routines:
#	user_id() - exports the user id in the variable UPDEBUG_UID
#	goany()	- allows debugging when UPDEBUG is set to YES.
#	Chk_Color_Console() - which sets TERM
#	Set_LANG() -	sets LANG, MENU_DIR, UPGRADE_MSGS, and menu_colors


UPDEBUG=NO
UPERR=/etc/inst/up/up.err
LOCALE_DIR=/etc/inst/locale
USRSBIN=/usr/sbin
[ "$SCRIPTS" ] || SCRIPTS=$USRSBIN/pkginst
PATH=$PATH:$USRSBIN:$SCRIPTS

[ ! -d /etc/inst/up ] && {
	mkdir -p /etc/inst/up 2>/dev/null
	# If mkdir fails, we'll reset UPERR
	[ $? != 0 ] && UPERR=/tmp/up.err
	in_goany=/dev/console
	out_goany=/dev/console
}

# non-superuser should be able to create and write to log file
[ -d /etc/inst/up -a -f /usr/bin/sed -a /usr/bin/cut ] && {
	user_id
	[ "$UPDEBUG_UID" != 0 ] && UPERR=/tmp/up.err
	in_goany=/dev/tty
	out_goany=/dev/tty
}

[ -f /usr/bin/date ] && DATE=`date`
# Add marker to help identify where the output is coming from.
echo "\nENTERING $0	$DATE" >>$UPERR
[ "$UPDEBUG" = "YES" ] && goany "LOG file for updebug is $UPERR"

