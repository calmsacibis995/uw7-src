#!/bin/sh
#
# @(#) xsconfig.sh 11.1 97/10/22
#
# Copyright (C) 1983-1991 The Santa Cruz Operation, Inc.
#
# The information in this file is provided for the exclusive use of the
# licensees of The Santa Cruz Operation, Inc.  Such users have the right 
# to use, modify, and incorporate this code into other products for purposes 
# authorized by the license agreement provided they include this notice 
# and the associated copyright notice with any such product.  The 
# information in this file is provided "AS IS" without warranty.
# 
#
# Purpose: 
#	xsconfig.sh will use X11 and system keyboard configuration files 
#	to create the server configuration file '.Xsco.cfg'.
#
# Usage: 
#       xsconfig.sh [-m] \ 
#         [[/usr/lib/X11/csxmaps/csxmap_file] /usr/lib/keyboard/keyboard_name]
#
#       In the absence of command line arguments, xsconfig.sh will generate
#       '.Xsco.cfg' from the information in the 'DEFAULT' files in each of
#       the subdirectories 'definitions', 'keyctrl', 'keysyms', 'modifiers',
#       and 'translations'.
#
#       If the optional parameter {keyboard_name} is given, then xsconfig.sh
#       will generate '.Xsco.cfg' from the information in the file named
#       {keyboard_name} in each of the subdirectories 'definitions',
#       'keyctrl', 'keysyms', 'modifiers', and 'translations'.  Whenever the
#       file name {keyboard_name} does not exist in any of these
#       sub-directories, then xsconfig.sh will use the 'DEFAULT' file.
#       
#	If the optional paramaters {csxmap_file} and {keyboard_name} are given 
#       and the file 'keysyms/{keyboard_name} does not exist, then
# 	this script will call 'mapkey -c' with those paramaters.  The output
#       of 'mapkey -c' will provide the default '[keysyms]' settings to be
#       used instead of the settings specified in the file 'keysyms/DEFAULT'.
#       In addition Mode_switch will be defined to be Mod3 unless the '-m'
#       option is specified.
#
#

# Define return values, variables, defaults
: ${OK=0} ${FAIL=1} ${STOP=10} ${HALT=11}

TMP=/tmp/xs$$
TMP_KEYSYMS=/tmp/keysyms$$
TMP_KEYCTRL=/tmp/keyctrl$$

# Define traps for critical and non critical code.
trap 'echo "\nInterrupted! Exiting ..."; Cleanup $FAIL' 1 2 3 15

Cleanup() {
	trap '' 1 2 3 15
        rm -f $TMP $TMP_KEYSYMS $TMP_KEYCTRL
	exit $1
}

Usage() {
	echo "Usage:" 
	echo "xsconfig.sh [-m] [keyboard_name]"
	Cleanup $FAIL
} 

NoFile() {
	echo "$PROG: Error, cannot open file <$1>"
	Usage
} 

