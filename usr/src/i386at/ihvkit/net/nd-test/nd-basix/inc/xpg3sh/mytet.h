EXIT=0				# Start EXIT at 0; Set to 1 at any Failure
PASS=PASS			# tet_result will use PASS or FAIL (1)
FAIL=FAIL			# tet_result will use PASS or FAIL (1)
WARNING=WARNING			# tet_result will use WARNING also !!
CORE_DUMP=CORE_DUMP		# tet_result will use CORE_DUMP to signify
				# that a core file was produced.
RESULT=${PASS}			# start with pass result
TET_NSIG=20			# set signal 1 higher than system high

get_preamble()
{
  PREAMBLE="TEST CASE: ${TET_PNAME}, TEST PURPOSE: ${tet_thistest}, MESSAGE: "
}

fatal_err() # simple function that handles fatal error messages
{
  get_preamble
  echo "FATAL ERROR -- ${PREAMBLE} $1"
  exit=2
}

nfatal_err() # simple functions that handles non-fatal error messages
{
  get_preamble
  RESULT=${FAIL}
  echo "NON-FATAL ERROR -- ${PREAMBLE} $1"
  EXIT=1	#Don't quit, but mark failure
}

check_stderr() # check that stderr matches expected error
{
  diff out.stderr out.experr >/dev/null 	# diff received and expected res
  if [ $? -ne 0 ]; then
    nfatal_err "Unexpected output written to stderr, as shown below"
    nfatal_err "Expected output:"
    cat out.experr				# Show this file
    nfatal_err "Received output:"
    cat out.stderr				# Show this file
    RESULT=${FAIL}				# set tp result to failure
    EXIT=1					# set flag for exit of test case
  fi
}

check_nostdout() # check that there is no stdout 
{
  if [ -f out.stdout ]; then
    nfatal_err "Unexpected output written to stout, as shown below"
    cat out.stdout				# Show this file
    RESULT=${FAIL}				# set tp result to failure
    EXIT=1					# set flag for exit of test case
  fi
}

check_nostderr() # check that there is no stderr with greater 0 size
{
  if [ -s out.stderr ]; then
    nfatal_err "Unexpected output written to stderr, as shown below"
    cat out.stderr				# Show this file
    RESULT=${FAIL}				# set tp result to failure
    EXIT=1					# set flag for exit of test case
  fi
}

send_tetinfo() # output tet_infoline w/message pluse testcase/purpose id
{
  get_preamble
  tet_infoline "${PREAMBLE} $1"
}

tp_test() # execute test purpose ${TEST_LINE}
{
  eval "${TEST_LINE}" >out.stdout 2>out.stderr  # execute test
  REC_CODE=$?					# save exit code
}

tpresult() # check expected exit code (41) against recived code ($2)
{
  if [ $1 != $2 ]; then
    nfatal_err "Expected exit code -$1; Received exit code - $2"
    RESULT=${FAIL}	# set tp result to failure
    EXIT=1		# set flag for exit
  fi
  tet_result ${RESULT}	# tell TET final result of tp
  unset EXP_CODE REC_CODE TEST_LINE	# cleanup variables
  RESULT=${PASS}

# Clean up the re-directed files

  if [ -f out.stdout ]; then
    rm -f out.stdout
  fi

  if [ -f out.stderr ]; then
    rm -f out.stderr
  fi

  if [ -f out.experr ]; then
    rm -f out.experr
  fi
  
}

start_message() # print test begin information
{
  tet_infoline "BEGIN TEST: ${TET_PNAME}, DATE : 'date'; TESTER: ${LOGNAME}"
}

end_message() # print test begin information
{
  tet_infoline "END TEST: ${TET_PNAME}, DATE : 'date'"
  #exit_grace
}

. $TET_ROOT/lib/xpg3sh/tcm.sh
. $TET_ROOT/lib/xpg3sh/tetapi.sh
