#!/sbin/sh -
# @(#)cmd.vxvm:common/voladm/vxadm_lib.sh	1.2 3/3/97 03:23:29 - cmd.vxvm:common/voladm/vxadm_lib.sh
#ident	"@(#)cmd.vxvm:common/voladm/vxadm_lib.sh	1.2"

# Copyright(C)1996 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
# UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
# LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
# IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
# OR DISCLOSURE.
# 
# THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
# TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
# OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
# EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
# 
#               RESTRICTED RIGHTS LEGEND
# USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
# SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
# (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
# COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
#               VERITAS SOFTWARE
# 1600 PLYMOUTH STREET, MOUNTAIN VIEW, CA 94043

: ${VOLROOT_DIR:=$__VXVM_ROOT_DIR}
: ${VOL_SCRIPTS_DIR:=$VOLROOT_DIR/usr/lib/vxvm/bin}
: ${VOL_SCRIPTS_LIB:=$VOLROOT_DIR/usr/lib/vxvm/lib}
: ${VOLADM_ROOT:=$VOLROOT_DIR/usr/lib/vxvm/voladm.d}
: ${VOLADM_BIN=$VOLADM_ROOT/bin}
: ${VOLADM_LIB=$VOLADM_ROOT/lib}
: ${VOLADM_HELP=$VOLADM_ROOT/help}
: ${VOL_CONFIG_DIR:=$VOLROOT_DIR/etc/vx}
: ${VOL_LOCK_DIR:=$VROOT_DIR/var/spool/locks}
: ${VOL_TYPE_DIR:=$VROOT_DIR/usr/lib/vxvm/type}

SYSROOT=$__VXVM_SYSTEM_ROOT_DIR

export VOL_SCRIPTS_DIR
export VOLADM_ROOT
export VOLADM_BIN
export VOLADM_LIB
export VOLADM_HELP
export VOL_CONFIG_DIR

CNTRL_EXCLUDE_FILE=${VOL_CONFIG_DIR}/cntrls.exclude
DISK_EXCLUDE_FILE=${VOL_CONFIG_DIR}/disks.exclude

SYSDIRS=
[ -n "$VOLROOT_DIR" ] && {
	SYSDIRS=$VOLROOT_DIR/usr/sbin:$VOLROOT_DIR/sbin:
}
SYSDIRS=$SYSDIRS/usr/sbin:/sbin:/usr/bin:/usr/ucb
: ${__VXADM_NEED_PATH:=$VOL_SCRIPTS_DIR:$VOLADM_BIN:$SYSDIRS}
case $PATH in
$__VXADM_NEED_PATH|$__VXADM_NEED_PATH:*) :;;
*)	PATH=$__VXADM_NEED_PATH:$PATH;;
esac
export PATH

voladm_menu_stack=
cleanup_list=cleanup_tempfiles
newline='
'

v_opt=
verbose=
if [ "X$1" = X-v ]
then
	verbose=yes
	v_opt=-v
	shift
fi

tmpfile1=${TMPDIR:-/tmp}/vx1.$$
tmpfile2=${TMPDIR:-/tmp}/vx2.$$
tmpfile3=${TMPDIR:-/tmp}/vx3.$$
tmpfile4=${TMPDIR:-/tmp}/vx4.$$
_vd_tmpfile=${TMPDIR:-/tmp}/vx5.$$
_cef_tmpfile=${TMPDIR:-/tmp}/vx6.$$
_vdskd_rawmatch=${TMPDIR:-/tmp}/vx7.$$
_vdskd_cntrlmatch=${TMPDIR:-/tmp}/vx8.$$
_vdskd_diskmatch=${TMPDIR:-/tmp}/vx9.$$

cleanup_tempfiles()
{
	rm -f $tmpfile1 $tmpfile2 $tmpfile3 $tmpfile4 \
	$_vd_tmpfile $_cef_tmpfile \
	$_vdskd_rawmatch $_vdskd_cntrlmatch $_vdskd_diskmatch
}

quit()
{
	trap "" INT HUP QUIT TERM
	for f in $cleanup_list
	do
		"$f"
	done
	exit $1
}

trap "quit 100" INT HUP QUIT TERM

exec 4<&0 5>&2

# doit - execute a command, printing the command in verbose mode
doit()
{
	[ "$verbose" ] && echo ! "$@" >&5
	"$@"
}

writemsg()
{
	echo "";
	for _wm_line in "$@"
	do
		echo "  $_wm_line"
	done
}

#
# usage: ewritemsg \
#	 [-a] [-M catalog_id] [-f file] \
#	 default_message [shell_parms]
#
# -a: append the list to 'file' instead of overwriting. This
#     flag only has affect when -f is also used.
# -M catalog_id: specify catalog id
# -f file: write the list to 'file' and display the 'file' 
#
ewritemsg()
{
	_ewm_append=no
	_ewm_cat=
	_ewm_file=

	OPTIND=1 # sh does not re-initialize OPTIND
	while getopts "aM:f:" c
	do
		case $c in
		a) _ewm_append=yes;;
		M) _ewm_cat=$OPTARG;;
		f) _ewm_file="$OPTARG";;
		\?) echo "ewritemsg: bad argument list"
		    return;;
		esac
	done
	shift `expr $OPTIND - 1`

	if [ -n "$_ewm_cat" ]
	then
		_ewm_msg="$1"
		shift 1
		_ewm_msg="`egettxt \"$_ewm_msg\" \"$_ewm_cat\" \"$@\"`"
	else
		_ewm_msg="$*"
	fi

	if [ -z "$_ewm_file" ]
	then
		writemsg "$_ewm_msg"
	else
		if [ "$_ewm_append" = "no" ]
		then
			> $_ewm_file
		fi

		# Save stdout and redirect stdout to $_ewm_file.
		exec 9>&0 >> $_ewm_file
		writemsg "$_ewm_msg"
		# Restore stdout and close $_ewm_file.
		exec >&9 9>&-
	fi
}

read_input()
{
	echo ""
	_once=
	if [ "X$1" = X-m ]
	then
		_once=yes
		shift
	fi
	_prompt="$1 "
	[ -z "$2" ] || _prompt="${_prompt}[$2,q,?] "
	[ -z "$3" ] || _prompt="${_prompt}(default: $3) "
	case $_prompt in
????????????????????????????????????????????????????????????????????*)
		if [ ! -z "$2" ] || [ ! -z "$3" ]
		then
			_prompt="$1$newline"
			[ -z "$2" ] || _prompt="${_prompt}[$2,q,?] "
			[ -z "$3" ] || _prompt="${_prompt}(default: $3) "
		fi
		;;
	esac
	while true
	do
		echo "$_prompt\c"
		read input <&4
		case $input in
		"")	input=$3; return 0;;
		"!")	${SHELL:-sh} <&4
			echo !
			[ ! -z "$_once" ] && return 1;;
		"!"*)	${SHELL:-sh} -c "`expr "$input" : '!\(.*\)'" <&4
			echo !
			[ ! -z "$_once" ] && return 1;;
		'?')	voladm_help; [ ! -z "$_once" ] && return 1;;
		'??')	voladm_help_help; [ ! -z "$_once" ] && return 1;;
		q|quit)	quit 101;;
		x)	quit 100;;
		*)	return 0;;
		esac
	done
}

pop_list()
{
	_list=$1
	eval "set -- \$$_list"
	shift
	eval "$_list=\$*"
}

first_item()
{
	echo $1
}

last_item()
{
	_list=$1
	eval "set -- \$$_list"
	eval "echo \${$#}"
}

list_item()
{
	_item=$1
	shift
	eval "echo \${$_item}"
}


