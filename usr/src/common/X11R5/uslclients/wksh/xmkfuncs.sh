#	Copyright (c) 1990, 1991 AT&T and UNIX System Laboratories, Inc.
#	All Rights Reserved
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	and UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)wksh:xmkfuncs.sh	1.1.1.1"

#
# Addbuttons - add simple buttons with callbacks to another widget
#
# usage: addbuttons $PARENT Label1 Callback1 Label2 Callback2 ...
#
# For example: addbuttons $MENU_MP Open do_Open Save do_Save Quit "exit"

addbuttons() {
	typeset W nowidget="" TMP=""

	if [ x"$1" = "x-w" ]
	then nowidget=false
	     shift
	fi
	W=$1
	shift
	while [ $# -gt 1 ]
	do
		if [ "$nowidget" = false ]
		then 
			cmw "$1" "$1" ${BUTTONTYPE:-pushButtonGadget} "$W" labelString:"$2" activateCallback:"$3"
			shift 3
		else 
			cmw TMP TMP ${BUTTONTYPE:-pushButtonGadget} "$W" labelString:"$1" activateCallback:"$2"
			shift 2
		fi
	done
}

# Usage: addfields $HANDLE [ varname label verify length ... ]
#
# Adds single line labeled text widgets to $HANDLE

addfields() {
	typeset TMP="" TMP2=""
	W="$1"

	shift
	while [ $# -gt 3 ]
	do
		cmw TMP tmp rowColumn $W orientation:horizontal
		cmw "TMP2" label label "$TMP" labelString:"$2" $LABARGS
		eval "$1_LAB=$TMP"
		cmw "$1" "$1" text "$TMP" activateCallback:"$3" \
			columns:$4 \
			$TFARGS
		shift 4
	done
}

addpulldowns() {	# $PARENT [VAR LABEL ...]
	typeset W="$1"

	shift
	while [ $# -gt 1 ]
	do
		crtpulldownm "$1" "$W" "$2"
		shift 2
	done
}

under() { # Widget [offset]
	echo "topWidget:$1 topAttachment:ATTACH_WIDGET topOffset:${2:-0}"
}

over() { # Widget [offset]
	echo "bottomWidget:$1 bottomAttachment:ATTACH_WIDGET bottomOffset:${2:-0}"
}

rightof() { # Widget [offset]
	echo "leftWidget:$1 leftAttachment:ATTACH_WIDGET leftOffset:${2:-0}"
}

leftof() { # Widget [offset]
	echo "rightWidget:$1 rightAttachment:ATTACH_WIDGET rightOffset:${2:-0}"
}

floatright() {	# [offset]
	echo "rightAttachment:ATTACH_FORM rightOffset:${1:-0}"
}

floatleft() {	# [offset]
	echo "leftAttachment:ATTACH_FORM leftOffset:${1:-0}"
}

floattop() {	# [offset]
	echo "topAttachment:ATTACH_FORM topOffset:${1:-0}"
}

floatbottom() {	# [offset]
	echo "bottomAttachment:ATTACH_FORM bottomOffset:${1:-0}"
}

spanwidth() {	# [leftoffset rightoffset]
	echo "leftAttachment:ATTACH_FORM leftOffset:${1:-0} rightAttachment:ATTACH_FORM rightOffset:${2:-0}"
}

spanheight() {	# [topoffset bottomoffset]
	echo "topAttachment:ATTACH_FORM topOffset:${1:-0} bottomAttachment:ATTACH_FORM bottomOffset:${2:-0}"
}

#
# Create a notice that warns about something, with an OK button
# With additional args also can set up cancel and help buttons.
#

warn() {
	typeset child

	if [ "$_WARNWIDGET" = "" ]
	then crtwarningd _WARNWIDGET $TOPLEVEL Warning
	fi
	sv $_WARNWIDGET \
		messageString:"$1"

	if [ ! "$2" ] 
	then sv $_WARNWIDGET okCallback:"umc $_WARNWIDGET"
	else sv $_WARNWIDGET okCallback:"$2"
	fi

	if [ ! "$3" ] 
	then umc $_WARNWIDGET_CAN
	else sv $_WARNWIDGET cancelCallback:"$3"
	fi
	if [ ! "$4" ] 
	then umc $_WARNWIDGET_HELP
	else sv $_WARNWIDGET helpCallback:"$4"
	fi
	mc $_WARNWIDGET
}

fatal() {
	typeset child

	if [ "$_FATALWIDGET" = "" ]
	then crterrord _FATALWIDGET $TOPLEVEL Fatal okCallback:"exit 1"
	     umc $_FATALWIDGET_CAN
	     umc $_FATALWIDGET_HELP
	fi
	sv $_FATALWIDGET \
		messageString:"$1"

	mc $_FATALWIDGET
}

working() {
	typeset child

	if [ "$_WORKINGWIDGET" = "" ]
	then crtworkingd _WORKINGWIDGET $TOPLEVEL Working
	fi
	sv $_WORKINGWIDGET \
		messageString:"$1"
	if [ ! "$2" ] 
	then sv $_WORKINGWIDGET okCallback:"umc $_WORKINGWIDGET"
	else sv $_WORKINGWIDGET okCallback:"$2"
	fi

	if [ ! "$3" ] 
	then umc $_WORKINGWIDGET_CAN
	else sv $_WORKINGWIDGET cancelCallback:"$3"
	fi
	if [ ! "$4" ] 
	then umc $_WORKINGWIDGET_HELP
	else sv $_WORKINGWIDGET helpCallback:"$4"
	fi

	mc $_WORKINGWIDGET
}

error() {
	typeset child

	if [ "$_ERRORWIDGET" = "" ]
	then crterrord _ERRORWIDGET $TOPLEVEL Working
	fi
	sv $_ERRORWIDGET \
		messageString:"$1"
	if [ ! "$2" ] 
	then sv $_ERRORWIDGET okCallback:"umc $_WORKINGWIDGET"
	else sv $_ERRORWIDGET okCallback:"$2"
	fi

	if [ ! "$3" ] 
	then umc $_ERRORWIDGET
	else sv $_ERRORWIDGET cancelCallback:"$3"
	fi
	if [ ! "$4" ] 
	then umc $_ERRORWIDGET
	else sv $_ERRORWIDGET helpCallback:"$4"
	fi

	mc $_ERRORWIDGET
}


#
# Create a notice that asks for confirmation.
# $1 = string indicating the nature of the decision
# $2 = a function to call if the user says "YES"
# $3 = a function to call if the user says "NO"
#

confirm() {
	typeset child

	if [ "$_CONFWIDGET" = "" ]
	then crtquestiond _CONFWIDGET $TOPLEVEL Question
	fi
	sv $_CONFWIDGET \
		messageString:"$1"

	if [ ! "$2" ] 
	then sv $_CONFWIDGET okCallback:"umc $_CONFWIDGET"
	else sv $_CONFWIDGET okCallback:"$2"
	fi

	if [ ! "$3" ] 
	then umc $_CONFWIDGET_CAN
	else sv $_CONFWIDGET cancelCallback:"$3"
	fi
	if [ ! "$4" ] 
	then umc $_CONFWIDGET_HELP
	else sv $_CONFWIDGET helpCallback:"$4"
	fi

	mc $_CONFWIDGET
}

#
# Addrows: add single row controlAreas to a parent.
#
# usage: addrows $PARENT [VAR ...]
#
# This is mainly useful to motif/open look compatibility.

addrows() { # $1 = parent widget handle, rest of args are variable names to set
	typeset W=$1 i tmp

	shift
	for i
	do
		cmw "$i" row rowColumn "$W" orientation:horizontal
	done
}

#
# Addcols: add single column controlAreas to a parent.
#
# usage: addcols $PARENT [VAR ...]
#
# This is mainly useful to motif/open look compatibility.

addcols() { # $1 = parent widget handle, rest of args are variable names to set
	typeset W=$1 i tmp

	shift
	for i
	do
		cmw "$i" col rowColumn "$W" orientation:vertical
	done
}
