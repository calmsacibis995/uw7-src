#!/sbin/sh
#ident	"@(#)dcu:scripts/dculib.sh	1.74.8.5"

#
# NOTE: if you want a blank line in a embedded string a string use
# ${IM_A_BLANK_LINE_DONT_TOUCH}
#

function dcuinit
{
# dcuinit()
# Called from main() to initialize the world
# declare/initialize all dcu globals HERE!!!.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

rsrcinit && return 0
# unset board and driver settings
set -A BDTYPE
set -A BDKEY
set -A BDCHGS
set -A BDCNFG
set -A BDCLEANED 
set -A BDMODNAME
set -A BDMODINDX
set -A BDINTERPSM

set -A BDNAME
set -A BDUNIT
set -A BDIPL
set -A BDITYPE
set -A BDIRQ
set -A BDPORTSTART
set -A BDPORTEND
set -A BDADDRSTART
set -A BDADDREND
set -A BDDMA
set -A BDCPU
set -A BDBUSTYPE
set -A BDID
set -A BDCLASS
set -A BDENTRYTYPE

# If set for a board, then board has changes to boards specific parameters
set -A BDHASBSP
set -A BDBSPPARAMS
set -A BDBSPVALS

set -A DRVMODCHOICES
set -A DRVISCCHOICES
set -A DRVUNITCHOICES
set -A DRVIPLCHOICES
set -A DRVITYPECHOICES
set -A DRVIRQCHOICES
set -A DRVPORTSCHOICES
set -A DRVPORTECHOICES
set -A DRVMEMSCHOICES
set -A DRVMEMECHOICES
set -A DRVDMACHOICES
set -A DRVCPUCHOICES

set -A DRVAUTOCONFIG
set -A DRVVERIFY
set -A DRVCATEGORY
set -A DRVBRAND

set -A DRVBUSTYPE
set -A DRVBOARDID
set -A DRVBOARDNAME
set -A DRVBOARDNAMEFULL
set -A DRVSTATIC
set -A DRVBCFGFILE

set -A MODNAME
set -A LOADMODS
set -A MODNAMECHOICES "$unused"
set -A BDNAMECHOICES 
set -A DEVNAMECHOICES 

set -A IHVDISKETTE
set -A IHVMODULES
set -A OLDLINETOBD
set -A OLDBOARDFLD
HBADISKETTE=0
CNFGWAITWIN=
CNFGBDFID=
LDHBAWAITWIN=
DCUCNFGIHV=
RSMGR_UPDATE=
BOARDXPND=
CONFBDTABLE_WIDTH=
SDVCNFG=""
SDV=""
SDVMODNAME=""
SDVIRQCHOICES=""
SDVPORTSCHOICES=""
SDVPORTECHOICES=""
SDVADDRSCHOICES=""
SDVADDRECHOICES=""
SDVPORTS=""
SDVADDRS=""
SDVSTATIC=""

IRQCHKD=""
IOSCHKD=""
IOECHKD=""
MEMSCHKD=""
MEMECHKD=""
DMACHKD=""

RMIOCHKD=""
RMMEMCHKD=""

DCU_HALT=N
DCU_ENTRY="ENTRY"
DCU_EXIT="EXIT"
DCU_EXIT_CODE=0
DCU_CONF_TYPE="$DCU_ENTRY"
APPLY_CONF=""
EXIT_CONF=""
ADD_KEYS=""
DEL_KEYS=""

NETCAT=0
HOSTCAT=0
COMMCAT=0
VIDEOCAT=0
SOUNDCAT=0
MISCCAT=0

DCU_REBOOT="REBOOT"
DCU_REBUILD="REBUILD"
DCU_MAX_WIDTH=78
DCU_MAX_HEIGHT=21

ADD_DEL_CHG=1
NAME_CHG=2
IRQ_CHG=4
IOS_CHG=8
IOE_CHG=16
MEMS_CHG=32
MEME_CHG=64
DMA_CHG=128
IPL_CHG=256
ITYPE_CHG=512
CPU_CHG=1024
BSP_CHG=2048
UNIT_CHG=4096

setpltfm
driversinit
boardsinit
return 1
}

function rsrcinit
{
# rsrcinit()
# Called when the DCU is invoked to open the resmgr interface.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

RMopen 2
if [ ! $? = 0 ]
then
	footer ""
	display -w "$RM_OPEN_RDONLY" -bg $RED -fg $WHITE
	call proc_loop

	RMopen 0
	if [ ! $? = 0 ]
	then
		footer ""
		display -w "$RM_OPEN_ERROR" -bg $RED -fg $WHITE
		call proc_loop
		return 0
	fi
fi
RMnextkey 0
RMKEY=$?
return 1
}