# Print the [keyctrl] section of file.
print_keyctrl () { 

	awk 'BEGIN { in_keyctrl= 0; } 
	/^\[keyctrl\]/  	{ in_keyctrl++; print $0; continue; }
	/^\[/ 			{ in_keyctrl = 0; }
				{ if (in_keyctrl) print $0; }
	' $1
}


# Print the [keysyms] section of file.
print_keysyms () { 

	awk 'BEGIN { in_keysyms= 0; } 
	/^\[keysyms\]/  	{ in_keysyms++; print $0; continue; }
	/^\[/ 			{ in_keysyms = 0; }
				{ if (in_keysyms) print $0; }
	' $1
}

# Search for configuration file.
kbdSearch () {

	CATEGORY=$1
	SPATH=${X11_ROOT}/lib/X11/xsconfig/
	if [ $SPATH != `pwd` ]
	then
		SPATH="`pwd` $HOME $SPATH"
	fi

        # Search for matching file name
	NAME=`basename $2`
	for i in $SPATH
	do
		if [ -f $i/$CATEGORY/$NAME ] 
		then
			echo $i/$CATEGORY/$NAME
			return 0
		fi
	done

        # Strip suffix off of file name and search
        NAME=`basename $2 '\..*'`
	for i in $SPATH
	do
		if [ -f $i/$CATEGORY/$NAME ] 
		then
			echo $i/$CATEGORY/$NAME
			return 0
		fi
	done

        # Look for default
	DEFAULT=$3
        if [ "$DEFAULT" != "" ]
        then
                if [ -f $DEFAULT ]
                then
                        echo $DEFAULT
                        return 0
                else
                	for i in $SPATH
                	do
        	        	if [ -f $i/$DEFAULT ] 
	                	then
                			echo $i/$DEFAULT
		                	return 0
	                	fi
                	done
                fi
        fi

	return 1
}




#
# main()
#
PROG=$0
if [ -d /usr/X11R6.1 ]
then
        X11_ROOT=/usr/X11R6.1
else
        X11_ROOT=/usr
fi
MODE_SWITCH=DEFAULT.MOD3

# Init DEFAULT keyboard settings (USA keyboard)
KEYBOARD=DEFAULT
DEFINITIONS=`kbdSearch definitions $KEYBOARD config.txt`
KEYSYMS=`kbdSearch keysyms $KEYBOARD default.kbd`
MISC_KEYSYMS=`kbdSearch keysyms/misc $KEYBOARD misc.kbd`
MODIFIERS=`kbdSearch modifiers $KEYBOARD mod.usa.kbd`
KEYCTRL=`kbdSearch keyctrl $KEYBOARD`
TRANSLATIONS=`kbdSearch translations $KEYBOARD trans101.kbd`
XKB=`kbdSearch xkb $KEYBOARD ""`

# Command line options
while getopts m opt
do
	case $opt in
	m) MODE_SWITCH=DEFAULT ;;
	?) Usage ;;
	esac
done

shift `expr $OPTIND - 1`

if [ $# -eq 1 ]
then

        # Use keyboard specific files
        KEYBOARD=`basename $1`

elif [ $# -eq 2 ]
then

	CSXMAP=$1  ;	[ -z "$CSXMAP" ] && Usage 
	KEYBOARD=$2; 	[ -z "$KEYBOARD" ] && Usage 

	# make sure the files exist.
	[ -f "$CSXMAP" ] || NoFile $CSXMAP 
	[ -f "$KEYBOARD" ] || NoFile $KEYBOARD 

	# run 'mapkey -c' to generate the mapkey.kbd file.
	mapkey -c $CSXMAP $KEYBOARD > $TMP

        if [ $MODE_SWITCH = "DEFAULT.MOD3" ]
        then
        	print_keysyms $TMP | sed -e "s/XK_Alt_R/XK_Mode_switch/g" > \
                        $TMP_KEYSYMS
                MODIFIERS=`kbdSearch modifiers DEFAULT.MOD3 $MODIFIERS`
        else
        	print_keysyms $TMP > $TMP_KEYSYMS
        fi

        print_keyctrl $TMP > $TMP_KEYCTRL

        KEYBOARD=`basename $KEYBOARD`
        KEYSYMS=$TMP_KEYSYMS
        KEYCTRL=$TMP_KEYCTRL
        MODIFIERS=`kbdSearch modifiers $MODE_SWITCH $MODIFIERS`

fi


DEFINITIONS=`kbdSearch definitions $KEYBOARD $DEFINITIONS`
KEYSYMS=`kbdSearch keysyms $KEYBOARD $KEYSYMS`
MISC_KEYSYMS=`kbdSearch keysyms/misc $KEYBOARD $MISC_KEYSYMS`
MODIFIERS=`kbdSearch modifiers $KEYBOARD $MODIFIERS`
KEYCTRL=`kbdSearch keyctrl $KEYBOARD $KEYCTRL`
TRANSLATIONS=`kbdSearch translations $KEYBOARD $TRANSLATIONS`
XKB=`kbdSearch xkb $KEYBOARD "$XKB"`

#
# Create the keyboard configuration file
#
${X11_ROOT}/bin/xsconfig -o .Xsco.cfg \
        $DEFINITIONS $TRANSLATIONS $KEYSYMS $MISC_KEYSYMS \
         $MODIFIERS $KEYCTRL $XKB

Cleanup $?
