#!/bin/sh
#
STRSTART=/tmp/strstart$$

strlog()
{
	echo strst | crash | awk \
		"/message/	{print \"message_headers=\" \$4} \
		 /buffer headers/	{print \"buffer_headers=\" \$4} \
		 /data block size/	{print \"data_block_\" \$4 \"=\" \$6}"
}

strcheck()
{
	[ $1 ] && tst=$1 || tst="strcheck"
	echoboth "$tst: Streams check"
	strlog | diff $STRSTART - > /tmp/tmp$$
	passfail $?
	[ $1 = "finish" ] && rm -f $STRSTART
}