function driversinit
{
# driversinit()
# Called once at startup from dcuinit to
# initialize the driver array elements using resmgr info.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

typeset i dn sn
DRVINDX=1
for i in ${ROOT}/etc/conf/sdevice.d/*
do
	sn=${i#*sdevice.d/}
	dn=${ROOT}/etc/conf/drvmap.d/${sn}
	if [ -s $dn ]
	then
		readdrvmap ${sn} $dn
	else
		if isdriver ${ROOT}/etc/conf/mdevice.d/${sn} 
		then
			readsdevice ${sn} $i $DRVINDX
			ALLDRIVERS="$ALLDRIVERS $DRVINDX"
			let DRVINDX+=1
		fi
	fi
done
}

function boardsinit
{
# boardsinit()
# Called once at startup from dcuinit to
# initialize the board array elements using resmgr info.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

integer brd drv i ok
LASTBOARD=1
integer curkey=$RMKEY
integer inst=0
BDCNFGCHOICES="$Y $N"

#get each board record from resmgr
while true
do

	ok=1
	rdRM_key $curkey $LASTBOARD $inst


        if [ $? = 2 ]
	then
		ok=0
	fi

	if [ $ok = 1 ]; then
		if grep "^\$interface psm" ${ROOT}/etc/conf/mdevice.d/${BDMODNAME[LASTBOARD]} 2> /dev/null > /dev/null
		then
			BDINTERPSM[LASTBOARD]="Y"
		fi
	fi

	RMnextkey $curkey
	if (( curkey=$? ))
	then
		if [ $ok = 1 ]
		then
			let LASTBOARD+=1
		fi
		RMKEY=$curkey
		inst=0
	else
		break
	fi
done

if [ "$DCUMAP" = Y ] 
then
	typeset id_matched

	brd=1
	while (( $brd <= $LASTBOARD ))
	do
		idmatched="N"

		if [ ! "${BDMODNAME[brd]}" = "$dash" ]
		then
			let brd+=1
			continue
		fi

		drv=1
		while (( $drv <= $DRVINDX ))
		do
			case "${BDID[brd]}" in
			${DRVBOARDID[drv]}) \
				if [ "${BDBUSTYPE[brd]}" = "${DRVBUSTYPE[drv]}" ]
				then
					if [ "${DRVVERIFY[drv]}" = V ]
					then
						silent_verify $brd $drv
						if [ $? -ne 0 ]
						then
							let drv+=1
							continue
						fi
					fi
					BDNAME[brd]="${DRVBOARDNAME[drv]}"
					BDMODNAME[brd]="${MODNAME[drv]}"
					silent_match $brd $drv
					if [ "${DRVSTATIC[drv]}" = "Y" ]
					then
					    if [ "${DRVAUTOCONFIG[drv]}" = "Y" ]
					    then
							dcu_action $DCU_REBOOT
					    else
							dcu_action $DCU_REBUILD
					    fi
					else
						if [ ! -f ${ROOT}/etc/conf/mod.d/${BDMODNAME[brd]} ]
						then
							addloadmods ${BDMODNAME[brd]}
						fi
					fi
					let drv=DRVINDX
					idmatched="Y"
				fi
				;;
			esac
			let drv+=1
		done

		drv=1
		[ ${idmatched} = "N" ] && while (( $drv <= $DRVINDX ))
		do
			case "CLASS${BDCLASS[brd]}" in
			${DRVBOARDID[drv]}) \
				if [ "${BDBUSTYPE[brd]}" = "${DRVBUSTYPE[drv]}" ]
				then
					if [ "${DRVVERIFY[drv]}" = V ]
					then
						silent_verify $brd $drv
						if [ $? -ne 0 ]
						then
							let drv+=1
							continue
						fi
					fi
					BDNAME[brd]="${DRVBOARDNAME[drv]}"
					BDMODNAME[brd]="${MODNAME[drv]}"
					silent_match $brd $drv
					if [ "${DRVSTATIC[drv]}" = "Y" ]
					then
					    if [ "${DRVAUTOCONFIG[drv]}" = "Y" ]
					    then
							dcu_action $DCU_REBOOT
					    else
							dcu_action $DCU_REBUILD
					    fi
					else
						if [ ! -f ${ROOT}/etc/conf/mod.d/${BDMODNAME[brd]} ]
						then
							addloadmods ${BDMODNAME[brd]}
						fi
					fi
					let drv=DRVINDX
				fi
				;;
			esac
			let drv+=1
		done
	
		let brd+=1
	done
fi


#init first free board array element
NXTFREE=0
let LASTBOARD+=1
pushfree $LASTBOARD
let LASTBOARD+=1
#configure dummy end of list record
setlastboard $LASTBOARD

if [ "$UNIX_INSTALL" = "Y" ] \
&& [ "$DCUMAP" = "Y" ]
then
	# then determine if first serial port is configured
	if /usr/sbin/check_devs -s /dev/tty00
	then
		:
	else
		brd=1
		while (( $brd <= $LASTBOARD ))
		do
			if [ "${BDMODNAME[brd]}" = "asyc" ] \
			&& [ "${BDPORTSTART[brd]}" = "3f8" ]
			then
				RMdelkey ${BDKEY[brd]}
				BDMODNAME[brd]=$dash
				BDNAME[brd]=$dash
				BDCNFG[brd]="$N"
				BDTYPE[brd]=$unused
				break
			fi
			let brd+=1
		done
	fi
	
	# now determine if second serial port is configured
	if /usr/sbin/check_devs -s /dev/tty01
	then
		:
	else
		brd=1
		while (( $brd <= $LASTBOARD ))
		do
			if [ "${BDMODNAME[brd]}" = "asyc" ] \
			&& [ "${BDPORTSTART[brd]}" = "2f8" ]
			then
				RMdelkey ${BDKEY[brd]}
				BDMODNAME[brd]=$dash
				BDNAME[brd]=$dash
				BDCNFG[brd]="$N"
				BDTYPE[brd]=$unused
				break
			fi
			let brd+=1
		done
	fi
	
	# get parallel port I/O addr from check_devs. 
	typeset Sio Eio Irq
	/usr/sbin/check_devs -p

	case $? in
	1)
		Sio=3BC
		Eio=3BF
		Irq=7
		;;
	2)
		Sio=378
		Eio=37F
		Irq=7
		;;
	3)
		Sio=278
		Eio=27F
		Irq=7
		;;
	*)
		Sio=0
		Eio=0
		;;
	esac

	brd=1

	while (( $brd <= $LASTBOARD ))
	do
		if [ "${BDMODNAME[brd]}" = "mfpd" ]
		then
			if [ "$Sio" != "0" ]
			then
				RMdelvals ${BDKEY[brd]} "IOADDR IRQ"
				RMputvals ${BDKEY[brd]} "IOADDR IRQ" "$Sio $Eio $Irq"
				BDPORTSTART[brd]="$Sio"
				BDPORTEND[brd]="$Eio"
				BDIRQ[brd]="$Irq"
			else
				RMdelkey ${BDKEY[brd]}
				BDMODNAME[brd]=$dash
				BDNAME[brd]=$dash
				BDCNFG[brd]="$N"
				BDTYPE[brd]=$unused
			fi
			break
		fi

		let brd+=1
	done
fi
RMentryconf
}

function popfree
{
# popfree()
# Called to pop a boards array element.
# It will grow boards array if free stack is empty.
# Calling/Exit State: next bd_array_index.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
integer i nxt=$NXTFREE

BDTYPE[$NXTFREE]=$unused
BDID[$NXTFREE]=''
BDCLASS[$NXTFREE]=''
BDIRQ[$NXTFREE]=''
BDPORTSTART[$NXTFREE]=''
BDPORTEND[$NXTFREE]=''
BDADDRSTART[$NXTFREE]=''
BDADDREND[$NXTFREE]=''
BDDMA[$NXTFREE]=''

BDKEY[$NXTFREE]=''
BDCHGS[$NXTFREE]=''
BDCNFG[$NXTFREE]=''
BDNAME[$NXTFREE]=''
BDMODNAME[$NXTFREE]=''
BDMODINDX[$NXTFREE]=''
BDID[$NXTFREE]=''
BDIRQ[$NXTFREE]=''
BDBUSTYPE[$NXTFREE]=''

NXTFREE="${BDLNK[$NXTFREE]}"
if (( !NXTFREE ))
then
	let i=LASTBOARD+1
	pushfree $i
	let i=LASTBOARD+2
	setlastboard $i
fi
return $nxt
}

function pushfree
{
# pushfree(bd_array_index)
# Called to add to the free boards array stack.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

BDLNK[$1]=$NXTFREE
NXTFREE=$1
BDTYPE[$1]=$unused
}

function setlastboard
{
# setlastboard(bd_array_indx)
# Called to assign last boardarray element.
# Calling/Exit State: void.
[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

BDTYPE[$LASTBOARD]=$unused
let LASTBOARD=$1
mkboard $LASTBOARD
BDTYPE[$LASTBOARD]=$none

}

function lookupdrv
{
# lookupdrv(bd_array_index)
# Called to verify that the board array index
# in question has a corresponding driver index.
# Calling/Exit State: 0 for failure and non-zero DRV index for success.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

integer rc=0 i=1
FNDDRVBOARDNAME=""
FNDMODNAME=""
while (( $i < $DRVINDX ))
do
 	if [ "${BDMODNAME[$1]}" = "${MODNAME[i]}" ]
	then
		FNDMODNAME="${MODNAME[i]}"
 		if [ "${MODNAME[i]}" = "${MODNAME[i-1]}" ] \
 		|| [ "${MODNAME[i]}" = "${MODNAME[i+1]}" ]
		then
			case ${DRVBUSTYPE[i]} in
            $isa|$unknown|"")
				if [ "$(cmd grep -c "^|ISA" /etc/conf/drvmap.d/${MODNAME[i]})" = "1" ]
				then
                	FNDDRVBOARDNAME="${DRVBOARDNAME[i]}"
				else
                	FNDDRVBOARDNAME="${DRVBRAND[i]}"
                	DRVBOARDNAMEFULL[i]="${DRVBRAND[i]}"

					if [ ${#FNDDRVBOARDNAME} -gt 20 ]
					then
						typeset -L20 trunc_bdname="${FNDDRVBOARDNAME}"
						FNDDRVBOARDNAME="$trunc_bdname"
					fi
				fi

                rc=$i
                break
				;;
            *)
                FNDDRVBOARDNAME="${MODNAME[i]}"
                ;;
            esac
		else
			FNDDRVBOARDNAME="${DRVBOARDNAME[i]}"
			if [ -n "${DRVBUSTYPE[i]}" ]
			then
				BDBUSTYPE[$1]="${DRVBUSTYPE[i]}"
			fi
			rc=$i
			break
		fi
		if [ "${BDID[$1]}" = "${DRVBOARDID[i]}" ]
		then
			FNDDRVBOARDNAME="${DRVBOARDNAME[i]}"
			if [ -n "${DRVBUSTYPE[i]}" ]
			then
				BDBUSTYPE[$1]="${DRVBUSTYPE[i]}"
			fi
			rc=$i
			break
		fi
	fi
	let i+=1
done
return $rc
}

function lookupisa
{
# lookupisa(bd_array_index)
# Called to get the first non-PCU instance in drvmap.d file.
# Calling/Exit State: 0 for failure and non-zero DRV index for success.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

integer rc=0 i=1
while (( $i < $DRVINDX ))
do
 	if [ "${BDMODNAME[$1]}" = "${MODNAME[i]}" ]
	then
		if [ -z "${DRVBUSTYPE[i]}" -o "${DRVBUSTYPE[i]}" = "$isa" ]
		then
			rc=$i
			break
		fi
	fi
	let i+=1
done
return $rc
}

function readdrvmap
{
# readdrvmap(MODNAME, drvmap_filename)
# Called to read drvmap file for hardware device drivers
# to initialize variables.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
integer drv=$DRVINDX
typeset ifs=$IFS
typeset bdid bustype drvname bdname bcfgfile autoconfig category verify brand
typeset mn dpath
typeset throaway
typeset -L20 trunc_bdname

dpath=${2%/*/*}
mn=sdevice.d/$1
[ ! -d ${dpath}/sdevice.d ] && mn=${1}/System
{
IFS="$PIPE"
while read drvname autoconfig verify category brand
do
	if [ ! "$drvname" = "$1" ]
	then
		continue
	fi
	while read throaway bustype bdid bdname bcfgfile
	do
		IFS=$ifs
		readsdevice $1 ${dpath}/$mn $drv
		DRVBOARDID[drv]=$bdid
		typeset -u bustype
		DRVBUSTYPE[drv]=$bustype
		MODNAME[drv]=$drvname
		DRVBOARDNAMEFULL[drv]="$bdname"
		integer Length=${#bdname}
		if [ Length -gt 20 ]
		then
			trunc_bdname="$bdname"
			DRVBOARDNAME[drv]="$trunc_bdname"
		else
			DRVBOARDNAME[drv]="$bdname"
		fi
		case "${category}" in
			"Network Interface Cards") \
				let NETCAT+=1
				DRVCATEGORY[drv]="$network_interface_cards"
				;;
			"Host Bus Adapters") \
				let HOSTCAT+=1 
				DRVCATEGORY[drv]="$host_bus_adapters"
				;;
			"Communications Cards") \
				let COMMCAT+=1
				DRVCATEGORY[drv]="$communications_cards"
				;;
			"Video Cards") \
				let VIDEOCAT+=1
				DRVCATEGORY[drv]="$video_cards"
				;;
			"Sound Boards") \
				let SOUNDCAT+=1
				DRVCATEGORY[drv]="$sound_boards"
				;;
			*)	let MISCCAT+=1
				DRVCATEGORY[drv]="$miscellaneous"
				;;
		esac
		DRVVERIFY[drv]="$verify"
		DRVAUTOCONFIG[drv]=$autoconfig
		DRVBRAND[drv]=$brand
		ALLDRIVERS="$ALLDRIVERS $drv"
		DRVBCFGFILE[drv]=""
		let drv+=1
		IFS="$PIPE"
	done 
done 
}<$2 
IFS=$ifs
DRVINDX=$drv
}