list_count()
{
	echo $#
}

push_list()
{
	_list=$1
	shift
	eval "set -- \$* \$$_list"
	eval "$_list=\$*"
}

append_list()
{
	_list=$1
	shift
	eval "set -- \$$_list \$*"
	eval "$_list=\$*"
}

list_member()
{
	_want=$1
	shift
	for i in $@
	do
		[ "X$_want" = "X$i" ] && return 0
	done
	return 1
}

#
# Set 'out_list' to be the 'elem_num' element of every 'num_elem_in_group'
# elements in the list 'in_list'.
#
# NOTE: The numbering for 'elem_num' starts from 1.
#
# usage: get_list_group num_elem_in_group elem_num in_list out_list
#
get_list_group()
{
	if [ $# -lt 4 ]
	then
		echo "get_list_group: bad argument list"
		return
	fi

	_glg_num_elem_in_group=$1
	_glg_elem_num=$2
	_glg_in_list=$3
	_glg_out_list=$4

	if [ $_glg_num_elem_in_group -lt $_glg_elem_num ]
	then
		echo "get_list_group: num_elem_in_group not > than elem_num"
		return
	fi

	if [ $_glg_num_elem_in_group -eq 0 -o $_glg_elem_num -eq 0 ]
	then
		echo "get_list_group: num_elem_in_group or elem_num are 0"
		return
	fi

	eval $_glg_out_list=

	set -- `eval echo "\\$$_glg_in_list"`
	while [ $# -gt 0 ]
	do
		eval $_glg_out_list=\`eval \
		\"echo \${$_glg_out_list} \${$_glg_elem_num}\"\`
		shift $_glg_num_elem_in_group
	done
}

#
# Remove every 'elem_num' element from every 'num_elem_in_group' in 'list'.
#
# NOTE: The numbering for 'elem_num' starts from 1.
#
# usage: change_list_group num_elem_in_group elem_num list
#
remove_list_group()
{
	if [ $# -lt 3 ]
	then
		echo "remove_list_group: bad argument list"
		return
	fi

	_rlg_num_elem_in_group=$1
	_rlg_elem_num=$2
	_rlg_list=$3

	if [ $_rlg_num_elem_in_group -lt $_rlg_elem_num ]
	then
		echo "remove_list_group: num_elem_in_group not > than elem_num"
		return
	fi

	if [ $_rlg_num_elem_in_group -eq 0 -o $_rlg_elem_num -eq 0 ]
	then
		echo "remove_list_group: num_elem_in_group or elem_num are 0"
		return
	fi

	set -- `eval echo "\\$$_rlg_list"`
	eval $_rlg_list=
	while [ $# -gt 0 ]
	do
		i=1
		while [ $i -le $_rlg_num_elem_in_group ]
		do
			if [ $i -ne $_rlg_elem_num ]
			then
				eval $_rlg_list=\"\$$_rlg_list $1\"
			fi
			inc i
			shift 1
		done
	done
}


#
# list_explode [-t fieldsep] listname var1 var2 ...
#
# Explode a list and store elements into the named variables.  If
# some variables could not be filled (because the named list didn't
# have enough members) return a non-zero status.
#
list_explode()
{
	_le_save=$IFS
	_le_ifs=$IFS
	[ "X$1" = X-t ] && {
		_le_ifs=$2
		shift 2
	}
	_le_list=$1
	shift
	_le_vars="$*"
	IFS="$_le_ifs"
	eval "set \$$_le_list"
	IFS="$_le_save"
	for _le_var in $_le_vars
	do
		[ $# -eq 0 ] && {
			return 1
		}
		eval "$_le_var=\$1"
		shift
	done
	return 0
}

not()
{
	"$@"
	[ $? -eq 0 ] && return 1
	return 0
}

add_cleanup()
{
	push_list cleanup_list $*
}

voladm_menu_push()
{
        if [ "$1" = "-M" ]
        then
                _mnu_def="$2"
                _mnu_cat="$3"
                shift 3
                _mnu_cur="`egettxt \"$_mnu_def\" \"$_mnu_cat\" \"$@\"`"
        else
                _mnu_cur="$*"
        fi

	push_list voladm_menu_stack $VOLADM_CURRENT_MENU
	if [ -z "$VOLADM_CURRENT_MENU" ]
	then
		VOLADM_CURRENT_MENU=$_mnu_cur
	else
		VOLADM_CURRENT_MENU=$VOLADM_CURRENT_MENU/$_mnu_cur
	fi
	export VOLADM_CURRENT_MENU
}

voladm_menu_pop()
{
	VOLADM_CURRENT_MENU=`first_item $voladm_menu_stack`
	export VOLADM_CURRENT_MENU
	pop_list voladm_menu_stack
}

voladm_menu_set()
{
        if [ "$1" = "-M" ]
        then
                _mnu_def="$2"
                _mnu_cat="$3"
                shift 3
                _mnu_cur="`egettxt \"$_mnu_def\" \"$_mnu_cat\" \"$@\"`"
        else
                _mnu_cur="$*"
        fi

	voladm_menu_pop
	voladm_menu_push $_mnu_cur
}

voladm_help_push()
{
	voladm_help_stack="$VOLADM_CURRENT_HELP $voladm_help_stack"
	VOLADM_CURRENT_HELP=$1
	export VOLADM_CURRENT_HELP
}

voladm_help_pop()
{
	VOLADM_CURRENT_HELP=`first_item $voladm_help_stack`
	export VOLADM_CURRENT_HELP
	pop_list voladm_help_stack
}

voladm_help_set()
{
	voladm_help_pop
	voladm_help_push $1
}

voladm_help_help()
{
	voladm_help vxadm.info
}

voladm_help()
{
	help=${1:-$VOLADM_CURRENT_HELP}
	if [ -x /usr/bin/tput ] && [ ! -z "$TERM" ]
	then
		tput clear
	fi
	( echo "$title\nMenu: $VOLADM_CURRENT_MENU\n"
	  grep -v "^#" $VOLADM_HELP/$help; echo "" ) | voladm_display
}

#
# display a file, using a pager, if needed
#
# usage: voladm_display [-acn] [-f out_file] [in_file]
#
# -a: append the list to 'out_file' instead of overwriting. This
#     flag is only meaningful when -f is used.
# -c: call voladm_continue called after the display is finished if the
#     pager was not used
# -f out_file: write the list to 'out_file' and display 'out_file' 
# -n: add new line after file
#
# If 'in_file' is not specified, read from standard input.
#
voladm_display()
{
	_vd_out_file=
	_vd_append=no
	_vd_pause=no
	_vd_nflag=no

	OPTIND=1 # sh does not re-initialize OPTIND
	while getopts "acnf:" c
	do
		case $c in
		a) _vd_append=yes;;
		c) _vd_pause=yes;;
		n) _vd_nflag=yes;;
		f) _vd_out_file="$OPTARG";;
		\?) echo "voladm_display: bad argument list"
		    return;;
		esac
	done
	shift `expr $OPTIND - 1`

	if [ -z "$_vd_out_file" ]
	then
		_vd_out_file=$_vd_tmpfile
		cat "$@" > $_vd_out_file
		if [ $_vd_nflag = "yes" ]
		then
			echo >> $_vd_out_file
		fi
	else
		if [ "$_vd_append" = "no" ]
		then
			cat "$@" > $_vd_out_file
		else
			cat "$@" >> $_vd_out_file
		fi
		if [ $_vd_nflag = "yes" ]
		then
			echo >> $_vd_out_file
		fi
	fi

	_vd_lines=`tput lines`
	[ -z "$_vd_lines" ] && _vd_lines=24
	if [ "`wc -l < $_vd_out_file`" -ge $_vd_lines ]
	then
		${PAGER:-more} $_vd_out_file <&4
	else
		cat $_vd_out_file
		if [ "$_vd_pause" = "yes" ]
		then
			voladm_continue
		fi
	fi
}

