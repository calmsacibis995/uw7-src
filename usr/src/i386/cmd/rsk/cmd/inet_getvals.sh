#!/usr/bin/xksh
#ident	"@(#)inet_getvals.sh	15.1"

function inet_setup
{
: #debug: print -u2 function inet_setup
: #debug:set -x
# process devices
legaldevs=
illegaldevs=
netdevs=`(
	awk -F\# '{print $1;}' </etc/confnet.d/netdrivers |
		awk '{print $1;}'
	awk -F\# '{print $1;}' </etc/confnet.d/inet/interface |
		awk -F: '{print $4;}' |
		awk -F/ '{node=$3; for (i = 4; i <= NF; ++i)
				node=node "/" $i;  print node;}' 
	) 2>/dev/null |
		egrep -v '(^|ppp|loop)[ 	]*$' 2>/dev/null |
		sort -u 2>/dev/null`

	for DR in route inet icmp ip tcp udp llcloop rawip arp app
	do
		/sbin/modadmin -l ${DR} >/dev/null 2>&1
	done
	#for copywrite messages
	for DR in tcp ip
	do	[[ -c /dev/$DR ]] && </dev/$DR
	done
	for DR in $netdevs
	do	if [[ -c /dev/$DR ]] && </dev/$DR
		then	legaldevs="${legaldevs} $DR"
		#later code uses both newlines as markers
		else	illegaldevs="${illegaldevs}
$DR
"
		fi
	done

	UNAME=`uname -n`
	if [[ -z "${UNAME}" ]]
	then	read UNAME </etc/nodename
	fi
	INET_nodename=${UNAME}

	typeset excess=
	[[ -z "${INET_device}" && -n "${legaldevs}" ]] &&
		read INET_device excess <<!
${legaldevs}
!
#	get_current_forwarding
	load_lib_data
}
#load libs and initial data structs
function load_lib_data
{
#set -x
	[[ -z "${inet_stat_libutil}" ]] &&
	  libload /usr/lib/libutil.so &&
	  inet_stat_libutil=ok

	[[ -z "${inet_stat_libnsl}" ]] &&
	  libload /usr/lib/libnsl.so &&
	  inet_stat_libnsl=ok

	[[ -z "${inet_stat_libsocket}" ]] &&
	  libload /usr/lib/libsocket.so &&
	  inet_stat_libsocket=ok

	[[ -z "${inet_stat_tcpip}" ]] &&
	  libload /usr/lib/tcpip.so &&
	  call init_winxksh_var_name_addr \
		'@longp:&env_set' \
		'@longp:&_tcpip_gethostbyname' \
		'@longp:&_tcpip_gethostbyaddr' &&
	  inet_stat_tcpip=ok
	: loaded libutil libnsl tcpip libsocket

	[[ -z "${readConfigFile}" ]] &&
	  ccall -r allocReadConfigFile 0 &&
	  readConfigFile=p$_RETX &&
	  cdecl -g intp ret_readConfigFile=p$_RETX

	[[ -z "${readInterfaceFile}" ]] &&
	  ccall -r allocReadConfigFile 1  &&
	  readInterfaceFile=p$_RETX &&
	  cdecl -g intp ret_readInterfaceFile=p$_RETX
}

#main()

#get the old values for a device.  note that this 
#interface file lookup by ip address would be quite different.
: #debug: print -u2 function get_all_old_vals
#set -x
	inet_setup
	load_lib_data

	INET_get_addr=
	INET_get_broadcast=
	INET_get_dns_1=
	INET_get_dns_2=
	INET_get_dns_3=
	INET_get_domain=
	INET_get_netmask=
	INET_get_nodename=
	INET_get_router=
	INET_get_slink=
	INET_get_rt_name=
	INET_get_nd_val=
	broadcast_ret=
