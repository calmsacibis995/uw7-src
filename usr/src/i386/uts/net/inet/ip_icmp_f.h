/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1995 Computer Associates International, Inc.
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
/*
 * Copyright (c) 1982-1995
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ident "$Id$"

/*
 * Interface Control Message Protocol Definitions. Per RFC 792, September
 * 1981. 
 */

/*
 * Structure of an icmp header. 
 */
struct icmp {
	u_char          icmp_type;	/* type of message, see below */
	u_char          icmp_code;	/* type sub code */
	u_short         icmp_cksum;	/* ones complement cksum of struct */
	union {
		u_char          ih_pptr;	/* ICMP_PARAMPROB */
		struct in_addr  ih_gwaddr;	/* ICMP_REDIRECT */
		struct ih_idseq {
			n_short         icd_id;
			n_short         icd_seq;
		}               ih_idseq;
		int             ih_void;
		/*
		 * Router Discovery (RFC 1256)
		 */
		struct {			/* advertisement */
			u_char	ira_naddrs;
			u_char	ira_asize;
			n_short	ira_life;
		} ih_radv;
		struct {			/* solicitation */
			u_long	irs_resv;
		} ih_rsol;
		/*
		 * PMTU Discovery (RFC 1191)
		 * (ICMP_UNREACH_NEEDFRAG)
		 */
		struct {
			n_short ipm_void;
			n_short ipm_nextmtu;
		} ih_pmtu;
	}               icmp_hun;
#define	icmp_pptr	icmp_hun.ih_pptr
#define	icmp_gwaddr	icmp_hun.ih_gwaddr
#define	icmp_id		icmp_hun.ih_idseq.icd_id
#define	icmp_seq	icmp_hun.ih_idseq.icd_seq
#define	icmp_void	icmp_hun.ih_void
#define	icmp_naddrs	icmp_hun.ih_radv.ira_naddrs
#define	icmp_asize	icmp_hun.ih_radv.ira_asize
#define	icmp_life	icmp_hun.ih_radv.ira_life
#define	icmp_resv	icmp_hun.ih_rsol.irs_resv
#define	icmp_pmvoid	icmp_hun.ih_pmtu.ipm_void
#define	icmp_nextmtu	icmp_hun.ih_pmtu.ipm_nextmtu
	union {
		struct id_ts {
			n_time          its_otime;
			n_time          its_rtime;
			n_time          its_ttime;
		}               id_ts;
		struct id_ip {
			struct ip       idi_ip;
			/* options and then 64 bits of data */
		}               id_ip;
		u_long          id_mask;
		char            id_data[1];
#define	icmp_otime	icmp_dun.id_ts.its_otime
#define	icmp_rtime	icmp_dun.id_ts.its_rtime
#define	icmp_ttime	icmp_dun.id_ts.its_ttime
#define	icmp_ip		icmp_dun.id_ip.idi_ip
#define	icmp_mask	icmp_dun.id_mask
#define	icmp_data	icmp_dun.id_data
	}               icmp_dun;
};