#
# Send a list to the fmt command and then display the output with a
# pager, if needed. If the number of elements in a group is greater
# than one, then by default (unless '-e elem_num' is used) all elements
# in the list surrounded by brackets will be displayed.
#
# usage: voladm_list_display \
#	[-abcn] [-f file] \
#	[-g num_elem_in_group] [-d elem_num] \
#	list...
# -a: append the list to 'file' instead of overwriting. This
#     flag only has affect when -f is also used.
# -b: put brackets around elements in the list. if there is more than
#     one element to be displayed separate the elements with forward
#     slashes '/'
# -c: call voladm_continue called after the display is finished if the
#     pager was not used
# -f file: write the list to 'file' and display the 'file' 
# -g num_elem_in_group: specify the number of elements in a group (default: 1)
# -d elem_num: specify the group element to display (default: 1)
# -n: add new line after list
#
voladm_list_display()
{
	_vld_append=no
	_vld_pause=no
	_vld_file=
	_vld_num_elem_in_group=1
	_vld_elem_num=1
	_vld_dflag=no
	_vld_use_bracket=no 
	_vld_nflag=no

	OPTIND=1 # sh does not re-initialize OPTIND
	while getopts "abcf:g:d:n" c
	do
		case $c in
		a) _vld_append=yes;;
		b) _vld_use_bracket=yes;;
		c) _vld_pause=yes ;;
		f) _vld_file="$OPTARG";;
		g) _vld_num_elem_in_group="$OPTARG";;
		d) _vld_dflag=yes; _vld_elem_num="$OPTARG";;
		n) _vld_nflag=yes;;
		\?) echo "voladm_list_display: bad argument list"
		    return;;
		esac
	done
	shift `expr $OPTIND - 1`

	if [ $_vld_num_elem_in_group -eq 0 ]
	then
		echo "voladm_list_display: num_elem_in_group is 0"
		return
	fi

	if [ $_vld_dflag = "yes" -a $_vld_elem_num != "all" -a \
	     $_vld_elem_num -eq 0 ]
	then
		echo "voladm_list_display: num_elem is 0"
		return
	fi

	# If all is specified, then use the default handling.
	if [ $_vld_dflag = "yes" -a $_vld_elem_num = "all" ]
	then
		_vld_dflag=no
	fi

	if [ -z "$_vld_file" ]
	then
		_vld_file=$_vd_tmpfile
		> $_vld_file
	else
		if [ "$_vld_append" = "no" ]
		then
			> $_vld_file
		fi
	fi

	echo >> $_vld_file
	while [ $# -gt 0 ]
	do
		# If '-d elem_num' was specified then display only the
		# element that was selected.
		if [ $_vld_dflag = "yes" ]
		then
			if [ $_vld_use_bracket = "yes" ]
			then
				eval "echo \"  [\${$_vld_elem_num}]\""
			else
				eval "echo \"  \${$_vld_elem_num}\""
			fi
			shift $_vld_num_elem_in_group
		else
			if [ $_vld_use_bracket = "yes" ]
			then
				echo "  [\c"
			fi
			i=$_vld_num_elem_in_group
			_vld_first=yes
			while [ $i -gt 0 ]
			do
				if [ $_vld_use_bracket = "yes" ]
				then
					if [ $_vld_first = "yes" ]
					then
						echo "$1\c"
						_vld_first=no
					else
						echo ",$1\c"
					fi
				else
					echo "  $1"
				fi
				dec i
				shift 1
			done
			if [ $_vld_use_bracket = "yes" ]
			then
				echo "]"
			fi
		fi
	done | fmt >> $_vld_file
	if [ $_vld_nflag = "yes" ]
	then
		echo >> $_vld_file
	fi
	_vld_lines=`tput lines`
	[ -z "$_vld_lines" ] && _vld_lines=24
	if [ "`wc -l < $_vld_file`" -ge $_vld_lines ]
	then
		${PAGER:-more} $_vld_file <&4
	else
		cat $_vld_file
		if [ "$_vld_pause" = "yes" ]
		then
			voladm_continue
		fi
	fi
}

voladm_call()
{
	_util=$1
	shift

	doit "$_util" $v_opt "$@" <&4
	status=$?
	if [ $status -eq 100 ] || [ $status -eq 101 ]
	then
		quit $status
	fi
	return $status
}

voladm_menu_call()
{
	_util=$1
	shift

	doit "$_util" $v_opt "$@" <&4
	status=$?
	if [ $status -eq 100 ]
	then
		quit 100
	fi
	return $status
}

voladm_begin_screen()
{
	if [ "$1" = "-M" ]
	then
		_vbs_cat="$2"
		_vbs_def="$3"
		shift 3
		_vbs_msg="`egettxt \"$_vbs_def\" \"$_vbs_cat\" \"$@\"`"
	else
		_vbs_msg="$1"
	fi
	tput clear
	echo "\n$_vbs_msg\nMenu: $VOLADM_CURRENT_MENU"
}

voladm_menu()
{
	_menu_h1="`egettxt \"Display help about menu\" vxvmshm:198`"
	_menu_h2="`egettxt \"Display help about the menuing system\" \
			vxvmshm:199`"
	_menu_h3="`egettxt \"Exit from menus\"  vxvmshm:233`"
	_menu_h4="`egettxt \"Select an operation to perform\"  vxvmshm:359`"
	_menu_h5="`egettxt \"default\"  vxvmshm:628`"

	read _menu_title
	_menu_default=
	_menu="$_menu_title\nMenu: $VOLADM_CURRENT_MENU\n"
	_menu_select='case $input in'
	while read _menu_key _menu_item _menu_cat _menu_text
	do
		if [ -z "$_menu_key" ]
		then
			_menu="$_menu\n"
		elif [ "X$_menu_item" = Xdefault ]
		then
			_menu_default=$_menu_key
		else
			_menu_select="$_menu_select '$_menu_key')VOLADM_MENU_SELECT=$_menu_item;break 2;;"
			if [ ! -z "$_menu_cat" ]
			then
				_menu_ctext="`egettxt \"$_menu_text\" \"$_menu_cat\"`"
				_menu="$_menu\n $_menu_key\t$_menu_ctext"
			fi
		fi
	done
	_menu_select="$_menu_select *) :;; esac"
	_menu="$_menu\n ?\t$_menu_h1"
	_menu="$_menu\n ??\t$_menu_h2"
	_menu="\n$_menu\n q\t$_menu_h3"
	_menu_prompt="${1:-$_menu_h4:}"
	if [ ! -z "$_menu_default" ]
	then
		_menu_prompt="$_menu_prompt [$_menu_h5: $_menu_default]"
	fi

	while true
	do
		tput clear
		echo "$_menu"
		while true
		do
			read_input -m "$_menu_prompt" "" $_menu_default
			if [ $? -ne 0 ]
			then
				voladm_continue
				continue 2
			fi
			eval "$_menu_select"

			case $input in
			x)	quit 100;;
			q|quit)	quit 101;;
			'?')	voladm_help; continue 2;;
			h)	voladm_help; continue 2;;
			help)	voladm_help; continue 2;;
			'??')	voladm_help_help; continue 2;;
			esac
			ewritemsg -M vxvmshm:364 "Selection not recognized, enter ?? for help"
		done
	done

	echo ""
}

