# @(#)do_tfadmin.sh	1.2
/sbin/tfadmin $1
if [ $? -ne 0 ]
then
	echo "\nPress <Return> to continue\c"
	read x
fi

