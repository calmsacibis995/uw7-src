#!/sbin/sh
#ident "@(#)ndcleanup.sh	19.1
#
# cleanup netX remnants if netcfg went belly up before
# removing the netX and MDI drivers ndcfg added to the system
#
# TODO: idinstall -d unconfigured netX drivers not in resmgr?
#

removed=true
netx=`netcfg -I dlpi`

cleanupnetX()
{
	removed=""

	for key in `resmgr | awk '$1 != "KEY" { print $1 }'`
	do
		r=`echo "resdump $key" | /usr/sbin/ndcfg | grep NETCFG_ELEMENT` && {
			rn=`echo $r | awk '{ print $7 }'`
			rogue=0
			for n in $netx
			do
				if [ "$n" = "$rn" ]
				then
					rogue=1
					break
				fi
			done
			if [ "$rogue" = "0" ]
			then
				echo "idremove $rn 0" | /usr/sbin/ndcfg
				removed=true
				break
			fi
		}
	done
}

while [ "$removed" ]
do
	cleanupnetX
done
/etc/conf/bin/idconfupdate