function readsdevice
{
# readsdevice(MODNAME, sdevice_filename, drv_array_index)
# Called to read sdevice file for hardware device drivers
# to initialize variables.
# Calling/Exit State: void.
# ALLDRIVERS variable is expanded and DRVINDX variable is incremented.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
integer drv=$3
typeset drvname isc
typeset -R8 name8
typeset -R8 isc8
typeset -R8 unit
typeset -R8 ipl
typeset -R8 itype
typeset -R8 irq
typeset -R8 ports
typeset -R8 porte
typeset -R8 mems
typeset -R8 meme
typeset -R8 dma
typeset -R8 cpu
integer t=0

if [  ! "${1}" = "$SDV" ]
then
	SDVCNFG=N
	SDV=$1
	SDVSTATIC=""
	SDVMODNAME=$1
	SDVMODCHOICES=""
	SDVISCCHOICES=""
	SDVUNITCHOICES=""
	SDVIPLCHOICES=""
	SDVITYPECHOICES=""
	SDVIRQCHOICES=""
	SDVPORTSCHOICES=""
	SDVPORTECHOICES=""
	SDVADDRSCHOICES=""
	SDVADDRECHOICES=""
	SDVDMACHOICES=""
	SDVCPUCHOICES=""
	SDVPORTS=""
	SDVADDRS=""
	while read drvname isc unit ipl itype irq ports porte mems meme dma cpu
	do
		if [ "$drvname" = "\$static" ]
		then
			SDVSTATIC="Y"
			continue
		fi
		if [ ! "$drvname" = "$1" ]
		then
			continue
		fi
		if [ "$isc" = Y ]
		then
			SDVCNFG=Y
			let t+=1
		fi
		name8="$drvname"
		if [ "$isc" = Y ]
		then
			isc8="$Y"
		else
			isc8="$N"
		fi
			
		SDVMODCHOICES="$SDVMODCHOICES	$name8"
		SDVISCCHOICES="$SDVISCCHOICES	$isc8"
		SDVUNITCHOICES="$SDVUNITCHOICES	$unit"
		SDVIPLCHOICES="$SDVIPLCHOICES	$ipl"
		SDVITYPECHOICES="$SDVITYPECHOICES	$itype"
		SDVIRQCHOICES="$SDVIRQCHOICES	$irq"
		SDVPORTSCHOICES="$SDVPORTSCHOICES	$ports"
		SDVPORTECHOICES="$SDVPORTECHOICES	$porte"
		SDVADDRSCHOICES="$SDVADDRSCHOICES	$mems"
		SDVADDRECHOICES="$SDVADDRECHOICES	$meme"
		SDVDMACHOICES="$SDVDMACHOICES	$dma"
		SDVCPUCHOICES="$SDVCPUCHOICES	$cpu"
	done <$2
	eval "$1"=\$t
	MODNAMECHOICES="$SDVMODNAME $MODNAMECHOICES"
fi
DRVSTATIC[drv]="$SDVSTATIC"
DRVCNFG[drv]="$SDVCNFG"
MODNAME[drv]="$SDVMODNAME"
DRVMODCHOICES[drv]="$SDVMODCHOICES"
DRVISCCHOICES[drv]="$SDVISCCHOICES"
DRVUNITCHOICES[drv]="$SDVUNITCHOICES"
DRVIPLCHOICES[drv]="$SDVIPLCHOICES"
DRVITYPECHOICES[drv]="$SDVITYPECHOICES"
DRVIRQCHOICES[drv]="$SDVIRQCHOICES"
DRVPORTSCHOICES[drv]="$SDVPORTSCHOICES"
DRVPORTECHOICES[drv]="$SDVPORTECHOICES"
DRVMEMSCHOICES[drv]="$SDVADDRSCHOICES"
DRVMEMECHOICES[drv]="$SDVADDRECHOICES"
DRVDMACHOICES[drv]="$SDVDMACHOICES"
DRVCPUCHOICES[drv]="$SDVCPUCHOICES"
DRVBOARDID[drv]="$SDVMODNAME"
DRVBOARDNAME[drv]="$SDVMODNAME"
DRVAUTOCONFIG[drv]=N
DRVVERIFY[drv]="N"
if [ ! -f ${ROOT}/etc/conf/drvmap.d/$1 ]
then
	DRVCATEGORY[drv]="$miscellaneous"
	let MISCCAT+=1
fi
DRVBUSTYPE[drv]="$unknown"
}

function mkboard
{
# mkboard(bd_array_index)
# Called from the setlastboard function
# to initialize variables for designated board.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

BDTYPE[$1]=$unused
BDCNFG[$1]=''
BDNAME[$1]=''
BDID[$1]=''
BDCLASS[$1]=''
BDIRQ[$1]=''
BDPORTSTART[$1]=''
BDPORTEND[$1]=''
BDADDRSTART[$1]=''
BDADDREND[$1]=''
BDDMA[$1]=''

}

function mkdriver
{
# mkdriver(drivername, drv_array_index) NOT_CURRENTLY_USED
# Called from the ? function
# to initialize variables for designated drivers.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

integer drv=$2
MODNAME[drv]="$1"
DRVBOARDNAME[drv]="$1"
DRVCNFG[drv]="N"
DRVIPLCHOICES[drv]=""
DRVITYPECHOICES[drv]=""
DRVIRQCHOICES[drv]=""
DRVPORTSCHOICES[drv]=""
DRVPORTECHOICES[drv]=""
DRVMEMSCHOICES[drv]=""
DRVMEMECHOICES[drv]=""
DRVDMACHOICES[drv]=""
DRVBOARDID[drv]="$1"
DRVAUTOCONFIG[drv]=N
DRVBUSTYPE[drv]=$unknown
}

function isdriver
{
# isdriver(mdevice_filename)
# Called to read the mdevice file is for a device driver
# checking for an 'h' or 'H' hardware flag to indicate that it is a
# driver for the DCU.
# Calling/Exit State: 0 for success and 1 for failure.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
typeset -u drvname pre char ord bmaj cmaj
typeset x w

w=${1#*mdevice.d/}
grep ^$w $1 2> /dev/null | read drvname pre char ord bmaj cmaj 
case "$char" in
*h*|*H*)
	return 0
	;;
*)
	return 1
	;;
esac
}

function choose_name
{
# choose_name(bd_array_index)
# Called when the Choices key for the Device Name field is selected
# to display the Choices.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

integer i=0 
typeset m

unset DEVNAMECHOICES
DEVNAMECHOICES[i]="${BDNAME[$1]}"
let i+=1

for m in $MODNAMECHOICES
do
	DEVNAMECHOICES[i]="$m"
	let i+=1
done

CHOOSE_TITLE="$boardname $CHOICES [${BDNAME[$1]}]"
CHOOSE_FOOTER="$DCU_CHOOSE_FOOTER"
choose "${BDNAME[$1]}" "${DEVNAMECHOICES[@]}"
}

function check_name
{
# check_name(bd_array_index)
# Called when the Device Name field is changed to perform validation checking.
# Calling/Exit State: 0 for success and 1 for failure.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
if (( !FLD_CHANGED ))
then
	return 0
fi

typeset m 
for m in $MODNAMECHOICES
do
	if [ "$m" = "${BDNAME[$1]}" ]
	then
		BDMODNAME[$1]="${BDNAME[$1]}"
		if [ "${BDNAME[$1]}" != "$unused" ]
		then
			set_adv_params $1
		fi
		error_pending=''
		integer x=${BDCHGS[$1]}
		let x\|=$NAME_CHG
		BDCHGS[$1]=$x
		return 0
	fi
done
msgnoerr "$BD_BAD_NAME"
return 1
}

function choose_unit
{
# choose_unit(bd_array_index)
# Called when the Choices key for the UNIT field is selected
# to display the GENERIC_NO_CHOICES message.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
msgnoerr "$GENERIC_NO_CHOICES"
}

function check_unit
{
# check_unit(bd_array_index)
# Called when the UNIT field is changed to perform validation checking.
# Calling/Exit State: 0 for success and 1 for failure.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

if (( !FLD_CHANGED ))
then
	return 0
fi

case "${BDUNIT[$1]}" in
-[0-9]*([0-9]))
	error_pending=''
	integer x=${BDCHGS[$1]}
	let x\|=$UNIT_CHG
	BDCHGS[$1]=$x
	return 0
	;;
[0-9]*([0-9]))
	error_pending=''
	integer x=${BDCHGS[$1]}
	let x\|=$UNIT_CHG
	BDCHGS[$1]=$x
	return 0
	;;
*)
	msgnoerr "$BD_BAD_UNIT"
	return 1
esac
}


function choose_ipl
{
# choose_ipl(bd_array_index)
# Called when the Choices key for the IPL field is selected
# to display the Choices.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
CHOOSE_TITLE="$boardipl $CHOICES"
CHOOSE_FOOTER="$DCU_CHOOSE_FOOTER"
choose "${BDIPL[$1]}" $IPLCHOICES
}

function check_ipl
{
# check_ipl(bd_array_index)
# Called when the IPL field is changed to perform validation checking.
# Calling/Exit State: 0 for success and 1 for failure.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

if (( !FLD_CHANGED ))
then
	return 0
fi

case "${BDIPL[$1]}" in
[0-9])
	if [ ${BDIPL[$1]} -lt 0 -o ${BDIPL[$1]} -gt 9 ]
	then
		msgnoerr "$BD_BAD_IPL"
		return 1
	else
		error_pending=''
		integer x=${BDCHGS[$1]}
		let x\|=$IPL_CHG
		BDCHGS[$1]=$x
		return 0
	fi
	;;
*)
	msgnoerr "$BD_BAD_IPL"
	return 1
esac
}

function choose_itype
{
# choose_itype(bd_array_index)
# Called when the Choices key for the ITYPE field is selected
# to display the GENERIC_NO_CHOICES message.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
CHOOSE_TITLE="$boarditype $CHOICES"
CHOOSE_FOOTER="$DCU_CHOOSE_FOOTER"
choose "${BDITYPE[$1]}" $ITYPECHOICES
}

function check_itype
{
# check_itype(bd_array_index)
# Called when the ITYPE field is changed to perform validation checking.
# Calling/Exit State: 0 for success and 1 for failure.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

if (( !FLD_CHANGED ))
then
	return 0
fi

case "${BDITYPE[$1]}" in
[0-4])
	if [ ${BDITYPE[$1]} -lt 0 -o ${BDITYPE[$1]} -gt 4 ]
	then
		msgnoerr "$BD_BAD_ITYPE"
		return 1
	else
		error_pending=''
		integer x=${BDCHGS[$1]}
		let x\|=$ITYPE_CHG
		BDCHGS[$1]=$x
		return 0
	fi
	;;
*)
	msgnoerr "$BD_BAD_ITYPE"
	return 1
esac
}

function choose_irq
{
# choose_irq(bd_array_index)
# Called when the Choices key for the IRQ field is selected
# to display the GENERIC_NO_CHOICES message.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
msgnoerr "$GENERIC_NO_CHOICES"
}

function check_irq
{
# check_irq(bd_array_index)
# Called when the IRQ field is changed to perform validation checking.
# Calling/Exit State: 0 for success and 1 for failure.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

if (( !FLD_CHANGED ))
then
	IRQCHKD=""
	return 0
fi

if [ "$IRQCHKD" = "${BDIRQ[$1]}" ]
then
	IRQCHKD=""
	return 0
fi


case "${BDIRQ[$1]}" in
-)
	error_pending=''
	integer x=${BDCHGS[$1]}
	let x\|=$IRQ_CHG
	BDCHGS[$1]=$x
	IRQCHKD=""
	return 0
	;;
[0-9]*([0-9]))
	
	if [ ${BDIRQ[$1]} -lt 0 -o ${BDIRQ[$1]} -gt 15 ]
	then
		msgnoerr "$BD_BAD_IRQ"
		return 1
	else
		error_pending=''
		integer x=${BDCHGS[$1]}
		let x\|=$IRQ_CHG
		BDCHGS[$1]=$x
		irq_conflict $1
		if [ "$?" = 0 ]
		then
			IRQCHKD=""
		fi
		return 0
	fi
	;;
*)
	msgnoerr "$BD_BAD_IRQ"
	return 1
	;;
esac
}

