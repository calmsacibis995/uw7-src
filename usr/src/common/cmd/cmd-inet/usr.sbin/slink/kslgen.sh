#!/bin/sh
#      @(#)kslgen.sh,v 6.5 1995/02/21 12:40:38 prem Exp - STREAMware TCP/IP  source
#
# Copyrighted as an unpublished work.
# (c) Copyright 1987-1994 Legent Corporation
# All rights reserved.
#
#      SCCS IDENTIFICATION

output=/tmp/ksl.space.c.$$
File=/tmp/ksl.$$
nodelist=/tmp/nodes.$$
tmpnode=/tmp/tmpnode.$$
devlist=/tmp/dev.$$
devdecl=/tmp/decl.$$

preamble()
{
	echo '/*'
	echo ' * This file was automatically generated on'
	echo ' * \c'
	echo `date`
	echo ' *'
	echo ' * Do not edit by hand.  Use kslgen to make any desired'
	echo ' * changes.'
	echo ' */'
	echo 
	echo "#include <sys/types.h>"
	echo "#include <sys/stream.h>"
	echo "#include <sys/stropts.h>"
	echo "#include <sys/file.h>"
	echo "#include <sys/inode.h>"
	echo 
	echo "#define KSL_MAX_FDS 100\n"
	echo "int ksl_fds[KSL_MAX_FDS];"
	echo "int ksl_int;"
	echo "int ksl_debug = 0;"
	echo "int ksl_panicflag = 1;"
	echo "char *__ksl_strcf_file = 0;"
	echo "char *__ksl_strcf_fn = 0;"
	echo "int __ksl_strcf_line = 0;"
	echo "long *ksl_trampoline;"
	echo "char ksl_str[128];\n"
	echo "struct iocqp {\n\tu_short iqp_type;\n\tu_short iqp_value;\n};"
	echo "\n#define IFNAMSIZ 16"
	echo "struct ifreq {\n\tchar ifr_name[IFNAMSIZ];"
	echo "\tunion {\n\t\tint ifru_metric;"
	echo "\t\tchar ifru_enaddr[6];"
	echo "\t\tchar ifru_junk[16];"
	echo "\t} ifr_ifru;\n};"
	echo "#define ifr_metric ifr_ifru.ifru_metric"
	echo "#define ifr_enaddr ifr_ifru.ifru_enaddr"
	echo
}

fheader()
{
	echo
	echo "$1()"
	echo "{"
	echo "\t__ksl_strcf_file = \"$1\";"
}

trailer()
{
	echo
	echo "int (*ksl_funcs[])() = {"
	for i in $FLIST; do
		echo "\t$i,"
	done
	echo "\t(int (*)())0"
	echo "};"
}

ROOT=${ROOT:-/}

set -- 'getopt r:'
if [ $? != 0 ]
then
	echo "usage: $0 [-r directory]"
	exit 2
fi
for i in $*
do
	case $i in
	-r)	ROOT=$3; shift; shift;;
	esac
done

cd $ROOT/etc/strcf.d
rm -f $File

FLIST=""

nodelist=/tmp/nodes.$$
tmpnode=/tmp/tmpnode.$$

rm -f $nodelist

for i in [0-9][0-9]*; do
	if [ -z "$FLIST" ]; then
		FLIST="k$i"
	else 
		FLIST=$FLIST" k$i"
	fi
	fheader "k"$i >> $File
	$ROOT/etc/slink -G -c $i -n $tmpnode>> $File
	cat $tmpnode >> $nodelist
	echo "}" >> $File
done

$ROOT/etc/conf/bin/idmknod -G $devlist

/usr/bin/awk -v nodespec=$nodelist '
	{
		devno[$1] = $3;
	}
END	{
		while ((getline < nodespec) > 0) {
			if ($1 == "extern") {
				printf "extern unsigned int %s;\n", $2;
			} else {
				if (devno[$1] == "") {
					printf "kslgen: cannot determine major/minor numbers for node %s.\n", $1 > "/dev/stderr";
					exit 1;
				}
				vars[$2] = devno[$1];
			}
		}
		for (i in vars) {
			printf "unsigned int %s = %s;\n", i, vars[i];
		}
		exit 0;
	}
' <$devlist >$devdecl

err=$?

[ $err = 0 ] && {
	preamble > $output
	cat $devdecl >>$output
	cat $File >> $output
	trailer >> $output
	mv $output $ROOT/etc/conf/pack.d/ksl/space.c
}

rm -f $File $nodelist $tmpnode $devlist $devdecl

exit $err
