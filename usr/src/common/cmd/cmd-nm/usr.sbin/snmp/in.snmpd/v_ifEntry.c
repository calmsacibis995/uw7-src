#ident	"@(#)v_ifEntry.c	1.5"
#ident	"$Header$"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1989, 1990 INTERACTIVE Systems Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ifndef lint
static char SNMPID[] = "@(#)v_ifEntry.c	1.5";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

/*
 * Revision History:
 *  1/31/89 JDC
 *  Amended copyright notice
 *
 *  Added SUN3 support for ifPhysAddress via ioctl call
 * 
 *  2/4/89 JDC
 *  Changed references from "gotone" to "snmpd"
 *
 *  Added support for ifLastChange
 *
 *  2/6/89 JDC
 *  Added counter on strncmp 2 places where it was missing
 *
 *  4/24/89 JDC
 *  Turned off some of the verbose debug output to cut down on the noise
 *
 *  5/17/89 KWK
 *  changed SUN3 to SUNOS35 and SUN4 to SUNOS40, since that's what they really 
 *  meant and were causing confusion
 *
 *  11/8/89 JDC
 *  Make it print pretty via tgrind
 *
 *  12/08/89 JDC
 *  Fix bug in get_if_entry for sun os 3.5 reported by xxx
 *
 *  01/10/90 JDC
 *  Fix type on ifSpeed in var_ifEntry.c as reported by xxx
 *
 *  05/28/90 JDC
 *  Fix botched fix on ifSpeed in var_ifEntry.c [sigh]
 *
 */

/*
 *  if entries for snmpd
 */

#include <sys/param.h>
#if !defined(SVR3) && !defined(SVR4)
#include <sys/vmmac.h>
#include <machine/pte.h>
#endif
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <nlist.h>
#include <stdio.h>
#if defined(SVR3) || defined(SVR4)
#include <fcntl.h>
#endif
#include <paths.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/time.h>	/* added to support ifLastChange JDC 2/4/89 */
#include <net/if.h>
#if defined(M_UNIX) || defined(SVR4)
#include <sys/dlpi.h>
#include <sys/scodlpi.h>
#include <sys/mdi.h>
#ifndef SVR4
#include <sys/macstat.h>
#else
#include <sys/dlpi_ether.h>
#endif
#endif
#include <netinet/in.h>
#include <sys/ioctl.h>
#ifdef SUNOS35
#include <sys/time.h>
#include <net/nit.h>
#endif
#ifndef SUNOS35
#include <netinet/in_var.h>
#endif
#if defined(SVR3) || defined(SVR4)
#ifdef PSE
#include <common.h>
#endif
#include <sys/stream.h>
#include <sys/stropts.h>
#include <net/route.h>
#include <netinet/ip_str.h>
#endif
#include <string.h>
#include <syslog.h>

#include "snmp.h"
#include "snmpuser.h"
#include "snmpd.h"

#define FALSE 0
#define TRUE 1

VarBindList *get_next_class();
OctetString *get_physaddr_from_name();

#define IF_TYPE 0
#define IF_SPEED 1

#if defined(SVR3) && !defined(M_UNIX)
#define MACIOC(x)       	(('M' << 8) | (x)) 
#define MACIOC_GETADDR		MACIOC(8)	/*get Physical Address*/
#define MACIOC_GETIFSTAT		MACIOC(7)	/*dump statistics*/
#endif

#ifdef SVR4

#define MAX_MAPPINGS 64
struct n_map {
	char ifname[IFNAMSIZ];
	char devname[128];
};
struct n_map ifmap[MAX_MAPPINGS];
static int n_mappings = 0;

static char *
cskip(p)
	char *p;
{
	if (!p)
		return;
	while (*p && *p != ':' && *p != '\n')
		++p;
	if (*p)
		*p++ = '\0';
	return p;
}