# voladm_yorn question default
voladm_yorn()
{
	if [ "$1" = "-M" ]
	then
		_vadm_cat="$2"
		_vadm_def="$3"
		_vadm_yon="$4"
		shift 4
		_vadm_msg="`egettxt \"$_vadm_def\" \"$_vadm_cat\" \"$@\"`"
	else
		_vadm_msg="$1"
		_vadm_yon="$2"
	fi
	while true
	do
		read_input "$_vadm_msg" "y,n" $_vadm_yon
		case $input in
		y|yes)	return 0;;
		n|no)	return 1;;
		*)	ewritemsg -M vxvmshm:310 \
			"Please respond with \\\"y\\\" or \\\"n\\\""
			continue;;
		esac
	done
}

#
# Ask "yes" or "no" for an entire list. Each element of the list may be
# reviewed separately or the same answer may be given for the entire list.
# Separate prompts and defaults are specified for the list and for elements
# of the list.
#
#
# usage: voladm_yorn_batch \
#	[-bh] [-f file] [-M list_catalog_id] [-m elem_catalog_id] \
#	[-g num_elem_in_group] [-p elem_num] [-d elem_num] \
#	list_prompt list_yon \
#	elem_prompt elem_yon \
#	list yes_list no_list [shell_parms]
#
# -b: add brackets around list items
# -h: push and pop voladm_help menus
# -f file: append the question output to 'file' and display 'file'
# -M list_catalog_id: specify list catalog id
# -m elem_catalog_id: specify element catalog id
# -g num_elem_in_group: specify the number of elements in a group (default: 1)
# -p elem_num: specify the group element to prompt (default: 1)
# -d elem_num: specify the group element to display.
# list_prompt: specify list prompt string
# list_yon: specify default list answer
# elem_prompt: specify element prompt string
# list_yon: specify default element answer
# list: list of elements to ask questions about (input)
# yes_list: elements with yes answers (output)
# no_list: elements with no answers (output)
#
# If -d 'elem_num' is not used or if "all" is specified, then all elements
# will displayed.
#
voladm_yorn_batch()
{
	_vadm_use_bracket=no
	_vadm_help=no
	_vadm_file=
	_vadm_list_cat=
	_vadm_elem_cat=
	_vadm_num_elem_in_group=1
	_vadm_prompt_num=1
	_vadm_dflag=no
	_vadm_disp_num=1

	OPTIND=1 # sh does not re-initialize OPTIND
	while getopts "bhf:M:m:g:p:d:" c
	do
		case $c in
		b) _vadm_use_bracket=yes;;
		h) _vadm_help=yes;;
		f) _vadm_file="$OPTARG";;
		M) _vadm_list_cat="$OPTARG";;
		m) _vadm_elem_cat="$OPTARG";;
		g) _vadm_num_elem_in_group="$OPTARG";;
		p) _vadm_prompt_num="$OPTARG";;
		d) _vadm_dflag=yes; _vadm_disp_num="$OPTARG";;
		\?) echo "voladm_yorn_batch: bad argument list"
		    return ;;
		esac
	done
	shift `expr $OPTIND - 1`

	if [ $# -lt 7 ]
	then
		echo "voladm_yorn_batch: bad argument list"
		return
	fi

	_vadm_list_msg="$1"
	_vadm_list_yon="$2"
	_vadm_elem_msg="$3"
	_vadm_elem_yon="$4"
	_vadm_select_list="$5"
	_vadm_yes_list="$6"
	_vadm_no_list="$7"
	shift 7

	if [ $_vadm_num_elem_in_group -eq 0 -o $_vadm_prompt_num -eq 0 ]
	then
		echo "voladm_yorn_batch: -g or -p arguments can not be 0"
		return
	fi

	if [ $_vadm_dflag = "yes" -a $_vadm_disp_num != "all" -a \
	     $_vadm_disp_num -eq 0 ]
	then
		echo "voladm_yorn_batch: -d argument can not be zero."
		return
	fi

	eval $_vadm_yes_list=
	eval $_vadm_no_list=

	# If a catalog entry was specified for the list message, get it.
	if [ -n "$_vadm_list_cat" ]
	then
		_vadm_list_msg="`\
			egettxt \"$_vadm_list_msg\" \"$_vadm_list_cat\" \"$@\"`"
	fi

	# If a catalog entry was specified for the element message, get it.
	if [ -n "$_vadm_elem_cat" ]
	then
		_vadm_elem_msg="`\
			egettxt \"$_vadm_elem_msg\" \"$_vadm_elem_cat\" \"$@\"`"
	fi

	_vadm_vld_darg=
	if [ $_vadm_dflag = "yes" ]
	then
		_vadm_vld_darg="-d $_vadm_disp_num"
	fi

	_vadm_vld_barg=
	if [ $_vadm_use_bracket = "yes" ]
	then
		_vadm_vld_barg="-b"
	fi

	if [ -z "$_vadm_file" ]
	then
		voladm_list_display \
		-g $_vadm_num_elem_in_group $_vadm_vld_darg \
		$_vadm_vld_barg `eval "echo \\$$_vadm_select_list"`
	else
		voladm_list_display -a -f $_vadm_file $_vadm_vld_barg \
		-g $_vadm_num_elem_in_group $_vadm_vld_darg \
		`eval "echo \\$$_vadm_select_list"`
	fi

	# If there is only one element on the list, call voladm_batch_single.
	set -- `eval "echo \\$$_vadm_select_list"`
	if [ $# -eq $_vadm_num_elem_in_group ]
	then
		voladm_batch_single
		return
	fi

	[ "$_vadm_help" = "yes" ] && voladm_help_push yorn_batch_list.help

	while true
	do
		read_input "$_vadm_list_msg" "Y,N,S(elect)" $_vadm_list_yon
		case $input in
		Y|Yes)
			eval $_vadm_yes_list=\"\$$_vadm_select_list\"
			break;;
		N|No)
			eval $_vadm_no_list=\"\$$_vadm_select_list\";
			break;;
		S|Select|s|select)
			voladm_batch_select
			break;;
		r|redisplay)
			voladm_list_display \
			-g $_vadm_num_elem_in_group $_vadm_vld_darg \
			$_vadm_vld_barg `eval "echo \\$$_vadm_select_list"`
			continue;;
		*)
			ewritemsg -M vxvmshm:309 \
		   	"Please respond with \\\"Y\\\", \\\"N\\\", or \\\"S\\\""
			continue;;
		esac
	done

	[ "$_vadm_help" = "yes" ] && voladm_help_pop
}

# function called by voladm_yorn_batch
voladm_batch_single()
{
	[ "$_vadm_help" = "yes" ] && voladm_help_push yorn_batch_single.help
	while true
	do
		read_input "$_vadm_elem_msg" "y,n" $_vadm_elem_yon
		case $input in
		y|yes)
			eval $_vadm_yes_list=\"\$$_vadm_select_list\"
			break;;
		n|no)
			eval $_vadm_no_list=\"\$$_vadm_select_list\";
			break;;
		r|redisplay)
			voladm_list_display \
			-g $_vadm_num_elem_in_group $_vadm_vld_darg \
			$_vadm_vld_barg `eval "echo \\$$_vadm_select_list"`
			continue;;
		*)
			ewritemsg -M vxvmshm:310 \
			"Please respond with \\\"y\\\" or \\\"n\\\""
			continue;;
		esac
	done
	[ "$_vadm_help" = "yes" ] && voladm_help_pop
}

