#ident	"@(#)sockaddr.h	1.3"
#ident	"$Header$"

/*
 * Public Release 3
 * 
 * $Id$
 */

/*
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1996, 1997 The Regents of the University of Michigan
 * All Rights Reserved
 *  
 * Royalty-free licenses to redistribute GateD Release
 * 3 in whole or in part may be obtained by writing to:
 * 
 * 	Merit GateDaemon Project
 * 	4251 Plymouth Road, Suite C
 * 	Ann Arbor, MI 48105
 *  
 * THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS OF THE
 * UNIVERSITY OF MICHIGAN AND MERIT DO NOT WARRANT THAT THE
 * FUNCTIONS CONTAINED IN THE SOFTWARE WILL MEET LICENSEE'S REQUIREMENTS OR
 * THAT OPERATION WILL BE UNINTERRUPTED OR ERROR FREE. The Regents of the
 * University of Michigan and Merit shall not be liable for
 * any special, indirect, incidental or consequential damages with respect
 * to any claim by Licensee or any third party arising from use of the
 * software. GateDaemon was originated and developed through release 3.0
 * by Cornell University and its collaborators.
 * 
 * Please forward bug fixes, enhancements and questions to the
 * gated mailing list: gated-people@gated.merit.edu.
 * 
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1990,1991,1992,1993,1994,1995 by Cornell University.
 *     All rights reserved.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT
 * LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * GateD is based on Kirton's EGP, UC Berkeley's routing
 * daemon	 (routed), and DCN's HELLO routing Protocol.
 * Development of GateD has been supported in part by the
 * National Science Foundation.
 * 
 * ------------------------------------------------------------------------
 * 
 * Portions of this software may fall under the following
 * copyrights:
 * 
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms are
 * permitted provided that the above copyright notice and
 * this paragraph are duplicated in all such forms and that
 * any documentation, advertising materials, and other
 * materials related to such distribution and use
 * acknowledge that the software was developed by the
 * University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote
 * products derived from this software without specific
 * prior written permission.  THIS SOFTWARE IS PROVIDED
 * ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */


typedef union {
    /* Generic address used only for referencing length and family */
    struct {
	byte	ga_len;
	byte	ga_family;
	byte	ga_data[1];
    } a;
    /* Unix domain address */
    struct {
	byte	gun_len;
	byte	gun_family;
	char	gun_path[1];
    } un;
#ifdef	PROTO_INET
    /* IP address.  Note that sin_zero has been removed */
    struct {
	byte	gin_len;
	byte	gin_family;
	u_int16	gin_port;
	struct in_addr gin_addr;
    } in;
#endif	/* PROTO_INET */
#ifdef	SOCKADDR_DL
    struct {
	u_char	gdl_len;	/* Total length of sockaddr */
	u_char	gdl_family;	/* AF_DLI */
	u_short	gdl_index;	/* if != 0, system given index for interface */
	u_char	gdl_type;	/* interface type */
	u_char	gdl_nlen;	/* interface name length, no trailing 0 reqd. */
	u_char	gdl_alen;	/* link level address length */
	u_char	gdl_slen;	/* link layer selector length */
	char	gdl_data[1];	/* work area */
    } dl;
#endif	/* SOCKADDR_DL */
#ifdef	PROTO_ISO
    struct {
	u_char	giso_len;
	u_char	giso_family;
	u_char	giso_addr[1];
    } iso;
#endif	/* PROTO_ISO */
    struct {
	u_char	gll_len;
	u_char	gll_family;
	u_char	gll_type;
	u_char	gll_addr[1];
    } ll;
    struct {
	byte	gs_len;
	byte	gs_family;
	char	gs_string[1];
    } s;
} sockaddr_un;

/* The maximum possible address length */
#define	SOCK_MAXADDRLEN	20

#define	AF_LL		253	/* Link level address */
#ifdef	notdef
#ifndef	AF_LINK
#define	AF_LINK		254	/* Link level interface info */
#endif	/* AF_LINK */
#endif	/* notdef */
#define	AF_STRING	255	/* String hack */

/* For compatibility with BSD 4.4 and later */
#define	socksize(x)	((x)->a.ga_len)
#define	socktype(x)	((x)->a.ga_family)
#define	sockcopy(x, y)	bcopy((caddr_t) (x), (caddr_t) (y), socksize(x))


#ifdef	SOCKET_LENGTHS
#define	unix_socksize(x)	((x)->sa_len)
#else	/* SOCKET_LENGTHS */
#ifdef __sgi
#define	unix_socksize(x)	(_FAKE_SA_LEN_DST(x) > 0 ? \
			 _FAKE_SA_LEN_DST(x) : sizeof (*x))
#else
#ifndef SOCKADDR_DL
#define	unix_socksize(x)	(sizeof (*x))
#else	/* SOCKADDR_DL */
#define	unix_socksize(x) \
	((x)->sa_family == AF_LINK ? sizeof(struct sockaddr_dl) : sizeof (*x))
#endif	/* SOCKADDR_DL */
#endif  /* __sgi */
#endif	/* SOCKET_LENGTHS */

#ifndef       SOCK_BUF_PAGES
#define       SOCK_BUF_PAGES  1
#endif        /* SOCK_BUF_PAGES */


/* Types for AF_LL */
#define	LL_OTHER	0	/* Unknown or Other */
#define	LL_SYSTEMID	1	/* ISO System ID */
#define	LL_8022		2	/* IEEE 802.2 Address */
#define	LL_X25		3	/* X.25 Address */
#define	LL_PRONET	4	/* Proteon Pronet */
#define	LL_HYPER	5	/* NSC Hyperchannel */

