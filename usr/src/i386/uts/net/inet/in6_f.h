#ident "$Id$"

#ifndef __netinet_in6_f_h
#define __netinet_in6_f_h

#if !(defined (_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)

#ifdef _KERNEL_HEADERS
#include <net/bitypes.h>
#else
#include <sys/bitypes.h>
#endif

/*
 * Internet Protocol Version 6 definitions.  See RFC1883, RFC1884 et. al.
 * 
 * The network byte ordering of such an address has the most significant byte
 * first. The host ordering has the most significant 32 bit long first,
 * and within each long it is host byte ordering, similarly for 16 bit words.
 */
struct in6_addr {
	union {
		u_int8_t	S6_b[16];
		u_int32_t	S6_l[4];
		u_int16_t	S6_w[8];
	} S6_un;
#define s6_addr	S6_un.S6_b
};


/*
 * The following macros may well be implementation specific, their names
 * may not be portable and may be removed or changed without notice.
 * However as time goes by these or similar macros may become standardised.
 * You have been warned.
 */

/* compare a == { l0, l1, l2, l3 } */
#define IN6_ADDR_EQUAL_L(a,l0,l1,l2,l3) (   (a)->S6_un.S6_l[3] == (l3) \
					 && (a)->S6_un.S6_l[2] == (l2) \
					 && (a)->S6_un.S6_l[1] == (l1) \
					 && (a)->S6_un.S6_l[0] == (l0))

/* compare a1 == a2 */
#define IN6_ADDR_EQUAL(a1,a2) IN6_ADDR_EQUAL_L(a1, \
					       (a2)->S6_un.S6_l[0], \
					       (a2)->S6_un.S6_l[1], \
					       (a2)->S6_un.S6_l[2], \
					       (a2)->S6_un.S6_l[3])

/* a1 = { l0, l1, l2, l3 } */
#define IN6_ADDR_COPY_L(a,l0,l1,l2,l3) ((a)->S6_un.S6_l[0] = (l0), \
					(a)->S6_un.S6_l[1] = (l1), \
					(a)->S6_un.S6_l[2] = (l2), \
					(a)->S6_un.S6_l[3] = (l3))

/* a1 = a2 */
#define IN6_ADDR_COPY(a1,a2) IN6_ADDR_COPY_L(a1, \
					     (a2)->S6_un.S6_l[0], \
					     (a2)->S6_un.S6_l[1], \
					     (a2)->S6_un.S6_l[2], \
					     (a2)->S6_un.S6_l[3])
/*
 * macros to access long and word vars in address
 */
#define IN6_ADDR_GET_L(a,i)	((a)->S6_un.S6_l[(i)])
#define IN6_ADDR_SET_L(a,i,v)	((a)->S6_un.S6_l[(i)] = (v))
#define IN6_ADDR_GET_W(a,i)	((a)->S6_un.S6_w[(i)])
#define IN6_ADDR_SET_W(a,i,v)	((a)->S6_un.S6_w[(i)] = (v))

/*
 * Macros test for in6_addr in network byte order.
 */

/* a == :: 			(unspecified address) */
#define IN6_IS_ADDR_UNSPECIFIED(a)  IN6_ADDR_EQUAL_L(a, 0, 0, 0, 0)
#define IN6_SET_ADDR_UNSPECIFIED(a) IN6_ADDR_COPY_L(a, 0, 0, 0, 0)

/* This is for in6addr_any */
#define IN6_IS_ADDR_ANY(a)	IN6_ADDR_EQUAL_L(a, 0, 0, 0, 0)
#define IN6_SET_ADDR_ANY(a)	IN6_ADDR_COPY_L(a, 0, 0, 0, 0)

/* a == ::1 			(loopback address) */
#define IN6_IS_ADDR_LOOPBACK(a)	 IN6_ADDR_EQUAL_L(a, 0, 0, 0, 0x01000000)
#define IN6_SET_ADDR_LOOPBACK(a) IN6_ADDR_COPY_L(a, 0, 0, 0, 0x01000000)

