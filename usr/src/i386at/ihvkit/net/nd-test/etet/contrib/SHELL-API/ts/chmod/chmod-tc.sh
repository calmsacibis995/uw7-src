:
# chmod-tc.sh : test case for chmod command

tet_startup=""				# no startup function
tet_cleanup="cleanup"			# cleanup function
iclist="ic1 ic2 ic3"			# list invocable components
ic1="tp1"				# functions for ic1
ic2="tp2"				# functions for ic2
ic3="tp3"				# functions for ic3


tp1() # simple chmod of file - successful: exit 0
{
    tpstart "SIMPLE CHMOD OF FILE: EXIT 0"

    echo x > chmod.1 2> out.stderr	# create file
    if [ ! -f chmod.1 ]
    then
        tet_infoline "Could not create test file: chmod.1"
        tet_infoline `cat out.stderr`
        tet_result UNRESOLVED
        return
    fi

    check_exit "chmod 777 chmod.1" 0	# check exit value

    MODE=`ls -l chmod.1 |cut -d" " -f1`	# get and check mode of file
    if [ X"$MODE" != X"-rwxrwxrwx" ]
    then
        tet_infoline "chmod 777 set mode to $MODE, expected -rwxrwxrwx"
        FAIL=Y
    fi

    check_nostdout			# should be no stdout
    check_nostderr			# should be no stderr

    tpresult				# set result code
}

tp2() # chmod of non-existent file : exit non-zero
{
    tpstart "CHMOD OF NON-EXISTENT FILE: EXIT NON-ZERO"

    # ensure test file does not exist
    rm -f chmod.2 2> out.stderr
    if [ -f chmod.2 ]
    then
        tet_infoline "Could not remove test file: chmod.2"
        tet_infoline `cat out.stderr`
        tet_result UNRESOLVED
        return
    fi

    check_exit "chmod 777 chmod.2" N	# check exit value

    check_nostdout			# should be no stdout
    check_stderr			# check error message

    tpresult				# set result code
}

tp3() # chmod with invalid syntax: exit non-zero
{
    tpstart "CHMOD WITH INVALID SYNTAX: EXIT NON-ZERO"

    # expected error message
    echo "chmod: illegal option -- :\n.*" > out.experr

    check_exit "chmod -:" N		# check exit value

    check_nostdout			# should be no stdout
    check_stderr out.experr		# check error message

    tpresult				# set result code
}

cleanup() # clean-up function
{
     rm -f out.stdout out.stderr out.experr
     rm -f chmod.1
}

# source common shell functions
. $TET_EXECUTE/lib/shfuncs

# execute shell test case manager - must be last line
. $TET_ROOT/lib/xpg3sh/tcm.sh
