# Copyright 1990 Open Software Foundation (OSF)
# Copyright 1990 Unix International (UI)
# Copyright 1990 X/Open Company Limited (X/Open)
# Copyright 1991 Hewlett-Packard Co. (HP)
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation, and that the name of HP, OSF, UI or X/Open not be used in
# advertising or publicity pertaining to distribution of the software
# without specific, written prior permission.  HP, OSF, UI and X/Open make
# no representations about the suitability of this software for any purpose.
# It is provided "as is" without express or implied warranty.
#
# HP, OSF, UI and X/Open DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
# EVENT SHALL HP, OSF, UI or X/Open BE LIABLE FOR ANY SPECIAL, INDIRECT OR
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
# USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
#
# ***********************************************************************
#
# SCCS:		@(#)tetapi.sh	1.8 03/09/92
# NAME:		Shell API Support Routines
# PRODUCT:	TET (Test Environment Toolkit)
# AUTHOR:	Andrew Dingwall, UniSoft Ltd.
# DATE CREATED:	1 November 1990
#
# DESCRIPTION:
#	This file contains shell functions for use with the shell API.
#	It is sourced automatically by the shell TCM.
#	In addition it should be sourced by test purposes that are written as
#	separate shell scripts, by means of the shell . command.
#
#	The following functions are provided:
#
#		tet_setcontext
#		tet_setblock
#		tet_infoline
#		tet_result
#		tet_delete
#		tet_reason
#
# MODIFICATIONS:
#
#	Kevin Currey    Friday, November 15, 1991
#		For HP-PA OSF/1 and Domain/OS and HP-UX
#		converted to ksh bindings
#
#	Geoff Clare, 29 Jan 1992
#		Rewrite tet_setcontext() so context number will change.
#
#	ETET1.10.2 Update.
#	TET TP numbering as per ETET scheme, which has consistency
#	between APIs and the TET specification.
#	Andrew Josey, UNIX System Labs, Inc. October 1993.
#
#	ETET1.10.2' 
#	Fix tet_xres handling
#	Andrew Josey, UNIX System Labs, Inc. November 1993.
#
#	Add TET_EXTENDED=T/F handling.
#	Andrew Josey, Novell UNIX System Labs, February 1994
# ***********************************************************************

#
# publicly available shell API functions
#

# set current context and reset block and sequence
# usage: tet_setcontext
# Note that the context cannot be set to $$ because when a subshell
# is started using "( ... )" the value of $$ does not change.
tet_setcontext(){
	# This sets context to a new, unused process ID without
	# generating a zombie process.
	TET_CONTEXT=`(:)& echo $!`
	TET_BLOCK=1
	TET_SEQUENCE=1
}

# increment the current block ID, reset the sequence number to 1
# usage: tet_setblock
tet_setblock(){
    let TET_BLOCK=${TET_BLOCK:?}+1
	TET_SEQUENCE=1
}

# print an information line to the execution results file
# and increment the sequence number
# usage: tet_infoline args [...]
tet_infoline(){
	tet_output 520 "${TET_TPNUMBER:?} ${TET_CONTEXT:?} ${TET_BLOCK:?} ${TET_SEQUENCE:?}" "$*"
      let TET_SEQUENCE=TET_SEQUENCE+1
}

# record a test result for later emmision to the execution results file
# by tet_tpend
# usage: tet_result result_name
# (note that a result name is expected, not a result code number)
tet_result(){
	TET_ARG1="${1:?}"
	if tet_getcode "$TET_ARG1"
	then
		: ok
	else
		tet_error "invalid result name \"$TET_ARG1\"" \
			"passed to tet_result"
		TET_ARG1=NORESULT
	fi

	echo $TET_ARG1 >> ${TET_TMPRES:?}
	unset TET_ARG1
}

# mark a test purpose as deleted
# usage: tet_delete test_name reason [...]
tet_delete(){
	TET_ARG1=${1:?}
	shift
	TET_ARG2N="$*"
	if test -z "$TET_ARG2N"
	then
		tet_undelete $TET_ARG1
		return
	fi

	if tet_reason $TET_ARG1 > /dev/null
	then
		tet_undelete $TET_ARG1
	fi

	echo "$TET_ARG1 $TET_ARG2N" >> ${TET_DELETES:?}
	unset TET_ARG1 TET_ARG2N
}

