#!/sbin/sh
#	copyright	"%c%"
#ident	"@(#)create_loc.sh	1.2"

#
#	Create message directories for specified codesets
#
SADMBIN=/usr/sadm/install/bin
LOCDIR=/usr/lib/locale

LABEL=UX:create_locale
CAT=uxels

makedir()
{
	[ -d $1 ] && return
	if mkdir $1 2>/dev/null
	then
		chmod +rx $1 2>/dev/null
	else
		pfmt -l ${LABEL} -s error -g ${CAT}:101 "Cannot create directory %s\n" $1
		exit 1
	fi
}

if [ $# -eq 0 ]
then
	pfmt -l ${LABEL} -s action -g ${CAT}:102 "Usage: create_locale locale ...\n"
	exit 1
fi
for locale
do
	if [ ! -d $LOCDIR/$locale -o ! -f $LOCDIR/$locale/LC_CTYPE ]
	then
		pfmt -l ${LABEL} -s error -g ${CAT}:103 "Locale is not installed by ELS.\n"
		exit 1
	fi
	CODE=`expr "$locale" : "[a-z]*_[A-Z]*.\([0-9]*\)"`
	if [ -z "$CODE" ]
	then
		pfmt -l ${LABEL} -s error -g ${CAT}:104 "Cannot determine destination codeset:\n"

		pfmt -l ${LABEL} -s nostd -g ${CAT}:105 "\t\tlocale argument must be in the form 'lang_TERRITORY.codeset'\n"
		exit 1
	fi
	SOURCE=`$SADMBIN/maplang -l $locale -f LC_MESSAGES/uxcore -d $LOCDIR 2>/dev/null`
	if [ -z "$SOURCE" -o "$SOURCE" = "C" ]
	then
		pfmt -l ${LABEL} -s error -g ${CAT}:106 "Cannot find appropriate source language.\n"
		exit 1
	fi
	if pkginfo -q ${SOURCE}le 2>/dev/null
	then
		PKG=${SOURCE}le
	else
		PKG=`$SADMBIN/maplang -l $SOURCE -f LC_MESSAGES/uxcore -d $LOCDIR 2>/dev/null`
		if pkginfo -q ${PKG}le 2>/dev/null
		then
			PKG=${PKG}le
		else
			pfmt -l ${LABEL} -s error -g ${CAT}:107 "Cannot determine source Language Extension package.\n"
			exit 1
		fi
	fi
	if [ ! -w $LOCDIR/$locale ]
	then
		pfmt -l ${LABEL} -s error -g ${CAT}:108 "Cannot write to %s/%s.\n" $LOCDIR $locale
		exit 1
	fi
	makedir $LOCDIR/$locale/LC_MESSAGES
#
#	presumption: maplang ensures that LC_MESSAGES/uxcore exists
#
	pfmt -l ${LABEL} -s nostd -g ${CAT}:109 "Creating messages for locale '%s' from locale '%s'.\n" $locale $SOURCE
	cd $LOCDIR/$SOURCE/LC_MESSAGES
	for file in *
	do
		[ $file = Xopen_info -a -f $LOCDIR/$locale/LC_MESSAGES/Xopen_info ] && continue
		if mapmsgs $CODE $file $LOCDIR/$locale/LC_MESSAGES/$file
		then
			chmod 0444 $LOCDIR/$locale/LC_MESSAGES/$file 2>/dev/null
			chown bin $LOCDIR/$locale/LC_MESSAGES/$file 2>/dev/null
			chgrp bin $LOCDIR/$locale/LC_MESSAGES/$file 2>/dev/null
			/usr/sbin/installf $PKG $LOCDIR/$locale/LC_MESSAGES/$file f 0444 bin bin 
		else
			exit 1	# mapmsgs produces error text
		fi
	done
done
exit 0
