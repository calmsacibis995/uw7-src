:
# uname-tc.sh : test case for uname command

tet_startup=""				# no startup function
tet_cleanup="cleanup"			# cleanup function
iclist="ic1 ic2"			# list invocable components
ic1="tp1"				# functions for ic1
ic2="tp2"				# functions for ic2


tp1() # simple uname of file - successful: exit 0
{
    tpstart "UNAME OUTPUT FOR MANUAL CHECK"

    check_exit "uname -a" 0		# check exit value

    infofile out.stdout			# send output to journal

    check_nostderr			# should be no stderr

    tpresult INSPECT			# set result code
}

tp2() # uname with invalid syntax: exit non-zero
{
    tpstart "UNAME WITH INVALID SYNTAX: EXIT NON-ZERO"

    # expected error message
    echo "uname: illegal option -- :\n.*" > out.experr

    check_exit "uname -:" N		# check exit value

    check_nostdout			# should be no stdout
    check_stderr out.experr		# check error message

    tpresult				# set result code
}

cleanup() # clean-up function
{
     rm -f out.stdout out.stderr out.experr
}

# source common shell functions
. $TET_EXECUTE/lib/shfuncs

# execute shell test case manager - must be last line
. $TET_ROOT/lib/xpg3sh/tcm.sh