char *
svr4_mapname(s)
	char *s;
{
	static FILE *iifp = (FILE *)0;
	char *prefix, *unit, *dev;
	static char buf[256];
	static char tmp[256];
	char *p;
	int i = 0;

	if (n_mappings) {
		while (i < n_mappings) {
			if (strcmp(ifmap[i].ifname, s) == 0)
				return ifmap[i].devname;
			i++;
		}
	}
	if (!iifp) {
		iifp = fopen("/etc/confnet.d/inet/interface", "r");
		if (!iifp)
			return s;
	}
	(void) rewind(iifp);
	while ((fgets(buf, sizeof(buf), iifp)) != (char *)0) {
		if (buf[0] == '#' || buf[0] == '\0')
			continue;

		p = prefix = buf;	/* prefix is first field */
		p = cskip(p);		/* unit is second */
		unit = p;
		p = cskip(p);		/* skip third */
		p = cskip(p);
		dev = p;		/* dev is fourth */
		p = cskip(p);		/* null terminate */

		strcpy(tmp, prefix);
		strcat(tmp, unit);
		if (strcmp(s, tmp) == 0) {
			if (n_mappings < MAX_MAPPINGS - 1) {
				strcpy(ifmap[n_mappings].devname, dev);
				n_mappings++;
			}
			return dev;
		}
	}
	return s;
}

#ifdef _DLPI
struct dlpi_stats res_stats;
char    hwdep_buf[1024];

int
dlpi_get_stats(ifname, ifstatp, dlpi_stats, dlpi_buf, convert)
char *ifname;
struct ifstats *ifstatp;
struct dlpi_stats **dlpi_stats;
char	**dlpi_buf;
int	convert;	/* A value of 1 that indicates we should */
			/* copy stuff to the ifstats structure.  */
{
	dl_get_statistics_req_t *req;
	dl_get_statistics_ack_t *ack;
	struct strbuf ctl, data;
	int flags;
	int fd;
	char stat_buf[DL_GET_STATISTICS_ACK_SIZE  + sizeof(struct dlpi_stats)];
	char devname[32];
 
	strcpy(devname, _PATH_DEV);
	strcat(devname, ifname);
	if ((fd = open(devname, O_RDWR)) < 0 ) {
		return(NOTOK);
	}
 
 
	bzero(stat_buf, sizeof(stat_buf));
	ctl.buf = stat_buf;
	ctl.len = sizeof(dl_get_statistics_req_t);
 
	req = (dl_get_statistics_req_t *) stat_buf;
	req->dl_primitive = DL_GET_STATISTICS_REQ;
 
 
	if (putmsg(fd, &ctl, NULL, RS_HIPRI) < 0) {
		close(fd);
		return(NOTOK);
	}
 
	bzero(stat_buf, sizeof(stat_buf));
	bzero(hwdep_buf, sizeof(hwdep_buf));

	ctl.buf = stat_buf; 
	ctl.maxlen = sizeof(stat_buf);
	ctl.len = 0;
 
	data.buf = hwdep_buf;
	data.maxlen = sizeof(hwdep_buf);
	data.len = 0;
 
	flags=RS_HIPRI;
 
	if (getmsg(fd, &ctl, &data, &flags) < 0) {
		close(fd);
		return(NOTOK);
	}
	close(fd);
 
	ack = (dl_get_statistics_ack_t *) stat_buf;
	if (ack->dl_primitive == DL_GET_STATISTICS_ACK) {
		if (ack->dl_stat_length != sizeof(struct dlpi_stats)) {
			close(fd);
			return(NOTOK);
		}
		memcpy((caddr_t)&res_stats, stat_buf+ack->dl_stat_offset,
			ack->dl_stat_length);
	}
	else {
		close(fd);
		return(NOTOK);
	}

	/* See if we need to convert. */
	if (!convert)  {
		*dlpi_stats = &res_stats;
		*dlpi_buf = hwdep_buf;
		return(OK);
	}
 
	/* Copy stuff we need into the ifstats structure. */
	ifstatp->ifspeed = res_stats.mac_ifspeed;
	ifstatp->ifinoctets = res_stats.mac_rx.mac_octets;
	ifstatp->ifoutoctets = res_stats.mac_tx.mac_octets;
	ifstatp->ifinucastpkts = res_stats.mac_rx.mac_ucast;
	ifstatp->ifinnucastpkts = res_stats.mac_rx.mac_bcast +
				  res_stats.mac_rx.mac_mcast;

	ifstatp->ifoutucastpkts = res_stats.mac_tx.mac_ucast;
	ifstatp->ifoutnucastpkts = res_stats.mac_tx.mac_bcast +
				   res_stats.mac_tx.mac_mcast;

	ifstatp->ifs_ierrors = res_stats.mac_rx.mac_error;
	ifstatp->ifs_oerrors =  res_stats.mac_tx.mac_error;
	ifstatp->ifinunkprotos = res_stats.dl_rxunbound;

	ifstatp->ifoutdiscards = 0;
 
	if (res_stats.mac_media_type == DL_CSMACD) {
		mac_stats_eth_t *eth_stats = (mac_stats_eth_t *) hwdep_buf;

		ifstatp->ifindiscards = eth_stats->mac_xs_coll +
					eth_stats->mac_carrier +
					eth_stats->mac_badlen +
					eth_stats->mac_frame_nosr;
	}
	else {
		/* Token Ring */
		mac_stats_tr_t *tr_stats = (mac_stats_tr_t *) hwdep_buf;
		ifstatp->ifindiscards = tr_stats->mac_receivecongestions +
					tr_stats->mac_badlen +
					tr_stats->mac_frame_nosr;
	}

	return(OK);
}
#endif /* _DLPI */

