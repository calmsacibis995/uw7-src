:
#
#	@(#) update.sh 12.1 95/05/09 SCOINC
#
#	Copyright (C) The Santa Cruz Operation, 1991-1993.
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#
#	Graphic Input Device initialization script
#       

PATH=/etc:/bin:/usr/bin
LANG=english_us.ascii
export PATH LANG

TMP_FILE=/tmp/class$$

GRAFINFO_DIR=/usr/lib/grafinfo
CLASSFILE=class.h
PARSER=vidparse
MKCFILES=mkcfiles
PARSER_DIR=/usr/lib/vidconf
export GRAFINFO_DIR CLASSFILE PARSER MKCFILES PARSER_DIR

GRAFDEF=/usr/lib/grafinfo/grafinfo.def
GRAFDEV=/usr/lib/grafinfo/grafdev
GRAFMON=/usr/lib/grafinfo/grafmon
DEVICES=/usr/lib/vidconf/devices

# Define system dependent variables
if [ -d /etc/conf/cf.d ]
then
    LINK_COMMAND=link_unix
    LINK_DIR=/etc/conf/cf.d
    CLASS_DIR=/etc/conf/pack.d/cn
else
    LINK_COMMAND=link_xenix
    LINK_DIR=/usr/sys/conf
    CLASS_DIR="where will this go"
    #echo "So oh! So confused!"
    #echo "XENIX system."
    exit 0
fi

export LINK_COMMAND LINK_DIR CLASS_DIR 


# Define return values
: ${OK=0} ${FAIL=1} ${STOP=10} ${HALT=11}

# FUNCTIONS
#########################

# ---------- STANDARD ROUTINES -------- These routines are common to scripts
#					requiring kernel relinking.

# Define traps for critical and non critical code.
set_trap()  {	
	trap 'echo "Interrupted! Exiting ..."; cleanup 1' 1 2 3 15
}
unset_trap()  {
	trap '' 1 2 3 15
}
 
# Remove temp files and exit with the status passed as argument
cleanup() {
	trap '' 1 2 3 15
	[ -f $TMP_FILE ] && rm $TMP_FILE
	exit $1
}

# Prompt for yes or no answer - returns non-zero for no
getyn() {
	while	echo "$* (y/n) \c">&2
	do	read yn rest
		case $yn in
		[yY])	return $OK 			;;
		[nN])	return $FAIL			;;
		*)	echo "Please answer y or n" >&2	;;
		esac
	done
}

# prompt function
prompt () { 
    while echo "\n${mesg}$quit \c" >&2
    do
        read cmd
        case $cmd in
            Q|q)
                return $FAIL
                ;;
            +x|-x)
                set $cmd
                ;;
            !*)
                eval `expr "$cmd" : "!\(.*\)"`
                ;;
            "") # "quit" is null when prompt is "press return to continue"
                [ -z "$quit" ] && return $OK
                # if there is an argument use it as the default
                # else loop until cmd is set
                [ "$1" ] && { 
                    cmd=$1
                    return $OK
                }
                error Invalid response
                ;;
            *)  return $OK
                ;;
        esac
    done
}

# Print an error message
error() {
	echo "\nError: $*" >&2
	return $FAIL
}

# Configure error message
conferr()  {
	error "\nConfigure failed to update system configuration.
Check /etc/conf/cf.d/conflog for details."
}

# perms list needed if link kit must be installed
permschk () {
	if [ -f /etc/perms/extmd ]; then
		PERM=/etc/perms/extmd
	elif [ -f /etc/perms/inst ]; then
		PERM=/etc/perms/inst
	else
		error "Cannot locate linkkit permlist needed to verify
 linkkit installation"
		cleanup $FAIL
	fi
}

# test to see if link kit is installed
linkchk()  {
	cd /
	until	fixperm -i -d LINK $PERM
	do	case $? in
		4)  echo "\nThe Link Kit is not installed." >&2	;;
		5)  echo "\nThe Link Kit is only partially installed." >&2  ;;
		*)  echo "\nError testing for Link Kit.  Exiting." >&2; cleanup $FAIL  ;;
		esac

		# Not fully installed. Do so here
		getyn "\nDo you want to install it now?" || {
			# answered no
			echo "
The link kit must be installed to run this program.  Exiting ..."
			cleanup $OK
		}

		# answered yes, so install link kit
		echo "\nInvoking /etc/custom\n"
		/etc/custom -o -i LINK || {
			# custom exited unsuccessfully
			error "custom failed to install Link Kit successfully.  Please try again."
			cleanup $FAIL
		}
	done
}