function irq_conflict
{
# irq_conflict(bd_array_index)
# Called when the IRQ field is changed to perform conflict checking.
# A special flag is set to allow the field to be exited
# if a conflict is displayed.
# Calling/Exit State: 0 for success and 1 for failure.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

if [ "${BDCNFG[$1]}" = "$N" ] \
|| [ -z "${BDNAME[$1]}" ] \
|| [ "${BDIRQ[$1]}" = "$dash" ] \
|| [ "${BDIRQ[$1]}" = "0" ] \
|| [ "${BDNAME[$1]}" = "$dash" ] \
|| [ "${BDNAME[$1]}" = "$unused" ] \
|| [ "${BDNAME[$1]}" = "$unknown" ] 
then
	return 0
fi

typeset MESSAGE=""
typeset brd=1
while (( $brd <= $LASTBOARD ))
do
	if [ "$brd" = "$1" ] \
	|| [ ! "${BDIRQ[brd]}" = "${BDIRQ[$1]}" ] \
	|| [ "${BDCNFG[brd]}" = "$N" ] \
	|| [ -z "${BDNAME[brd]}" ] \
	|| [ "${BDIRQ[brd]}" = "$dash" ] \
	|| [ "${BDNAME[brd]}" = "$dash" ] \
	|| [ "${BDNAME[brd]}" = "$unused" ] \
	|| [ "${BDNAME[brd]}" = "$unknown" ] 
	then
		let brd+=1
		continue
	fi
	if [ "${BDINTERPSM[$1]}" = "Y" ] \
	&& [ "${BDINTERPSM[brd]}" = "Y" ]
	then
		let brd+=1
		continue
	fi

	MESSAGE=""
	if [ "${BDITYPE[brd]}" = 1 ] || [ "${BDITYPE[$1]}" = 1 ] 
	then
		MESSAGE="$IRQNOSHARE"
		break
	fi

	if [ "${BDITYPE[brd]}" = 2 ] && [ "${BDITYPE[$1]}" = 2 ] \
	&& [ ! "${BDMODNAME[brd]}" = "${BDMODNAME[$1]}" ]
	then
		MESSAGE="$IRQSELFSHARE"
		break
	fi

	if [ "${BDITYPE[brd]}" = 3 ] && [ "${BDITYPE[$1]}" = 4 ]
	then
		MESSAGE="$IRQNOSHARE"
		break
	fi

	if [ "${BDITYPE[brd]}" = 4 ] && [ "${BDITYPE[$1]}" = 3 ]
	then
		MESSAGE="$IRQNOSHARE"
		break
	fi

	let brd+=1
done

if [ -n "$MESSAGE" ]
then
	IRQCHKD="${BDIRQ[$1]}"
	display  -w "ERROR:
$boardname=${BDNAME[$1]}, $boardipl=${BDIPL[$1]}, $boarditype=${BDITYPE[$1]}, $boardirq=${BDIRQ[$1]}.
${IM_A_BLANK_LINE_DONT_TOUCH}
$MESSAGE
${IM_A_BLANK_LINE_DONT_TOUCH}
$boardname=${BDNAME[brd]}, $boardipl=${BDIPL[brd]}, $boarditype=${BDITYPE[brd]}, $boardirq=${BDIRQ[brd]}." \
		-bg $RED -fg $WHITE
	footer $GENERIC_CONTINUE_FOOTER
	return 1
fi
return 0
}

function choose_ports
{
# choose_ports(bd_array_index)
# Called when the Choices key for the IOStart field is selected
# to display the GENERIC_NO_CHOICES message.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
msgnoerr "$GENERIC_NO_CHOICES"
}

function check_ports
{
# check_ports(bd_array_index)
# Called when the IOStart field is changed to perform validation checking.
# Calling/Exit State: 0 for success and 1 for failure.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

if (( !FLD_CHANGED ))
then
        IOSCHKD=""
	return 0
fi

if [ "$IOSCHKD" = "${BDPORTSTART[$1]}" ]
then
        IOSCHKD=""
        return 0
fi

case "${BDPORTSTART[$1]}" in
"-")
	integer x=${BDCHGS[$1]}
	let x\|=$IOS_CHG
	BDCHGS[$1]=$x
	error_pending=''
        IOSCHKD=""
	return 0
	;;
[0-9abcdefABCDEF]*([0-9abcdefABCDEF]))
	integer x=${BDCHGS[$1]}
	let x\|=$IOS_CHG
	BDCHGS[$1]=$x
	error_pending=''
	ports_conflict $1
	if [ "$?" = 0 ]
	then
        	IOSCHKD=""
	fi
	return 0
	;;
*)
	msgnoerr "$BD_BAD_PORT"
	return 1
	;;
esac
}

function ports_conflict
{
# ports_conflict(bd_array_index)
# Called when the IOStart field is changed to perform conflict checking.
# A special flag is set to allow the field to be exited
# if a conflict is displayed.
# Calling/Exit State: 0 for success and 1 for failure.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

if [ "${BDCNFG[$1]}" = "$N" ] \
|| [ -z "${BDNAME[$1]}" ] \
|| [ "${BDNAME[$1]}" = "$dash" ] \
|| [ "${BDNAME[$1]}" = "$unused" ] \
|| [ "${BDNAME[$1]}" = "$unknown" ] \
|| [ "${BDPORTSTART[$1]}" = "$dash" ] \
|| [ "${BDPORTSTART[$1]}" = "0" -a "${BDPORTEND[$1]}" = "0" ]
then
	return 0
fi

typeset x ports ios ioe
x=${BDPORTSTART[$1]}
typeset -i16 ports=16#$x

typeset brd=1
while (( $brd <= $LASTBOARD ))
do
	if [ "$brd" = "$1" ] \
	|| [ "${BDCNFG[brd]}" = "$N" ] \
	|| [ -z "${BDNAME[brd]}" ] \
	|| [ "${BDPORTSTART[brd]}" = "$dash" ] \
	|| [ "${BDNAME[brd]}" = "$dash" ] \
	|| [ "${BDNAME[brd]}" = "$unused" ] \
	|| [ "${BDMODNAME[brd]}" = "$unknown" ] 
	then
		let brd+=1
		continue
	fi

	if [ "${BDINTERPSM[$1]}" = "Y" ] \
	&& [ "${BDINTERPSM[brd]}" = "Y" ]
	then
		let brd+=1
		continue
	fi

	if [ "${BDPORTSTART[brd]}" = "$dash" ] \
	|| [ "${BDPORTEND[brd]}" = "$dash" ]
	then
		let brd+=1
		continue
	fi

	if [ "${BDPORTSTART[brd]}" = "0" ] \
	&& [ "${BDPORTEND[brd]}" = "0" ]
	then
		let brd+=1
		continue
	fi

	x=${BDPORTSTART[brd]}
	typeset -i16 ios=16#$x
	x=${BDPORTEND[brd]}
	typeset -i16 ioe=16#$x

	if [ ports -ge ios -a ports -le ioe ]
	then
		typeset allow1 allow2
		allow_conflict ${ROOT}/etc/conf/mdevice.d/${BDMODNAME[$1]} O
		allow1=$?
		allow_conflict ${ROOT}/etc/conf/mdevice.d/${BDMODNAME[brd]} O
		allow2=$?
		if (( allow1 && allow2 ))
		then
			let brd+=1
			continue
		fi
        	IOSCHKD="${BDPORTSTART[$1]}"
		display  -w "ERROR:
$boardname=${BDNAME[$1]}, $boardios=${BDPORTSTART[$1]}, $boardioe=${BDPORTEND[$1]}.
${IM_A_BLANK_LINE_DONT_TOUCH}
$IOSCONF
${IM_A_BLANK_LINE_DONT_TOUCH}
$boardname=${BDNAME[brd]}, $boardios=${BDPORTSTART[brd]}, $boardioe=${BDPORTEND[brd]}." \
			-bg $RED -fg $WHITE
		footer $GENERIC_CONTINUE_FOOTER
		return 1
	fi
	let brd+=1
done
return 0
}

function choose_porte
{
# choose_porte(bd_array_index)
# Called when the Choices key for the IOEnd field is selected
# to display the GENERIC_NO_CHOICES message.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
msgnoerr "$GENERIC_NO_CHOICES"
}

function check_porte
{
# check_porte(bd_array_index)
# Called when the IOEnd field is changed to perform validation checking.
# Calling/Exit State: 0 for success and 1 for failure.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

if [ ! "${BDPORTSTART[$1]}" = "$dash" ] \
&& [   "${BDPORTEND[$1]}"   = "$dash" ]
then
	if [ "$IOECHKD" = "${BDPORTEND[$1]}" ]
	then
	        IOECHKD=""
	        return 0
	fi
       	IOECHKD="${BDPORTEND[$1]}"
	display  -w "ERROR:
$boardname=${BDNAME[$1]}, $boardios=${BDPORTSTART[$1]}, $boardioe=${BDPORTEND[$1]}.
${IM_A_BLANK_LINE_DONT_TOUCH}
$IOEREQ" \
		-bg $RED -fg $WHITE
	footer $GENERIC_CONTINUE_FOOTER
	return 1
fi

if [   "${BDPORTSTART[$1]}" = "$dash" ] \
&& [ ! "${BDPORTEND[$1]}"   = "$dash" ]
then
	if [ "$IOECHKD" = "${BDPORTEND[$1]}" ]
	then
	        IOECHKD=""
	        return 0
	fi
       	IOECHKD="${BDPORTEND[$1]}"
	display  -w "ERROR:
$boardname=${BDNAME[$1]}, $boardios=${BDPORTSTART[$1]}, $boardioe=${BDPORTEND[$1]}.
${IM_A_BLANK_LINE_DONT_TOUCH}
$IOSREQ" \
		-bg $RED -fg $WHITE
	footer $GENERIC_CONTINUE_FOOTER
	return 1
fi

if (( !FLD_CHANGED ))
then
        IOECHKD=""
	return 0
fi

if [ "$IOECHKD" = "${BDPORTEND[$1]}" ]
then
        IOECHKD=""
        return 0
fi

case "${BDPORTEND[$1]}" in
"-")
	error_pending=''
	integer x=${BDCHGS[$1]}
	let x\|=$IOE_CHG
	BDCHGS[$1]=$x
	IOECHKD=""
	return 0
	;;
[0-9abcdefABCDEF]*([0-9abcdefABCDEF]))
	error_pending=''
	integer x=${BDCHGS[$1]}
	let x\|=$IOE_CHG
	BDCHGS[$1]=$x
	porte_conflict $1
	if [ "$?" = 0 ]
	then
		IOECHKD=""
	fi
	return 0
	;;
*)
	msgnoerr "$BD_BAD_PORT"
	return 1
esac
}

