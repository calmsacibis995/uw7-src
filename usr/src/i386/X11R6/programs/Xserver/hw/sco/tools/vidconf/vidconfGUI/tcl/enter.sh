:
#
#	@(#) enter.sh 12.1 95/05/09 
#
# Copyright (C) 1994 The Santa Cruz Operation, Inc.
#
# The information in this file is provided for the exclusive use of the
# licensees of The Santa Cruz Operation, Inc.  Such users have the right 
# to use, modify, and incorporate this code into other products for purposes 
# authorized by the license agreement provided they include this notice 
# and the associated copyright notice with any such product.  The 
# information in this file is provided "AS IS" without warranty.
# 
#

# wrapper for scripts, binaries - provide a chance to see results/errors

set_trap()  {
	trap 'echo "Interrupted! Giving up..."; cleanup 1' 1 2 3 15
}

unset_trap()  {
	trap '' 1 2 3 15
}
 
cleanup() {
	trap '' 1 2 3 15
	echo "\n\007Press [Enter] to continue.\c">&2
	read x
	exit $1
}

# main
set_trap

# run the positional parameters
/bin/sh -c "$*"
status="$?"
cleanup $status