asklink()  {
	getyn "
You must create a new kernel to effect the driver change you specified.
Do you want to create a new kernel now?"  ||  {
		echo "
To create a new kernel execute $LINK_DIR/$LINK_COMMAND. Then you
must reboot your system by executing  /etc/shutdown -i0  before the 
changes you have specified will be implemented.\n"
			return $FAIL 
		}
	return $OK
}

# re-link new kernel
klink() {
	cd $LINK_DIR
	./$LINK_COMMAND || { 
		echo "\nError: Kernel link failed."
		cleanup $FAIL
	}
	return $OK
}

# make C-files
createCfiles()
{
    echo "Creating loadable grafinfo files"
    $PARSER_DIR/$MKCFILES $GRAFINFO_DIR 
    if [ $? != 0 ]
    then
        exit
    fi

    for DIR in $GRAFINFO_DIR/*
    do
        if [ -d $DIR ]
        then
             cd $DIR
             for CFILE in *.c
             do
                 if [ $CFILE != "*.c" ]
                 then
                     FILE=`echo $CFILE | sed 's/\.c//'`
                     XGIFILE=$FILE.xgi
                     OFILE=$FILE.o
                     SFILE=$FILE.s
                     XFILE=X$FILE.o

                     /lib/idcpp -I$PARSER_DIR $CFILE | /lib/idcomp >$SFILE
                     [ $? = 0 ] && /bin/idas -o $OFILE $SFILE \
                                && rm $CFILE $SFILE
                     [ $? = 0 ] && /bin/idld -r -x -e CfunctionTab \
                                  -o $XFILE $OFILE && rm $OFILE
                     [ -f $CFILE ] && \
                        echo "Error: `pwd`/$XGIFILE could not be converted"
                 fi
             done
        fi
    done
}

# create grafinfo.def, grafdev, grafmon from args and class.h 
createStuff () {
    if [ $# != 3 ]
    then
    	echo Incorrect number of arguments
	cleanup $FAIL
    fi

    CARD=$1
    MODE=$2
    MON=$3

    CARDMODE=$CARD.$MODE

    rm -f $GRAFDEF
    echo $CARDMODE > $GRAFDEF

    if [ -f $DEVICES/* ]
    then
	rm -f $GRAFDEV
	echo Creating grafdev file.
	touch $GRAFDEV
	for i in $DEVICES/*
	do
		echo `cat $i`:$CARDMODE >> $GRAFDEV
	done
    fi

    echo Creating grafmon file.

    rm -f $GRAFMON
    echo $CARD:$MON > $GRAFMON

    # parsit
    $PARSER_DIR/$PARSER $TMP_FILE
    if [ "$?" != "0" ]
    then
        echo "\nUnable to update system configuration."
        cleanup $FAIL
    fi

    # create C-files and compile
    #createCfiles

    mv $TMP_FILE $CLASS_DIR/$CLASSFILE

    cleanup $OK
}


# main()
#########################

#
# $PARSER will generate the file $TMP_FILE
#
# If $TMP_FILE is different from the file 
# $CLASS_DIR/class.h then replace $CLASS_DIR/class.h
# with $TMP_FILE and rebuild the kernel. 
#
# Kernel should also be rebuilt if $PARSER_DIR/.new_unix
# exists.  This file might be created by a ~/vidconf/script
# for a video card driver.

set_trap
if [ $# != 0 ] && [ $1 = -f ]
then
	createStuff $2 $3 $4
fi

echo "Updating system configuration.  Please wait..."
$PARSER_DIR/$PARSER $TMP_FILE
if [ "$?" != "0" ]
then
    echo "\nUnable to update system configuration."
    cleanup $FAIL
fi

echo "The system configuration has been updated."

# create C-files and compile
#createCfiles

status=1
if [ -f $CLASS_DIR/$CLASSFILE ]; then
    diff $TMP_FILE $CLASS_DIR/$CLASSFILE > /dev/null 
    status=$?
fi

if [ $status != 0 ]; then
    mv $TMP_FILE $CLASS_DIR/$CLASSFILE
elif [ ! -f ${PARSER_DIR}/.new_unix ]; then
    cleanup $OK
fi

rm -f ${PARSER_DIR}/.new_unix

#
# relink kernel occurs when there is a new class.h file, when 
# $CLASS_DIR/$CLASSFILE is missing or when $PARSER_DIR/.new_unix
# is found.
permschk
linkchk
if [ "$_RELINK" -o "$_NOPROMPT" ]
then
    klink
else
    asklink && klink  
fi
cleanup $OK
