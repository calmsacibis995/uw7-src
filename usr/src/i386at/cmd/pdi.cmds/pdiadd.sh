#! /sbin/sh
#ident	"@(#)pdi.cmds:pdiadd.sh	1.16"
#ident	"$Header$"

#set -x

#
#	pdiadd/pdirm - user-level shell commands to modify the
#			PDI configuration on a UNIX system.
#

#
#	First, some function definitions and initialization.
#

#
# This function replaces /bin/fold -w 48 | while read listin
# 	with a function call to just 'pdifold 48'.
#
# The echoes were moved into this function for loop efficiency.
#

label=UX:$0
msgdb=uxpdiadd

pdifold() {
	while read input
	do
		output=`/bin/expr "$input" : "\(.\{1,$1\}\)"`
		pfmt -l $label -s '       ' -g $msgdb:1 "%s\n" "$output"
		left=`/bin/expr "$input" : ".\{1,$1\}\(.*\)"`
		while [ -n "$left" ]
		do
			output=`/bin/expr "$left" : "\(.\{1,$1\}\)"`
			pfmt -l $label -s '       ' -g $msgdb:1 "%s\n" "$output"
			left=`/bin/expr "$left" : ".\{1,$1\}\(.*\)"`
		done
	done
}

#
# This function replaces:
#
#	echo $list | /bin/tr ' 	' '\012' | /bin/cut -f1 -d'-' | paste -s -d' ' -
#
# 	with a function call to just 'pdimerge $list'.
#

pdimerge()
{
	output=""

	for element
	do
		piece=`/bin/expr "${element}" : '\([0-9a-fA-FzX]*\)'`
		if [ -n "${piece}" ]
		then
			output="${output} ${piece}"
		fi
	done

	echo "${output}"
}