# function called by voladm_yorn_batch
voladm_batch_select()
{
	[ "$_vadm_help" = "yes" ] && voladm_help_push yorn_batch_elem.help
	set -- `eval echo "\\$$_vadm_select_list"`
	while [ $# -gt 0 ]
	do
		if [ $_vadm_use_bracket = "yes" ]
		then
			ewritemsg `eval echo "[\\${$_vadm_prompt_num}]\"`
		else
			ewritemsg `eval echo "\\${$_vadm_prompt_num}\"`
		fi
		while true
		do
			read_input "$_vadm_elem_msg" "y,n,Y,N" $_vadm_elem_yon
			case $input in
			y|yes)
				i=$_vadm_num_elem_in_group
				while [ $i -gt 0 ]
				do
					eval \
					$_vadm_yes_list=\"\$$_vadm_yes_list $1\"
					dec i
					shift 1
				done
				break;;
			n|no)
				i=$_vadm_num_elem_in_group
				while [ $i -gt 0 ]
				do
					eval \
					$_vadm_no_list=\"\$$_vadm_no_list $1\"
					dec i
					shift 1
				done
				break;;
			Y|Yes)
				eval $_vadm_yes_list=\"\$$_vadm_yes_list $*\"
				break 2;;
			N|No)
				eval $_vadm_no_list=\"\$$_vadm_no_list $*\"
				break 2;;
			r|redisplay)
				if [ $_vadm_use_bracket = "yes" ]
				then
					ewritemsg \
					`eval echo "[\\${$_vadm_prompt_num}]\"`
				else
					ewritemsg \
					`eval echo "\\${$_vadm_prompt_num}\"`
				fi
				continue;;
			*)
				ewritemsg -M vxvmshm:311 \
"Please respond with \\\"y\\\", \\\"n\\\", \\\"Y\\\", or \\\"N\\\""
				continue;;
			esac
		done
	done
	[ "$_vadm_help" = "yes" ] && voladm_help_pop
}

#
# usage: voladm_continue [-n] [prompt_string]
# -n: precede prompt with a newline
#
voladm_continue()
{
	if [ $# -ge 1 -a "$1" = "-n" ]
	then
		echo "\n${2:-Hit RETURN to continue.}\c"
	else
		echo "${1:-Hit RETURN to continue.}\c"
	fi
	read _continue <&4
	case $_continue in
	q|quit)	quit 101;;
	x)	quit 100;;
	*)	return 0;;
	esac
}

# Like voladm_continue, but no quit option.
voladm_do_continue()
{
	echo "${1:-Hit RETURN to continue.}\c"
	read _continue <&4
	return 0
}

#
# check_exclude_files
#
# Check the disks.exclude and cntrls.exclude files for consistency.
# This function should be called before either of these files is
# used.
#
# If 1 is returned, the program should tell the user to fix the
# problems and then it should exit. If 0 is returned, the files
# were found to be non-existent or correct so processing can continue.
#
check_exclude_files()
{
	bad_disk_list=
	bad_cntrl_list=

	if [ -f $DISK_EXCLUDE_FILE ]
	then
		for disk in `cat $DISK_EXCLUDE_FILE`
		do
			if not dogi_name_is_device $disk
			then
				append_list bad_disk_list $disk
			fi
		done
	fi

	if [ -f $CNTRL_EXCLUDE_FILE ]
	then
		for cntrl in `cat $CNTRL_EXCLUDE_FILE`
		do
			if not dogi_name_is_cntrlr $cntrl
			then
				append_list bad_cntrl_list $cntrl
			fi
		done
	fi

	if [ -z "$bad_disk_list" -a -z "$bad_cntrl_list" ]
	then
		return 0	# No errors found.
	else 
		if [ -n "$bad_cntrl_list" ]
		then
			if [ `list_count $bad_cntrl_list` -eq 1 ]
			then
				export CNTRL_EXCLUDE_FILE bad_cntrl_list; \
				ewritemsg -M vxvmshm:407 \
"The following controller address in the controller exclude file
  ${CNTRL_EXCLUDE_FILE} is invalid: $bad_cntrl_list"
			else
				export CNTRL_EXCLUDE_FILE; \
				ewritemsg -f $_cef_tmpfile -M vxvmshm:408 \
"The following controller addresses in the controller exclude file
  ${CNTRL_EXCLUDE_FILE} are invalid:"
				voladm_list_display -acn \
				-f $_cef_tmpfile $bad_cntrl_list
			fi
		fi

		if [ -n "$bad_disk_list" ]
		then
			if [ `list_count $bad_disk_list` -eq 1 ]
			then
				export CNTRL_EXCLUDE_FILE bad_disk_list; \
				ewritemsg -M vxvmshm:415 \
"The following disk address in the disk exclude file
  ${DISK_EXCLUDE_FILE} is invalid: $bad_disk_list"
			else
				export CNTRL_EXCLUDE_FILE; \
				ewritemsg -f $_cef_tmpfile -M vxvmshm:416 \
"The following disk addresses in the disk exclude file
  ${DISK_EXCLUDE_FILE} are invalid:"
				voladm_list_display -acn \
				-f $_cef_tmpfile $bad_disk_list
			fi
		fi
		return 1	# Errors found.
	fi
}

#
# Read a single disk address from the standard input and set its
# address in 'device'. Honor the disks.exclude and cntrls.exclude
# files.
# 
# usage: voladm_disk_device \
#	 [-M catalog_id]
#	 prompt-string [default-result [select-list]]
#
# -M catalog_id: specify catalog id
#
voladm_disk_device()
{
	if [ "$1" = "-M" ]
	then
		_vdskd_msg="`egettxt \"$3\" \"$2\"`"
		shift 2 # Leave message as $1.
	else
		_vdskd_msg="$1"
	fi

	_vdskd_def=
	_vdskd_sel=

	if [ $# -ge 2 ]
	then
		_vdskd_def="$2"
		if [ $# -ge 3 ]
		then
			_vdskd_sel="$3"
		fi
	fi

	while true
	do
		#
		# If the exclude files exist they should have already been
		# verified with the check_exclude_files() function.
		#
		# Create egrep patterns from the files that ensure that only
		# the disk or controller specified is excluded.
		#
		# Build the exclude egrep patterns inside the input while
		# loop in case the files are edited after an exclusion is
		# reported.
		# 
		cntrl_egrep=
		disk_egrep=

		if [ -f $CNTRL_EXCLUDE_FILE ]
		then
			for cntrl in `cat $CNTRL_EXCLUDE_FILE`
			do
				add_cntrlr_expression $cntrl cntrl_egrep
			done
		fi

		if [ -f $DISK_EXCLUDE_FILE ]
		then
			for disk in `cat $DISK_EXCLUDE_FILE`
			do
				add_device_expression $disk disk_egrep
			done
		fi

		read_input "$_vdskd_msg" "${_vdskd_sel:-<address>,list}" \
			$_vdskd_def
		
		device=$input

		#
		# If the default value that was specified to read_input
		# was returned, then return now after having set 'device'.
		#
		if [ -n "$_vdskd_def" -a "$device" = "$_vdskd_def" ]
		then
			return 0
		fi

		if [ "X$device" = "X" ] ; then 
			ewritemsg -M vxvmshm:252 \
			"Input not recognized, enter ?? for help"
			continue
		fi

		if vxcheckda $device > /dev/null 2> /dev/null
		then
			if dogi_name_is_slice $device
			then
				daname=$device
			else
				dogi_name_daname $device daname
			fi
			
			if [ -n "$cntrl_egrep" ]
			then
				echo $daname | egrep -s -e "$cntrl_egrep"
				if [ $? -eq 0 ]
				then
					export CNTRL_EXCLUDE_FILE; \
					ewritemsg -M vxvmshm:496 \
"This disk that you specified has been excluded by the
  $CNTRL_EXCLUDE_FILE file: $device"
					device=
					daname=
					continue
				fi
			fi

			if [ -n "$disk_egrep" ]
			then
				echo $daname | egrep -s -e "$disk_egrep"
				if [ $? -eq 0 ]
				then
					export DISK_EXCLUDE_FILE; \
					ewritemsg -M vxvmshm:497 \
"This disk that you specified has been excluded by the
  $DISK_EXCLUDE_FILE file: $device"
					device=
					daname=
					continue
				fi
			fi

			return 0

		else
			case $device in
			list|l)	(echo ""; vxdevlist ) | voladm_display
				continue;;
			*) ewritemsg -M vxvmshm:252 \
			   "Input not recognized, enter ?? for help";;
	
			esac
		fi
	done
}

