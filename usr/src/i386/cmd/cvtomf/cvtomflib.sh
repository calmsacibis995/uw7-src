#ident	"@(#)cvtomf:cvtomflib.sh	1.7"

# Enhanced Application Compatibility Support
#
# cvtomflib - convert OMF libraries to ELF
#

PATH=/usr/bin:/usr/sbin:/sbin

ScoAr=/usr/eac/lib/ar
SystemAr=/usr/bin/ar
Cvtomf=cvtomf

out_file=""
progname=`basename $0`
this_dir=`pwd`
tmp_dir=/var/tmp/col$$.d
tmp_lib=/var/tmp/col$$.a
unset verbose

ErrorMsg()
{
	echo "${progname}:" $* 1>&2
}

Msg()
{
	if [ "${verbose}" ]; then
		echo "\n$*\n"
	fi
}

UsageError()
{
	ErrorMsg "usage: ${progname} [-v] [-o <outfile>] <library> {<library>}"
	exit 2
}

while getopts vo: opt; do
	case ${opt} in
	v)
		verbose=1
		;;
	o)
		out_file=${OPTARG}
		;;
	*)
		UsageError
		;;
	esac
done
shift `expr ${OPTIND} - 1`

if [ $# -lt 1 ]; then
	UsageError
elif [ $# -gt 1 -a "${out_file}" ]; then
	ErrorMsg "Can not specify output file for more than one input"
	UsageError
fi


trap "cd /; rm -rf ${tmp_dir} ${tmp_lib}; exit 1" 1 2 3 15
mkdir ${tmp_dir}

for i in $*; do
	if [ ! -f ${i} -o ! -r ${i} ]; then
		ErrorMsg "${i}: Can not open"
		continue
	fi

	case ${i} in
	/*)
		lib=${i}
		;;
	*)
		lib=${this_dir}/${i}
		;;
	esac

	case "${out_file}" in
	/*)
		new_lib=${out_file}
		;;
	"")
		new_lib=${lib}
		out_file=${i}
		;;
	*)
		new_lib=${this_dir}/${out_file}
		;;
	esac

	out_dir=`dirname ${out_file}`
	: ${out_dir:=.}

	if [ ! -d ${out_dir} -o ! -w ${out_dir} ]; then
		ErrorMsg "${out_dir}: Directory is not writable"
		continue
	fi

	if [ -f ${out_file} -a ! -w ${out_file} ]; then
		ErrorMsg "${out_file}: Can not overwrite"
		continue
	fi

	# Preserve file order (but leave out symbol table):
	lib_files=`${ScoAr} t ${i} | grep -v '^__.SYMDEF$`

	cd ${tmp_dir}
	rm -f *

	Msg "Extracting OMF objects"

	${ScoAr} x ${lib}			# Extract the files

	Msg "Translating OMF objects to COFF"

	${Cvtomf} ${verbose:+-v} *.o		# Convert OMF objects to COFF

	Msg "Translating COFF objects to ELF"

	if [ "${verbose}" ]; then
		cof2elf *.o
	else
		cof2elf -q *.o
	fi

	Msg "Constructing new archive"

	${SystemAr} qc ${tmp_lib} ${lib_files}	# Package them up again
	mv ${tmp_lib} ${new_lib}

	cd ${this_dir}
done

rm -rf ${tmp_dir}
exit 0
# End Enhanced Application Compatibility Support
