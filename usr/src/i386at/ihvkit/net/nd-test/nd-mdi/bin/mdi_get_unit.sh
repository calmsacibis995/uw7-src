#!/bin/sh
#ident "@(#)mdi_get_unit.sh	28.1"
#
#	Copyright (C) The Santa Cruz Operation, 1997
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#
#
# Test machine configuration - order is important here:
#	1) Remove any currently configured chains with netcfg
#	2) Machine has new dlpi Driver.o (includes mdi_get_unit)
		# rm /etc/conf/pack.d/dlpi/Driver_*.o
		# cp <top-of-tree_dlpi_Driver.o> /etc/conf/pack.d/dlpi
#	3) Machine has top-of-tree /etc/conf/interface.d/mdi.2 (mdi_get_unit)
#	4) Update /usr/lib/netcfg/bin/ndcfg (use top-of-tree)
#	5) Entire kernel rebuilt on machine: /etc/conf/bin/idbuild -B; init 6
#	6) Remove old MDI driver(s) from /etc/inst/nd/<driver>/Driver_*.o
#	7) Add new MDI driver into /etc/inst/nd/<driver>/Driver.o
#	8) Configure the MDI driver being tested with netcfg
#
# Script assumptions:
#	* Only one adapter of this type is physically in the test machine
#	* That adapter is configured at minor #0
#	* In init state 1, without the adapter loaded
#

reset_minor_zero()
{
	/sbin/modadmin -U $drv 2>/dev/null
	# reset minor device number to zero

	if [ "$key" ]
	then
		/sbin/resmgr -k $key -p NDCFG_UNIT,n -v 0
		/sbin/resmgr -k $key -p DEV_NAME,s -v /dev/mdi/${drv}0
	fi
	echo "net0:${drv}0:N:0::N" > /etc/inst/nd/dlpimdi
}

cleanup()
{
	reset_minor_zero
	exit 1
}

checksys()
{
	# verify init state (one)
	runlevel=`who -r | awk '{ print $3 }'`
	if [ "$runlevel" != "1" ]
	then
		echo "$0 must be run in init state 1"
		cleanup
	fi

	# verify only one driver is configured, and arg passed in is correct
	numconf=`/etc/nd getmdi | wc -l`
	if [ "$numconf" = "1" ]
	then
		netx=`cat /etc/inst/nd/dlpimdi | awk -F: '{ print $1 }'`
		if [ "$netx" != "net0" ]
		then
			echo "netX should be net0"
			cleanup
		fi
		mdi=`cat /etc/inst/nd/dlpimdi | awk -F: '{ print $2 }'`
		if [ ${drv}0 != $mdi ]
		then
			echo "$mdi doesn't match ${drv}0"
			cleanup
		fi
	else
		echo "too many MDI drivers configured, $numconf"
		cleanup
	fi

	numbrds=`/sbin/resmgr | grep $drv | wc -l`
	if [ "$numbrds" != 1 ]
	then
		echo "only one $drv board allowed in machine, found $numbrds"
		cleanup
	fi
}

# verify no other devices open except the minor passed in
checkopen()
{
	for f in /dev/mdi/$drv*
	do
		if [ "$f" = "/dev/mdi/$drv$1" ]
		then
			sleep 1 < $f || {
				echo "open minor $1 FAILED unexpectedly"
				cleanup
			} && {
				/etc/nd start
				echo "test tx/rx minor $1: \c"
				echo "test net0" | /usr/sbin/ndcfg 2>/dev/null
				/etc/nd stop
			}
		else
			unit=`echo $f | sed -e "s:/dev/mdi/$drv::"`
			sleep 1 < $f && {
				echo "open minor $unit SUCCEEDED, supposed to fail"
				cleanup
			}
		fi
	done
}

check_minors()
{
	# test driver configured with minor device 0 
	/sbin/modadmin -l $drv
	checkopen 0

	# map driver to different minor devices verify opens
	last=`echo /dev/mdi/$drv* | sed -e "s:.*/dev/mdi/$drv::"`
	i=1
	while [ $i -le $last ]
	do
		/sbin/modadmin -U $drv
		/sbin/resmgr -k $key -p NDCFG_UNIT,n -v $i
		/sbin/resmgr -k $key -p DEV_NAME,s -v /dev/mdi/$drv$i
		echo "net0:$drv$i:N:0::N" > /etc/inst/nd/dlpimdi
		# /etc/conf/bin/idconfupdate
		/sbin/modadmin -l $drv
		checkopen $i
		i=`expr $i + 1`
	done
}

check_resmgr_stubs()
{
	/sbin/resmgr -a -p MODNAME -v $drv
	# fill out new resmgr entry at $reskey
	unit=`/sbin/resmgr -k $key -p NDCFG_UNIT,n`
	if [ "$unit" != "0" ]
	then
		echo "check_resmgr_stubs() expected unit 0, found $unit"
		cleanup
	fi
	brdid=`/sbin/resmgr -k $key -p BRDID`
	bus=`/sbin/resmgr -k $key -p BRDBUSTYPE`
	/sbin/resmgr -m $drv -p BRDID -v $brdid -i 1
	/sbin/resmgr -m $drv -p BRDBUSTYPE -v $bus -i 1

	/sbin/modadmin -l $drv || echo "load with stub resmgr entries failed"

	/etc/nd start
	echo "test tx/rx w/ stub resmgr entries: \c"
	echo "test net0" | /usr/sbin/ndcfg 2>/dev/null
	/etc/nd stop

	# remove new stub
	/sbin/resmgr -r -m $drv -i 1
}

# main Main MAIN
# verify driver arg passed in
if [ $# -ne 1 ]
then
	echo "Usage: $0 <mdi_driver_name>"
	cleanup
fi

drv=$1

checksys

key=`/sbin/resmgr | grep $drv | awk '{ print $1 }'`
unit=`resmgr -k $key -p NDCFG_UNIT,n`
if [ "$unit" != 0 ]
then
	echo "unit should be zero, found $unit"
	cleanup
fi

# if driver is loaded, unload it - ignore error if driver not yet loaded
/etc/nd stop
/sbin/modadmin -U $drv 2>/dev/null

check_minors
reset_minor_zero

# We are done checking minor device mappings.
# If it's an ISA board, this is really all that's necessary;
# we've verified the driver removal problem, and autoconf will
# not add funkey driver stubs to the resmgr for non-autodetectable
# adapters.  However, the following check for this won't hurt anything...

check_resmgr_stubs
/sbin/modadmin -U $drv

exit 0
