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
# SCCS:		@(#)tcm.sh	1.9 03/09/92
# NAME:		Shell Test Case Manager
# PRODUCT:	TET (Test Environment Toolkit)
# AUTHOR:	Andrew Dingwall, UniSoft Ltd.
# DATE CREATED:	1 November 1990
#
# DESCRIPTION:
#	This file contains the support routines for the sequencing and control
#	of invocable components and test purposes.
#	It should be sourced (by means of the shell . command) into a shell
#	script containing definitions of the invocable components and test
#	purposes that may be executed, after those definitions have been made.
#	Test purposes may be written as shell functions or executable
#	shell scripts.
#
#	This file sources tetapi.sh which contains the shell API functions.
#	Test purposes written as separate shell scripts must also source
#	tetapi.sh in order to use those functions.
#
#	The user-supplied shell variable iclist should contain a list of all
#	the invocable components in the testset;
#	these are named ic1, ic2 ... etc.
#	For each invocable component thus specified, the user should define
#	a variable whose name is the same as that of the component.
#	Each such variable should contain the names of the test purposes
#	associated with each invocable component; for example:
#		iclist="ic1 ic2"
#		ic1="test1-1 test1-2 test1-3"
#		ic2="test2-1 test2-2"
#
#	The NUMBERS of the invocable components to be executed are specified
#	on the command line.
#	In addition, the user may define the variables tet_startup and
#	tet_cleanup; if defined, the related functions (or shell scripts)
#	are executed at the start and end of processing, respectively.
#
#	The TCM makes the NAME of the currently executing test purpose
#	available in the environment variable tet_thistest.
#
#	The TCM reads configuration variables from the file specified by the
#	TET_CONFIG environment variable; these are placed in the environment
#	and marked as readonly.
#	This file (or the environment) should contain an assignment for
#	TET_NSIG which should be set to one greater than the highest signal
#	number supported by the implementation.
#
# MODIFICATIONS:
#
#	Geoff Clare, 11 Oct 1991
#		Replace signal lists with markers to be edited by make INSTALL.
#		Remove local TET_VERSION to avoid conflict with env. variable.
#
#	Kevin Currey    Friday, November 15, 1991
#		For HP-PA OSF/1 and Domain/OS and HP-UX
#		converted to ksh bindings
#                   Thursday, December 12, 1991
#       enhanced tet_setsigs functionality
#                   Wednesday, July 20, 1992
#       added exec to make sure tests do not grap TCM's stdin
#                   Thursday, October 8, 1992
#       fixed {1-3,5-7} scenario bug
#
#	Geoff Clare, 29 Jan 1992
#		Implement TET_TRAP_FUNCTION in place of tet_setsigs(), and
#		TET_DEFAULT_SIGS in place of tet_defaultsigs().
#
#		ETET 1.10.2 Update. Add TET_NSIG_NUM to allow NSIG to
#		be set in the makefile. Other ETET enhancements are
#		in TP numbering and in journal path handling.
#		Andrew Josey, UNIX System Labs. Inc. October 1993
#
#		ETET1.10.2'. Bug fix to tet_xres handling.
#		Andrew Josey, UNIX System Labs. Inc. November 1993.
#
#		ETET1.10.2 Patch level 4. Further bug fixes for
#		xpg3sh binding compatibility (jss@apollo.hp.com).
#		Andrew Josey, UNIX System Labs. Inc. January 1994.
#
#		Bug fix as per xpg3sh/api. It was possible for grep -s
#		to produce unwanted output in some situations.
#		Also add support for TET_EXTENDED=T/F
#		Andrew Josey, Novell UNIX System Labs, February 1994
#
#		Increment TET_VERSION to 1.10.3
#		Andrew Josey, Novell UNIX System Labs, March 1994
#
# ***********************************************************************

if let TET_XXXXX=1
then unset TET_XXXXX
else echo "ERROR: this is not a Korn Shell"
     exit 2
fi

#
# TCM signal definitions
#
# The XXX_SIGNAL_LIST markers are replaced with proper lists by make INSTALL
#

# standard signals - may not be specified in TET_SIG_IGN and TET_SIG_LEAVE
TET_STD_SIGNALS="STD_SIGNAL_LIST"

# signals that are always unhandled
TET_SPEC_SIGNALS="SPEC_SIGNAL_LIST"


