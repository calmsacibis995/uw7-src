#ident	"@(#)proxy.c	1.2"
#ident	"$Header$"
extern char *gettxt();
#ident "@(#) $Id: proxy.c,v 1.8 1994/12/16 15:14:06 neil Exp"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1995 Legent Corporation
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
/*      SCCS IDENTIFICATION        */
#ifndef _KMEMUSER
#define _KMEMUSER
#endif
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/termiox.h>
#include <stdio.h>
#include <sys/errno.h>
#include <string.h>
#include <fcntl.h>
#include <stropts.h>
#include <sys/stream.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>

#ifndef _KERNEL
#define _KERNEL
#include <sys/ksynch.h>
#undef _KERNEL
#else
#include <sys/ksynch.h>
#endif

#include <netinet/ppp_lock.h>
#include <netinet/ppp.h>
#include <sys/dlpi.h>
#include "paths.h"
#include <errno.h>
#include "pppd.h"
#include <syslog.h>
#include <sys/ksym.h>
#include <sys/elf.h>
#include <nlist.h>
#include <netinet/ip_str.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/dlpi_ether.h>
#include <pfmt.h>
#include <locale.h>
#include <unistd.h>

#include "pppu_proto.h"


#define MAX_MAC_ADDR_SZ  128 /* A little over kill */

/* 
 * Globals
 */
int read_symbols = 0;
int kmem = -1;

/*
 * add proxy arp entry 
 * return
 *    -1: fail
 *    0: no need to add proxy entry 
 *    1: add proxy arp entry
 */
int
proxyadd(fd, dst, src)
	int	fd;
	ulong	dst;
	ulong	src;
{
  struct ifreq_all if_all;
  u_char        mac_addr[MAX_MAC_ADDR_SZ];
  ulong         laddr, mask;
  int           ret = 0;
  int index;
  int len;



  for (index = 1; get_if_all(&index, &if_all); index++) {

    if ((if_all.if_entry.if_flags & IFF_POINTOPOINT) ||
        (if_all.if_entry.if_flags & IFF_LOOPBACK)) {
      continue;
    }
    
    
    mask = ((struct sockaddr_in *)&if_all.addrs[0].netmask)->sin_addr.s_addr;
    laddr = ((struct sockaddr_in *)&if_all.addrs[0].addr)->sin_addr.s_addr;
    
    /* mask is in network order */
    mask = ntohl(mask);
    
    if ((laddr & mask) == (dst & mask) || (laddr & mask) == (src & mask)) {
      int     src_set = 0;
      

      if ((len = get_macaddr(if_all.if_entry.if_name, (char *) mac_addr, sizeof(mac_addr))) == -1) {
	/* 
	 * Assume an SVR4 style ethernet interface, and try it again.
	 */
	len = 6;
	if (get_physaddr_from_name(if_all.if_entry.if_name, mac_addr, sizeof(mac_addr))) {
	  ppp_syslog(LOG_WARNING, gettxt(":329", "Unable to get mac address for %s: %m"), if_all.if_entry.if_name);
	  continue;
	}
      }

      if ((laddr & mask) == (src & mask)) {
	if (proxy_arp_set(src, (char *) mac_addr, len) < 0)
	  return -1;
	else
	  src_set = 1;
	
      }
      if ((laddr & mask) == (dst & mask)) {
	if (proxy_arp_set(dst, (char *) mac_addr, len) < 0) {
	  (void) deletearp(src);
	  return -1;
	}
      }
      ret = 1;
    }
  }

  return(ret);
}
    
/*
 * Add a proxy arp entry.
 * 
 * Parameters:
 *  inet_addr: IP address.
 *  mac_addr:  Mac address
 *  len:       length of mac_addr in btyes
 *
 * Returns:
 *  0 success
 *  -1 failure
 */