get_ifstats(ifname, ifsp)
	char *ifname;
	struct ifstats *ifsp;
{
	int fd, ret;
	char devname[32];
	DL_mib_t ms;
	struct strioctl strioc;

	strcpy(devname,svr4_mapname(ifname));
	if ((fd = open(devname, O_RDWR)) >= 0) {
		strioc.ic_len = sizeof(ms);
		strioc.ic_timout = 0;
		strioc.ic_dp = (char *) &ms;
		strioc.ic_cmd = DLIOCGMIB;
		ret = ioctl(fd, I_STR, &strioc);
		close(fd);
		if ((ret < 0) || (strioc.ic_len != sizeof(ms))) {
#ifdef _DLPI
			if (!dlpi_get_stats(ifname,ifsp,NULL,NULL,1)) {
				return 1;
			}
#endif /* _DLPI */
			return 0;
		} else {
			/*
			 * copy the stuff from the USL way to our way
			 */
			ifsp->iftype = ms.ifType;
			ifsp->ifspeed = ms.ifSpeed;
			ifsp->ifinoctets = ms.ifInOctets;
			ifsp->ifoutoctets = ms.ifOutOctets;
			ifsp->ifinucastpkts = ms.ifInUcastPkts;
			ifsp->ifinnucastpkts = ms.ifInNUcastPkts;
			ifsp->ifoutucastpkts = ms.ifOutUcastPkts;
			ifsp->ifoutnucastpkts = ms.ifOutNUcastPkts;
			ifsp->ifindiscards = ms.ifInDiscards;
			ifsp->ifs_ierrors = ms.ifInErrors;
			ifsp->ifs_oerrors = ms.ifOutErrors;
			ifsp->ifinunkprotos = ms.ifInUnknownProtos;
			ifsp->ifoutdiscards = ms.ifOutDiscards;
			return 1;
		}
	} 
	return 0;
}

#endif

int
get_ifcnt(ifcnt)
	u_long *ifcnt;
{
	struct strioctl strioc;
	struct ifreq ifr;
	int fd;

	if ((fd = open("/dev/ip", O_RDONLY)) < 0) {
		return(FALSE);
	}

	strioc.ic_cmd = SIOCGIFNUM;
	strioc.ic_dp = (char *)&ifr;
	strioc.ic_len = sizeof(struct ifreq);
	strioc.ic_timout = -1;

	if (ioctl(fd, I_STR, &strioc) < 0) {
		(void)close(fd);
		return(FALSE);
	}
	*ifcnt = ifr.ifr_nif;
	(void)close(fd);
	return(TRUE);
}

VarBindList *
var_if_num_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  char ifname[16];
  char buffer[256];
  
  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);
  
  /* see if get next and fully qualified */
  if ((type_search == NEXT) && (in_name_ptr->length >= (var_name_ptr->length + 1)))
    return(get_next_class(var_next_ptr));

  if_num = 255;  /* start with a large number of interfaces */
  /* will get back actual number (should be less than this */

  if ((cc = get_ifcnt(&if_num)) == FALSE) {
    return(NULL);
  }

  /*
   * check that the OID exactly matches what we expect
   */
  if (type_search == EXACT && 
      *(in_name_ptr->oid_ptr + (in_name_ptr->length - 1)) != 0)
    return(NULL);
  sprintf(buffer,"ifNumber.0");
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, if_num, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList *
var_if_index_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  char ifname[16];
  char buffer[256];
  struct ifreq_all if_all;
  
  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);
  
  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
  
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;
  
  cc = get_if_all(&if_num, &if_all);
  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }
  
  sprintf(buffer,"ifIndex.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, if_num, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList *
var_if_name_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OctetString *os_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  char ifname[16];
  char buffer[256];
  struct ifreq_all if_all;
  
  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);
  
  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
  
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_all(&if_num, &if_all);
  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }
  
  sprintf(buffer,"ifDescr.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  os_ptr = make_octet_from_text((unsigned char *)if_all.if_entry.if_name);
  vb_ptr = make_varbind(oid_ptr, DisplayString, 0, 0, os_ptr, NULL);
  oid_ptr = NULL;
  os_ptr = NULL;
  return(vb_ptr);
}