function porte_conflict
{
# porte_conflict(bd_array_index)
# Called when the IOEnd field is changed to perform conflict checking.
# A special flag is set to allow the field to be exited
# if a conflict is displayed.
# Calling/Exit State: 0 for success and 1 for failure.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

if [ "${BDCNFG[$1]}" = "$N" ] \
|| [ -z "${BDNAME[$1]}" ] \
|| [ "${BDNAME[$1]}" = "$dash" ] \
|| [ "${BDNAME[$1]}" = "$unused" ] \
|| [ "${BDNAME[$1]}" = "$unknown" ]
then
	return 0
fi

[ "${BDPORTSTART[$1]}" = "$dash" -a "${BDPORTEND[$1]}" = "$dash" ] && return 0
[ "${BDPORTSTART[$1]}" = "0" -a "${BDPORTEND[$1]}" = "0" ] && return 0

typeset x ports porte
x=${BDPORTSTART[$1]}
typeset -i16 ports=16#$x
x=${BDPORTEND[$1]}
typeset -i16 porte=16#$x
typeset brd=1
if [ "${BDPORTSTART[$1]}" != "$dash" ] \
&& [ "${BDPORTEND[$1]}" != "$dash" ]
then
	if [ porte -lt ports ]
	then
		if [ "$IOECHKD" = "${BDPORTEND[$1]}" ]
		then
		        IOECHKD=""
		        return 0
		fi
	       	IOECHKD="${BDPORTEND[$1]}"
		display  -w "ERROR:
$boardname=${BDNAME[$1]}, $boardios=${BDPORTSTART[$1]}, $boardioe=${BDPORTEND[$1]}.
${IM_A_BLANK_LINE_DONT_TOUCH}
$BD_BAD_PORTE." \
			-bg $RED -fg $WHITE
		footer $GENERIC_CONTINUE_FOOTER
		return 1
	fi
fi

while (( $brd <= $LASTBOARD ))
do
	if [ "$brd" = "$1" ] \
	|| [ "${BDCNFG[brd]}" = "$N" ] \
	|| [ -z "${BDNAME[brd]}" ] \
	|| [ "${BDNAME[brd]}" = "$dash" ] \
	|| [ "${BDNAME[brd]}" = "$unused" ] \
	|| [ "${BDNAME[brd]}" = "$unknown" ]
	then
		let brd+=1
		continue
	fi

	if [ "${BDINTERPSM[$1]}" = "Y" ] \
	&& [ "${BDINTERPSM[brd]}" = "Y" ]
	then
		let brd+=1
		continue
	fi

	if [ "${BDPORTSTART[brd]}" = "$dash" ] \
	|| [ "${BDPORTEND[brd]}" = "$dash" ]
	then
		let brd+=1
		continue
	fi

	if [ "${BDPORTSTART[brd]}" = "0" ] \
	&& [ "${BDPORTEND[brd]}" = "0" ]
	then
		let brd+=1
		continue
	fi

	x=${BDPORTSTART[brd]}
	typeset -i16 ios=16#$x
	x=${BDPORTEND[brd]}
	typeset -i16 ioe=16#$x

	if [ porte -ge ios -a ports -le ioe ]
	then
		typeset allow1 allow2
		allow_conflict ${ROOT}/etc/conf/mdevice.d/${BDMODNAME[$1]} O
		allow1=$?
		allow_conflict ${ROOT}/etc/conf/mdevice.d/${BDMODNAME[brd]} O
		allow2=$?
		if (( allow1 && allow2 ))
		then
			let brd+=1
			continue
		fi
       		IOECHKD="${BDPORTEND[$1]}"
		display  -w "ERROR:
$boardname=${BDNAME[$1]}, $boardios=${BDPORTSTART[$1]}, $boardioe=${BDPORTEND[$1]}.
${IM_A_BLANK_LINE_DONT_TOUCH}
$IOECONF
${IM_A_BLANK_LINE_DONT_TOUCH}
$boardname=${BDNAME[brd]}, $boardios=${BDPORTSTART[brd]}, $boardioe=${BDPORTEND[brd]}." \
			-bg $RED -fg $WHITE
		footer $GENERIC_CONTINUE_FOOTER
		return 1
	fi
	let brd+=1
done
return 0
}

function choose_addrs
{
# choose_addrs(bd_array_index)
# Called when the Choices key for the MemStart field is selected
# to display the GENERIC_NO_CHOICES message.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
msgnoerr "$GENERIC_NO_CHOICES"
}

function check_addrs
{
# check_addrs(bd_array_index)
# Called when the MemEnd field is changed to perform validation checking.
# Calling/Exit State: 0 for success and 1 for failure.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
if (( !FLD_CHANGED ))
then
        MEMSCHKD=""
	return 0
fi

if [ "$MEMSCHKD" = "${BDADDRSTART[$1]}" ]
then
        MEMSCHKD=""
        return 0
fi

if [ -z "${BDADDRSTART[$1]}" ]
then
        MEMSCHKD=""
	return 0
fi

case "${BDADDRSTART[$1]}" in
"-")
	error_pending=''
	integer x=${BDCHGS[$1]}
	let x\|=$MEMS_CHG
	BDCHGS[$1]=$x
        MEMSCHKD=""
	return 0
	;;
[0-9abcdefABCDEF]*([0-9abcdefABCDEF]))
	error_pending=''
	integer x=${BDCHGS[$1]}
	let x\|=$MEMS_CHG
	BDCHGS[$1]=$x
	addrs_conflict $1
	if [ "$?" = 0 ]
	then
        	MEMSCHKD=""
	fi
	return 0
	;;
*)
	msgnoerr "$BD_BAD_MEM"
	return 1
esac
}

function addrs_conflict
{
# addrs_conflict(bd_array_index)
# Called when the MemStart field is changed to perform conflict checking.
# A special flag is set to allow the field to be exited
# if a conflict is displayed.
# Calling/Exit State: 0 for success and 1 for failure.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

if [ "${BDCNFG[$1]}" = "$N" ] \
|| [ -z "${BDNAME[$1]}" ] \
|| [ "${BDNAME[$1]}" = "$dash" ] \
|| [ "${BDNAME[$1]}" = "$unused" ] \
|| [ "${BDNAME[$1]}" = "$unknown" ] \
|| [ "${BDADDRSTART[$1]}" = "$dash" ] \
|| [ "${BDADDRSTART[$1]}" = "0" -a "${BDADDREND[$1]}" = "0" ]
then
	return 0
fi

typeset x addrs mems meme
x=${BDADDRSTART[$1]}
typeset -i16 addrs=16#$x

typeset brd=1
while (( $brd <= $LASTBOARD ))
do
	if [ "$brd" = "$1" ] \
	|| [ "${BDCNFG[brd]}" = "$N" ] \
	|| [ -z "${BDNAME[brd]}" ] \
	|| [ "${BDNAME[brd]}" = "$dash" ] \
	|| [ "${BDNAME[brd]}" = "$unused" ] \
	|| [ "${BDNAME[brd]}" = "$unknown" ] 
	then
		let brd+=1
		continue
	fi
	if [ "${BDINTERPSM[$1]}" = "Y" ] \
	&& [ "${BDINTERPSM[brd]}" = "Y" ]
	then
		let brd+=1
		continue
	fi

	if [ "${BDADDRSTART[brd]}" = "$dash" ] \
	|| [ "${BDADDREND[brd]}" = "$dash" ]
	then
		let brd+=1
		continue
	fi

	if [ "${BDADDRSTART[brd]}" = "0" ] \
	&& [ "${BDADDREND[brd]}" = "0" ]
	then
		let brd+=1
		continue
	fi

	x=${BDADDRSTART[brd]}
	typeset -i16 mems=16#$x
	x=${BDADDREND[brd]}
	typeset -i16 meme=16#$x

	if [ addrs -ge mems -a  addrs -le meme ]
	then
        	MEMSCHKD="${BDADDRSTART[$1]}"
		display  -w "ERROR:
$boardname=${BDNAME[$1]}, $boardmems=${BDADDRSTART[$1]}, $boardmeme=${BDADDREND[$1]}.
${IM_A_BLANK_LINE_DONT_TOUCH}
$MEMSCONF
${IM_A_BLANK_LINE_DONT_TOUCH}
$boardname=${BDNAME[brd]}, $boardmems=${BDADDRSTART[brd]}, $boardmeme=${BDADDREND[brd]}." \
			-bg $RED -fg $WHITE
		footer $GENERIC_CONTINUE_FOOTER
		return 1
	fi
	let brd+=1
done
return 0
}

function choose_addre
{
# choose_addre(bd_array_index)
# Called when the Choices key for the MemEnd field is selected
# to display the GENERIC_NO_CHOICES message.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
msgnoerr "$GENERIC_NO_CHOICES"
}

function check_addre
{
# check_addre(bd_array_index)
# Called when the MemEnd field is changed to perform validation checking.
# This functions checks for the existence of MemStart and
# allows either '-' or any hex number.
# Calling/Exit State: 0 for success and 1 for failure.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

if [ ! "${BDADDRSTART[$1]}" = "$dash" ] \
&& [   "${BDADDREND[$1]}" = "$dash" ]
then
       	if [ "$MEMECHKD" = "${BDADDREND[$1]}" ]
	then
       		MEMECHKD=""
		return 0
	fi
       	MEMECHKD="${BDADDREND[$1]}"
	display  -w "ERROR:
$boardname=${BDNAME[$1]}, $boardmems=${BDADDRSTART[$1]}, $boardmeme=${BDADDREND[$1]}.
${IM_A_BLANK_LINE_DONT_TOUCH}
$MEMEREQ" \
		-bg $RED -fg $WHITE
	footer $GENERIC_CONTINUE_FOOTER
	return 1
fi

if [   "${BDADDRSTART[$1]}" = "$dash" ] \
&& [ ! "${BDADDREND[$1]}" = "$dash" ]
then
       	if [ "$MEMECHKD" = "${BDADDREND[$1]}" ]
	then
       		MEMECHKD=""
		return 0
	fi
       	MEMECHKD="${BDADDREND[$1]}"
	display  -w "ERROR:
$boardname=${BDNAME[$1]}, $boardmems=${BDADDRSTART[$1]}, $boardmeme=${BDADDREND[$1]}.
${IM_A_BLANK_LINE_DONT_TOUCH}
$MEMSREQ" \
		-bg $RED -fg $WHITE
	footer $GENERIC_CONTINUE_FOOTER
	return 1
fi

if (( !FLD_CHANGED ))
then
        MEMECHKD=""
	return 0
fi

if [ "$MEMECHKD" = "${BDADDREND[$1]}" ]
then
        MEMECHKD=""
        return 0
fi

case "${BDADDREND[$1]}" in
"-")
	error_pending=''
	integer x=${BDCHGS[$1]}
	let x\|=$MEME_CHG
	BDCHGS[$1]=$x
       	MEMECHKD=""
	return 0
	;;
