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
 * TCP header. Per RFC 793, September, 1981. 
 *
 * 0	   4	   8		   16	   20	   24		 31
 * +-------------------------------+-------------------------------+
 * | Source Port		   | Destination Port		   |
 * +-------------------------------+-------------------------------+
 * | Sequence Number						   |
 * +---------------------------------------------------------------+
 * | Acknowledgement Number					   |
 * +-------+-------+---------------+-------------------------------+
 * | Offset| RSRVD | Flags	   | Window			   |
 * +-------+-------+---------------+-------------------------------+
 * | Chucksum			   | Urgent Pointer		   |
 * +-------------------------------+-------------------------------+
 */
struct tcphdr {
	u_short         th_sport;	/* source port */
	u_short         th_dport;	/* destination port */
	tcp_seq         th_seq;	/* sequence number */
	tcp_seq         th_ack;	/* acknowledgement number */
	u_char		th_x2_off;	/*(unused field) & data offset */
#define	TH_X2_LEN	4		/* unused field len */
#define	TH_OFF_LEN	4		/* data offset field len half */
#define	TH_X2_POS	3		/* lower half */
#define	TH_OFF_POS	7		/* higher half */
	u_char          th_flags;
#define	TH_FIN	0x01
#define	TH_SYN	0x02
#define	TH_RST	0x04
#define	TH_PUSH	0x08
#define	TH_ACK	0x10
#define	TH_URG	0x20
	u_short         th_win;	/* window */
	u_short         th_sum;	/* checksum */
	u_short         th_urp;	/* urgent pointer */
};
