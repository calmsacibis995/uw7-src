#ident "@(#)pick.set.sh	15.1	97/12/19"
#!/usr/bin/ksh

# Copyright (c) 1997 The Santa Cruz Operation, Inc.. All Rights Reserved. 
#                                                                         
#        THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF THE               
#                   SANTA CRUZ OPERATION INC.                             
#                                                                         
#   The copyright notice above does not evidence any actual or intended   
#   publication of such source code.                                      


#ident  "@(#)pick.set.sh	13.1	97/12/06"

#Script to edit the scripts that go on the boot-floppy.
#This script will make different boot-floppies for installing
#different sets. In UnixWare 2.1, there is only one set - the 
#UnixWare set replaces the AS and PE sets.

function do_disk_ele
{
	rm -f $PROTO/stage/disk.ele
	ln -s $PROTO/stage/$SET.ele $PROTO/stage/disk.ele
}

function get_pkginfo
{
	PKGINFO=$ROOT/usr/src/$WORK/set/$SET/pkginfo
	[ -s $PKGINFO ] || {
		print -u2 Fatal Error -- $PKGINFO does not exist or is empty.
		exit 2
	}
	. $PKGINFO
}

function get_release
{
	RELEASE=$(grep 'define.*REL' $ROOT/$MACH/etc/conf/pack.d/name/space.c)
	RELEASE=${RELEASE%\"*}	# strip the final quote and the rest of the line
	RELEASE=${RELEASE#*\"}	# strip up to and including the first quote
	VERSION=$(grep 'define.*VER' $ROOT/$MACH/etc/conf/pack.d/name/space.c)
	VERSION=${VERSION%\"*}
	VERSION=${VERSION#*\"}
}

function get_relocatable
{
	typeset pkg param value
	unset RELPKGS

	cd $ROOT/$SPOOL
	for pkg in *
	do

# If the package isn't in the $SET/setinfo file, we don't care if
# it has relocatable files or not, since it's not in the set we'll be
# installing from the boot floppy.  Don't bother to check that package

		grep "^$pkg[ 	]" $SET/setinfo >/dev/null || continue

		if [ -d $pkg/reloc* ]
		then
			param=$(nawk '
				BEGIN {
					FS = "[ /]"
				}
				$4 ~ /^\$/ { print $4 }' $pkg/pkgmap |
				sort -u
			)
			case $(print $param) in
			*" "*)
				print -u2 "$CMD: ERROR: $ROOT/$SPOOL/$pkg/pkgmap"
				print -u2 "\tcontains more than one variable."
				exit 1
				;;
			esac
			param=${param#?} #strip off the dollar sign
			[ -f $pkg/install/response ] || {
				print -u2 "$CMD: ERROR: $ROOT/$SPOOL/$pkg/install/response"
				print -u2 "\tdoes not exist."
				exit 1
			}
			[ -z "$param" ] && continue
			value=$(grep $param $pkg/install/response)
			value=${value#*=} #strip off everything up to the = sign
			value=$(eval print $(print $value)) #strip off quotes, if any
			RELPKGS="$RELPKGS $pkg:$param=$value"
		fi
	done
	RELPKGS=${RELPKGS#?} #strip off the space
	cd $PROTO
}

function do_edits
{
	sed \
		-e "/REL_FULLNAME=.XXX/s/XXX/$REL_FULLNAME/" \
		-e "/RELEASE=.XXX/s/XXX/$RELEASE/" \
		-e "/VERSION=.XXX/s/XXX/$VERSION/" \
		-e "/FULL_SET_NAME=.XXX/s/XXX/$NAME/" \
		-e "/SET_NAME=.XXX/s/XXX/$PKG/" \
		-e "/LANG=.XXX/s/XXX/$LANG/" \
		-e "/LC_CTYPE=.XXX/s/XXX/$LANG/" \
		-e "/RELPKGS=.XXX/s,XXX,$RELPKGS," \
		$PROTO/stage/desktop/scripts/inst.gen \
			> $PROTO/stage/desktop/scripts/inst
	sed \
		-e "/REL_FULLNAME=.XXX/s/XXX/$REL_FULLNAME/" \
		-e "/RELEASE=.XXX/s/XXX/$RELEASE/" \
		-e "/VERSION=.XXX/s/XXX/$VERSION/" \
		-e "/FULL_SET_NAME=.XXX/s/XXX/$NAME/" \
		-e "/SET_NAME=.XXX/s/XXX/$PKG/" \
		-e "/LANG=.XXX/s/XXX/$LANG/" \
		-e "/LC_CTYPE=.XXX/s/XXX/$LANG/" \
		-e "/RELPKGS=.XXX/s,XXX,$RELPKGS," \
		$PROTO/stage/desktop/scripts/do_install.gen \
			> $PROTO/stage/desktop/scripts/do_install
}

#main()
#Check the environment
varerr=false
[ -z "${ROOT}" ] && {
	print -u2 ROOT is not set
	varerr=true
}
[ -z "${MACH}" ] && {
	print -u2 MACH is not set
	varerr=true
}
[ -z "${WORK}" ] && {
	print -u2 WORK is not set
	varerr=true
}
[ -z "${PROTO}" ] && {
	print -u2 PROTO is not set
	varerr=true
}

. ${ROOT}/${MACH}/var/sadm/dist/rel_fullname
[ -z "${REL_FULLNAME}" ] && {
	print -u2 REL_FULLNAME not set
	varerr=true
}
$varerr && exit 1

#Parse command line
CMD=$0
function Usage
{
	print -u2 "Usage: ${CMD}  -u [-l locale]"
	exit 1
}

unset SET
LANG=C
while getopts ul: c
do
	case $c in
	u)
		SET=UnixWare
		;;
	l)
		LANG=$OPTARG
		;;
	\?)	Usage
		;;
	*)	print -u2 Internal error during getopts.
		exit 1
		;;
	esac
done

[ -n "$SET" ] || Usage

#main body
do_disk_ele
get_pkginfo
get_release
get_relocatable

# since all pkgs may not be present when cutting boot floppy,
# use a default string. use the output of get_relocatable
# only as verification.
# get_relocatable and the default should actually be set specific
# and will change in a future load.

#DEFREL="UWdocs:DOC_ROOT=/usr/doc SDKdocs:DOC_ROOT=/usr/doc deUWdocs:DOC_ROOT=/usr/doc dynatext:DOC_ROOT=/usr/doc esUWdocs:DOC_ROOT=/usr/doc frUWdocs:DOC_ROOT=/usr/doc itUWdocs:DOC_ROOT=/usr/doc jaUWdocs:DOC_ROOT=/usr/doc"


DEFREL="TEDlogin:TED_DIR=/usr/dt TEDdesk:TED_DIR=/usr/dt TEDdocs:TED_DIR=/usr/dt TEDhelp:TED_DIR=/usr/dt TEDman:TED_DIR=/usr/dt TEDlde:TED_DIR=/usr/dt TEDlfr:TED_DIR=/usr/dt TEDljpe:TED_DIR=/usr/dt TEDhde:TED_DIR=/usr/dt TEDhfr:TED_DIR=/usr/dt TEDhjpe:TED_DIR=/usr/dt TEDles:TED_DIR=/usr/dt visionfs:GLOBDIR=/usr/local/vision termlite:GLOBDIR=/usr/local/vision"

if [ "X${RELPKGS}" != "X${DEFREL}" ]
then
	echo "WARNING: pkgs are not made or relocatable info may have changed."
	RELPKGS=$DEFREL
fi

do_edits