[0-9abcdefABCDEF]*([0-9abcdefABCDEF]))
	error_pending=''
	integer x=${BDCHGS[$1]}
	let x\|=$MEME_CHG
	BDCHGS[$1]=$x
	addre_conflict $1
	if [ "$?" = 0 ]
	then
		MEMECHKD=""
	fi
	return 0
	;;
*)
	msgnoerr "$BD_BAD_MEM"
	return 1
esac
}

function addre_conflict
{
# addre_conflict(bd_array_index)
# Called when the MemEnd field is changed to perform conflict checking.
# A special flag is set to allow the field to be exited
# if a conflict is displayed.
# Calling/Exit State: 0 for success and 1 for failure.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

if [ "${BDCNFG[$1]}" = "$N" ] \
|| [ -z "${BDMODNAME[$1]}" ] \
|| [ "${BDMODNAME[$1]}" = "$dash" ] \
|| [ "${BDMODNAME[$1]}" = "$unused" ] \
|| [ "${BDMODNAME[$1]}" = "$unknown" ]
then
	return 0
fi

[ "${BDADDRSTART[$1]}" = "$dash" -a "${BDADDREND[$1]}" = "$dash" ] && return 0
[ "${BDADDRSTART[$1]}" = "0" -a "${BDADDREND[$1]}" = "0" ] && return 0

typeset x addrs addre
x=${BDADDRSTART[$1]}
typeset -i16 addrs=16#$x
x=${BDADDREND[$1]}
typeset -i16 addre=16#$x
typeset brd=1
if [ "${BDADDRSTART[$1]}" != "$dash" ] \
&& [ "${BDADDREND[$1]}" != "$dash" ]
then
	if [ addre -lt addrs ]
	then
       		if [ "$MEMECHKD" = "${BDADDREND[$1]}" ]
		then
       			MEMECHKD=""
			return 0
		fi
      		MEMECHKD="${BDADDREND[$1]}"
		display  -w "ERROR:
$boardname=${BDNAME[$1]}, $boardmems=${BDADDRSTART[$1]}, $boardmeme=${BDADDREND[$1]}.
${IM_A_BLANK_LINE_DONT_TOUCH}
$BD_BAD_MEME " \
			-bg $RED -fg $WHITE
		footer $GENERIC_CONTINUE_FOOTER
		return 1
	fi
fi

while (( $brd <= $LASTBOARD ))
do
	if [ "$brd" = "$1" ] \
	|| [ "${BDCNFG[brd]}" = "$N" ] \
	|| [ -z "${BDMODNAME[brd]}" ] \
	|| [ "${BDMODNAME[brd]}" = "$dash" ] \
	|| [ "${BDMODNAME[brd]}" = "$unused" ] \
	|| [ "${BDMODNAME[brd]}" = "$unknown" ]
	then
		let brd+=1
		continue
	fi

	if [ "${BDINTERPSM[$1]}" = "Y" ] \
	&& [ "${BDINTERPSM[brd]}" = "Y" ]
	then
		let brd+=1
		continue
	fi

	if [ "${BDADDRSTART[brd]}" = "$dash" ] \
	&& [ "${BDADDREND[brd]}" = "$dash" ]
	then
		let brd+=1
		continue
	fi

	if [ "${BDADDRSTART[brd]}" = "0" ] \
	&& [ "${BDADDREND[brd]}" = "0" ]
	then
		let brd+=1
		continue
	fi

	x=${BDADDRSTART[brd]}
	typeset -i16 mems=16#$x
	x=${BDADDREND[brd]}
	typeset -i16 meme=16#$x
	
	if [ addre -ge mems -a  addrs -le meme ]
	then
		typeset allow1 allow2
		allow_conflict ${ROOT}/etc/conf/mdevice.d/${BDMODNAME[$1]} M
		allow1=$?
		allow_conflict ${ROOT}/etc/conf/mdevice.d/${BDMODNAME[brd]} M
		allow2=$?
		if (( allow1 && allow2 ))
		then
			let brd+=1
			continue
		fi
       		MEMECHKD="${BDADDREND[$1]}"
		display  -w "ERROR:
$boardname=${BDNAME[$1]}, $boardmems=${BDADDRSTART[$1]}, $boardmeme=${BDADDREND[$1]}.
${IM_A_BLANK_LINE_DONT_TOUCH}
$MEMECONF
${IM_A_BLANK_LINE_DONT_TOUCH}
$boardname=${BDNAME[brd]}, $boardmems=${BDADDRSTART[brd]}, $boardmeme=${BDADDREND[brd]}." \
			-bg $RED -fg $WHITE
		footer $GENERIC_CONTINUE_FOOTER
		return 1
	fi
	let brd+=1
done
return 0
}

function choose_dma
{
# choose_dma(bd_array_index)
# Called when the Choices key for the DMA field is selected
# to display the Coices.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
CHOOSE_TITLE="$boarddma $CHOICES"
CHOOSE_FOOTER="$DCU_CHOOSE_FOOTER"
choose "${BDDMA[$1]}" $DMACHOICES
}

function check_dma
{
# check_dma(bd_array_index)
# Called when the MemEnd field is changed to perform validation checking.
# Calling/Exit State: 0 for success and 1 for failure.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

if (( !FLD_CHANGED ))
then
	DMACHKD=""
	return 0
fi

if [ "$DMACHKD" = "${BDDMA[$1]}" ]
then
	DMACHKD=""
	return 0
fi

case "${BDDMA[$1]}" in
"-")
	error_pending=''
	integer x=${BDCHGS[$1]}
	let x\|=$DMA_CHG
	BDCHGS[$1]=$x
	DMACHKD=""
	return 0
	;;
-1)
	error_pending=''
	integer x=${BDCHGS[$1]}
	let x\|=$DMA_CHG
	BDCHGS[$1]=$x
	DMACHKD=""
	return 0
	;;
[0-7])
	if [ ${BDDMA[$1]} -lt 0 -o ${BDDMA[$1]} -gt 7 ]
	then
		msgnoerr "$BD_BAD_DMA"
		return 1
	else
		error_pending=''
		integer x=${BDCHGS[$1]}
		let x\|=$DMA_CHG
		BDCHGS[$1]=$x
		dma_conflict $1
		if [ "$?" = 0 ]
		then
			DMACHKD=""
		fi
		return 0
	fi
	;;
*)
	msgnoerr "$BD_BAD_DMA"
	return 1
esac
}

function dma_conflict
{
# dma_conflict(bd_array_index)
# Called when the DMA field is changed to perform conflict checking.
# A special flag is set to allow the field to be exited
# if a conflict is displayed.
# Calling/Exit State: 0 for success and 1 for failure.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

if [ "${BDCNFG[$1]}" = "$N" ] \
|| [ -z "${BDNAME[$1]}" ] \
|| [ "${BDDMA[$1]}" = "$dash" ] \
|| [ "${BDDMA[$1]}" = "-1" ] \
|| [ "${BDNAME[$1]}" = "$dash" ] \
|| [ "${BDNAME[$1]}" = "$unused" ] \
|| [ "${BDNAME[$1]}" = "$unknown" ] 
then
	return 0
fi

typeset brd=1
while (( $brd <= $LASTBOARD ))
do
	if [ "$brd" = "$1" ] \
	|| [ ! "${BDDMA[brd]}" = "${BDDMA[$1]}" ] \
	|| [ "${BDCNFG[brd]}" = "$N" ] \
	|| [ -z "${BDNAME[brd]}" ] \
	|| [ "${BDIRQ[brd]}" = "$dash" ] \
	|| [ "${BDNAME[brd]}" = "$dash" ] \
	|| [ "${BDNAME[brd]}" = "$unused" ] \
	|| [ "${BDNAME[brd]}" = "$unknown" ] 
	then
		let brd+=1
		continue
	fi
	if [ "${BDINTERPSM[$1]}" = "Y" ] \
	&& [ "${BDINTERPSM[brd]}" = "Y" ]
	then
		let brd+=1
		continue
	fi

	typeset allow1 allow2
	allow_conflict ${ROOT}/etc/conf/mdevice.d/${BDMODNAME[$1]} D
	allow1=$?
	allow_conflict ${ROOT}/etc/conf/mdevice.d/${BDMODNAME[brd]} D
	allow2=$?
	if (( allow1 && allow2 ))
	then
		let brd+=1
		continue
	fi

	DMACHKD="${BDDMA[$1]}"
	display  -w "ERROR:
$DMACONF
${IM_A_BLANK_LINE_DONT_TOUCH}
$boardname=${BDNAME[$1]}, $boarddma=${BDDMA[$1]},
${IM_A_BLANK_LINE_DONT_TOUCH}
$boardname=${BDNAME[brd]}, $boarddma=${BDDMA[brd]}." \
		-bg $RED -fg $WHITE
	footer $GENERIC_CONTINUE_FOOTER
	return 1
done
return 0
}

function choose_cpu
{
# choose_cpu(bd_array_index)
# Called when the Choices key for the BindCPU field is selected
# to display the Choices.
# Calling/Exit State: void

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
typeset i ncpu cpuchoices
CHOOSE_TITLE="$boardcpu $CHOICES"
CHOOSE_FOOTER="$DCU_CHOOSE_FOOTER"
cpuchoices='- 0'
call sysconfig 12	# _CONFIG_NENGINE=12
ncpu=$?
i=1
while (( i < ncpu ))
do
	cpuchoices="$cpuchoices $i"
	let i+=1
done
choose -f "${BDCPU[$1]}" $cpuchoices
}

function check_cpu
{
# check_cpu(bd_array_index)
# Called when the BindCPU field is changed to perform validation checking.
# Calling/Exit State: 0 for success and 1 for failure.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
typeset ncpu badmsg 

call sysconfig 12	# _CONFIG_NENGINE=12
ncpu=$?
if [ "$ncpu" = "1" ]
then
	badmsg="$BD_ONE_CPU"
else
	badmsg="$BD_BAD_CPU ${ncpu}."
fi
let ncpu-=1

if (( !FLD_CHANGED ))
then
	return 0
fi

case "${BDCPU[$1]}" in
"-")
	error_pending=''
	integer x=${BDCHGS[$1]}
	let x\|=$CPU_CHG
	BDCHGS[$1]=$x
	return 0
	;;