## TET_EXTENDED
if [ "$TET_EXTENDED" != "" ] ; then
	TET_EXTENDED=`echo $TET_EXTENDED|tr "[a-z]" "[A-Z]"|cut -c1`
fi

#
# TCM global variables
#

tet_thistest=""; export tet_thistest

#
# "private" TCM variables
#

TET_CWD=`pwd`

#start ETET additions
TET_HOSTNAME=`uname -n`
TET_TMP_DIR=${TET_TMP_DIR:=.}
TET_PRIVATE_TMP=$TET_TMP_DIR/${TET_HOSTNAME}$$; readonly TET_PRIVATE_TMP
export TET_PRIVATE_TMP
mkdir $TET_PRIVATE_TMP
if [ $? != 0 ]; then
        echo Cannot Make temporary directory
        exit 1
fi

TET_DELETES=$TET_PRIVATE_TMP/tet_deletes; readonly TET_DELETES; export TET_DELETES
TET_RESFILE=$TET_CWD/tet_xres; readonly TET_RESFILE; export TET_RESFILE
TET_STDERR=$TET_PRIVATE_TMP/tet_stderr; readonly TET_STDERR; export TET_STDERR
TET_TESTS=$TET_PRIVATE_TMP/tet_tests; readonly TET_TESTS
TET_TMPRES=$TET_PRIVATE_TMP/tet_tmpres; readonly TET_TMPRES; export TET_TMPRES

#end ETET additions

TET_BLOCK=0; export TET_BLOCK
TET_CONTEXT=0; export TET_CONTEXT
TET_EXITVAL=0
TET_SEQUENCE=0; export TET_SEQUENCE
TET_TPCOUNT=0; export TET_TPCOUNT

#start ETET addition
TET_TPNUMBER=0; export TET_TPNUMBER
TET_TP_LIST=""
#end ETET addition

TET_TMP1=$TET_PRIVATE_TMP/tet1.$$
TET_TMP2=$TET_PRIVATE_TMP/tet2.$$

# ***********************************************************************

#
# "private" TCM date and tr functions
#

