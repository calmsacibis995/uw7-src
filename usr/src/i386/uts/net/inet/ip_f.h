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
#ident "@(#)ip_f.h	1.3"
#ident "$Header$"

#ifndef __netinet_ip_f_h
#define __netinet_ip_f_h


/*
 * Structure of an internet header, naked of options. 
 */

/*
 * Definitions for internet protocol version 4.
 * Per RFC 791, September 1981.
 *
 * 0	   4	   8		   16	   20	   24		 31
 * +-------+-------+---------------+-------------------------------+
 * | Vers  |Length |Type Of Service| Total Length		   |
 * +-------+-------+---------------+-------+-----------------------+
 * | Identification		   | Flags | Fragment Offset Field |
 * +---------------+---------------+-------+-----------------------+
 * | Time To Live  | Protocol	   | Header Checksum		   |
 * +---------------+---------------+-------------------------------+
 * | Source Internet Address					   |
 * +---------------------------------------------------------------+
 * | Destination Internet Address				   |
 * +---------------------------------------------------------------+
 */
struct ip {
#if !defined(__NO_BITFIELDS__)
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char	ip_hl:4,		/* header length */
		ip_v:4;			/* version */
#else	/* BYTE_ORDER */
	u_char	ip_v:4,			/* version */
		ip_hl:4;		/* header length */
#endif	/* BYTE_ORDER */
#else	/* __NO_BITFIELDS__ */
	u_char          ip_hl_v;	/* header length & version */
#define	IP_HL_LEN	4		/* header lenegth field len */
#define	IP_V_LEN	4		/* version field len */
#define	IP_HL_POS	3		/* lower half */
#define	IP_V_POS	7		/* higher half */
#endif	/* __NO_BITFIELDS__ */
	u_char          ip_tos;		/* type of service in IP hdr */
	u_short         ip_len;		/* total length */
	u_short         ip_id;		/* identification */
	u_short         ip_off;		/* fragment offset field */
#define	IP_DF 0x4000			/* dont fragment flag */
#define	IP_MF 0x2000			/* more fragments flag */
	u_char          ip_ttl;		/* time to live */
	u_char          ip_p;		/* protocol */
	u_short         ip_sum;		/* checksum */
	struct in_addr  ip_src, ip_dst;	/* source and dest address */
};

/*
 * Time stamp option structure.
 *
 * 0		   8		   16		   24	   28	 31
 * +---------------+---------------+---------------+-------+-------+
 * | Code = 68	   | Length	   | Pointer	   |OvrFlow| Flags |
 * +---------------+---------------+---------------+-------+-------+
 * | First Internet Address					   |
 * +---------------------------------------------------------------+
 * | First Timestamp						   |
 * +---------------------------------------------------------------+
 */
struct ip_timestamp {
	u_char          ipt_code;	/* IPOPT_TS */
	u_char          ipt_len;	/* size of structure (variable) */
	u_char          ipt_ptr;	/* index of current entry */
	u_char          ipt_flg_oflw;	/* flags & overflow counter,see below */
#define	IPT_FLG_LEN	 4		/* flags len */
#define	IPT_OFLW_LEN  	 4		/* overflow len */
#define	IPT_FLG_POS	 3		/* lower half */
#define	IPT_OFLW_POS  	 7		/* higher half */
	union ipt_timestamp {
		n_long          ipt_time[1];
		struct ipt_ta {
			struct in_addr  ipt_addr;
			n_long          ipt_time;
		}               ipt_ta[1];
	} ipt_timestamp;
};

#endif /*__netinet_in_f_h */