#
# This function replaces:
#
#		/bin/grep -h '^NAMEL=' /etc/conf/pack.d/*/disk.cfg |
#			cut -f2 -d'=' > ${TEMPFILE}1
#		/bin/grep -h '^NAMES=' /etc/conf/pack.d/*/disk.cfg |
#			cut -f2 -d'=' | /bin/paste - ${TEMPFILE}1 |
#		pdifold 48;
#
# 	with a function call to just 'pdipaste '^NAMES=' '^NAMEL=''.
#
pdipaste()
{
	state="odd"

	eval /bin/egrep -h "'${1}|${2}'" /etc/conf/pack.d/*/disk.cfg |
	while read input
	do
		if [ "${state}" = "odd" ]
		then
			state="even"
			first="${input}"
		else
			state="odd"
			second="${input}"
			output=`/bin/expr "${first}" : "$1\(.*\)"`
			output="${output}	-	`/bin/expr "${second}" : "$2\(.*\)"`"
			pfmt -l $label -s '       ' -g $msgdb:1 "%s\n" "$output"
		fi
	done
}

#
#	This function returns the value of a single hex digit
#
hex_digit() {
	case $1 in
	[0-9])
		echo $1;;
	[aA])
		echo 10;;
	[bB])
		echo 11;;
	[cC])
		echo 12;;
	[dD])
		echo 13;;
	[eE])
		echo 14;;
	[fF])
		echo 15;;
	esac
}

#
#	This function returns the value of a string of hex digits.
#	any leading 0's or [xX] are ignored
#
hex_value() {
	DIGIT=`/bin/expr "$1" : '\(.\)'`
	REST=`/bin/expr "$1" : '.\(.*\)'`
	VALUE=`hex_digit $DIGIT`

	while [ -n "$REST" ]
	do
		DIGIT=`/bin/expr "$REST" : '\(.\)'`
		REST=`/bin/expr "$REST" : '.\(.*\)'`
		NUMBER=`hex_digit $DIGIT`
		VALUE=`/bin/expr $VALUE \* 16 + $NUMBER`
	done

	echo $VALUE
}

#
#	This function compares two hex numbers and returns
#	1 if they are equal and 0 if they are not equal
#
hex_compare() {
	FIRST=`/bin/expr "$1" : '0\{0,1\}[xX]\{0,1\}0*\(.*\)$'`
	SECOND=`/bin/expr "$2" : '0\{0,1\}[xX]\{0,1\}0*\(.*\)$'`

	FIRST=`hex_value $FIRST`
	SECOND=`hex_value $SECOND`

	[ "$FIRST" -eq "$SECOND" ] && return 1 || return 0
}

adding() {
# update for adding

F1=$NAMES
F2=$NAMEL
F3=$DEVICE
F5=-1
F6=0
F7=$DMA_CHAN
F8=0
F9=$IPL
F10=$INT_VECT
F11=$SHAR_FLAG
F12=$IO_ADDR
F13=$EIO_ADDR
F14=$MEM_ADDR
F15=$EMEM_ADDR

#
#	Now let's actually process thru and edit the input for diskcfg
#

OIFS="$IFS"
IFS="	"

if [ $NO_COUNT -gt 0 ]
then
	WRITTEN=false
	while read driver full type conf rest_of_line
	do
		if [ "$driver" = "$TARGET_DEVICE" -a "$conf" = "N" ]
		then
			if [ "$WRITTEN" = "false" ]
			then
				echo "$F1\t\"$F2\"\t$F3\tY\t$F5\t$F6\t$F7\t$F8\t$F9\t$F10\t$F11\t0x$F12\t0x$F13\t0x$F14\t0x$F15" >> ${TEMPFILE}3
				WRITTEN=true
			fi
		else
			echo "$driver\t$full\t$type\t$conf\t$rest_of_line" >> ${TEMPFILE}3
		fi
	done < ${TEMPFILE}1
elif [ $YES_COUNT -eq 0 ]
then
	cat ${TEMPFILE}1 > ${TEMPFILE}3
	echo "$F1\t\"$F2\"\t$F3\tY\t$F5\t$F6\t$F7\t$F8\t$F9\t$F10\t$F11\t0x$F12\t0x$F13\t0x$F14\t0x$F15" >> ${TEMPFILE}3
else
	WRITTEN=false
	while read driver full type conf rest_of_line
	do
		if [ "$driver" = "$TARGET_DEVICE" ]
		then
			YES_COUNT=`expr $YES_COUNT - 1`
			echo "$driver\t$full\t$type\t$conf\t$rest_of_line" >> ${TEMPFILE}3
			if [ "$WRITTEN" = "false" -a "$YES_COUNT" -lt 1 ]
			then
				echo "$F1\t\"$F2\"\t$F3\tY\t$F5\t$F6\t$F7\t$F8\t$F9\t$F10\t$F11\t0x$F12\t0x$F13\t0x$F14\t0x$F15" >> ${TEMPFILE}3
				WRITTEN=true
			fi
		else
			echo "$driver\t$full\t$type\t$conf\t$rest_of_line" >> ${TEMPFILE}3
		fi
	done < ${TEMPFILE}1
fi

IFS="$OIFS"
}

remove_single() {

OIFS="$IFS"
IFS="	"

WRITTEN=false
while read driver full type conf rest_of_line
do
	if [ "$driver" = "$TARGET_DEVICE" ]
	then
		if [ "$conf" = "Y"  -a "$WRITTEN" = "false" ]
		then
			echo "$driver\t$full\t$type\tN\t$rest_of_line" >> ${TEMPFILE}3
			WRITTEN=true
		fi
	else
		echo "$driver\t$full\t$type\t$conf\t$rest_of_line" >> ${TEMPFILE}3
	fi
done < ${TEMPFILE}1

IFS="$OIFS"
}

remove_multi() {

OIFS="$IFS"
IFS="	"

WRITTEN=false
while read driver full type conf rest_of_line
do
	if [ "$driver" = "$TARGET_DEVICE" ]
	then
		if [ "$conf" = "Y" ]
		then
			MATCH_COUNT=`expr $MATCH_COUNT - 1`
			if [ "$WRITTEN" = "false" -a "$MATCH_COUNT" -lt 1 ]
			then
				WRITTEN=true
			else
				echo "$driver\t$full\t$type\t$conf\t$rest_of_line" >> ${TEMPFILE}3
			fi
		fi
	else
		echo "$driver\t$full\t$type\t$conf\t$rest_of_line" >> ${TEMPFILE}3
	fi
done < ${TEMPFILE}1

IFS="$OIFS"
}

fix_unit() {

/bin/rm -f ${TEMPFILE}6

OIFS="$IFS"
IFS="	"

UNIT=0
while read driver full type conf unit rest_of_line
do
	if [ "$conf" = "Y" ]
	then
		if [ "$unit" -eq "0" ]
		then
			echo "$driver\t$full\t$type\t$conf\t$unit\t$rest_of_line" >> ${TEMPFILE}6
		else
			echo "$driver\t$full\t$type\t$conf\t-1\t$rest_of_line" >> ${TEMPFILE}6
		fi
	else
		echo "$driver\t$full\t$type\t$conf\t$unit\t$rest_of_line" >> ${TEMPFILE}6
	fi
done < ${TEMPFILE}3
IFS="$OIFS"

/bin/cat ${TEMPFILE}6 > ${TEMPFILE}3
}

PDIADD=pdiadd
PDIRM=pdirm
PROGNAME=`/bin/basename $0`
REMOVING=`[ "$PROGNAME" = $PDIRM ] && echo true || echo false`
ADDING=`[ "$PROGNAME" = $PDIADD ] && echo true || echo false`
if [ $ADDING = $REMOVING ]
then
	pfmt -l $label -s error -g $msgdb:2 "Invalid command invocation -- %s\n" "$0";
	pfmt -l $label -s action -g {$msgdb:3 "Try typing /etc/scsi/pdiadd -?\n";
	exit 2;
elif [ $ADDING = true ]
then
	PDI_PREFIX=add
else
	PDI_PREFIX=rm
fi
TO_STDOUT=false
FR_STDIN=false
N_ARG=""
HOT=false

TEMPFILE=/tmp/$$.${PDI_PREFIX}

giveusage()
{
	pfmt -l $label -s action -g $msgdb:4 "Usage:\n"
	if [ $ADDING = true ]
	then
		pfmt -l $label -s "       " -g $msgdb:52 "pdi%s [-O] [-I] [-d dma] [-v vector] [-s sharing] [-i i/o_address] [-m memory_address] device\n" "$PDI_PREFIX"
	fi
	if [ $PDI_PREFIX = add ]
	then
		pfmt -l $label -s "       " -g $msgdb:50 "pdiadd -h [-n] disk_number\n"
	else
		pfmt -l $label -s "       " -g $msgdb:51 "pdirm -h [-n] disk_number\n"
	fi
}

mem_in_use()
{ 
	RESULT=`$IDCHECK -c -l $1 -u $2 $ID_OPT -r`
	if [ $? -ne 0 ]
	then
		pfmt -l $label -s error -g $msgdb:5 "Conflicting starting memory address specified -- %s\n" "$1";
		pfmt -l $label -s error -g $msgdb:6 "Memory address %s is in use by device %s\n" "$1" "$RESULT";
		return 0
	fi
	return 1
}

io_in_use()
{ 
	RESULT=`$IDCHECK -a -l $1 -u $2 $ID_OPT -r`
	if [ $? -ne 0 ]
	then
		pfmt -l $label -s error -g $msgdb:7 "Conflicting starting I/O address specified -- %s\n" "$1";
		pfmt -l $label -s error -g $msgdb:8 "I/O address %s is in use by device %s\n" "$1" "$RESULT";
		return 0
	fi
	return 1
}

vector_in_use()
{ 
	RESULT=`$IDCHECK -v$1 $ID_OPT -r`
	RET=$?
	if [ $RET -ne 0 ]
	then
		if [ $RET -eq 1 -o $RET -ne $2 ]
		then
			pfmt -l $label -s error -g $msgdb:9 "Conflicting Interrupt Vector specified -- %s\n" "$1";
			pfmt -l $label -s error -g $msgdb:10 "Interrupt Vector %s is in use by device %s\n" "$1" "$RESULT";
			return 0
		fi
	fi
	return 1
}

dma_in_use()
{ 
	RESULT=`$IDCHECK -d$1 $ID_OPT -r`
	if [ $? -ne 0 ]
	then
		pfmt -l $label -s error -g $msgdb:11 "Conflicting DMA channel specified -- %s\n" "$1";
		pfmt -l $label -s error -g $msgdb:12 "DMA channel %s is in use by device %s\n" "$1" "$RESULT";
		return 0
	fi
	return 1
}

validate_list()
{ 
	TEMP=`echo $2 | /bin/tr ' 	' '\012' | /bin/grep -i \^$1-`
	[ -z "$TEMP" ] && return 0
	/bin/expr $TEMP : '[^-]*-\(.*\)'
	return 1
}

validate()
{ 
	[ "$1" -eq 0 ] && return 0
	for next in $2
	do
		if [ "$next" -eq "$1" ]
		then
			return 1
		fi
	done
	return 0
}

#main()
while getopts OId:v:i:m:R:nhs: c
do
	case $c in
	n)
		N_ARG=-n
		;;
	h)
		HOT=true
		;;
	O)
		TO_STDOUT=true
		;;
	I)
		FR_STDIN=true
		;;
	s)
		if [ "$OPTARG" = 0 ]
		then
			SHAR_FLAG=0
		else
			SHAR_FLAG=`/bin/expr $OPTARG : '\([0-4]\)$'`
			if [ $? -ne 0 ]
			then
				pfmt -l $label -s error -g $msgdb:53 "Invalid type of interrupt sharing specified -- %s\n" "$OPTARG";
				pfmt -l $label -s action -g $msgdb:54 "Try a single digit between 0 and 4\n";
				pfmt -l $label -s '	  ' -g $msgdb:55 "If the device does not use an interrupt, use 0\n";
				exit 2;
			fi
		fi
		;;
	d)
		if [ "$OPTARG" = 0 ]
		then
			DMA_CHAN=0
		else
			DMA_CHAN=`/bin/expr $OPTARG : '\([0-7]\)$'`
			if [ $? -ne 0 ]
			then
				pfmt -l $label -s error -g $msgdb:13 "Invalid DMA channel specified -- %s\n" "$OPTARG";
				pfmt -l $label -s action -g $msgdb:14 "Try a single digit between 1 and 7\n";
				pfmt -l $label -s '	  ' -g $msgdb:15 "If the device does not use a DMA channel, use 0\n";
				exit 2;
			fi
		fi
		;;
	v)
		if [ "$OPTARG" = 0 ]
		then
			INT_VECT=0
		else
			INT_VECT=`/bin/expr $OPTARG : '\(1\{0,1\}[0-9]\{0,1\}\)$'`
			if [ $? -ne 0 ]
			then
				pfmt -l $label -s error -g $msgdb:16 "Invalid Interrupt Vector specified -- %s\n" "$OPTARG";
				pfmt -l $label -s action -g $msgdb:17 "Try a number between 1 and 15\n";
				exit 2;
			fi
		fi
		;;
	i)
# this nonsense with checking for 0 or x0 or 0x0 is to workaround an expr bug
		if [ "$OPTARG" = 0 -o "$OPTARG" = x0 -o "$OPTARG" = 0x0 ]
		then
			IO_ADDR=0
		else
			IO_ADDR=`/bin/expr $OPTARG : '0\{0,1\}[xX]\{0,1\}\([a-fA-F0-9]\{1,4\}\)$'`
			if [ $? -ne 0 ]
			then
				pfmt -l $label -s error -g $msgdb:18 "Invalid I/O address value specified -- %s\n" "$OPTARG";
				pfmt -l $label -s action -g $msgdb:19 "Try a hexadecimal number less than %s\n" "0x10000";
				exit 2;
			fi
		fi
		;;
	m)
# this nonsense with checking for 0 or x0 or 0x0 is to workaround an expr bug
		if [ "$OPTARG" = 0 -o "$OPTARG" = x0 -o "$OPTARG" = 0x0 ]
		then
			MEM_ADDR=0
		else
			MEM_ADDR=`/bin/expr $OPTARG : '0\{0,1\}[xX]\{0,1\}\([a-fA-F0-9]\{1,7\}\)$'`
			if [ $? -ne 0 ]
			then
				pfmt -l $label -s error -g $msgdb:20 "Invalid memory address value specified -- %s\n" "$OPTARG";
				pfmt -l $label -s action -g $msgdb:19 "Try a hexadecimal number less than %s\n" "0x100000000";
				exit 2;
			fi
		fi
		;;
	R)	CONF_ROOT=$OPTARG;;
	*)	giveusage; exit 2;;
	
	esac
done

DISKCFG=$CONF_ROOT/etc/scsi/diskcfg
PDICONFIG=$CONF_ROOT/etc/scsi/pdiconfig
IDCHECK=$CONF_ROOT/etc/conf/bin/idcheck
IDBUILD=$CONF_ROOT/etc/conf/bin/idbuild
PDI_HOT=$CONF_ROOT/etc/scsi/pdi_hot

if [ -z "$CONF_ROOT" ]
then
	ID_OPT=""
	PDI_OPT=""
else
	ID_OPT="-R $CONF_ROOT/etc/conf"
	PDI_OPT="-R $CONF_ROOT"
fi

shift `/bin/expr $OPTIND - 1`

if [ $HOT = true ]
then
	if [ $PDI_PREFIX = add ] 
	then
		$PDI_HOT -i $N_ARG $1
	else
		$PDI_HOT -r $N_ARG $1
	fi
	exit $?
fi

if [ $REMOVING = true ]
then
	pfmt -l $label -s warning -g $msgdb:56 "This command no longer supports board removal.\n";
	pfmt -l $label -s "       " -g $msgdb:57 "To remove board instances from your configuration,\n";
	pfmt -l $label -s "       " -g $msgdb:58 "run the dcu and select \"Hardware Device Configuration\".\n";
	exit 1
fi

if [ -z "$1" ]
then
	if [ $OPTIND -eq 1 ]
	then
		pfmt -l $label -s error -g $msgdb:21 "Required argument missing -- device\n";
		pfmt -l $label -s action -g $msgdb:22 "Choose a device from the first column of this list:\n";
		pdipaste '^NAMES=' '^NAMEL=';
	fi
	giveusage;
	exit 2;
fi
TARGET_DEVICE=$1

if [ ! -x $PDICONFIG ]
then
	pfmt -l $label -s error -g $msgdb:23 "You do not have sufficient privilege to use a required program -- %s\n" "$PDICONFIG";
	exit 3;
fi

if [ ! -x $DISKCFG ]
then
	pfmt -l $label -s error -g $msgdb:23 "You do not have sufficient privilege to use a required program -- %s\n" "$DISKCFG";
	exit 3;
fi

if [ ! -x $IDBUILD ]
then
	pfmt -l $label -s error -g $msgdb:23 "You do not have sufficient privilege to use a required program -- %s\n" "$IDBUILD";
	exit 3;
fi

if [ ! -x $IDCHECK ]
then
	pfmt -l $label -s error -g $msgdb:23 "You do not have sufficient privilege to use a required program -- %s\n" "$IDCHECK";
	exit 3;
fi

#
# If FR_STDIN flag is on, then get configuration information from standard
# input.  Otherwise, get configuration information using pdiconfig.
#
if [ $FR_STDIN = true ]
then
	/bin/cat > ${TEMPFILE}1
else
	eval "$PDICONFIG $PDI_OPT ${TEMPFILE}1"
	if [ $? -ne 0 ]
	then
		pfmt -l $label -s error -g $msgdb:24 "A problem occurred running %s\n" "$PDICONFIG";
		exit 4;
	fi
fi

#
#	Well, I thought it was over.  Anyway, validate the device specified
#	a little further and then read in it's disk.cfg file.
#
#	I should explain a bit.  The disk.cfg file is .'ed in since
#	it is structured as a list of Shell variable definitions.
#

DISK_CFG=$CONF_ROOT/etc/conf/pack.d/$TARGET_DEVICE/disk.cfg
SYSTEM=$CONF_ROOT/etc/conf/cf.d/sdevice

if [ -r $DISK_CFG -a -f $DISK_CFG -a -s $DISK_CFG ]
then
	YES_COUNT=`/bin/cat ${TEMPFILE}1 | /bin/cut -f1,4 | /bin/grep 'Y$' | /bin/cut -f1 | /bin/grep -c "^${TARGET_DEVICE}$"`
	NO_COUNT=`/bin/cat ${TEMPFILE}1 | /bin/cut -f1,4 | /bin/grep 'N$' | /bin/cut -f1 | /bin/grep -c "^${TARGET_DEVICE}$"`
	if [ $YES_COUNT -ne 0 ]
	then
		/bin/cut -f1,4,7,10,12,14 ${TEMPFILE}1 | /bin/grep "${TARGET_DEVICE}	Y	" > ${TEMPFILE}4
	fi
	if [ $NO_COUNT -ne 0 ]
	then
		/bin/cut -f1,4,7,10,12,14 ${TEMPFILE}1 | /bin/grep "${TARGET_DEVICE}	N	" > ${TEMPFILE}5
	fi
else
	pfmt -l $label -s error -g $msgdb:25 "Invalid device specified -- %s\n" "$TARGET_DEVICE";
	pfmt -l $label -s action -g $msgdb:26 "Choose a device from this list:\n";
	if [ $ADDING = true ]
	then
		pfmt -l $label -s '	  ' -g $msgdb:1 "%s\n" "`/bin/cat ${TEMPFILE}1 | /bin/cut -f1 | /bin/pr -8 -w72 -a -t -s,`";
	else
		pfmt -l $label -s '	  ' -g $msgdb:1 "%s\n" "`/bin/cat ${TEMPFILE}1 | /bin/cut -f1,4 | /bin/grep -v 'N$' | /bin/cut -f1 | /bin/pr -8 -w72 -a -t -s,`";
	fi
	exit 2;
fi

. $DISK_CFG

#
#	Now, validate the user's input against the allowable values
#	contained in the disk.cfg file.  If there is no value in the
#	disk.cfg file for validation of any of the values we need,
#	we exit here because there must be a value line for each of
#	the values we are concerned with.  All of the disk.cfg files
#	delivered with the system meet this requirement.
#
#	If the user has not specified a value for any of the parameters in
#	disk.cfg that this prog is concerned with, the default value in the
#	disk.cfg file is used ( the default is the first one in the list ).
#
#	Even if we take the default from the disk.cfg file, the validation
#	functions are used to check for sdevice conflicts.  We always have
#	to invoke validate_list for those things that have a starting and
#	ending valid value in the disk.cfg file ( specified like 330-337 ).
#

if [ -z "$SHAR_FLAG" ]
then
	SHAR_FLAG="$SHAR"
fi
if [ -z "$SHAR_FLAG" ]
then
	SHAR_FLAG=1
fi

if [ -z "$DMA1" ]
then
	pfmt -l $label -s error -g $msgdb:27 "There is no value for DMA1 in the disk.cfg file for %s.\n" "$TARGET_DEVICE";
	pfmt -l $label -s action -g $msgdb:28 "Correct the disk.cfg file for %s.\n" "$TARGET_DEVICE";
	pfmt -l $label -s '	  ' -g $msgdb:29 "This file is in /etc/conf/pack.d/%s.\n" "$TARGET_DEVICE";
	pfmt -l $label -s '	  ' -g $msgdb:30 "Consult the disk.cfg(4) manual page for more details.\n";
	exit 2;
elif [ "$DMA1" != "0" ]
then
	if [ -z "$DMA_CHAN" ]
	then
		DMA_CHAN=`/bin/expr "$DMA1" : '\([0-9]\)'`
	elif validate $DMA_CHAN "$DMA1";
	then
		pfmt -l $label -s error -g $msgdb:13 "Invalid DMA channel specified -- %s\n" "$DMA_CHAN";
		pfmt -l $label -s action -g $msgdb:31 "Choose a DMA channel from this list:\n";
		pfmt -l $label -s '	  ' -g $msgdb:1 "%s\n" "$DMA1";
		exit 2;
	fi

	if [ $ADDING = true ]
	then
		if dma_in_use $DMA_CHAN;
		then
			pfmt -l $label -s action -g $msgdb:32 "Choose another DMA channel from this list:\n";
			pfmt -l $label -s '	  ' -g $msgdb:1 "%s\n" "$DMA1";
			exit 2;
		fi
	fi
else
	DMA_CHAN="$DMA1"
fi

if [ -z "$IVEC" ]
then
	pfmt -l $label -s error -g $msgdb:33 "There is no value for IVEC in the disk.cfg file for %s.\n" "$TARGET_DEVICE";
	pfmt -l $label -s action -g $msgdb:28 "Correct the disk.cfg file for %s.\n" "$TARGET_DEVICE";
	pfmt -l $label -s '	  ' -g $msgdb:29 "This file is in /etc/conf/pack.d/%s.\n" "$TARGET_DEVICE";
	pfmt -l $label -s '	  ' -g $msgdb:30 "Consult the disk.cfg(4) manual page for more details.\n";
	exit 2;
elif [ "$IVEC" != "0" ]
then
	if [ -z "$INT_VECT" ]
	then
		INT_VECT=`/bin/expr "$IVEC" : '\([0-9]*\)'`
	elif validate $INT_VECT "$IVEC";
	then
		pfmt -l $label -s error -g $msgdb:16 "Invalid Interrupt Vector specified -- %s\n" "$INT_VECT";
		pfmt -l $label -s action -g $msgdb:34 "Choose an Interrupt Vector from this list:\n";
		pfmt -l $label -s '	  ' -g $msgdb:1 "%s\n" "$IVEC";
		exit 2;
	fi

	if [ $ADDING = true ]
	then
		if vector_in_use $INT_VECT $SHAR_FLAG;
		then
			pfmt -l $label -s action -g $msgdb:35 "Choose another Interrupt Vector from this list:\n";
			pfmt -l $label -s '	  ' -g $msgdb:1 "%s\n" "$IVEC";
			exit 2;
		fi
	fi
else
	INT_VECT="$IVEC"
fi

if [ -z "$IOADDR" ]
then
	pfmt -l $label -s error -g $msgdb:36 "There is no value for IOADDR in the disk.cfg file for %s.\n" "$TARGET_DEVICE";
	pfmt -l $label -s action -g $msgdb:28 "Correct the disk.cfg file for %s.\n" "$TARGET_DEVICE";
	pfmt -l $label -s '	  ' -g $msgdb:29 "This file is in /etc/conf/pack.d/%s.\n" "$TARGET_DEVICE";
	pfmt -l $label -s '	  ' -g $msgdb:30 "Consult the disk.cfg(4) manual page for more details.\n";
	exit 2;
elif [ "$IOADDR" != "0-0" ]
then
	IO_LIST=`pdimerge $IOADDR`
	if [ -z "$IO_ADDR" ]
	then
		IO_ADDR=`/bin/expr "$IOADDR" : '\([^-]*\)'`
	fi

	EIO_ADDR=`validate_list $IO_ADDR "$IOADDR"`
	if [ -z "$EIO_ADDR" ]
	then
		pfmt -l $label -s error -g $msgdb:37 "Invalid starting I/O address specified -- %s\n" "$IO_ADDR";
		pfmt -l $label -s action -g $msgdb:38 "Choose an I/O address from this list:\n";
		echo $IO_LIST | pdifold 48;
		exit 2;
	fi

	if [ $ADDING = true ]
	then
		if io_in_use $IO_ADDR $EIO_ADDR;
		then
			pfmt -l $label -s action -g $msgdb:39 "Choose another I/O address from this list:\n";
			echo $IO_LIST | pdifold 48;
			exit 2;
		fi
	fi
else
	IO_ADDR=0
	EIO_ADDR=0
fi

if [ -z "$MEMADDR" ]
then
	pfmt -l $label -s error -g $msgdb:40 "There is no value for MEMADDR in the disk.cfg file for %s.\n" "$TARGET_DEVICE";
	pfmt -l $label -s action -g $msgdb:28 "Correct the disk.cfg file for %s.\n" "$TARGET_DEVICE";
	pfmt -l $label -s '	  ' -g $msgdb:29 "This file is in /etc/conf/pack.d/%s.\n" "$TARGET_DEVICE";
	pfmt -l $label -s '	  ' -g $msgdb:30 "Consult the disk.cfg(4) manual page for more details.\n";
	exit 2;
elif [ "$MEMADDR" != "0-0" ]
then
	MEM_LIST=`pdimerge $MEMADDR`
	if [ -z "$MEM_ADDR" ]
	then
		MEM_ADDR=`/bin/expr "$MEMADDR" : '\([^-]*\)'`
	fi

	EMEM_ADDR=`validate_list $MEM_ADDR "$MEMADDR"`
	if [ -z "$EMEM_ADDR" ]
	then
		pfmt -l $label -s error -g $msgdb:41 "Invalid starting memory address specified -- %s\n" "$MEM_ADDR";
		pfmt -l $label -s action -g $msgdb:42 "Choose a memory address from this list:\n";
		echo $MEM_LIST | pdifold 48;
		exit 2;
	fi

	if [ $ADDING = true ]
	then
		if mem_in_use $MEM_ADDR $EMEM_ADDR;
		then
			pfmt -l $label -s action -g $msgdb:43 "Choose another memory address from this list:\n";
			echo $MEM_LIST | pdifold 48;
			exit 2;
		fi
	fi
else
	MEM_ADDR=0
	EMEM_ADDR=0
fi

if [ $REMOVING = true ]
then
	if [ $YES_COUNT -gt 1 ]
	then
		MATCH=`/bin/grep "	Y	$DMA_CHAN	" ${TEMPFILE}4`
			if [ -z "${MATCH}" ]
		then
			pfmt -l $label -s error -g $msgdb:13 "Invalid DMA channel specified -- %s\n" "$DMA_CHAN";
			pfmt -l $label -s '	  ' -g $msgdb:44 "DMA channel %s is not in use by device %s\n" "$DMA_CHAN" "$TARGET_DEVICE";
			pfmt -l $label -s action -g $msgdb:32 "Choose another DMA channel from this list:\n";
			pfmt -l $label -s '	  ' -g $msgdb:1 "%s\n" "`/bin/cut -f3 ${TEMPFILE}4 | /bin/pr -8 -w72 -a -t -s,`";
			exit 2;
		fi
		RESULT=`echo "${MATCH}" | /bin/cut -f4`
		if [ $RESULT -ne ${INT_VECT} ]
		then
			pfmt -l $label -s error -g $msgdb:16 "Invalid Interrupt Vector specified -- %s\n" "$INT_VECT";
			pfmt -l $label -s '	  ' -g $msgdb:45 "Interrupt Vector %s and DMA channel %s\nare not used by the same instance of %s\n" "$INT_VECT" "$DMA_CHAN" "$TARGET_DEVICE";
			pfmt -l $label -s action -g $msgdb:35 "Choose another Interrupt Vector from this list:\n";
			pfmt -l $label -s '	  ' -g $msgdb:1 "%s\n" "`/bin/cut -f4 ${TEMPFILE}4 | /bin/pr -8 -w72 -a -t -s,`";
			exit 2;
		fi
		RESULT=`echo "${MATCH}" | /bin/cut -f5`
		if hex_compare "${RESULT}" "${IO_ADDR}"
		then
			pfmt -l $label -s error -g $msgdb:37 "Invalid starting I/O address specified -- %s\n" "$IO_ADDR";
			pfmt -l $label -s '	  ' -g $msgdb:46 "I/O address %s, Interrupt Vector %s and DMA channel %s\nare not used by the same instance of %s\n" "$IO_ADDR" "$INT_VECT" "$DMA_CHAN" "$TARGET_DEVICE";
			pfmt -l $label -s action -g $msgdb:39 "Choose another I/O address from this list:\n";
			pfmt -l $label -s '	  ' -g $msgdb:1 "%s\n" "`/bin/cut -f5 ${TEMPFILE}4 | /bin/pr -8 -w72 -a -t -s,`";
			exit 2;
		fi
		RESULT=`echo "${MATCH}" | /bin/cut -f6`
		if hex_compare "${RESULT}"  "${MEM_ADDR}"
		then
			pfmt -l $label -s error -g $msgdb:41 "Invalid starting memory address specified -- %s\n" "$MEM_ADDR";
			pfmt -l $label -s '	  ' -g $msgdb:47 "Memory address %s, I/O address %s, Interrupt Vector %s\nand DMA channel %s are not used by the same instance of %s\n" "$MEM_ADDR" "$IO_ADDR" "$INT_VECT" "$DMA_CHAN" "$TARGET_DEVICE";
			pfmt -l $label -s action -g $msgdb:43 "Choose another memory address from this list:\n";
			pfmt -l $label -s '	  ' -g $msgdb:1 "%s\n" "`/bin/cut -f6 ${TEMPFILE}4 | /bin/pr -8 -w72 -a -t -s,`";
			exit 2;
		fi
		MATCH_COUNT=`/bin/grep -n "	Y	$DMA_CHAN	" ${TEMPFILE}4 | /bin/cut -f1 -d':'`
	else
		MATCH_COUNT=1
	fi
fi

if [ $ADDING = true ]
then
	adding;
else
	if [ "$YES_COUNT" -gt 0 ]
	then
		if [ "$YES_COUNT" -eq 1 ]
		then
			remove_single;
		else
			remove_multi;
		fi
	else
		/bin/cat ${TEMPFILE}1 > ${TEMPFILE}3
	fi
	fix_unit;
fi

#
# 	OK, now let's do the diskcfg and then an idbuild so the config will
#	automatically update upon reboot.
#

eval "$DISKCFG $PDI_OPT ${TEMPFILE}3"
if [ $? -ne 0 ]
then
	pfmt -l $label -s error -g $msgdb:48 "An error has occurred while running %s\n" "$DISKCFG";
	pfmt -l $label -s info -g $msgdb:49 "No changes have been made to your system.\n";
	exit 4;
fi

if [ $TO_STDOUT = true ]
then
	MACH= ROOT=$CONF_ROOT $IDBUILD 1>/dev/null
	/bin/cat ${TEMPFILE}3
else
	MACH= ROOT=$CONF_ROOT $IDBUILD
fi

/bin/rm -f ${TEMPFILE}*