extern const bits ll_type_bits[];


struct sock_info {
    u_int	si_family;	/* Address family */
    u_int	si_offset;	/* Offset to beginning of address */
    u_int	si_size;	/* Maximum size */
    block_t	si_index;	/* Pointer to allocation block */
    u_int	si_mask_count;	/* Number of masks */
    sockaddr_un	*si_mask_min;	/* Minimum length mask */
    sockaddr_un	*si_mask_max;	/* Maximum length mask */
    adv_entry	*si_martians;	/* Martians for this family */
};
#define	SI_FROM_AF(af)		(&sock_info[(af)])
#define	SI_OFFSET(af)		SI_FROM_AF(af)->si_offset
#define	SI_SIZE(af)		SI_FROM_AF(af)->si_size
#define	SI_INDEX(af)		SI_FROM_AF(af)->si_index
#define	SI_MASK_MIN(af)		SI_FROM_AF(af)->si_mask_min
#define	SI_MASK_MAX(af)		SI_FROM_AF(af)->si_mask_max
#define	SI_MASK_COUNT(af)	SI_FROM_AF(af)->si_mask_count
#define	SI_MARTIANS(af)		SI_FROM_AF(af)->si_martians

extern struct sock_info sock_info[256];

#define	sockfree(addr) \
{ \
    register block_t block_index = SI_INDEX(socktype(addr)); \
    if (block_index) { \
	task_block_free(block_index, (void_t) addr); \
    } else { \
	task_mem_free((task *) 0, (caddr_t) addr); \
    } \
}

/**/

/* Locating masks given prefixes and vice versa */

#define	mask_from_prefix_si(si, pfx) \
	(((u_int) (pfx) < 0 || (pfx) > (si)->si_mask_count) \
	 ? (sockaddr_un *) 0 \
	 : (sockaddr_un *) ((void_t) ((byte *) (si)->si_mask_min + ((si)->si_size * (pfx)))))

#define	mask_from_prefix(af, pfx) mask_from_prefix_si(SI_FROM_AF(af), pfx)

#define	mask_to_prefix_si(si, mask) \
	(((mask) >= (si)->si_mask_min && (mask) <= (si)->si_mask_max) \
	 ? (u_int) ((byte *) (mask) - (byte *) (si)->si_mask_min) / (si)->si_size \
	 : (u_int) -1)

#define	mask_to_prefix(mask)	mask_to_prefix_si(SI_FROM_AF(socktype(mask)), mask)

/**/

PROTOTYPE(sockclean,
	  extern void,
	  (sockaddr_un *));
PROTOTYPE(sockaddrcmp2,
	  extern int,
	  (sockaddr_un *,
	   sockaddr_un *));
PROTOTYPE(sockaddrcmp,
	  extern int,
	  (sockaddr_un *,
	   sockaddr_un *));
PROTOTYPE(sockaddrcmp_mask,
	  extern int,
	  (sockaddr_un *,
	   sockaddr_un *,
	   sockaddr_un *));
PROTOTYPE(sockmask,
	  extern void,
	  (sockaddr_un *,
	   sockaddr_un *));
PROTOTYPE(sockishost,
	  extern int,
	  (sockaddr_un *,
	   sockaddr_un *));
PROTOTYPE(sockhostmask,
	  extern sockaddr_un *,
	  (sockaddr_un *));
PROTOTYPE(sockdup,
	  extern sockaddr_un *,
	  (sockaddr_un *));
PROTOTYPE(mask_locate,
	  extern sockaddr_un *,
	  (sockaddr_un *));
PROTOTYPE(mask_insert,
	  void,
	  (sockaddr_un *));
PROTOTYPE(mask_contig,
	  extern int,
	  (sockaddr_un *));
PROTOTYPE(mask_bits,
	  extern int,
	  (sockaddr_un *));
PROTOTYPE(mask_contig_bits,
	  extern int,
	  (sockaddr_un *));
PROTOTYPE(mask_refines,
	  extern int,
	  (sockaddr_un *,
	   sockaddr_un *));
PROTOTYPE(mask_dump,
	  extern void,
	  (FILE *));
PROTOTYPE(sock2unix,
	  extern struct sockaddr *,
	  (sockaddr_un *,
	   int *));
PROTOTYPE(sock2gated,
	  extern sockaddr_un *,
	  (struct sockaddr *,
	   size_t));
PROTOTYPE(sockbuild_un,
	  extern sockaddr_un *,
	  (const char *));
PROTOTYPE(sockbuild_in,
	  extern sockaddr_un *,
	  (u_short,
	   u_int32));
#ifdef	PROTO_ISO
PROTOTYPE(sockbuild_iso,
	  extern sockaddr_un *,
	  (byte *,
	   size_t));
#endif	/* PROTO_ISO */
PROTOTYPE(sockbuild_str,
	  extern sockaddr_un *,
	  (const char *));
PROTOTYPE(sockbuild_byte,
	  extern sockaddr_un *,
	  (u_char *,
	   size_t));
#ifdef	SOCKADDR_DL
PROTOTYPE(sockbuild_dl,
	  extern sockaddr_un *,
	  (int,
	   int,
	   const char *,
	   size_t,
	   byte *,
	   size_t,
	   byte *,
	   size_t));
#endif	/* SOCKADDR_DL */
PROTOTYPE(sockbuild_ll,
	  extern sockaddr_un *,
	  (int,
	   byte *,
	   size_t));
PROTOTYPE(sock_init,
	  extern void,
	  (void));
PROTOTYPE(sock_init_family,
	  extern void,
	  (u_int,
	   u_int,
	   size_t,
	   byte *,
	   size_t,
	   const char *));