# print the reason why a test purpose has been deleted
# return 0 if the test purpose has been deleted, 1 otherwise
# usage: tet_reason test_name
tet_reason(){
	: ${1:}
        let TET_return=1
		while read TET_A TET_B
		do
			if test X"$TET_A" = X"$1"
			then
				echo "$TET_B"
                let TET_return=0
				break
			fi
		done < ${TET_DELETES:?}

	return $TET_return
}

# ******************************************************************

#
# "private" functions for internal use by the shell API
# these are not published interfaces and may go away one day
#


# tet_getcode
# look up a result code name in the result code definition file
# return 0 if successful with the result number in TET_RESNUM and TET_ABORT
# set to YES or NO
# otherwise return 1 if the code could not be found
tet_getcode(){
	TET_ABORT=NO
	TET_RESNUM=-1
	: ${TET_CODE:?}

	TET_A="${1:?}"
    while read TET_B
	do
		eval set -- $TET_B
		if test X$2 = X$TET_A
		then
			TET_RESNUM=$1
			TET_ABACTION=$3
			break
		fi
    done < $TET_CODE

	case "$TET_RESNUM" in
	-1)
		unset TET_ABACTION
		return 1
		;;
	esac

	case "$TET_ABACTION" in
	""|Continue)
		TET_ABORT=NO
		;;
	Abort)
		TET_ABORT=YES
		;;
	*)
		tet_error "invalid action field \"$TET_ABACTION\" in file" \
			$TET_CODE
		TET_ABORT=NO
		;;
	esac

	unset TET_ABACTION
	return 0
}

# tet_undelete - undelete a test purpose
tet_undelete(){
	echo "g/^${1:?} /d
w
q" | ed - ${TET_DELETES:?}
}

# tet_error - print an error message to stderr and on TCM Message line
tet_error(){
	echo "$TET_PNAME: $*" 1>&2
	if [ "$TET_EXTENDED" != "T" ]
	then
		echo "510|${TET_ACTIVITY:-0}|$*" >> ${TET_RESFILE:?}
	else
		echo "510|${TET_ACTIVITY:-0}|$*" >> ${TET_JOURNAL_PATH}
	fi
}


# tet_output - print a line to the execution results file
tet_output(){


> ${TET_STDERR:?}
TET_arg1=${1:?}
TET_arg2=$2
TET_arg3=$3
TET_activity=${TET_ACTIVITY:-0}

if (( ${#TET_arg2} > 0 ))
then TET_sp=" "
else TET_sp=""
fi

TET_line="${TET_arg1}|${TET_activity}${TET_sp}${TET_arg2}|$TET_arg3"

# ensure no newline characters in data

TET_tmp="
"
if (( ${#TET_line} != 0 ))
then TET_nline=${TET_line##*${TET_tmp}*}
     if (( ${#TET_nline} == 0 ))
     then TET_x=$TET_line
          unset TET_n
          unset TET_nx
          TET_n=${TET_x##*$TET_tmp}
          while (( ${#TET_n} != ${#TET_x} ))
          do
            TET_x=${TET_x%%${TET_tmp}$TET_n}
            TET_nx=${TET_n}' '$TET_nx
            TET_n=${TET_x##*$TET_tmp}
          done
          TET_line=${TET_n}' '$TET_nx
     fi
fi

# journal lines must not exceed 512 bytes

if (( ${#TET_line} > 511 ))
then TET_nline=${TET_line%${TET_line#???????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????}}
     TET_line=$TET_nline
     print -R "warning: results file line truncated: prefix: ${TET_arg1}|${TET_activity}${TET_sp}${TET_arg2}|" > $TET_STDERR
fi

# line is now OK to print

	if [ "$TET_EXTENDED" != "T" ]
	then
		print -R "$TET_line" >> ${TET_RESFILE:?}
	else
		print -R "$TET_line" >> ${TET_JOURNAL_PATH:?}
	fi

if test -s $TET_STDERR
then unset TET_error_line
     while read TET_i
     do
       TET_error_line=${TET_error_line}${TET_i}
     done < $TET_STDERR
     tet_error "$TET_error_line"
     > $TET_STDERR
fi

}
