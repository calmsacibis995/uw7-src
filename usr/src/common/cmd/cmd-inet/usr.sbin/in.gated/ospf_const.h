#ident	"@(#)ospf_const.h	1.3"
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
 * ------------------------------------------------------------------------
 * 
 *                 U   U M   M DDDD     OOOOO SSSSS PPPPP FFFFF
 *                 U   U MM MM D   D    O   O S     P   P F
 *                 U   U M M M D   D    O   O  SSS  PPPPP FFFF
 *                 U   U M M M D   D    O   O     S P     F
 *                  UUU  M M M DDDD     OOOOO SSSSS P     F
 * 
 *     		          Copyright 1989, 1990, 1991
 *     	       The University of Maryland, College Park, Maryland.
 * 
 * 			    All Rights Reserved
 * 
 *      The University of Maryland College Park ("UMCP") is the owner of all
 *      right, title and interest in and to UMD OSPF (the "Software").
 *      Permission to use, copy and modify the Software and its documentation
 *      solely for non-commercial purposes is granted subject to the following
 *      terms and conditions:
 * 
 *      1. This copyright notice and these terms shall appear in all copies
 * 	 of the Software and its supporting documentation.
 * 
 *      2. The Software shall not be distributed, sold or used in any way in
 * 	 a commercial product, without UMCP's prior written consent.
 * 
 *      3. The origin of this software may not be misrepresented, either by
 *         explicit claim or by omission.
 * 
 *      4. Modified or altered versions must be plainly marked as such, and
 * 	 must not be misrepresented as being the original software.
 * 
 *      5. The Software is provided "AS IS". User acknowledges that the
 *         Software has been developed for research purposes only. User
 * 	 agrees that use of the Software is at user's own risk. UMCP
 * 	 disclaims all warrenties, express and implied, including but
 * 	 not limited to, the implied warranties of merchantability, and
 * 	 fitness for a particular purpose.
 * 
 *     Royalty-free licenses to redistribute UMD OSPF are available from
 *     The University Of Maryland, College Park.
 *       For details contact:
 * 	        Office of Technology Liaison
 * 		4312 Knox Road
 * 		University Of Maryland
 * 		College Park, Maryland 20742
 * 		     (301) 405-4209
 * 		FAX: (301) 314-9871
 * 
 *     This software was written by Rob Coltun
 *      rcoltun@ni.umd.edu
 */


/* Values are on second boundarys */
#ifndef CONST_H
#define CONST_H
#define	SECONDS	1
#define	MINUTES	60
#define HOURS	(60 * MINUTES)

#define	IfChkAlrmTime	(30)
#define DbAgeTime	(900)
#define	AseAgeTime	(61)
#define LSRefreshTime	(1800)
#define	MinLSInterval	(5)
#define MaxAgeDiff	((u_int) 900)
#define MaxAge		((u_int) 3600)
#define	CheckAge	(300)

/*
 * Every AseAgeTime age ASE_AGE_NDX_ADD hash buckets so we don't
 * cause massive flooding
 * This may be adjusted - change with AseAgeTime
 */
#define ASE_AGE_NDX_ADD       2

/* a few offsets so events occur at the same time very infrequently */
#define OFF1	(37)
#define OFF2	(131)
#define OFF3	(199)
#define OFF4	(263)
#define OFF5	(327)
#define OFF6	(391)

#define BaseSeq		htonl(0x80000000)
#define HBaseSeq	0x80000000	/* host order */
#define FirstSeq	htonl((HBaseSeq) + 1)
#define HFirstSeq	((HBaseSeq) + 1)
#define MaxSeqNum	0x7FFFFFFF

#define	ODIFF(A, B)	((A) > (B) ? (A) - (B) : (B) - (A))

/* for these macros, the version that is held in the lsdb should be B */
#define MORE_RECENT(A, B, Elapse) \
	((s_int32) ntohl((A)->ls_seq) > (s_int32) ntohl((B)->ls_seq) \
	 || ((A)->ls_seq == (B)->ls_seq \
	     && (((A)->ls_chksum != (B)->ls_chksum \
		  && (u_int) ntohs((A)->ls_chksum) > (u_int) ntohs((B)->ls_chksum)) \
		 || ((A)->ls_age == MaxAge \
		     && (B)->ls_age + (u_int) Elapse < MaxAge) \
		 || ((A)->ls_age != MaxAge \
		     && (B)->ls_age + (u_int) Elapse < MaxAge \
		     && ODIFF((A)->ls_age, (B)->ls_age + (u_int) Elapse) > MaxAgeDiff \
		     && (A)->ls_age < ((B)->ls_age + (u_int) Elapse)))))

#define SAME_INSTANCE(A,B,Elapse) \
	((A)->ls_seq == (B)->ls_seq \
	 && (A)->ls_chksum == (B)->ls_chksum \
	 && ((A)->ls_age == MaxAge) == ((B)->ls_age + (u_int) Elapse >= MaxAge) \
	 && ODIFF((A)->ls_age, MIN((B)->ls_age + (u_int) Elapse, MaxAge)) <= MaxAgeDiff)

#define NEXTSEQ(S) ((((S) + 1) == HBaseSeq) ? HFirstSeq : ((S) + 1))

#define NEXTNSEQ(S) (htonl(NEXTSEQ(ntohl(S))))	/* from net to host and back */


#define RTRLSInfinity	0xFFFF
#define SUMLSInfinity	0xFFFFFF
#define ASELSInfinity	0xFFFFFF

#define	OSPF_ADDR_ALLSPF	0xe0000005	/* 224.0.0.5 */
#define	OSPF_ADDR_ALLDR		0xe0000006	/* 224.0.0.6 */

/*
 * Default configuration defines
 */
#define OSPF_BC_DFT_HELLO	10
#define OSPF_NBMA_DFT_HELLO	30
#define OSPF_PTP_DFT_HELLO	30
#define OSPF_VIRT_DFT_HELLO	30

#define OSPF_DFT_RETRANS	5
#define OSPF_VIRT_DFT_RETRANS	30
#define OSPF_DFLT_POLL_INT	120
#define	OSPF_DFLT_TRANSDLY	1
#define	OSPF_VIRT_DFLT_TRANSDLY	4
#define OSPF_DFLT_COST		1

#define	OSPF_T_ACK		1

/* Ls_ase are originated every MinASEInterval over a period of LSRefreshTime
 *    - Just do ASEGenLimit at a time
 * This may be adjusted
*/

#define	ASEGenLimit	100	/* # of LS_ASE generated tq_AseLsa period */
#define	MinASEInterval	(1)	/* Can send ASEGenLimit over this value */

#define OSPF_MAXVPKT  	512	/* Maxmimum tx pkt size less ip hdr stuff */

#endif	/* CONST_H */