int 
proxy_arp_set(inet_addr, mac_addr, len)
     ulong inet_addr;
     char *mac_addr;
     int len;
{
  struct        arpreq  ar;
  struct        sockaddr_in *sin;
  int   s;
  struct        strioctl strioc;
  
  if (len > sizeof(ar.arp_ha.sa_data)) {
    ppp_syslog(LOG_WARNING,gettxt(":330", "proxy_arp_set: Address length too long"));
    return(-1);
  }

  memset((char *) & ar, 0, sizeof ar);
  sin = (struct sockaddr_in *) & ar.arp_pa;
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = inet_addr;
  memcpy((u_char *) ar.arp_ha.sa_data, mac_addr, len);
  ar.arp_flags = ATF_PERM | ATF_PUBL;

  s = open(_PATH_ARP, 0);
  if (s < 0) {
    ppp_syslog(LOG_WARNING, gettxt(":331", "proxy_arp_set(): open(%s) failed: %m"), 
		_PATH_ARP);
    close(s);
    return -1;
  }
  strioc.ic_cmd = SIOCSARP;
  strioc.ic_timout = -1;
  strioc.ic_len = sizeof(struct arpreq);
  strioc.ic_dp = (char *) &ar;
  if (ioctl(s, I_STR, (long) & strioc) < 0) {
    ppp_syslog(LOG_WARNING, gettxt(":332", "proxy_arp_set(): ioctl(SIOCSARP) failed: %m"));
    close(s);
    return -1;
  }
  close(s);
  
  ppp_syslog(LOG_INFO, gettxt(":333", "Added proxy arp entry: %s <--> %s"), 
	     inet_ntoa(inet_addr), mac_sprintf((u_char *) mac_addr, len));
  return 0;

}



int 
deletearp(inet_addr)
     ulong inet_addr;
{
  struct        arpreq  ar;
  struct        sockaddr_in *sin;
  int   s;
  struct        strioctl strioc;

  memset((char *) & ar, 0, sizeof ar);
  sin = (struct sockaddr_in *) & ar.arp_pa;
  sin->sin_family = AF_INET;
  sin->sin_addr.s_addr = inet_addr;

  s = open(_PATH_ARP, 0);
  if (s < 0) {
    ppp_syslog(LOG_WARNING, gettxt(":334", "deletearp(): open(%s) failed: %m"), 
		_PATH_ARP);
  }
  strioc.ic_cmd = SIOCDARP;
  strioc.ic_timout = -1;
  strioc.ic_len = sizeof(struct arpreq);
  strioc.ic_dp = (char *) &ar;
  if (ioctl(s, I_STR, (long) & strioc) < 0) {
    ppp_syslog(LOG_WARNING, gettxt(":335", "deletearp(): ioctl(SIOCDARP) failed"));
    close(s);
    return -1;
  }
  close(s);
  return 0;

}

#define MAX_MAPPINGS 64
struct n_map {
        char ifname[IFNAMSIZ];
        char devname[128];
};
struct n_map ifmap[MAX_MAPPINGS];
static int n_mappings = 0;

char *
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

                p = prefix = buf;       /* prefix is first field */
                p = cskip(p);           /* unit is second */
                unit = p;
                p = cskip(p);           /* skip third */
                p = cskip(p);
                dev = p;                /* dev is fourth */
                p = cskip(p);           /* null terminate */

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


/*
 * Return ethernet addr for interface.
 * 
 * Parameters:
 *   name : interface name
 *   addr: Buffer to hold mac address
 *   len:  Length of addr in bytes
 */

int
get_physaddr_from_name(name, addr, len)
    char *name;
    u_char *addr;
    int len;
{
  int fd, ret;
  char devname[32];
  struct strioctl strioc;

  if (len < 6) {
    ppp_syslog(LOG_WARNING, gettxt(":337", "get_physaddr_from_name: buffer too small"));
    return(-1);
  }
      
  strcpy(devname,svr4_mapname(name));
  if (strcmp(devname, "/dev/loop") == 0)        /* XXX fix loopback */
        goto oy;
  if ((fd = open(devname, O_RDWR)) >= 0) {
    strioc.ic_len = 6;
    strioc.ic_timout = 0;
    strioc.ic_dp = (char *)addr;
    strioc.ic_cmd = DLIOCGENADDR;
    ret  = ioctl(fd, I_STR, &strioc);
    close(fd);
    if (ret < 0)
      return(-1);
    else
      return(0);
  } else
oy:
    return(-1);
}




/*
 * Get mac address
 * return
 *    -1: fail
 *    otherwise mac addr length is returned.
 */