VarBindList *
var_if_mtu_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  char ifname[16];
  char buffer[256];
  struct ifreq_all if_all;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;
  cc = get_if_all(&if_num, &if_all);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  sprintf(buffer,"ifMtu.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, if_all.if_entry.if_maxtu, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}



#if defined(SUNOS35) || defined(SVR4) || defined(TCP40)
/*#ifdef BOGUS*/
VarBindList
*var_if_physaddr_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OctetString *os_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  char ifname[16];
  char buffer[256];
  unsigned long speed;
  struct ifreq_all if_all;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_all(&if_num, &if_all);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  /* mach. dependent routine for each interface */
  os_ptr = get_physaddr_from_name(if_all.if_entry.if_name,
					if_all.if_entry.if_dl_version); 

  if (os_ptr == NULL) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);             /* Signal failure */
  }
  sprintf(buffer,"ifPhysAddress.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, OCTET_PRIM_TYPE, 0, 0, os_ptr, NULL);
  os_ptr = NULL;
  oid_ptr = NULL;
  return(vb_ptr);
}
#endif


#ifdef _DLPI
dlpi_physaddr_from_name(hw_addr, devname)
unsigned char *hw_addr;
char *devname;
{
        dl_phys_addr_req_t *phys_addr_req;
        dl_phys_addr_ack_t *phys_addr_ack;
        char phys_addr_buf[DL_PHYS_ADDR_ACK_SIZE + MAC_ADDR_SZ];
	struct strbuf ctl;
	int flags = 0;
	int fd = 0;

	bzero(hw_addr, sizeof(hw_addr));
	bzero(phys_addr_buf, sizeof(phys_addr_buf));

	if ((fd = open(devname, O_RDWR)) < 0) {
		return 0;
	}

	ctl.buf = phys_addr_buf;
	ctl.len = sizeof(dl_phys_addr_req_t);
	ctl.maxlen = 0;
 
	phys_addr_req = (dl_phys_addr_req_t *) phys_addr_buf;
	phys_addr_req->dl_primitive = DL_PHYS_ADDR_REQ;
 
	if (putmsg(fd, &ctl, NULL, 0) < 0) {
		close(fd);
		return 0;
	}
 
	flags = 0;
	bzero(phys_addr_buf, sizeof(phys_addr_buf));
 
	ctl.buf = phys_addr_buf;
	ctl.len = 0;
	ctl.maxlen = sizeof(phys_addr_buf);
 
	if (getmsg(fd, &ctl, NULL, &flags) < 0) {
		close(fd);
		return 0;
	}
	close(fd);
 
	phys_addr_ack = (dl_phys_addr_ack_t *) phys_addr_buf;
 
	if (phys_addr_ack->dl_primitive != DL_PHYS_ADDR_ACK) {
		return 0;
	}
	memcpy(hw_addr, phys_addr_buf + phys_addr_ack->dl_addr_offset,
				phys_addr_ack->dl_addr_length);
	return 1;
}
#endif /* _DLPI */


OctetString *
get_physaddr_from_name(name,dl_version)
    char *name;
    int dl_version;
{
  int fd, ret;
  char devname[32];
  struct strioctl strioc;
  unsigned char eaddr[6];

  strcpy(devname,svr4_mapname(name));

#ifdef _DLPI
        if ((dl_version >= 1) && dlpi_physaddr_from_name(eaddr, devname)) {
                return(make_octetstring(&eaddr[0], 6));
        }
#endif  /* _DLPI */

  if (strcmp(devname, "/dev/loop") == 0)	/* XXX fix loopback */
	goto oy;
  if ((fd = open(devname, O_RDWR)) >= 0) {
    strioc.ic_len = 6;
    strioc.ic_timout = 0;
    strioc.ic_dp = (char *)eaddr;
    strioc.ic_cmd = DLIOCGENADDR;
    ret  = ioctl(fd, I_STR, &strioc);
    close(fd);
    if (ret < 0)
	    return(make_octetstring((unsigned char *)"", 0));
    else
	    return(make_octetstring((unsigned char *)(&eaddr[0]), 6));
  } else
oy:
	  return(make_octetstring((unsigned char *)"", 0));
}