[0-9]*([0-9]))
	if [ ${BDCPU[$1]} -gt "$ncpu" ]
	then
		msgnoerr "$badmsg"
		return 1
	else
		error_pending=''
		integer x=${BDCHGS[$1]}
		let x\|=$CPU_CHG
		BDCHGS[$1]=$x
		return 0
	fi
	;;
*)
	msgnoerr "$badmsg"
	return 1
esac
}

function reset_name
{
# reset_name() NOT_CURRENTLY_USED"
# Callback to clear name field of BDLINE board array
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

integer chce="${BOARDFLD[$BDLINE]}"
fld_change $BDFID $chce ''
return 0
}

function rdRM_key
{
# rdRM_key(key, bd_array_index, inst)
# query the resource manager for key information
# return: 0 - valid data found in resmgr
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

integer i retd
integer n=$3
typeset parm_list="MODNAME UNIT IPL ITYPE IRQ IOADDR MEMADDR DMAC BINDCPU BRDID SCLASSID BRDBUSTYPE ENTRYTYPE"
RMgetvals $1 "$parm_list" $n
if (( i=$? ))
then
        if [ "$RMIRQ" = "$dash" -a "$RMPORTSTART" = "$dash" -a "$RMADDRSTART" = "$dash" -a "$RMDMAC" = "$dash" -a "$RMBRDBUSTYPE" = "$dash" ]
        then
                return 2 # no hardware info -> ignore
        fi

	BDKEY[$2]="$1"
	if [ "$RMBRDBUSTYPE" = "$isa" -o "$RMBRDBUSTYPE" = "$unk" ]
	then
		BDTYPE[$2]=$prompt
	else
		BDTYPE[$2]=$noprompt
	fi
	if [ "$RMENTRYTYPE" = "2" ]
	then
		BDTYPE[$2]=$noprompt
	fi
	BDBUSTYPE[$2]="$RMBRDBUSTYPE"
	BDID[$2]="$RMBRDID"
	BDCLASS[$2]="$RMSCLASSID"
	BDMODNAME[$2]="$RMMODNAME"
	BDCNFG[$2]="$Y"
	BDIRQ[$2]="$RMIRQ"
	BDUNIT[$2]="$RMUNIT"
	BDIPL[$2]="$RMIPL"
	BDITYPE[$2]="$RMITYPE"
	BDPORTSTART[$2]="$RMPORTSTART"
	BDPORTEND[$2]="$RMPORTEND"
	BDADDRSTART[$2]="$RMADDRSTART"
	BDADDREND[$2]="$RMADDREND"
	BDDMA[$2]="$RMDMAC"
	BDCPU[$2]="$RMBINDCPU"
	BDENTRYTYPE[$2]="$RMENTRYTYPE"
else
	return 1
fi

if [ "$RMMODNAME" = "$unused" ]
then
	BDNAME[$2]="$unused"
	return 0
fi

# search driver knowledge base for match on board
lookupdrv $2
if (( i=$? ))
then
	BDNAME[$2]="$FNDDRVBOARDNAME"
	BDMODNAME[$2]="$FNDMODNAME"
 	BDMODINDX[$2]="$i"

	if [ "$RMBRDBUSTYPE" = "$isa" -o "$RMBRDBUSTYPE" = "$unk" ]
	then
		if [ "${DRVBUSTYPE[i]}" = "$isa" -o "${DRVBUSTYPE[i]}" = "$unk" ] \
		|| [ -z "${DRVBUSTYPE[i]}" ]
		then
			BDTYPE[$2]=$prompt
		else
			BDTYPE[$2]=$noprompt
		fi
	fi
	if [ "$RMENTRYTYPE" = "2" ]
	then
		BDTYPE[$2]=$noprompt
	fi
else
	if [ -n "$FNDMODNAME" ]
	then
		BDNAME[$2]="$FNDDRVBOARDNAME"
		BDMODNAME[$2]="$FNDMODNAME"
	else
		BDNAME[$2]=$unknown
	fi
fi
return 0
}

function wrtBSP_key
{
# wrtBSP_key(bd_array_index)
# Called to write the board specific parameter record to
# the resource manager database for the given board array index key.
# Calling/Exit State: 0 for success and see RMputvals(3) for failure.

typeset parm_list
typeset val_list
typeset bd=$1

# If there are no BSPs to write, just return
if [ -z "${BDBSPPARAMS[bd]}" ]
then
	return 0
fi
val_list="${BDBSPVALS[bd]}"

# All BSPs are type string.  We add a ',s' to tell the RM this
OIFS=$IFS
IFS=" "
set ${BDBSPPARAMS[bd]}
parm_list="$1,s"
shift
while (( $#>0 ))
do
	parm_list="$parm_list $1,s"
	shift
done
IFS=$OIFS

RMdelvals ${BDKEY[bd]} "$parm_list"
RMputvals ${BDKEY[bd]} "$parm_list" "$val_list"
return $?
}

function wrtRM_key
{
# wrtRM_key(bd_array_index)
# Called to write a record to the resource manager database for the given board
# array index key.
# Calling/Exit State: 0 for success and see RMputvals(3) for failure

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
typeset bdmodname bdunit bdipl bditype bdirq bdportstart bdportend bdaddrstart bdaddrend bddma bdcpu bdid bdclass bdbustype bdentrytype
typeset m val_list
typeset parm_list="MODNAME UNIT IPL ITYPE IRQ IOADDR MEMADDR DMAC BINDCPU BRDID SCLASSID BRDBUSTYPE ENTRYTYPE"
RMdelvals ${BDKEY[$1]} "$parm_list"

bdmodname="${BDMODNAME[$1]}"
bdunit="${BDUNIT[$1]}"
bdipl="${BDIPL[$1]}"
bditype="${BDITYPE[$1]}"
bdirq="${BDIRQ[$1]}"
bdportstart="${BDPORTSTART[$1]}"
bdportend="${BDPORTEND[$1]}"
bdaddrstart="${BDADDRSTART[$1]}"
bdaddrend="${BDADDREND[$1]}"
bddma="${BDDMA[$1]}"
bdcpu="${BDCPU[$1]}"
bdid="${BDID[$1]}"
bdclass="${BDCLASS[$1]}"
bdbustype="${BDBUSTYPE[$1]}"
bdentrytype="${BDENTRYTYPE[$1]}"

for m in bdmodname bdunit bdipl bditype bdirq bdportstart bdportend bdaddrstart bdaddrend bddma bdcpu bdid bdclass bdbustype bdentrytype
do
	eval $m=\$\{$m:-$dash\}
done

val_list="$bdmodname $bdunit $bdipl $bditype $bdirq $bdportstart $bdportend $bdaddrstart $bdaddrend $bddma $bdcpu $bdid $bdclass $bdbustype $bdentrytype" 

if [ "$val_list" = "$dash $dash $dash $dash $dash $dash $dash $dash $dash $dash $dash $dash $dash $dash" ]
then
	RMdelkey ${BDKEY[$1]}
	return 0
fi
RMputvals ${BDKEY[$1]} "$parm_list" "$val_list"
return $?
}

function setpltfm
{
# setpltfm()
# Called to set the platform specific choices for IPL, ITYPE and DMA.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

IPLCHOICES='0 1 2 3 4 5 6 7 8 9'
ITYPECHOICES='0 1 2 3 4'
DMACHOICES='- -1 0 1 2 3 4 5 6 7'
}

function boardset
{
# boardset(bd_array_index)
# Field exit callback when the Configure field is exited.
# Calling/Exit State: 0 for success and 1 for failure.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
if (( !FLD_CHANGED ))
then
	return 0
fi
typeset -u cnfg="${BDCNFG[$1]}"
case "$cnfg"
in
"$Y"|"$N")
	fld_change $BDFID ${BOARDFLD[$BDLINE]} $cnfg 
	integer x=${BDCHGS[$1]}
	let x\|=$ADD_DEL_CHG
	BDCHGS[$1]=$x
	return 0
	;;
*)
	msgnoerr "$BD_ERR_CNFG"
	return 1
esac
}

function boardxpnd
{
# boardxpnd()
# Called when the F6=Info key is selected from the Hardware Device Configuration
# screen to display additional Configuration Information.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
integer bd=LINETOBD[BDLINE]
integer drv
typeset -R28 imsg
typeset -L48 msg

place_window $DCU_MAX_WIDTH $DCU_MAX_HEIGHT-$FOOTER_HEIGHT-1 -below 0 -title "$CONFIG_INFO" 

imsg="$DriverName"
msg=" : ${BDMODNAME[bd]}"
wprintf $CURWIN "%s %s\n $imsg $msg"

imsg="$BoardName"
lookupdrv $bd
if (( drv=$? ))
then
	msg=" : ${DRVBOARDNAMEFULL[drv]}"
else
	msg=" : ${BDMODNAME[bd]}"
fi
wprintf $CURWIN "%s %s\n $imsg $msg"

imsg="$UNITs"
msg=" : ${BDUNIT[bd]}"
wprintf $CURWIN "%s %s\n $imsg $msg"

imsg="$IPLs"
msg=" : ${BDIPL[bd]}"
wprintf $CURWIN "%s %s\n $imsg $msg"

imsg="$ITYPEs"
msg=" : ${BDITYPE[bd]}"
wprintf $CURWIN "%s %s\n $imsg $msg"

imsg="$IRQs"
msg=" : ${BDIRQ[bd]}"
wprintf $CURWIN "%s %s\n $imsg $msg"

imsg="$boardios"
msg=" : ${BDPORTSTART[bd]}"
wprintf $CURWIN "%s %s\n $imsg $msg"

imsg="$boardioe"
msg=" : ${BDPORTEND[bd]}"
wprintf $CURWIN "%s %s\n $imsg $msg"

imsg="$boardmems"
msg=" : ${BDADDRSTART[bd]}"
wprintf $CURWIN "%s %s\n $imsg $msg"

imsg="$boardmeme"
msg=" : ${BDADDREND[bd]}"
wprintf $CURWIN "%s %s\n $imsg $msg"

imsg="$DMAs"
msg=" : ${BDDMA[bd]}"
wprintf $CURWIN "%s %s\n $imsg $msg"

imsg="$boardcpu"
msg=" : ${BDCPU[bd]}"
wprintf $CURWIN "%s %s\n $imsg $msg"

imsg="$BoardId"
msg=" : ${BDID[bd]}"
wprintf $CURWIN "%s %s\n $imsg $msg"

imsg="$BoardClass"
msg=" : ${BDCLASS[bd]}"
wprintf $CURWIN "%s %s\n $imsg $msg"

imsg="$BoardBusType"
msg=" : ${BDBUSTYPE[bd]}"
wprintf $CURWIN "%s %s\n $imsg $msg"

footer "$ESCXNDFOOTER"
integer bdxwin="$CURWIN"
call getkey
wclose $bdx_win
footer "$BOARDXPND"
}