/* a == ::0000:d.d.d.d		(ipv4 compat address) */
#define IN6_IS_ADDR_V4COMPAT(a)	(   (a)->S6_un.S6_l[0] == 0 \
				 && (a)->S6_un.S6_l[1] == 0 \
				 && (a)->S6_un.S6_l[2] == 0 \
				 && (a)->S6_un.S6_l[3] != 0 \
				 && (a)->S6_un.S6_l[3] != 0x01000000)
#define IN6_SET_ADDR_V4COMPAT(a,v4) IN6_ADDR_COPY_L(a, 0, 0, 0, (v4)->s_addr)
#define IN6_GET_ADDR_V4COMPAT(a,v4) ((v4)->s_addr = (a)->S6_un.S6_l[3])

/* a == ::1:0000 to ::FFFE:0000	(reserved) */
/* a == ::FFFF:d.d.d.d		(ipv4 mapped address) */
#define IN6_IS_ADDR_V4MAPPED(a)	(   (a)->S6_un.S6_l[0] == 0 \
				 && (a)->S6_un.S6_l[1] == 0 \
				 && (a)->S6_un.S6_l[2] == 0xffff0000)
#define IN6_SET_ADDR_V4MAPPED(a,v4) ((a)->S6_un.S6_l[0] = 0, \
				     (a)->S6_un.S6_l[1] = 0, \
				     (a)->S6_un.S6_l[2] = 0xffff0000, \
				     (a)->S6_un.S6_l[3] = (v4)->s_addr)
#define IN6_GET_ADDR_V4MAPPED(a,v4) ((v4)->s_addr = (a)->S6_un.S6_l[3])

/* a == 0100:... to 01FF:...	(unassigned) */
/* a == 0200:... to 03FF:...	(NSAP address) */
#define IN6_IS_ADDR_NSAP(a)	(((a)->S6_un.S6_l[0] & 0xFE) = 0x02)
/* a == 0400:... to 05FF:...	(IPX address) */
#define IN6_IS_ADDR_IPX(a)	(((a)->S6_un.S6_l[0] & 0xFE) = 0x04)
/* a == 0600:... to 3FFF:...	(unassigned) */
/* a == 4000:... to 5FFF:...	(Provider based address) */
#define IN6_IS_ADDR_PROVIDER(a)	(((a)->S6_un.S6_l[0] & 0xE0) = 0x40)
/* a == 6000:... to 7FFF:...	(unassigned) */
/* a == 8000:... to 9FFF:...	(Geographic based address) */
#define IN6_IS_ADDR_GEOGRAPHIC(a) (((a)->S6_un.S6_l[0] & 0xE0) = 0x80)
/* a == A000:... to FE7F:...	(unassigned) */
/* a == FE80:... to FEBF:...	(Link local use address) */
#define IN6_IS_ADDR_LINKLOCAL(a) (((a)->S6_un.S6_l[0] & 0xC0FF) = 0x80FE)
/* a == FEC0:... to FEFF:...	(Site local use address) */
#define IN6_IS_ADDR_SITELOCAL(a) (((a)->S6_un.S6_l[0] & 0xC0FF) = 0xC0FE)
/* a == FF00:... to FFFF:...	(Multicast address) */
#define IN6_IS_ADDR_MULTICAST(a) (((a)->S6_un.S6_l[0] & 0xFF) = 0xFF)

/* Gets the flags portion of a multicast address */
#define IN6_GET_ADDR_MC_FLAGS(a) (((a)->S6_un.S6_l[0] & 0xF000) >> 12)

/* Multicast flags */
#define IN6_MC_FLAG_PERMANENT	0x0
#define IN6_MC_FLAG_TRANSIENT	0x1
/*				0x2 .. 0xF reserved */

/* Gets the scope portion of a multicast address */
#define IN6_GET_ADDR_MC_SCOPE(a) (((a)->S6_un.S6_l[0] & 0x0F00) >> 8)