#
# Read or take as an argument a disk address pattern list and
# return a list of matching disks in 'device_list'. Honor the
# disks.exclude and cntrls.exclude files.
#
# The following disk address specifications are acceptable:
#
# all:          all disks
# c3 c4t2:      all disks on both controller 3 and controller 4, target 2
# c3t4d2:       a single disk
#
# usage: voladm_disk_device_list \
#	 [-M catalog_id] [-i input-string] \
#	 prompt-string [default-result [select-list]]
#
# -M catalog_id: specify catalog id
# -i input-string: don't call read_input, use input-string as input.
#
voladm_disk_device_list()
{
	_vdskd_iflag=no
	_vdskd_input=
	_vdskd_cat=

	OPTIND=1 # sh does not re-initialize OPTIND
	while getopts "i:M:" c
	do
		case $c in
		M) _vdskd_cat=$OPTARG;;
		i) _vdskd_iflag=yes; _vdskd_input="$OPTARG";;
		\?) echo "voladm_disk_device_list: bad argument list"
		    return 1;;
		esac
	done
	shift `expr $OPTIND - 1`

	if [ -n "$_vdskd_cat" ]
	then
		_vdskd_msg="`egettxt \"$1\" \"$_vdskd_cat\"`"
	else
		_vdskd_msg="$1"
	fi

	_vdskd_def=
	_vdskd_sel=

	if [ $# -ge 2 ]
	then
		_vdskd_def="$2"
		if [ $# -ge 3 ]
		then
			_vdskd_sel="$3"
		fi
	fi

	while true
	do
		#
		# If the exclude files exist they should have already been
		# verified with the check_exclude_files() function.
		#
		# Create egrep patterns from the files that ensure that only
		# the disk or controller specified is excluded.
		#
		# Build the exclude egrep patterns inside the input while
		# loop in case the files are edited after an exclusion is
		# reported.
		# 
		cntrl_egrep=
		disk_egrep=
		if [ -f $CNTRL_EXCLUDE_FILE ]
		then
			for cntrl in `cat $CNTRL_EXCLUDE_FILE`
			do
				add_cntrlr_expression $cntrl cntrl_egrep
			done
		fi
		if [ -f $DISK_EXCLUDE_FILE ]
		then
			for disk in `cat $DISK_EXCLUDE_FILE`
			do
				add_device_expression $disk disk_egrep
			done
		fi

		if [ "$_vdskd_iflag" = "yes" ]
		then
			if [ -n "$_vdskd_input" ]
			then
				input="$_vdskd_input"
				_vdskd_input=
			else
				# Return after one iteration.
				return 0
			fi
		else
			read_input "$_vdskd_msg" \
			"${_vdskd_sel:-<pattern-list>,all,list}" $_vdskd_def

			#
			# If the default value that was specified to
			# read_input was returned, then set device_list
			# and return.
			#
			if [ -n "$_vdskd_def" -a "$input" = "$_vdskd_def" ]
			then
				device_list="$_vdskd_def"
				return 0
			fi

			if [ -z "$input" ]
			then
				ewritemsg -M vxvmshm:252 \
				"Input not recognized, enter ?? for help"
				continue
			fi
		fi

		list=
		first_pass=yes

		cntrl_exclude_list=
		disk_exclude_list=

		# Process each disk address or pattern entered.
		set -- $input
		while [ $# -gt 0 ]
		do
			spec=$1
			pattern=

			if [ "$spec" = "all" -a "$first_pass" = "no" -o $# -gt 1 ]
			then
				ewritemsg -M vxvmshm:83 \
"'all' should appear alone, enter ?? for help"
				continue 2
			fi
			

			if [ "$spec" = "list"  -o "$spec" = "l" ]
			then
				if [ "$first_pass" = "no" -o $# -gt 1 ]
				then
					ewritemsg -M vxvmshm:84 \
"'list' should appear alone, enter ?? for help"
				else
					(echo ""; vxdevlist) | voladm_display
				fi
				continue 2;
			fi
			
			expand_device_wildcard list \
			 "$disk_egrep" "$cntrl_egrep" \
			 disk_exclude_list cntrl_exclude_list $spec

			if [ $? -ne 0 ]
			then
				if [ "$_vdskd_iflag" = "yes" ]
				then
					ewritemsg -M vxvmshm:253 \
"Input not recognized: \"$spec\""
					return 1
				fi
				continue 2
			fi
			
			first_pass=no
			shift
		done

		#
		# Shouldn't reach here unless ready to return. Sort the
		# list and remove any duplicates, then return.
		#
		device_list=`for d in $list; do echo $d; done | sort | uniq`

		if [ -n "$cntrl_exclude_list" -o -n "$disk_exclude_list" ]
		then
			cntrl_exclude_list=`
			for c in $cntrl_exclude_list; do echo $c; done |
			sed "s%/dev/rdsk/\(.*\)$VOL_FULL_SLICE$%\1%"   |
			sed "s%/dev/ap/rdsk/\(.*\)$VOL_FULL_SLICE$%\1%"` 

			disk_exclude_list=`
			for d in $disk_exclude_list; do echo $d; done |
			sed "s%/dev/rdsk/\(.*\)$VOL_FULL_SLICE$%\1%"  |
			sed "s%/dev/ap/rdsk/\(.*\)$VOL_FULL_SLICE$%\1%"`

			disp_file=$_vdskd_rawmatch

			if [ -n "$cntrl_exclude_list" ]
			then
				if [ `list_count $cntrl_exclude_list` -eq 1 ]
				then
					export \
					cntrl_exclude_list CNTRL_EXCLUDE_FILE; \
					ewritemsg -M vxvmshm:495 \
"This disk that you specified has been excluded by the
  $CNTRL_EXCLUDE_FILE file: $cntrl_exclude_list"
					voladm_continue -n
				else
					export CNTRL_EXCLUDE_FILE; \
					ewritemsg -f $disp_file -M vxvmshm:482 \
"These disks that you specified have been excluded by the
  $CNTRL_EXCLUDE_FILE file:"
				fi
				voladm_list_display -anc \
				-f $disp_file $cntrl_exclude_list
			fi

			if [ -n "$disk_exclude_list" ]
			then
				if [ `list_count $disk_exclude_list` -eq 1 ]
				then
					export \
					disk_exclude_list DISK_EXCLUDE_FILE; \
					ewritemsg -M vxvmshm:498 \
"This disk that you specified has been excluded by the
  $DISK_EXCLUDE_FILE file: $disk_exclude_list"
					voladm_continue -n
				else
					export DISK_EXCLUDE_FILE; \
					ewritemsg -f $disp_file -M vxvmshm:483 \
"These disks that you specified have been excluded by the
  $DISK_EXCLUDE_FILE file:"
					voladm_list_display -anc \
					-f $disp_file $disk_exclude_list
				fi
			fi

			if [ -z "$device_list" ]
			then
				continue
			fi
		fi
		return 0
	done
}

# voladm_disk_group purpose default
voladm_disk_group()
{
	_new_group_okay=
	if [ X$1 = X-n ]
	then
		_new_group_okay=yes
		shift
	fi
	_opt="<group>,list"
	[ $# -eq 3 ] && _opt="<group>,none,list"
	_default=$2
	_prompt=$1
	while true
	do
		_riput=`egettxt "Which disk group" vxvmshm:582`
		read_input "$_riput" "$_opt" $_default
		dgname=$input
		case $dgname in
		"")	ewritemsg -M vxvmshm:90 "A disk group name is required."
			continue;;
		list|l)	(echo ""; vxdg list ) | voladm_display
			continue;;
		*" "* | *"	"*)
			export dgname; ewritemsg -M vxvmshm:195 "Disk group name $dgname is not valid."
			continue;;
		none)	[ $# -eq 3 ] && return 0;;
		esac
		_voldg_output=`vxdg -q list | grep "^\$dgname[ 	]"`
		if [ `list_count $_voldg_output` -ne 3 ]
		then
			export dgname; ewritemsg -M vxvmshm:466 "There is no active disk group named $dgname."

			if [ -n "$_new_group_okay" ] &&
			   voladm_yorn -M vxvmshm:149 "Create a new group named $dgname?" y
			then
				return 10
			fi
			continue
		fi
		if [ "X`list_item 2 $_voldg_output`" != Xenabled ]
		then
			export dgname; ewritemsg -M vxvmshm:193 \
		 "Disk group $dgname is not currently usable due to errors."
			continue
		fi
		return 0
	done
}

voladm_new_disk_dmname()
{
	_default_dmname=`vxnewdmname "$1"`

	if [ "$2" ] ; then
		disk="$2"; export disk 
		_riput=`egettxt "Enter disk name for $disk" vxvmshm:227`
	else
		_riput=`egettxt "Enter disk name" vxvmshm:226`
	fi

	# gather all known names currently in use in the disk group
	while true ; do
		read_input "$_riput" "<name>" $_default_dmname
		if [ "X$input" != "X$_default_dmname" ] &&
		   not vxnewdmname -c "$input" "$1"
		then
			export input;  ewritemsg -M vxvmshm:636 \
		      "dmname $input already in use. Please enter a new one."
			_riput=`egettxt "Enter disk name" vxvmshm:226`
			continue
		else
                        input_len=`strlen $input`
                        if [ `expr $input_len \>= 28` = 1 ] ; then
                                ewritemsg -M vxvmshm:398\
                "The disk name is too long. Please re-enter."
				_riput=`egettxt "Enter disk name" vxvmshm:226`
                                continue
                        fi
		fi
		break
	done

	dmname=$input
	return 0
}

#
# usage: voladm_get_disk_dmname \
#	 [-r] [-g disk-group] [-h on|off] \
#	 prompt default
# -r: restrict search to removed disks
# -h on|off: restrict to disks with the hot-spare flag on or off
# -g disk-group: restrict search to disks in disk-group
#
voladm_get_disk_dmname()
{
	_dm_want_removed=
	_dm_want_spec_dg=
	_dm_want_hotspare=

	OPTIND=1 # sh does not re-initialize OPTIND
	while getopts "rg:h:" c
	do
		case $c in
		r) _dm_want_removed=yes;;
		g) _dm_want_spec_dg="$OPTARG";;
		h) _dm_want_hotspare="$OPTARG";; 
		\?) echo "voladm_get_disk_dmname: bad argument list"
		    return 1;;
		esac
	done
	shift `expr $OPTIND - 1`

	if [ -n "$_dm_want_hotspare" -a \
	    "$_dm_want_hotspare" != "on" -a "$_dm_want_hotspare" != "off" ]
	then
		echo "voladm_get_disk_dmname: -h takes only 'on' or 'off'"
		return
	fi

	_dm_prompt="${1:-Enter disk name}"
	_default=$2

	while true
	do
		read_input "$_dm_prompt" "<disk>,list" $_default
		dmname=$input
		case $input in
		"")	ewritemsg -M vxvmshm:307 \
	 "Please enter a disk name, or type \\\"list\\\" to list all disks."
			continue;;
		$_default)
			return 0;;
		list|l)	if [ "$_dm_want_removed" ]
			then
				(echo ""
				 if [ -z "$_dm_want_spec_dg" ]
				 then
				 	vxprint -Atd
				 else
				 	vxprint -td -g "$_dm_want_spec_dg"
				 fi |
					awk '$1!="dm" || $3=="-" {print $0;}'
				 echo "") | voladm_display
			elif [ "$_dm_want_hotspare" ]
			then
				(echo ""
				 if [ -z "$_dm_want_spec_dg" ]
				 then
				 	vxdisk list
				 else
				 	vxdisk -g "$_dm_want_spec_dg"
				 fi |
				 if [ "$_dm_want_hotspare" = "on" ]
				 then
					awk '{
					if ($1 == "DEVICE" || $6 == "spare")
						print $0
					}'
				 else
					awk '{
					if ($1 == "DEVICE" || $6 == "")
						print $0
					}'
				 fi ) | voladm_display
			else
				(echo ""; vxprint -Atd ) | voladm_display
			fi
			continue
			;;
		esac
		# XXX - The disks below should be filtered if _dm_want_removed
		# or _dm_want_hotspare have been specified.
		if [ -z "$_dm_want_spec_dg" ]
		then
			set -- `vxprint -AQF "%dgname" "\$dmname" 2> /dev/null`
		else
			set -- `vxprint -g "$_dm_want_spec_dg" \
					-F "%dgname" "\$dmname" 2> /dev/null`
		fi
		if [ $# -eq 0 ] && [ -z "$_dm_want_spec_dg" ]
		then
			export dmname; ewritemsg -M vxvmshm:467 \
"There is no disk named $dmname in any disk group configuration.
  To get a list of disks enter \\\"list\\\"."
			continue
		elif [ $# -eq 0 ]
		then
			export dmname _dm_want_spec_dg; \
			ewritemsg -M vxvmshm:468 \
"There is no disk named $dmname in disk group $_dm_want_spec_dg.
  To get a list of disks enter \\\"list\\\"."
		fi
		break
	done
	if [ $# -eq 1 ]
	then
		dgname=$1
		return 0
	fi
	export dmname; ewritemsg -M vxvmshm:464 \
"There is a disk named $dmname in each of the following disk groups:
  	$*" "$@"
	while true
	do
		_riput=`egettxt "Enter the disk group to use" vxvmshm:230`
		read_input "$_riput" "group" $1
		for dgname in $*
		do
			if [ "X$dgname" = "X$input" ]
			then
				return 0
			fi
		done
		ewritemsg -M vxvmshm:303 \
			"Please choose a disk group from the list:\n\n\t$*" "$@"
	done
}

# re-online all online disks
voladm_reonline_all()
{
	vxdisk -a online > /dev/null 2>&1
}

# get the system hostid from the volboot file in system_hostid
voladm_system_hostid()
{
	system_hostid=
	system_hostid=`vxdctl list | grep '^hostid:' |
			{ read key hostid; echo "$hostid"; }`
	if [ -z "$system_hostid" ]
	then
		ewritemsg -M vxvmshm:514 \
	 "Unexpected internal error -- Cannot get the system's hostid."
		return 1
	fi
	return 0
}

strlen()
{
	echo $1 | awk '{ printf("%d",length($1)) }'
}

#
# table look-up increment for small positive integers
#
inc()
{
	if [ $# -ne 1 ]
	then
		echo "usage: inc var-name"
		return
	fi

	_i_var=$1
	eval "_i_val=\$$_i_var"

	case $_i_val in
	0) _i_new_val=1;;
	1) _i_new_val=2;;
	2) _i_new_val=3;;
	3) _i_new_val=4;;
	4) _i_new_val=5;;
	5) _i_new_val=6;;
	6) _i_new_val=7;;
	7) _i_new_val=8;;
	8) _i_new_val=9;;
	9) _i_new_val=10;;
	10) _i_new_val=11;;
	11) _i_new_val=12;;
	12) _i_new_val=13;;
	13) _i_new_val=14;;
	14) _i_new_val=15;;
	15) _i_new_val=16;;
	16) _i_new_val=17;;
	17) _i_new_val=18;;
	18) _i_new_val=19;;
	19) _i_new_val=20;;
	20) _i_new_val=21;;
	21) _i_new_val=22;;
	22) _i_new_val=23;;
	23) _i_new_val=24;;
	24) _i_new_val=25;;
	25) _i_new_val=26;;
	26) _i_new_val=27;;
	27) _i_new_val=28;;
	28) _i_new_val=29;;
	29) _i_new_val=30;;
	30) _i_new_val=31;;
	31) _i_new_val=32;;
	32) _i_new_val=33;;
	33) _i_new_val=34;;
	34) _i_new_val=35;;
	35) _i_new_val=36;;
	36) _i_new_val=37;;
	37) _i_new_val=38;;
	38) _i_new_val=39;;
	39) _i_new_val=40;;
	40) _i_new_val=41;;
	41) _i_new_val=42;;
	42) _i_new_val=43;;
	43) _i_new_val=44;;
	44) _i_new_val=45;;
	45) _i_new_val=46;;
	46) _i_new_val=47;;
	47) _i_new_val=48;;
	48) _i_new_val=49;;
	49) _i_new_val=50;;
	50) _i_new_val=51;;
	51) _i_new_val=52;;
	52) _i_new_val=53;;
	53) _i_new_val=54;;
	54) _i_new_val=55;;
	55) _i_new_val=56;;
	56) _i_new_val=57;;
	57) _i_new_val=58;;
	58) _i_new_val=59;;
	59) _i_new_val=60;;
	60) _i_new_val=61;;
	61) _i_new_val=62;;
	62) _i_new_val=63;;
	63) _i_new_val=64;;
	64) _i_new_val=65;;
	65) _i_new_val=66;;
	66) _i_new_val=67;;
	67) _i_new_val=68;;
	68) _i_new_val=69;;
	69) _i_new_val=70;;
	70) _i_new_val=71;;
	71) _i_new_val=72;;
	72) _i_new_val=73;;
	73) _i_new_val=74;;
	74) _i_new_val=75;;
	75) _i_new_val=76;;
	76) _i_new_val=77;;
	77) _i_new_val=78;;
	78) _i_new_val=79;;
	79) _i_new_val=80;;
	80) _i_new_val=81;;
	81) _i_new_val=82;;
	82) _i_new_val=83;;
	83) _i_new_val=84;;
	84) _i_new_val=85;;
	85) _i_new_val=86;;
	86) _i_new_val=87;;
	87) _i_new_val=88;;
	88) _i_new_val=89;;
	89) _i_new_val=90;;
	90) _i_new_val=91;;
	91) _i_new_val=92;;
	92) _i_new_val=93;;
	93) _i_new_val=94;;
	94) _i_new_val=95;;
	95) _i_new_val=96;;
	96) _i_new_val=97;;
	97) _i_new_val=98;;
	98) _i_new_val=99;;
	99) _i_new_val=100;;
	*) _i_new_val=`expr $_i_val + 1`
	esac

	eval $_i_var=$_i_new_val
}