#set -x
integer num=0
for i in $INET_device
do

	INET_dev_node=$i
	num=num+1
	#sigh. these are in sub-shells to avoid some sort of corruption
	#that prevents the correct values from being gotten the Nth time.
	(
	  [[ -n "${readConfigFile}" ]] &&
	     ccall -r simple_getDefaultRouter ret_readConfigFile &&
	     typeset ret_router=p$RET_X
	     : #debug: print -u2 retrieved INET_get_router= "<$INET_get_router>"

	  typeset tmp_bootp_name=${save_bootp_name:-${INET_nodename}}
	  #call simple_getNetmask to get this device entry
	  #  information from the interface file.
	  #If the entry can not be found, then do a gethostbyname
	  #  on the nodename.
	  #If there is no device entry, request with null addresses
	  #  to force the gethostbyname only.
	  if [[ -n "${INET_dev_node}" ]]
	  then	[[ -z "${readInterfaceFile}" ]] &&
			cdecl -g intp ret_readInterfaceFile=p0x0
		ccall simple_getNetmask ret_readInterfaceFile \
			"@string_t:!${INET_dev_node}!" \
			"@string_t:!${tmp_bootp_name}!"
	  else	cdecl -g intp TEMP_readInterfaceFile=p0x0
		ccall simple_getNetmask TEMP_readInterfaceFile \
			TEMP_readInterfaceFile \
			"@string_t:!${tmp_bootp_name}!"
	  fi

	  [[ -n "${readInterfaceFile}" && -n "${INET_dev_node}" ]] && {
	      ccall simple_getBroadcast ret_readInterfaceFile \
	  	"@string_t:!${INET_dev_node}!" &&
	      	broadcast_ret=$_RETX
	    }
	  : #debug: print -u2 retrieved INET_get_addr= "<$INET_get_addr>"
	  : #debug: print -u2 retrieved INET_get_nodename= "<$INET_get_nodename>"
	  : #debug: print -u2 retrieved INET_get_nd_val= "<$INET_get_nd_val>"
	
	  # we need to expand a null uname when we are in a zero nics
	  # case (where ethinfo_gray is non-null)
	  # or this nic is in the interface file (where broadcast_ret
	  # is a real pointer)
	  # or this is not a multiple board install
	  [[ -z "${INET_get_nodename}" &&
	     ( -n "${broadcast_ret}" ||
	       -n "${ethinfo_gray}" ||
	       "${multi_b}" = 0  ) ]] &&
	    INET_get_nodename="${UNAME}" &&
	  : #debug: print -u2 retrieved INET_get_nodename= "<$INET_get_nodename>"

	  [[ -r ${INET_ROOT}/etc/resolv.conf ]] && 
		cut -d\# -f1 ${INET_ROOT}/etc/resolv.conf |
		grep '^[ 	]*domain[ 	]' |
		read _literal INET_get_domain _rest

	  [[ -r ${INET_ROOT}/etc/resolv.conf ]] && 
		cut -d\# -f1 ${INET_ROOT}/etc/resolv.conf |
		grep '^[ 	]*nameserver[ 	]' | {
			read _literal INET_get_dns_1 _rest
			read _literal INET_get_dns_2 _rest
			read _literal INET_get_dns_3 _rest
			echo ${INET_get_dns_1} ${INET_get_dns_2} ${INET_get_dns_3}
		} |
		read INET_get_dns_1 INET_get_dns_2 INET_get_dns_3
	echo INET_device[$num]="${INET_dev_node}"
	echo INET_nodename[$num]="${INET_get_nodename}"
	echo INET_YOUR_IP_ADDRESS[$num]="${INET_get_addr}"
	echo INET_SUBNET_MASK[$num]="${INET_get_netmask}"
	echo INET_BROADCAST_ADDRESS[$num]="${INET_get_broadcast}"
	echo INET_NW_PROTOCOL[$num]=silent
	echo INET_NW_TYPE[$num]=silent
	echo local_frame[$num]=add_interface
	echo INET_DNS_SERVER[0]="${INET_get_dns_1}"
	echo INET_DNS_SERVER[1]="${INET_get_dns_2}"
	echo INET_DNS_SERVER[2]="${INET_get_dns_3}"
	echo INET_DOMAIN_NAME="${INET_get_domain}"
	echo INET_ROUTER[0]="${INET_get_router}"
	) 
done
echo INET_NIC_COUNT=$num