VarBindList *
var_if_adminstatus_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  char buffer[256];
  unsigned long status;
  struct ifreq_all if_all;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_all(&if_num, &if_all);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  if ((if_all.if_entry.if_flags & IFF_UP) == 0) 
    status = 2;			/* down */
  else 
    status = 1;			/* up */

  sprintf(buffer,"ifAdminStatus.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, status, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}


int
var_if_adminstatus_test(var_name_ptr, in_name_ptr, arg, value)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     ObjectSyntax *value;
{
  int if_num = 0;
  int cc;
  char ifname[16];
  char buffer[256];
  unsigned long status;
  struct ifreq_all if_all;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if (in_name_ptr->length != (var_name_ptr->length + 1))
    return(FALSE);

  /* Now find out which interface they are interested in */
  if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  
  cc = get_if_all(&if_num, &if_all);

  if (cc == FALSE) {
    return(FALSE);		/* No such interface */
  }

  if ((value->sl_value < 1) || (value->sl_value > 2)) 
    return(FALSE);

  return(TRUE);
}

int
var_if_adminstatus_set(var_name_ptr, in_name_ptr, arg, value)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     ObjectSyntax *value;
{
  int if_num = 0;
  int cc;
  char ifname[16];
  unsigned long status;
  struct ifreq ifr;
  int s;
  struct strioctl ioc;
  struct ifreq_all if_all;

  struct timeval tv;
  struct timezone tz;
  long timenow;
  int t1, t2;

extern struct timeval global_tv;
extern struct timezone global_tz;

  long old_state;
  long new_state;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if (in_name_ptr->length != (var_name_ptr->length + 1))
    return(FALSE);

  /* Now find out which interface they are interested in */
  if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */

  cc = get_if_all(&if_num, &if_all);
  
  if (cc == FALSE) 
    return(FALSE);		/* No such interface */

  if ((value->sl_value < 1) || (value->sl_value > 2)) 
    return(FALSE);

  sprintf(ifr.ifr_name, "%s", if_all.if_entry.if_name);

  s = open("/dev/ip", O_RDWR);
  if (s < 0) {
    return(FALSE);
  }

  ioc.ic_cmd = SIOCGIFFLAGS;
  ioc.ic_timout = -1;
  ioc.ic_len = sizeof(ifr);
  ioc.ic_dp = (char *) &ifr;
  if (ioctl(s, I_STR, (char *) &ioc) < 0) {
    syslog(LOG_WARNING, gettxt(":169", "SIOCGIFFLAGS: %m.\n"));
    close(s);
    return(FALSE);
  }
  /* save the old state for later compare */
  old_state = ifr.ifr_flags & IFF_UP;

  if (value->sl_value == 1)
    ifr.ifr_flags |= IFF_UP;
  else
    ifr.ifr_flags &= ~IFF_UP;

  new_state = ifr.ifr_flags & IFF_UP;

  ioc.ic_cmd = SIOCSIFFLAGS;
  ioc.ic_timout = -1;
  ioc.ic_len = sizeof(ifr);
  ioc.ic_dp = (char *) &ifr;
  if (ioctl(s, I_STR, (char *) &ioc) < 0) {
    syslog(LOG_WARNING, gettxt(":170", "SIOCSIFFLAGS: %m.\n"));
    close(s);
    return(FALSE);
  }

/*
	save the current time in timeticks into the array which holds
	the time of the last change 
	zero whereas interfaces are numbered from 1
*/
  /*  first, get the current time */
   gettimeofday(&tv, &tz);

  /*  now, convert to timeticks */
    t1 = ((tv.tv_sec - global_tv.tv_sec) * 100);
    t2 = ((tv.tv_usec - global_tv.tv_usec) / 10000);
    timenow = ((tv.tv_sec - global_tv.tv_sec) * 100) +
      ((tv.tv_usec - global_tv.tv_usec) / 10000);

  close(s);
  return(TRUE);
}




VarBindList *
var_if_operstatus_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  char ifname[16];
  char buffer[256];
  unsigned long status;
  struct ifreq_all if_all;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_all(&if_num, &if_all);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }


  if ((if_all.if_entry.if_flags & IFF_UP) == 0) 
    status = 2;			/* down */
  else 
    status = 1;

  sprintf(buffer,"ifOperStatus.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, status, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList *
var_if_up_time_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int number_of_interfaces_found;
  int cc;
  char ifname[16];
  char buffer[256];
  unsigned long status;
  struct ifreq_all if_all;


  struct timeval tv;
  struct timezone tz;
  long lasttime;
  int t1, t2;

extern struct timeval global_tv;
extern struct timezone global_tz;

  /* Check that an exact search only has exactly one sub-id field */
  /* that field to communicate if number of probe */
  /* if it doesn't, it isn't exact */
  /* note that we are assuming that the number of interfaces will fit in */
  /* a single byte */

  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after
var_name */
   
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  /* now, find out how many interfaces we have */
  number_of_interfaces_found = 255;

  /*  note:  ok not to check returned value here  */
  cc = get_ifcnt(&number_of_interfaces_found);

  /* if we have too many, punt */
  if ((number_of_interfaces_found > MAXINTERFACES) || 
	(if_num <= 0) ||
	(if_num > number_of_interfaces_found)) {
      if (number_of_interfaces_found > MAXINTERFACES) {

	syslog(LOG_WARNING, gettxt(":171", "var_if_up_time_get: too many interfaces -- recompile with a larger number for MAXINTERFACES.\n"));
    }
    if (type_search == EXACT) {
       return(NULL);
    }
    else {
       return(get_next_class(var_next_ptr));
    }
  }
  else {
     struct timeval tv;
     cc = get_if_all(&if_num, &if_all);

     tv = *((struct timeval *)&if_all.if_entry.if_lastchange);
     /*
      * lasttime is in HZ -- convert to hundredths and subtract global_tv
      */
     lasttime = ((tv.tv_sec - global_tv.tv_sec) * 100) +
	     ((tv.tv_usec - global_tv.tv_usec) / 10000);
     if (lasttime < 0)
	     lasttime = 0;
    sprintf(buffer, "ifLastChange.%d", if_num);
    oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
    vb_ptr = make_varbind(oid_ptr, TIME_TICKS_TYPE, 0, lasttime, NULL, NULL);
    oid_ptr = NULL;
    return(vb_ptr);
  }
}