function silent_match
{
# silent_match(bd_array_index drv_array_index)
# Called during the silent boardid mapping phase when a
# boardid and board bus type pairing is matched.
# It fills in the UNKNOWN PCU entry and attempts to
# delete any "default" entries.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

typeset drvname isc unit ipl itype irq ports porte mems meme dma cpu

while read drvname isc unit ipl itype irq ports porte mems meme dma cpu
do
	if [ ! "$drvname" = "${BDMODNAME[$1]}" ]
	then
		continue
	fi
	break
done < ${ROOT}/etc/conf/sdevice.d/${BDMODNAME[$1]}

for m in unit ipl itype irq ports porte mems meme dma cpu
do
eval $m=\$\{$m:-$dash\}
done

if [ "${DRVBUSTYPE[$2]}" = "mca" -o "${DRVBUSTYPE[$2]}" = "MCA" ]
then
	if [ "${DRVAUTOCONFIG[$2]}" = "Y" ]
	then
 		BDIPL[$1]="$ipl"
 		if [ "${BDITYPE[$1]}" = "$dash" ]
		then
 			BDITYPE[$1]="$itype"
		fi
		if [ "${BDCPU[$1]}" = "$dash" ]
		then
			BDCPU[$1]="$cpu"
		fi

	else
 		BDIPL[$1]="$ipl"
 		BDITYPE[$1]="$itype"
 		BDIRQ[$1]="$irq"
 		BDPORTSTART[$1]="$ports"
 		BDPORTEND[$1]="$porte"
 		BDADDRSTART[$1]="$mems"
 		BDADDREND[$1]="$meme"
 		BDDMAC[$1]="$dma"
 		BDCPU[$1]="$cpu"
	fi
else
 	BDIPL[$1]="$ipl"
 	if [ "${BDITYPE[$1]}" = "$dash" ]
	then
 		BDITYPE[$1]="$itype"
	fi
 	if [ "${BDCPU[$1]}" = "$dash" ]
	then
 		BDCPU[$1]="$cpu"
	fi
fi

wrtRM_key $1
silent_clean ${BDMODNAME[$1]} $1
}

function silent_clean
{
# silent_clean(MODNAME, bd_array_index)
# Called during the silent boardid mapping phase after
# the PCU entry has been updated.
# Its purpose is to delete any default entries for the
# corresponding MODNAME.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
integer i=1

#if [ "${BDCLEANED[$2]}" = "Y" ]
#then
#	return 0
#fi

while (( i <= ${LASTBOARD} ))
do

 	if [ "$1" = "${BDMODNAME[i]}" ]
	then
		if [ "${BDENTRYTYPE[i]}" = "1" ]
		then
			RMdelkey ${BDKEY[i]}
#			${BDCLEANED[i]}="Y" 
		fi
	fi
	let i+=1
done
}


function verifyconf
{
# verifyconf(formid, bd_array_index)
# Called from both the verifydriver() and verifyhdw() functions
# to verify the configuration for board index specified after
# syncing input on the form specified.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
typeset found=0
typeset lMODNAME=""
integer drv
integer bd=0

call fld_sync $1
BDTYPE[0]=$changed
RMnewkey
BDKEY[0]=$?
BDMODNAME[0]="${BDMODNAME[$2]}"
BDNAME[0]="${BDNAME[$2]}"
BDUNIT[0]="${BDUNIT[$2]}"
BDIPL[0]="${BDIPL[$2]}"
BDITYPE[0]="${BDITYPE[$2]}"
BDIRQ[0]="${BDIRQ[$2]}"
BDPORTSTART[0]="${BDPORTSTART[$2]}"
BDPORTEND[0]="${BDPORTEND[$2]}"
BDADDRSTART[0]="${BDADDRSTART[$2]}"
BDADDREND[0]="${BDADDREND[$2]}"
BDDMA[0]="${BDDMA[$2]}"
BDCPU[0]="${BDCPU[$2]}"
BDBUSTYPE[0]="${BDBUSTYPE[$2]}"
BDID[0]="${BDID[$2]}"
BDCLASS[0]="${BDCLASS[$2]}"
BDCNFG[0]="$Y"
BDCHGS[0]=$ADD_DEL_CHG

for drv in $ALLDRIVERS
do
	if [ "${BDMODNAME[bd]}" = "${MODNAME[drv]}" ]
	then
		found=1
		break
	fi
done
if [ $found = 0 ]
then
	RMdelkey ${BDKEY[0]}
	return
fi
if [ ${DRVVERIFY[drv]} = N ]
then
	drv_noverify $bd
	RMdelkey ${BDKEY[0]}
	return
fi
footer "$VERIFYFOOTER"
if [ -n "${BDKEY[$bd]}" ]
then
	#update resmgr for board record
	lMODNAME="${BDMODNAME[$bd]}"
	BDMODNAME[$bd]="$dash"
	RMadd "$bd"
	BDMODNAME[$bd]="$lMODNAME"
	if verify $bd $drv
	then
		display -w "$BoardName ${BDMODNAME[bd]} $VERIFYSUCCESS"
		footer "$GENERIC_CONTINUE_FOOTER"
	else
		display -w "$BoardName ${BDMODNAME[bd]} $VERIFYFAIL" -bg $RED -fg $WHITE
		footer "$GENERIC_CONTINUE_FOOTER"
	fi
else
	display -w "$BD_NOSAVED" -bg $RED -fg $WHITE
	footer "$GENERIC_CONTINUE_FOOTER"
fi
RMdelkey ${BDKEY[0]}
}


function drv_noverify
{
# drv_noverify(bd_array_index)
# Called to display a message when the F4=Verify hotkey is invoked and
# the corresponding drvmap.d file has an 'N' instead of a 'Y' for the
# verify function.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
display -w "$BoardName ${BDMODNAME[$1]} $DRV_NOVERIFY" -bg $MAGENTA -fg $WHITE
footer "$GENERIC_CONTINUE_FOOTER"
}

function dcu_action
{
# dcu_action(action)
# Called to set the DCU_ACTION parameter to either $DCU_REBOOT or $DCU_REBUILD
# for the special RM_KEY.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}

RMputvals -1 "DCU_ACTION,s" $1
if [ ! "$?" = "0" ] \
&& [ "$UNIX_INSTALL" = "N" ]
then
	display -w "$DCUACTION" -bg $RED -fg $WHITE
	footer "$GENERIC_CONTINUE_FOOTER"
	call proc_loop
fi
}

function allow_conflict
{
# allow_conflict(MODNAME, flag)
# Read the /etc/conf/mdevice.d/[MODNAME] file for a device driver
# checking for flag that indicates conflict override capability.
# Calling/Exit State: 0 for false and 1 for true(override).

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
typeset -u drvname pre char ord bmaj cmaj
typeset x w

w=${1#*mdevice.d/}
grep ^$w $1 2> /dev/null | read drvname pre char ord bmaj cmaj 
case "$char" in
*$2*)
	return 1
	;;
*)
	return 0
	;;
esac
}

function set_adv_params
{
# set_adv_params(bd_array_index)
# Called to read sdevice file for a IPL, ITYPE and BINDCPU fields
# and set them for the corresponding board.
# Only set ITYPE if not currently set.
# Calling/Exit State: void.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
typeset drvname isc unit ipl itype irq ports porte mems meme dma cpu

while read drvname isc unit ipl itype irq ports porte mems meme dma cpu
do
	if [ ! "$drvname" = "${BDMODNAME[$1]}" ]
	then
		continue
	fi
	BDIPL[$1]="$ipl"
	if [ "${BDITYPE[$1]}" = "$dash" ]
	then
		BDITYPE[$1]="$itype"
	fi
	if [ -z "$cpu" ]
	then
		BDCPU[$1]="$dash"
	else
		BDCPU[$1]="$cpu"
	fi
done <${ROOT}/etc/conf/sdevice.d/"${BDMODNAME[$1]}"
}


function silent_verify
{
# silent_verify(bd_array_index, drv_array_index)
# Called when performing the silent boardid mapping and
# the driver's drvmap file has a 'V' in the verify field
# so that the verify() is invoked before the silent matching.
# If this is not during Installation,
# try to idbuild the driver when the verify initially fails.
# Calling/Exit State: 0 for success and 1 for failure.

[ "$DCUDEBUG" -gt 3 ] && {
print -u2 "$0 called"
set -x
}
	integer brd=$1
	integer drv=$2
	typeset CLEAR_ITYPE=N

	verify $brd $drv
	if [ $? -eq 0 ]
	then
		return 0
	fi
	if [ "$UNIX_INSTALL" = "Y" ]
	then
		return 1
	fi
# check to see if the error was ENOENT (2).  If it was, this means
# that the verify failed because the driver was not loaded and we
# try to load it.  If the error was something else, then we just fail.
	cdecl intp sv_errno='&Xk_errno'
	cprint -v sv_errno sv_errno
	if [ "$sv_errno" != 2 ]
	then
		return 1
	fi
	BDNAME[brd]="${DRVBOARDNAME[drv]}"
	BDMODNAME[brd]="${MODNAME[drv]}"
	[ -z BDITYPE[brd] ] && CLEAR_ITYPE=Y
	set_adv_params $brd
	BDMODNAME[brd]="$dash"
	wrtRM_key $brd
	RMclose
	/etc/conf/bin/idbuild -c -M ${MODNAME[drv]} > /dev/null 2>&1
	RMopen 2
	verify $brd $drv
	if [ $? -eq 0 ]
	then
		BDMODNAME[brd]="${MODNAME[drv]}"
		wrtRM_key $brd 
		return 0
	fi
	BDNAME[brd]="$dash"
	BDMODNAME[brd]="$dash"
	if [ "$CLEAR_ITYPE" = Y ]
	then
		BDITYPE[brd]="$dash"
	fi
	wrtRM_key $brd
	return 1
}