# initialize $SECONDS
TET_date1=`date`
TET_temp=${TET_date1%??:??:??*}
TET_date1=${TET_date1#$TET_temp}
TET_temp=${TET_date1#??:??:??}
TET_date1=${TET_date1%$TET_temp}
TET_h1=${TET_date1%:??:??}
TET_s1=${TET_date1#??:??:}
TET_m1=${TET_date1#$TET_h1:}
TET_m1=${TET_m1%:$TET_s1}
let SECONDS=TET_s1+60*TET_m1+3600*TET_h1
typeset -RZ2 TET_h1 TET_m1 TET_s1

# update $TET_DATE using $SECONDS
tet_getdate(){
     let TET_s1=SECONDS%60
     let TET_m1=SECONDS/60%60
     let TET_h1=SECONDS/3600%24
     TET_DATE=${TET_h1}:${TET_m1}:$TET_s1
}

#
# this function emulates the command tr for simple substitutions
# Usage: ksh_tr $1 $2 $3
# where ksh_tr substitutes $2 for $1 in $3
#
tet_ksh_tr(){
     unset TET_out
     TET_in=$3
     TET_token=${TET_in%%$1*}
     while (( ${#TET_token} != ${#TET_in} ))
     do
       TET_in=${TET_in##${TET_token}$1}
       TET_out=${TET_out}${TET_token}$2
       TET_token=${TET_in%%$1*}
     done
     TET_out=${TET_out}$TET_in
     print -R "$TET_out"
}

#
# "private" TCM function definitions
# these interfaces may go away one day
#

# tet_ismember - return 0 if $1 is in the set $2 ...
# otherwise return 1
tet_ismember(){
	TET_X=${1:?}
	shift
	for TET_Y in $*
	do
		if test 0$TET_X -eq $TET_Y
		then
			TET_MEMBER=$TET_Y
			return 0
		fi
	done
	return 1
}

# tet_abandon - signal handler used during startup and cleanup
tet_abandon(){
	TET_CAUGHTSIG=$1
	if test 15 -eq ${TET_CAUGHTSIG:?}
	then
		tet_sigterm $TET_CAUGHTSIG
	else
		tet_error "Abandoning testset: caught unexpected signal $TET_CAUGHTSIG"
	fi
	TET_EXITVAL=$TET_CAUGHTSIG exit
}

# tet_sigterm - signal handler for SIGTERM
tet_sigterm(){
	TET_CAUGHTSIG=$1
	tet_error "Abandoning test case: received signal ${TET_CAUGHTSIG:?}"
	tet_docleanup
	TET_EXITVAL=$TET_CAUGHTSIG exit
}

# tet_sigskip - signal handler used during test execution
tet_sigskip(){
	TET_CAUGHTSIG=$1
	tet_infoline "unexpected signal ${TET_CAUGHTSIG:?} received"
	tet_result UNRESOLVED
	if test 15 -eq ${TET_CAUGHTSIG:?}
	then
		tet_sigterm $TET_CAUGHTSIG
	else
		continue
	fi
}

# tet_tpend - report on a test purpose
tet_tpend(){
	TET_TPARG1=${1:?}
	TET_RESULT=
		while read TET_NEXTRES
		do
			if test -z "$TET_RESULT"
			then
				TET_RESULT="$TET_NEXTRES"
				continue
			fi
			case "$TET_NEXTRES" in
			PASS)
				;;
			FAIL)
				TET_RESULT="$TET_NEXTRES"
				;;
			UNRESOLVED|UNINITIATED)
				if test FAIL != "$TET_RESULT"
				then
					TET_RESULT="$TET_NEXTRES"
				fi
				;;
			NORESULT)
				if test FAIL != "$TET_RESULT" -a \
					UNRESOLVED != "$TET_RESULT" -a \
					UNINITIATED != "$TET_RESULT"
				then
					TET_RESULT="$TET_NEXTRES"
				fi
				;;
			UNSUPPORTED|NOTINUSE|UNTESTED)
				if test PASS = "$TET_RESULT"
				then
					TET_RESULT="$TET_NEXTRES"
				fi
				;;
			*)
				if test PASS = "$TET_RESULT" -o \
					UNSUPPORTED = "$TET_RESULT" -o \
					NOTINUSE = "$TET_RESULT" -o \
					UNTESTED = "$TET_RESULT"
				then
					TET_RESULT="$TET_NEXTRES"
				fi
				;;
			esac
        done < $TET_TMPRES
        TET_RESULT=$TET_RESULT

	> $TET_TMPRES

	TET_ABORT=NO
	if test -z "$TET_RESULT"
	then
		TET_RESULT=NORESULT
		TET_RESNUM=7
	elif tet_getcode "$TET_RESULT"		# sets TET_RESNUM, TET_ABORT
	then
		: ok
	else
		TET_RESULT="NO RESULT NAME"
		TET_RESNUM=-1
	fi

      tet_getdate
      tet_output 220 "$TET_TPARG1 $TET_RESNUM $TET_DATE" "$TET_RESULT"

	if test YES = "$TET_ABORT"
	then
		TET_TRAP_FUNCTION=tet_abandon
		tet_output 510 "" \
			"ABORT on result code $TET_RESNUM \"$TET_RESULT\""
		if test -n "$tet_cleanup"
		then
			tet_docleanup
		fi
		TET_EXITVAL=1 exit
	fi
}

# tet_docleanup - execute the tet_cleanup function
tet_docleanup(){
	tet_thistest=
	TET_TPCOUNT=0
	TET_BLOCK=0
	tet_setblock
	eval $tet_cleanup
}

tet_get_tpnumber(){
	TET_TPNUMARG1=${1:?}
	TET_I=`echo $TET_TP_LIST|sed "s/\<$TET_TPNUMARG1\>.*\$//
		s/[^    ][^     ]*//g"|wc -c`
	return $TET_I
}

tet_add_tp(){
        TET_TPADDARG1=${1:?}
        TET_TMP_RESULT=`echo $TET_TP_LIST|(grep -s $TET_TPADDARG1 >/dev/null 2>&1; echo $?)`
        if [ $TET_TMP_RESULT = 1 ] ; then
                TET_TP_LIST="$TET_TP_LIST $TET_TPADDARG1"
        fi
        unset TET_TMP_RESULT
}

# ***********************************************************************

# read in API functions
. ${TET_ROOT:?}/lib/ksh/tetapi.ksh

# ***********************************************************************

#
# TCM main flow
#

# capture command line args before they disappear
TET_TCM_ARGC=$#
TET_TCM_ARGS="$*"
TET_PNAME="$0"; readonly TET_PNAME; export TET_PNAME

# arrange to clean up on exit
unset TET_TMPFILES
trap 'exit 1' 1 2 3 15
trap 'rm -f $TET_TMPFILES; rm -rf $TET_PRIVATE_TMP; exit $TET_EXITVAL' 0

# open execution results file
umask 0; rm -f $TET_RESFILE $TET_DELETES $TET_STDERR $TET_TESTS $TET_TMP1 $TET_TMPRES; > $TET_RESFILE
if [ -s $TET_RESFILE ]
then TET_EXITVAL=1; exit 1
fi

# open other local files
for TET_A in $TET_DELETES $TET_STDERR $TET_TESTS \
	$TET_TMP1 $TET_TMPRES
do
      TET_TMPFILES=${TET_TMPFILES}' '$TET_A
	> $TET_A
done

# read in configuration variables and make them readonly
# strip comments and other non-variable assignments
# protect embedded spaces and single quotes in the value part
if test -n "$TET_CONFIG"
then
	if test ! -r "$TET_CONFIG"
	then
		tet_error "can't read config file" $TET_CONFIG
	else
            while read TET_line
            do
              TET_nline=${TET_line%%\#*}
              if (( ${#TET_nline} != 0 ))
              then readonly "$TET_nline"
              fi
            done < $TET_CONFIG
	fi
fi

# set current context to process ID
tet_setcontext

# set up default results code file if so required
if test ! -r ${TET_CODE:=tet_code}
then
	if test tet_code != "$TET_CODE"
	then
		tet_error "could not open results code file" \"$TET_CODE\"
	fi
      TET_TMPFILES=${TET_TMPFILES}' '$TET_TMP2
	echo "
0	PASS		Continue
1	FAIL		Continue
2	UNRESOLVED	Continue
3	NOTINUSE	Continue
4	UNSUPPORTED	Continue
5	UNTESTED	Continue
6	UNINITIATED	Continue
7	NORESULT	Continue" > $TET_TMP2
	TET_CODE=$TET_TMP2
fi
case $TET_CODE in
/*)
	;;
*)
	TET_CODE=`pwd`/$TET_CODE
	;;
esac
readonly TET_CODE; export TET_CODE

# process command-line args
if test 1 -gt $TET_TCM_ARGC
then
	TET_TCM_ARGS=all
fi
TET_ICLAST=-1
      TET_ICLIST=$(tet_ksh_tr ic "" "$iclist")
: ${TET_ICLIST:=0}
      TET_ICLIST=$(print $TET_ICLIST)
      TET_ICFIRST_DEF=${TET_ICLIST%%\ *}
for TET_A in $(tet_ksh_tr , " " "$TET_TCM_ARGS")
do
	case $TET_A in
	all*)
		if test 0 -ge $TET_ICLAST
		then
			TET_ICFIRST=$TET_ICFIRST_DEF
			for TET_B in $TET_ICLIST
			do
				if test $TET_B -le $TET_ICFIRST
				then
					TET_ICFIRST=$TET_B
				fi
			done
		else
                let TET_ICFIRST=TET_ICLAST+1
		fi
		TET_ICLAST=$TET_ICFIRST
		for TET_B in $TET_ICLIST
		do
			if test $TET_B -gt $TET_ICLAST
			then
				TET_ICLAST=$TET_B
			fi
		done
		if test $TET_ICLAST -gt ${TET_B:=0}
		then
			TET_ICLAST=$TET_B
		fi
		;;
	*)
        TET_ICFIRST=${TET_A%%-*}
        TET_ICLAST=${TET_A##*-}
        if [ X$TET_ICFIRST = X$TET_ICLAST ]
        then TET_ICLAST=
        fi
		;;
	esac
	TET_ICNO=${TET_ICFIRST:-$TET_ICFIRST_DEF}
	while test $TET_ICNO -le ${TET_ICLAST:=$TET_ICNO}
	do
		if tet_ismember $TET_ICNO $TET_ICLIST
		then
			test -n "`eval echo \\${ic$TET_MEMBER}`" && \
				echo ic$TET_MEMBER
		else
			tet_error "IC $TET_MEMBER is not defined" \
				"for this test case"
		fi
            let TET_ICNO=TET_ICNO+1
	done >> $TET_TESTS
done
      TET_ICCOUNT=0
      while read -r TET_line
      do
        let TET_ICCOUNT=TET_ICCOUNT+1
      done < $TET_TESTS

# print startup message to execution results file
tet_output 15 "1.10.3 $TET_ICCOUNT" "TCM Start"

# do initial signal list processing
if (( 0 == ${#TET_SIG_LEAVE} + ${#TET_SIG_IGN} ))
then print TET_SIG_LEAVE2=\"\\n\"\\nTET_SIG_IGN2=\"\\n\" > $TET_TMP1
else for TET_A in TET_SIG_LEAVE TET_SIG_IGN
do
	echo ${TET_A}2=\"
    eval TET_temp="\$$TET_A"
	tet_ksh_tr , "
" "$TET_temp" | while read TET_B TET_JUNK
	do
		if test -z "$TET_B"
		then
			continue
		elif tet_ismember $TET_B $TET_STD_SIGNALS $TET_SPEC_SIGNALS
		then
			tet_error "warning: illegal entry $TET_B" \
				"in $TET_A ignored"
		else
			echo $TET_B
		fi
	done
	echo \"
done > $TET_TMP1
fi
. $TET_TMP1
TET_SIG_LEAVE2="$TET_SIG_LEAVE2 $TET_SPEC_SIGNALS"
TET_A=1
#: ${TET_NSIG:?}
if [ -z "$TET_NSIG" ] ; then
        TET_NSIG=TET_NSIG_NUM; export TET_NSIG
fi

TET_TRAP_FUNCTION=tet_abandon
TET_DEFAULT_SIGS=
while test $TET_A -lt $TET_NSIG
do
	if tet_ismember $TET_A $TET_SIG_LEAVE2
	then
		:
	elif tet_ismember $TET_A $TET_SIG_IGN2
	then
		trap "" $TET_A
	else
		trap "trap \"\" $TET_A; \$TET_TRAP_FUNCTION $TET_A" $TET_A
		TET_DEFAULT_SIGS="$TET_DEFAULT_SIGS $TET_A"
	fi
      let TET_A=TET_A+1
done

#start ETET addition
for TET_ICNAME in $iclist
do
        eval TET_TPLIST=\"\$$TET_ICNAME\"
        for TET_TP in $TET_TPLIST
        do
                tet_add_tp $TET_TP
        done
done
#end ETET addition


# do startup processing
eval $tet_startup

# do main loop processing
exec 3< $TET_TESTS
while read -u3 -r TET_ICNAME
do
	eval TET_TPLIST=\"\$$TET_ICNAME\"
      TET_ICNUMBER=${TET_ICNAME##?[!0123456789]}
      let TET_TPCOUNT=0
      for TET_temp in $TET_TPLIST
      do
        let TET_TPCOUNT=TET_TPCOUNT+1
      done
      tet_getdate
      tet_output 400 "$TET_ICNUMBER $TET_TPCOUNT $TET_DATE" "IC Start"
	TET_TPCOUNT=0
	for tet_thistest in $TET_TPLIST
	do
            let TET_TPCOUNT=TET_TPCOUNT+1
#        tet_output 200 "$TET_TPCOUNT $TET_DATE" "TP Start"
#		> $TET_TMPRES
	tet_get_tpnumber $tet_thistest
	TET_TPNUMBER=$?
	# this forces BLOCK and SEQUENCE to 1
        TET_CONTEXT=0
        tet_setcontext
        tet_getdate
        tet_output 200 "$TET_TPNUMBER $TET_DATE" "TP Start"

		TET_REASON="`tet_reason $tet_thistest`"
		if test $? -eq 0
		then
			tet_infoline "$TET_REASON"
			tet_result UNINITIATED
		else
			TET_TRAP_FUNCTION=tet_sigskip
			(
				trap $TET_DEFAULT_SIGS
				unset TET_DEFAULT_SIGS
				"$tet_thistest"
			)
		fi
		tet_tpend $TET_TPNUMBER
	done
	TET_TPNUMBER=0
      tet_getdate
      tet_output 410 "$TET_ICNUMBER $TET_TPCOUNT $TET_DATE" "IC End"
done

# do cleanup processing
TET_TRAP_FUNCTION=tet_abandon
if test -n "$tet_cleanup"
then
	tet_docleanup
fi

# successful exit
TET_EXITVAL=0 exit