int
get_macaddr(name, addr, len)
	char *name;
	char *addr;
        int len;
{
	int	fd;
	int r;
	char iface[40];
	union DL_primitives dl;
	struct strbuf ctl;
	char *pt;
	int flags;
	int addr_sz;

#ifdef _DLPI
	dl_phys_addr_req_t *phys_addr_req;
	dl_phys_addr_ack_t *phys_addr_ack;


	char phys_addr_buf[DL_PHYS_ADDR_ACK_SIZE + MAX_MAC_ADDR_SZ];
#endif  /* _DLPI */

#ifndef SCO
	strcpy(iface, _PATH_DEV);
	strcat(iface, name);
#endif

	{
	  char *p;
	  p = svr4_mapname(name);
	  if (p == name) {
	    return -1;
	  }
	  strcpy(iface, p);
	}
	if ((fd = open(iface, O_RDWR, 0)) < 0) {
		ppp_syslog(LOG_WARNING,gettxt(":338", "Can't open %s: %m"), iface);	
		return -1;
	}



#ifdef _DLPI

	/*
	 * Otherwise, try using DLPI primitives.
	 * We really should try these first, but
	 * some of the 3.1.1 LLI drivers (like i3B0)
	 * are confused, and won't nak the request.
	 */

	memset(phys_addr_buf, 0, sizeof(phys_addr_buf));
	
	ctl.buf = phys_addr_buf;
	ctl.len = sizeof(dl_phys_addr_req_t);
	ctl.maxlen = 0;
	
	phys_addr_req = (dl_phys_addr_req_t *) phys_addr_buf;
	phys_addr_req->dl_primitive = DL_PHYS_ADDR_REQ;
	phys_addr_req->dl_addr_type = DL_CURR_PHYS_ADDR;

	if (putmsg(fd, &ctl, NULL, 0) < 0) {
		close(fd);
		ppp_syslog(LOG_WARNING,gettxt(":340", "putmsg fail"));
		return -1;
	}

	flags = 0;
	memset(phys_addr_buf, 0, sizeof(phys_addr_buf));
	
	ctl.buf = phys_addr_buf;
	ctl.len = 0;
	ctl.maxlen = sizeof(phys_addr_buf);

	if (getmsg(fd, &ctl, NULL, &flags) < 0) {
		ppp_syslog(LOG_WARNING, gettxt(":341", "getmsg fail"));
		goto info_get;
	}

	phys_addr_ack = (dl_phys_addr_ack_t *) phys_addr_buf;
	
	if (phys_addr_ack->dl_primitive != DL_PHYS_ADDR_ACK) {
	  close (fd);
	  return(-1);
	}
	
	if (phys_addr_ack->dl_addr_length <= len) 
	  memcpy(addr, phys_addr_buf + phys_addr_ack->dl_addr_offset,
		 (addr_sz = phys_addr_ack->dl_addr_length));
	else {
	  ppp_syslog(LOG_WARNING,gettxt(":342", "get_macaddr: supplied buffer too small"));
	  return(-1);
	}

#else
	close(fd);
	return (-1);

#endif /* _DLPI */

info_get:
	close(fd);
	return(addr_sz);
}


/*
 * char *mac_sprintf(ether_addr_t *addr)
 *      Convert Mac address to printable (loggable) representation.
 *
 * Calling/Exit State:
 *      Arguments:
 *        addr  mac address to convert into ascii
 *        len   len of 'addr' in bytes

 *      Possbile Returns:
 *        Always returns a static buffer with mac address in
 *        ASCII form terminated by '\0';
 */
char *
mac_sprintf(addr, len)
     u_char *addr;
     int len;
{
        int i;
        unsigned char *ap = (unsigned char *)addr;
        static char etherbuf[MAX_MAC_ADDR_SZ];
        char *cp = etherbuf;
        static char digits[] = "0123456789abcdef";

        for (i = 0; i < len; i++) {
                if (*ap > 15)
                        *cp++ = digits[*ap >> 4];
                *cp++ = digits[*ap++ & 0xf];
                *cp++ = ':';
        }
        *--cp = 0;
        return etherbuf;
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
		return(0);

	if_all->if_entry.if_index = *if_num;
	strioc.ic_cmd = SIOCGIFALL;
	strioc.ic_dp = (char *)if_all;
	strioc.ic_len = sizeof (struct ifreq_all);
	strioc.ic_timout = -1;

	if (ioctl(fd, I_STR, &strioc) < 0) {
		(void)close(fd);
		return(0);
	}
	(void)close(fd);
	*if_num = if_all->if_entry.if_index;
	return(1);
}
