#!/bin/sh

#ident	"@(#)dtadmin:floppy/dtbackup.sh	1.10"
#copyright	"%c%"


# FIX: i18N the usage message
Cmd=`/usr/bin/basename $0`
USAGE="USAGE: $Cmd -s source -t backup_type -h home_dir -l local_only -e extension -n tape_num\n[ -i ] [ -p ptyname ] List of files/folders to backup read from standard input\n"

BKUP_COMPL=0
BKUP_INCR=1
BKUP_FILES=2

unset Ignore Newer Immediate Source Type PtyName Home DoGrep Local

while getopts is:p:t:h:l:e:n: opt
do
	case $opt in
	i)	Immediate="-v";;
	s)	Source=$OPTARG;;
	p)	PtyName="-M \"EoM %d\" -G $OPTARG";;
	t)	Type=$OPTARG;;
	h)	Home=$OPTARG;;
	l)	Local=$OPTARG;;
	e)	ext=$OPTARG;;
	n)	Tapenum=$OPTARG;;
	\?)	echo $USAGE
		exit 1;;
	esac
done
shift `expr $OPTIND - 1`

if [ -z "$Source" -o -z "$Type" -o -z "$Home" ] 
then
	echo $USAGE
	exit 2
fi

if /sbin/tfadmin -t cpio > /dev/null 2>& 1
then
	CpioCmd="/sbin/tfadmin cpio"
else
	CpioCmd="/usr/bin/cpio"
fi

ListFile=/tmp/FFILES.$ext

if [ "$Type" != "$BKUP_FILES" ]
then
		if [ -r $Home/Ignore ]
		then
			DoGrep=yes
			ListFile=/tmp/GFILES.$ext
			Ignore="$Home/Ignore"
		fi
fi
if [ "$Type" = "$BKUP_INCR" ]
then
		if [ -r $Home/.lastpartial ]
		then
			Newer="-newer $Home/.lastpartial"
		elif [ -r $Home/.lastbackup ]
		then
			Newer="-newer $Home/.lastbackup"
		fi
fi
	
# To support devices with variable block size mode, use 512-bytes
# block  size for creating backup archives, as well as for reading/restoring.
# 512 is the safest size since it is supported by all drivers.
# This should avoid a block size mismatch when attempting to read a tape 
# with a backup archive, if this was created via the MediaMgr.
#
# If Tapenum is different to 0, we are writing to a tape drive, and Tapenum
# contains the number of the drive.  

if [ $Tapenum -ne 0 ]
then
	if [ -x /usr/bin/tapecntl ]
	then
		/usr/bin/tapecntl -f 512 /dev/rmt/ctape$Tapenum
	fi
fi

# Set up the arguments for find

if [ $Local -eq 1 ]
then
	findargs="! -local -prune -o $Newer -print"
else
	findargs="$Newer -print"
fi	

# Make sure that if there is a request to back up a symbolic link
# that the link is followed.

while read line
do
        (cd "$line" > /dev/null 2>&1  && /bin/pwd) || echo "$line"

done | /usr/bin/xargs -i /usr/bin/find {} $findargs > $ListFile

if [ -n "$DoGrep" ]
then
	/usr/bin/sed 's;\(.*\);\^\1\/;g' $Ignore > /tmp/Ignore.$ext
	/usr/bin/egrep -v -f /tmp/Ignore.$ext < /tmp/GFILES.$ext > /tmp/FFILES.$ext
fi
${XWINHOME:-/usr/X}/adm/dtindex -p $ext $Immediate
${XWINHOME:-/usr/X}/adm/privindex -p $ext $Immediate
/usr/bin/rm -f /tmp/?FILES.$ext
if [ -n "$Immediate" ]
then
	/usr/bin/cut -f2 < /tmp/flp_index.$ext \
	| grep -v BLOCKS=                    \
	| $CpioCmd -odlucvB -O $Source           \
	>/tmp/bkupout.$$ 2>/tmp/bkuperr.$ext
else
	/usr/bin/cut -f2 < /tmp/flp_index.$ext | /usr/bin/grep -v BLOCKS= \
	| $CpioCmd -odlucvB -O $Source $PtyName 2>&1
fi
if [ "$?" = "0" ]
then
	if [ "$Type" = "$BKUP_INCR" ]
	then
		/usr/bin/touch $Home/.lastpartial
	elif [ "$Type" = "$BKUP_COMPL" ]
	then
		/usr/bin/touch $Home/.lastbackup
	fi
fi
/usr/bin/rm -f /tmp/*.$ext
/usr/bin/rm -f /tmp/Ignore.$ext
exit 0