VarBindList *
var_if_inerrors_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  struct ifstats ifstats;
  char ifname[16];
  char buffer[256];
  struct ifreq_all if_all;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_all(&if_num, &if_all);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  sprintf(buffer,"ifInErrors.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, if_all.if_stats.ifs_ierrors, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList *
var_if_outerrors_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  char ifname[16];
  char buffer[256];
  struct ifreq_all if_all;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_all(&if_num, &if_all);
  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  sprintf(buffer,"ifOutErrors.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, if_all.if_stats.ifs_oerrors, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList *
var_if_outqlen_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  char ifname[16];
  char buffer[256];
  struct ifreq_all if_all;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_all(&if_num, &if_all);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  sprintf(buffer,"ifOutQLen.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, GAUGE_TYPE, if_all.if_outqlen, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

/* Return the generic if stats for the IP interface listed. */
int
get_if_all(if_num, if_all)
	int *if_num;
	struct ifreq_all *if_all;
{
	struct strioctl strioc;
	int fd;

	if ((fd = open("/dev/ip", O_RDONLY)) < 0)
		return(FALSE);

	if_all->if_entry.if_index = *if_num;
	strioc.ic_cmd = SIOCGIFALL;
	strioc.ic_dp = (char *)if_all;
	strioc.ic_len = sizeof (struct ifreq_all);
	strioc.ic_timout = -1;

	if (ioctl(fd, I_STR, &strioc) < 0) {
		(void)close(fd);
		return(FALSE);
	}
	(void)close(fd);
	if (!(if_all->if_entry.if_flags & (IFF_LOOPBACK | IFF_POINTOPOINT))) {
		(void)get_ifstats(if_all->if_entry.if_name, &if_all->if_stats);
	}
	*if_num = if_all->if_entry.if_index;
	return(TRUE);
}

VarBindList *
var_if_indiscards_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  char buffer[256];
  struct ifreq_all if_all;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_all(&if_num, &if_all);
  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  sprintf(buffer,"ifInDiscards.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, if_all.if_stats.ifindiscards, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList *
var_if_innucast_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  char buffer[256];
  struct ifreq_all if_all;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_all(&if_num, &if_all);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  sprintf(buffer,"ifInNUcastPkts.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, if_all.if_stats.ifinnucastpkts, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}


VarBindList *
var_if_inoctets_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  char buffer[256];
  struct ifreq_all if_all;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_all(&if_num, &if_all);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  sprintf(buffer,"ifInOctets.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, if_all.if_stats.ifinoctets, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList *
var_if_inucast_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  char buffer[256];
  struct ifreq_all if_all;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_all(&if_num, &if_all);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  sprintf(buffer,"ifInUcastPkts.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, if_all.if_stats.ifinucastpkts, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList *
var_if_inunkprotos_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  char buffer[256];
  struct ifreq_all if_all;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_all(&if_num, &if_all);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  sprintf(buffer,"ifInUnknownProtos.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, if_all.if_stats.ifinunkprotos, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList
*var_if_outdiscards_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  char buffer[256];
  struct ifreq_all if_all;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_all(&if_num, &if_all);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  sprintf(buffer,"ifOutDiscards.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, if_all.if_stats.ifoutdiscards, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList *
var_if_outnucast_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  char buffer[256];
  struct ifreq_all if_all;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_all(&if_num, &if_all);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  sprintf(buffer,"ifOutNUcastPkts.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, if_all.if_stats.ifoutnucastpkts, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList *
var_if_outoctets_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  char buffer[256];
  struct ifreq_all if_all;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_all(&if_num, &if_all);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  sprintf(buffer,"ifOutOctets.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, if_all.if_stats.ifoutoctets, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList *
var_if_outucast_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  char buffer[256];
  struct ifreq_all if_all;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_all(&if_num, &if_all);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  sprintf(buffer,"ifOutUcastPkts.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, if_all.if_stats.ifoutucastpkts, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList *
var_if_speed_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  char buffer[256];
  struct ifreq_all if_all;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_all(&if_num, &if_all);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  sprintf(buffer,"ifSpeed.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, GAUGE_TYPE, if_all.if_stats.ifspeed, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList *
var_if_type_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  char buffer[256];
  struct ifreq_all if_all;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_all(&if_num, &if_all);
  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  sprintf(buffer,"ifType.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, if_all.if_stats.iftype, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

struct ziftype {
	int type;
	char *oid;
};

struct ziftype iftypes[] = {
        IFOTHER,      "0.0",                 /* Other */
        IFDDN_X25,    "transmission.5",
        IFRFC877_X25, "transmission.5",
        IFETHERNET_CSMACD,      "transmission.7",
        IFISO88023_CSMACD,   "transmission.7",
        IFISO88024,   "transmission.8",
        IFISO88025_TOKENRING,   "transmission.9",
        IFISO88026,   "0.0",
        IFSTARLAN,    "transmission.7",
        IFFDDI,       "transmission.15",
        IFLAPB,       "transmission.16",
        IFT1,         "transmission.18",
        IFCEPT,       "transmission.18",
        IFPPP,        "transmission.23",
        IFLOOPBACK,       "0.0",
        IFSLIP,       "0.0",
        IFT3,         "transmission.30",
        IFSIP,        "transmission.31",
        IFFRAME,      "transmission.32"
};

int n_iftypes = sizeof(iftypes) / sizeof(struct ziftype);

VarBindList *
var_if_specific_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  OID oidvalue_ptr;
  int if_num = 0;
  int cc;
  char buffer[256];
  int i;
  char *mibp;
  struct ifreq_all if_all;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_all(&if_num, &if_all);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }
  if (if_all.if_stats.iftype < IFOTHER)
	if_all.if_stats.iftype = IFOTHER;
  for (i = 0; i < n_iftypes; i++) {
	if (iftypes[i].type == if_all.if_stats.iftype) {
		mibp = iftypes[i].oid;
		break;
	}
  }
  if (i > n_iftypes)
	mibp = iftypes[0].oid;
  sprintf(buffer,"ifSpecific.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  oidvalue_ptr = make_obj_id_from_dot((unsigned char *)mibp);
  vb_ptr = make_varbind(oid_ptr, OBJECT_ID_TYPE, 0, 0, NULL, oidvalue_ptr);
  oid_ptr = NULL;
  oidvalue_ptr = NULL;
  return(vb_ptr);
}