#
# table look-up decrement for small positive integers
#
dec()
{
	if [ $# -ne 1 ]
	then
		echo "usage: dev var-name"
		return
	fi

	_d_var=$1
	eval "_d_val=\$$_d_var"

	case $_d_val in
	0) _d_new_val=-1;;
	1) _d_new_val=0;;
	2) _d_new_val=1;;
	3) _d_new_val=2;;
	4) _d_new_val=3;;
	5) _d_new_val=4;;
	6) _d_new_val=5;;
	7) _d_new_val=6;;
	8) _d_new_val=7;;
	9) _d_new_val=8;;
	10) _d_new_val=9;;
	11) _d_new_val=10;;
	12) _d_new_val=11;;
	13) _d_new_val=12;;
	14) _d_new_val=13;;
	15) _d_new_val=14;;
	16) _d_new_val=15;;
	17) _d_new_val=16;;
	18) _d_new_val=17;;
	19) _d_new_val=18;;
	20) _d_new_val=19;;
	21) _d_new_val=20;;
	22) _d_new_val=21;;
	23) _d_new_val=22;;
	24) _d_new_val=23;;
	25) _d_new_val=24;;
	26) _d_new_val=25;;
	27) _d_new_val=26;;
	28) _d_new_val=27;;
	29) _d_new_val=28;;
	30) _d_new_val=29;;
	31) _d_new_val=30;;
	32) _d_new_val=31;;
	33) _d_new_val=32;;
	34) _d_new_val=33;;
	35) _d_new_val=34;;
	36) _d_new_val=35;;
	37) _d_new_val=36;;
	38) _d_new_val=37;;
	39) _d_new_val=38;;
	40) _d_new_val=39;;
	41) _d_new_val=40;;
	42) _d_new_val=41;;
	43) _d_new_val=42;;
	44) _d_new_val=43;;
	45) _d_new_val=44;;
	46) _d_new_val=45;;
	47) _d_new_val=46;;
	48) _d_new_val=47;;
	49) _d_new_val=48;;
	50) _d_new_val=49;;
	51) _d_new_val=50;;
	52) _d_new_val=51;;
	53) _d_new_val=52;;
	54) _d_new_val=53;;
	55) _d_new_val=54;;
	56) _d_new_val=55;;
	57) _d_new_val=56;;
	58) _d_new_val=57;;
	59) _d_new_val=58;;
	60) _d_new_val=59;;
	61) _d_new_val=60;;
	62) _d_new_val=61;;
	63) _d_new_val=62;;
	64) _d_new_val=63;;
	65) _d_new_val=64;;
	66) _d_new_val=65;;
	67) _d_new_val=66;;
	68) _d_new_val=67;;
	69) _d_new_val=68;;
	70) _d_new_val=69;;
	71) _d_new_val=70;;
	72) _d_new_val=71;;
	73) _d_new_val=72;;
	74) _d_new_val=73;;
	75) _d_new_val=74;;
	76) _d_new_val=75;;
	77) _d_new_val=76;;
	78) _d_new_val=77;;
	79) _d_new_val=78;;
	80) _d_new_val=79;;
	81) _d_new_val=80;;
	82) _d_new_val=81;;
	83) _d_new_val=82;;
	84) _d_new_val=83;;
	85) _d_new_val=84;;
	86) _d_new_val=85;;
	87) _d_new_val=86;;
	88) _d_new_val=87;;
	89) _d_new_val=88;;
	90) _d_new_val=89;;
	91) _d_new_val=90;;
	92) _d_new_val=91;;
	93) _d_new_val=92;;
	94) _d_new_val=93;;
	95) _d_new_val=94;;
	96) _d_new_val=95;;
	97) _d_new_val=96;;
	98) _d_new_val=97;;
	99) _d_new_val=98;;
	*) _d_new_val=`expr $_d_val - 1`
	esac

	eval $_d_var=$_d_new_val
}

. ${VOLADM_LIB:-/usr/lib/vxvm/voladm.d/lib}/vxadm_syslib.sh
