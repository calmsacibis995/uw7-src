#ident  "@(#)ivar.ksh	15.1	97/12/19"

#
# Be sure /funcrc has already been dotted in
#
#		IVAR
#
# This is the central storage repository for program-flow information and
# install data. Don't store message strings, or lists of every possible
# package. Do store hostname, and wether the user wanted cdrom or tape.
# This module should be replaced with something much faster later.
#
# The interface to the rest of the install has purposly been left very
# simple, so it may be replaced (with a C program?) later, and the rest
# of the install will not have to be changed. 
#
# There are only four ways to call ivar:
#
#	ivar get VAR
#		Lookup the value for the varaible VAR. The value is outputted
#		to stdout. If the varaible is not defined, that is not an
#		error. Nothing is written to stdout.
#
#	ivar set VAR VALUE
#		Set the value of the variable VAR to the value VALUE. VAR
#		may already exist, or this may be a new variable.
#
#	ivar check VAR
#		This is just a convienence function, and could be implemented
#		with "ivar get VAR". It looks at the value of the variable
#		and if it is "1", "true", "True" or "TRUE", then it returns
#		0 (sucess), else it returns 1 (failure). 
#		This option exists to make shell programming cleaner. For
#		example:
#			temp=`ivar get foo`
#			if [ "$temp" = true ]
#			then....
#		can be replaced with:
#			if ivar check foo
#			then....
#
#	ivar dump
#		This just dumps out all the variables. It is implemented with
#		a cat now, but in the future the variables may be held in
#		memory, so you would need a functional interface (not just
#		cat-ing the file).
#
#
# Temporary undocumented feature:
#	
#	ivar comment COMMENT
#		If debugging is on, add COMMENT to the log file. 
#

libload libivar.so

call ivar_init

# Get next available file descriptor. If it is > 9 then DIE

call open "/etc/passwd" 0
FD3=$?
call open "/etc/group" 0
FD4=$?
if (( FD3 > 9 )) || (( FD4 > 9 ))
then
	faultvt "Internal Error: Too many open files"
	halt
fi
call close $FD3
call close $FD4
export FD3
export FD4

function ivar
{
	case $1 in
	set)
		call ivar_set "$2" "@string_t:!$3!"
		;;
	get)
		call ivar_get "$2"
		PTR=$RET
		call altprintf '%s' $PTR
		call free $PTR
		;;
	check)
		call ivar_get "$2"
		PTR=$RET
		value=$(call altprintf '%s' $PTR)
		call free $PTR
		case "$value" in
			1)                      return 0;;
			[Tt][Rr][Uu][Ee])       return 0;;
			[Yy][Ee][Ss])           return 0;;
			*)                      return 1;;
		esac
		;;
	save)
		call ivar_dump /isl/ifile
		;;
	stop)
		call ivar_shutdown
		;;
	restart)
		call ivar_init
		;;
	source)
		while read line
		do
			name="${line%%=*}"
			value="${line#*=\"}"
			value="${value%\"}"
			call ivar_set "$name" "@string_t:!$value!"
		done < $2
		;;
	esac
}


function svar
{
	case $1 in
	set)
		call svar_set "$2" "@string_t:!$3!"
		;;
	get)
		call svar_get "$2"
		PTR=$RET
		call altprintf '%s' $PTR
		call free $PTR
		;;
	check)
		call svar_get "$2"
		PTR=$RET
		value=$(call altprintf '%s' $PTR)
		call free $PTR
		case "$value" in
			1)                      return 0;;
			[Tt][Rr][Uu][Ee])       return 0;;
			[Yy][Ee][Ss])           return 0;;
			*)                      return 1;;
		esac
		;;
	save)
		call svar_dump /isl/sfile
		;;
	source)
		while read line
		do
			name="${line%%=*}"
			value="${line#*=\"}"
			value="${value%\"}"
			call ivar_set "$name" "@string_t:!$value!"
		done < $2
		;;
	esac
}
if [[ -f /isl/ifile_add ]]; then
	ivar source /isl/ifile_add
	rm /isl/ifile_add
fi
if [[ -f /isl/sfile_add ]]; then
	svar source /isl/sfile_add
	rm /isl/sfile_add
fi

function long_mul {
	typeset a
	typeset PTR

	call long_mul "@string_t:!$1!" "@string_t:!$2!"
	PTR=$RET	
	a=$(call altprintf "%s" $PTR)
	call free $PTR
	a=${a%%.*}
	echo $a
}

function long_div {
	typeset a
	typeset PTR

	call long_div "@string_t:!$1!" "@string_t:!$2!"
	PTR=$RET	
	a=$(call altprintf "%s" $PTR)
	call free $PTR
	a=${a%%.*}
	echo $a
}

function long_add {
	typeset a
	typeset PTR

	call long_add "@string_t:!$1!" "@string_t:!$2!"
	PTR=$RET	
	a=$(call altprintf "%s" $PTR)
	call free $PTR
	a=${a%%.*}
	echo $a
}

function long_sub {
	typeset a
	typeset PTR

	call long_sub "@string_t:!$1!" "@string_t:!$2!"
	PTR=$RET	
	a=$(call altprintf "%s" $PTR)
	call free $PTR
	a=${a%%.*}
	echo $a
}

function long_cmp {
	call long_cmp "@string_t:!$1!" "@string_t:!$2!"
	call altprintf "%d" $RET
}
