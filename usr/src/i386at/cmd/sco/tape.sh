#!/sbin/sh
#ident	"@(#)sco:tape.sh	1.1.1.5"
#
# Enhanced Application Compatibility Support
# This shell script is intended to behave like SCO's tape command.
# The tape command is implemented as an interface to USL's tapecntl command.
# The following SCO's tape command options are not supported.
#		status	- get the status of the tape 
#		amount	- amount of current or last transfer 
#		wfm	- write filemark 
# Also, options and arguments related to the QIC-40 and QIC-80 minitapes
# are not supported.
# Since one driver now supports both SCSI and non-SCSI devices, the -s and
# -c options are not necessary.  However, they will continue to be supported
# for compatibility.
#
# Name: tape
# Usage:
#	tape usage: tape [-<type>] <command> [device]
#	  type: c (non-SCSI) or s (SCSI)
#	  Cartridge tape commands:
#		reten	- retension the tape
#		erase	- erase and retension tape
#		reset	- reset controller and drive
#		rewind	- rewind tape controller
#		rfm	- skip to next file
#
# exit codes
#	1: Special device files /dev/rmt/ctape1 and ctape2 don't exist
#	2: Specified device does not exist
#	3: Illegal options/arguments

TAPECNTL="/usr/bin/tapecntl"
DFLT_DEVICE=""
command=""
USAGE="\ntape usage: tape [-<type>] <command> [device]\n\
	  type: c (non-SCSI) or s (SCSI)\n\
	  Cartridge tape commands:\n\
	\treten\t- retension the tape\n\
	\terase\t- erase and retension tape\n\
	\treset\t- reset controller and drive\n\
	\trewind\t- rewind tape controller\n\
	\trfm\t- skip to next file\n"

# Check if the default tape file exists
if [ -f /etc/default/tape ]
then
	DFLT_DEVICE=`grep device /etc/default/tape | cut -f2 -d "="` 
fi

# Check if tape device on system
if [ -c /dev/rmt/ctape1 ]
then
	TAPEDEVICE=/dev/rmt/ctape1
elif [ -c /dev/rmt/ctape2 ]
then
	TAPEDEVICE=/dev/rmt/ctape2
else
	echo "$0: ERROR: /dev/rmt/ctape1 or /dev/rmt/ctape2: No such files"
	exit 1
fi

####
## Note: the -c and -s options are maintained for backward compatibility,
##       and are no longer needed since the advent of PDI. With PDI both
##       SCSI and Non-SCSI tapes are handled in a common manner via
##       the st01(7) target driver.
####
while getopts cs OPTION
do
	case ${OPTION} in
	c)
		;;
	s)
		;;
	\?)
		echo ${USAGE}
		exit 3
	esac
done

shift `expr ${OPTIND} - 1`

command="${1}"

if [ "x${2}" = "x" ]
then
	if [ "${DFLT_DEVICE}" ]
	then
		TAPEDEVICE=${DFLT_DEVICE}
	fi
else
	if [ -c ${2} ]
	then
		TAPEDEVICE=${2}
	elif [ -c /dev/rmt/${2} ]
	then
		TAPEDEVICE="/dev/rmt/${2}"
	else
		echo "Invalid device: ${2}"
		echo ${USAGE}
		exit 3
	fi
fi

case ${command} in 
	reten)
		$TAPECNTL -t $TAPEDEVICE
		;;
	erase)
		$TAPECNTL -e $TAPEDEVICE
		;;
	reset)
		$TAPECNTL -r $TAPEDEVICE
		;;
	rewind)
		$TAPECNTL -w $TAPEDEVICE
		;;
	rfm)
# need to strip off first character (c) and replace it with n - ctape1 -> ntape1
		$TAPECNTL -p 1 `echo ${TAPEDEVICE} | sed 's/ctape/ntape/'`
		;;
	*)
		echo "Invalid command: ${command}"
		echo ${USAGE}
		exit 3
		;;
esac
# End Enhanced Application Compatibility Support