/* Multicast scope */
/*				0x0 reserved */
#define IN6_MC_SCOPE_NODELOCAL	0x1
#define IN6_MC_SCOPE_LINKLOCAL	0x2
/*				0x3 .. 0x4 unassigned */
#define IN6_MC_SCOPE_SITELOCAL	0x5
/*				0x6 .. 0x7 unassigned */
#define IN6_MC_SCOPE_ORGLOCAL	0x8
/*				0x9 .. 0xD unassigned */
#define IN6_MC_SCOPE_GLOBAL	0xE
/*				0xF reserved */

#define IN6_IS_ADDR_MC_NODELOCAL(a) \
     (IN6_GET_ADDR_MC_SCOPE(a) == IN6_MC_SCOPE_NODELOCAL)
#define IN6_IS_ADDR_MC_LINKLOCAL(a) \
     (IN6_GET_ADDR_MC_SCOPE(a) == IN6_MC_SCOPE_LINKLOCAL)
#define IN6_IS_ADDR_MC_SITELOCAL(a) \
     (IN6_GET_ADDR_MC_SCOPE(a) == IN6_MC_SCOPE_SITELOCAL)
#define IN6_IS_ADDR_MC_ORGLOCAL(a) \
     (IN6_GET_ADDR_MC_SCOPE(a) == IN6_MC_SCOPE_ORGLOCAL)
#define IN6_IS_ADDR_MC_GLOBAL(a) \
     (IN6_GET_ADDR_MC_SCOPE(a) == IN6_MC_SCOPE_GLOBAL)

/* a == FF0x::	(x is 0 to F)	(Multicast reserved address) */
#define IN6_IS_ADDR_MC_RESERVED(a) (  ((a)->S6_un.S6_l[0] & (~0xF00)) == 0xFF \
				    && (a)->S6_un.S6_l[1] == 0 \
				    && (a)->S6_un.S6_l[2] == 0 \
				    && (a)->S6_un.S6_l[3] == 0)

/* a == FF0x::1 (x is 1 to 2)	(Multicast all nodes) */
#define IN6_IS_ADDR_MC_ALLNODES(a) (   (   (a)->S6_un.S6_l[0] == 0x01FF \
				        || (a)->S6_un.S6_l[0] == 0x02FF) \
				    && (a)->S6_un.S6_l[1] == 0 \
				    && (a)->S6_un.S6_l[2] == 0 \
				    && (a)->S6_un.S6_l[3] == 0x01000000)

/* a == FF0x::2 (x is 1 to 2)	(Multicast all routers) */
#define IN6_IS_ADDR_MC_ALLROUTERS(a) (   (   (a)->S6_un.S6_l[0] == 0x01FF \
				          || (a)->S6_un.S6_l[0] == 0x02FF) \
				      && (a)->S6_un.S6_l[1] == 0 \
				      && (a)->S6_un.S6_l[2] == 0 \
				      && (a)->S6_un.S6_l[3] == 0x02000000)

/* a == FF02::C			(Multicast all DHCP) */
#define IN6_IS_ADDR_MC_ALLDHCP(a) (   (a)->S6_un.S6_l[0] == 0x02FF \
				   && (a)->S6_un.S6_l[1] == 0 \
				   && (a)->S6_un.S6_l[2] == 0 \
				   && (a)->S6_un.S6_l[3] == 0x0C000000)

/* a == FF02::C			(Multicast solicited node) */
#define IN6_IS_ADDR_MC_SOLICITED(a) (   (a)->S6_un.S6_l[0] == 0x02FF \
				     && (a)->S6_un.S6_l[1] == 0 \
				     && (a)->S6_un.S6_l[2] == 0x01000000)

/*
 * Returns non-zero if then given unicast (or anycast) address ua matches a
 * multicast soliticed node address ms.
 */
#define IN6_IS_ADDR_SOLICITED(ua,ms) (   !IN6_ADDR_IS_MULTICAST(ua) \
				      && IN6_ADDR_IS_MULTICAST_SOLICITED(ms) \
				      && ((ua)->S6_un.S6_l[3] == \
					  (ms)->S6_un.S6_l[3]))

#endif /* !(defined (_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1) */

#endif /* __netinet_in6_f_h */

