#!/sbin/sh

#ident	"@(#)up_merge.sh	15.1"
#ident	"$Header$"

## This script merges changes in UnixWare 1.1 files from the original
## installed files.
## The merging is done using the "patch" command. 
## The patch command, a patch file as its input, when applied on a 
## file will result in a merged file.
##
## patch returns 0 if everything is OK; non-zero, otherwise.
## If patch fails it tells which hunk failed and the failed hunk(s)
## are saved in reject files.
##
## upgrade_merge return codes:
##
##	0	Success
##	2	Failure
##	3	Usage Error
##	
## USAGE=upgrade_merge 	<file>

process_merge() {

	[ "$UPDEBUG" = "YES" ] && set -x

	while read FILENAME CONV CONV_CMD PATCH
	do
		echo $FILENAME $CONV $CONV_CMD $PATCH
		[ "$UPDEBUG" = "YES" ] && goany 

		USERFILE=${UP_USER}/${FILENAME}
		SVR4_2FILE=${SVR4_2DIR}/${FILENAME}
		MERGEDFILE=${MERGEDIR}/${FILENAME}
		REJECTFILE=${REJECTDIR}/${FILENAME}
		RTNCODE1=0
		RTNCODE2=0

		rm -f ${MERGEDFILE}* ${REJECTFILE}*

		# Check if the (saved) file exists; Also check if saved 
		# file is not a symbolic link

		[ ! -f ${USERFILE} -o -h ${USERFILE} ] && {
			continue
		}

		case ${FILENAME} in

		[a-zA-Z]*)

			mkdir -p `dirname ${MERGEDFILE}` 2>/dev/null

# If special merge script indicated, run it and continue to next file

			if [ "${CONV}" = "Y" ]
			then
				export ROOT FILENAME UP_ORIG UP_USER MERGEDFILE
				$CONV_CMD 
				RTNCODE1=${?}

			elif [ "${PATCH}" = "Y" ]
			then

		# If there is no original file, restore the saved version
				[ ! -f ${UP_ORIG}/${FILENAME} ] && {
					cp $UP_USER/$FILENAME ${ROOT}/${FILENAME} 2>> ${UPERR}
					[ ${?} -ne 0 ] && RTNCODE=1
					continue
				}


			# Certain files are considered special: these files will not
			# be merged; they will be saved at a standard place.

				diff -e $UP_ORIG/$FILENAME $UP_USER/$FILENAME >/tmp/patchfile
				if [ ${?} -ne 0 ]
				then
					# merge needed.

					[ "$UPDEBUG" = "YES" ] && goany 

					if [ "${PATCH}" = "Y" ]
					then
						mkdir -p `dirname ${REJECTFILE}` 2>/dev/null
						${PATCHCMD} ${OPTIONS} -i /tmp/patchfile -o ${MERGEDFILE} -r ${REJECTFILE}  /${FILENAME} 2>> ${UPERR}
					fi
					RTNCODE2=${?}

				fi
			fi
			;;

		*) continue ;;
		esac

		if [ ${RTNCODE1} -ne 0  -o ${RTNCODE2} -ne 0 ]
		then
			echo ${FILENAME} >> ${REJECTFILE_LIST}
		else

			# copy merged file to appropriate place
			# cp does preserve modes/owner/group

			cp ${MERGEDFILE} ${ROOT}/${FILENAME} 2>> ${UPERR}
		fi
	done < ${1}

	
}

########################################################
## MAIN starts here

SBINPKGINST=/usr/sbin/pkginst

. $SBINPKGINST/updebug

[ "$UPDEBUG" = "YES" ] && set -x

OPTIONS="-e" 	## options to patch tool
PATCHCMD=$SBINPKGINST/patch	 ## path for PATCH command 
TMP=/tmp
ROOT=/

UPGRADEDIR=${ROOT}/etc/inst
SVR4_2DIR=${UPGRADEDIR}/SVR4.2
REJECTDIR=${UPGRADEDIR}/reject
MERGEDIR=${UPGRADEDIR}/merge
UP_ORIG=$UPGRADEDIR/save.orig
UP_USER=$UPGRADEDIR/save.user
FILE_LIST=${1}
REJECTFILE_LIST=${1}.rej
rm -f ${REJECTFILE_LIST}

[ "$UPDEBUG" = "YES" ] && goany

for i in ${REJECTDIR} ${MERGEDIR} 
do
	if [ ! -d ${i} ]
	then mkdir -p ${i}
	fi
done

USAGE="\nupgrade_merge usage: \tupgrade_merge <file>"

# Remove comments and blank lines

cat $FILE_LIST |
	grep -v '^[ 	]*#' |
	grep -v '^[ 	]*$' >/tmp/merge.$$

[ "$UPDEBUG" = "YES" ] && goany
 
if [ ${#} -eq 1  ]
then
	process_merge /tmp/merge.$$
else
	echo ${USAGE}
   	exit 3
fi

rm -f /tmp/merge.$$			# clean up /tmp

[ "$UPDEBUG" = "YES" ] && goany

if [ -f ${REJECTFILE_LIST} ]
then
	exit 2		# at least one failure
else
	exit 0		# succcess
fi

