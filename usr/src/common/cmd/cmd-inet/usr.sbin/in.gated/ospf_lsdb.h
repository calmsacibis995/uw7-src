#ident	"@(#)ospf_lsdb.h	1.3"
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


/*
 *	 STRUCTURES FOR THE SPF ALGORITHM
 */


/* Link State Database */


#define HTBLSIZE	251
#define HTBLMOD		(HTBLSIZE)	/* sb prime (or there aboutst) */
#define XHASH(A1,A2)	((A1) % HTBLMOD)

#define	OSPF_HASH_QUEUE	31
/* Hash size for queues */
#define	XHASH_QUEUE(lsdb)	(LS_ID(lsdb) % OSPF_HASH_QUEUE)


union LSA_PTR {			/* advertisements */
    struct RTR_LA_HDR *rtr;
    struct NET_LA_HDR *net;
    struct SUM_LA_HDR *sum;
    struct ASE_LA_HDR *ase;
    struct GM_LA_HDR *gm;
};

#define ADVNULL ((struct RTR_LA_HDR *) 0)

#define DB_CAN_BE_FREED(DB)\
	(DB_FREEME(DB) &&\
	(!((DB)->lsdb_route || (DB)->lsdb_asb_rtr)) &&\
	((!(DB)->lsdb_retrans)) &&\
	(ospf.nbrEcnt == ospf.nbrFcnt))

#define ADV_RTR(DB) 	DB_RTR(DB)->ls_hdr.adv_rtr
#define ADV_AGE(DB)	(LS_AGE(DB) + (time_sec - (DB)->lsdb_time))
#define LS_AGE(DB) 	DB_RTR(DB)->ls_hdr.ls_age
#define LS_ID(DB) 	DB_RTR(DB)->ls_hdr.ls_id
#define LS_TYPE(DB) 	DB_RTR(DB)->ls_hdr.ls_type
#define LS_SEQ(DB) 	DB_RTR(DB)->ls_hdr.ls_seq
#define LS_CKS(DB) 	ntohs(DB_RTR(DB)->ls_hdr.ls_chksum)
#define LS_LEN(DB) 	DB_RTR(DB)->ls_hdr.length
#define	LS_ASE_TAG(V)	DB_ASE((V))->tos0.ExtRtTag
#define LS_ID_NORMALIZE(N,M)    (sock2ip(N) | ((sock2ip(N) >> (32 - mask_bits(M)) & 0x00000001) ? 0 : ~(sock2ip(M))))
#define LS_ID_NORMALIZE_RT(N)   LS_ID_NORMALIZE((N)->rt_dest, (N)->rt_dest_mask)
#define DB_WHERE(DB) 	(DB)->lsdb_where
#define DB_DIRECT(DB) 	(DB)->lsdb_direct
#define DB_VIRTUAL(DB) 	(DB)->lsdb_virtual
#define DB_FREEME(DB) 	(DB)->lsdb_freeme
#define DB_RTR(DB) 	(DB)->lsdb_adv.rtr
#define DB_NET(DB) 	(DB)->lsdb_adv.net
#define DB_SUM(DB) 	(DB)->lsdb_adv.sum
#define DB_ASE(DB) 	(DB)->lsdb_adv.ase
#define DB_LS_HDR(DB)	DB_RTR(DB)->ls_hdr
#define DB_MASK(DB) 	DB_NET(DB)->net_mask
#define DB_NETNUM(DB)  	(LS_ID(DB) & DB_MASK(DB))
#define	DB_ASE_TAG(DB)	DB_ASE((DB))->tos0.ExtRtTag
#define	DB_ASE_FORWARD(DB)	DB_ASE((DB))->tos0.ForwardAddr

struct LSDB {
    struct LSDB *lsdb_forw;		/* for candidate, sum and ase list */
    struct LSDB *lsdb_back;

    struct LSDB *lsdb_next;		/* LSDB list */

    union LSA_PTR lsdb_adv;		/* advertisement */

    block_t lsdb_index;			/* Allocation index */

    union {
	rt_entry *route;		/* pointer to the routing table entry */
	struct OSPF_ROUTE *ab_rtr;	/* Area bdr rtr */
    } lsdb_un1;
#define	lsdb_route	lsdb_un1.route
#define	lsdb_ab_rtr	lsdb_un1.ab_rtr
    struct OSPF_ROUTE *lsdb_asb_rtr;	/* if it is ASB Router */
    struct LSDB *lsdb_border;		/* sum or ase - lsdb of border rtr */
    flag_t lsdb_where:8,		/* where this vertex is: on candidatelst or
					 * spftree for ls_ase could be on
					 * ase_infinity or ase_list */
#define UNINITIALIZED	0
#define	ON_CLIST	1
#define ON_SPFTREE	2
#define ON_RTAB		2
#define ON_SUMASB_LIST	3		/* reachable asb from attached area */
#define ON_SUMNET_LIST	4		/* reachable net from attached area */
#define ON_INTER_LIST	5		/* on inter-area list - imported from bb */
#define	ON_SUM_INFINITY 6
#define ON_ASE_LIST	7
#define ON_ASE_INFINITY 8
	lsdb_direct:1,			/* net attached to this node */
	lsdb_freeme:1,			/* flag to free this lsdb entry */
	lsdb_seq_max:1,			/* note if entry has reached max seq number */
	lsdb_virtual:1,			/* LSDB associated with virtual link */
        lsdb_net_range:1;		/* LSDB associated with area net range. */
    struct AREA *lsdb_trans_area;	/* For resolution of virtual nbrs */
    struct AREA *lsdb_area;		/* for keeping count of db's in each area */
    u_int16 lsdb_hash;			/* this db's hash */
    u_int16 lsdb_nhcnt;			/* number of in-use parents (< RT_N_MULTIPATH) */
    struct NH_BLOCK *lsdb_nh[RT_N_MULTIPATH];	/* list of next hops */
    u_int32 lsdb_dist;			/* distance to root */
    time_t lsdb_time;			/* for keeping age - stamped when arrived */
    struct NBR_LIST *lsdb_retrans;	/* nbrs pointing to this lsdb */
};

#define	LSDB_LIST(list, db) \
	do { \
	    register struct LSDB **Xdbp; \
	    for (Xdbp = &DBH_LIST(list); \
		 ((db) = *Xdbp); \
		 Xdbp = (db == *Xdbp) ? &(*Xdbp)->lsdb_next : Xdbp)
#define	LSDB_LIST_END(list, db) \
	} while (0)
#define	LSDB_LIST_DELETE(list, db)	*Xdbp = (db)->lsdb_next

#define  GOT_A_BDR(L) 	   (L)->lsdb_border
#define  ABRTR_ACTIVE(L)   (L)->lsdb_border->lsdb_ab_rtr
#define  ASBRTR_ACTIVE(L)  (L)->lsdb_border->lsdb_asb_rtr

#define LSDBNULL	((struct LSDB *)0)

/* Head of hash lists */
struct LSDB_HEAD {
    struct LSDB *dbh_lsdb;	/* LSDB list */
    u_int dbh_rerun;		/* rerun this row - partial update */
};
#define	DBH_LIST(dbh)	(dbh)->dbh_lsdb
#define DBH_RERUN(dbh) 	(dbh)->dbh_rerun

#define	LSDB_HEAD_LIST(head, dbh, start, end) \
	do { \
		register struct LSDB_HEAD *Xhead = &(head)[start]; \
		for ((dbh) = Xhead; (dbh) < &(head)[end]; (dbh)++)
#define	LSDB_HEAD_LIST_END(head, dbh, start, end) \
	} while (0)
	    
/**/

/*
 * sizeof this area's router advertisement
 */
#define MY_RTR_ADV_SIZE(A)\
	(RTR_LA_HDR_SIZE +\
	  (((A)->ifcnt * RTR_LA_PIECES_SIZE) * 2) +\
	  (((A)->hostcnt * RTR_LA_PIECES_SIZE)) +\
	  (((A)->area_id == OSPF_BACKBONE) ? ((ospf.vcnt * RTR_LA_PIECES_SIZE)) : 0))

/*
 * sizeof this area's network advertisement
 */
#define MY_NET_ADV_SIZE(I)\
	 (NET_LA_HDR_SIZE + (((I)->nbrIcnt + 1) * NET_LA_PIECES_SIZE))

/*
 * Link State Advertisements
 */
#define	ADV_ALLOC(adv, index, len) \
	do { \
	    register block_t *Xbp; \
	    register size_t Xlen = len; \
	    if (Xlen > 64) { \
		Xlen = ROUNDUP(len, 16); \
		Xbp = &ospf_lsa_index_16[Xlen >> 4]; \
	    } else { \
		Xlen = ROUNDUP(len, 4); \
		Xbp = &ospf_lsa_index_4[Xlen >> 2]; \
	    } \
	    if (!*Xbp) { \
		*Xbp = task_block_init(Xlen, "ospf_LSA"); \
	    } \
	    (index) = *Xbp; \
	    (adv).rtr = (struct RTR_LA_HDR *) task_block_alloc(*Xbp); \
	} while (0)

#define	DBADV_ALLOC(db, len) ADV_ALLOC((db)->lsdb_adv, (db)->lsdb_index, len)

#define	ADV_FREE(adv, index) \
	do { \
	    task_block_free((index), (void_t) (adv).rtr); \
	    (index) = (block_t) 0; \
	    (adv).rtr = NULL; \
	} while (0)

#define	DBADV_FREE(db) ADV_FREE((db)->lsdb_adv, (db)->lsdb_index)

/*
 * COPY CALLS
 */
#define ADV_COPY(FROM,TO,LEN) bcopy((caddr_t) FROM, (caddr_t) TO, (size_t) (LEN))

/*
 * COMPARE CALLS
 */
#define RTR_LINK_CMP(R1,R2,LEN)\
	bcmp((caddr_t) &(R1)->link, (caddr_t) &(R2)->link, (size_t) (LEN))

#define NET_ATTRTR_CMP(N1,N2,LEN)\
	bcmp((caddr_t) &(N1)->att_rtr, (caddr_t) &(N2)->att_rtr, (size_t) (LEN))

#define ASE_TOS_CMP(T1,T2) bcmp((caddr_t) (T1), (caddr_t) (T2), ASE_LA_PIECES_SIZE)

/**/

/* structures for keeping track of retransmission lists */

/* lsdb keep track of nbrs pointing to it for tx and retx */
struct NBR_LIST {
    struct NBR_LIST *ptr[2];
    struct NBR *nbr;
};

#define NLNULL	((struct NBR_LIST *) 0)

/* General queue structure for sending advertisements */
struct ospf_lsdb_list {
    struct ospf_lsdb_list *ptr[2];
    struct LSDB *lsdb;
    int flood;			/* true if flooding this one */
};

#define LLNULL	((struct ospf_lsdb_list *) 0)


/* Return values for nbr_ret_req */
#define REQ_NOT_FOUND 		0
#define REQ_LESS_RECENT 	1
#define REQ_SAME_INSTANCE 	2
#define REQ_MORE_RECENT 	3

#define ASE_COST_LESS(A_ETYPE,A_COST,A_TYPE2COST,B_ETYPE,B_COST,B_TYPE2COST)\
    ( ((!A_ETYPE) && (B_ETYPE)) ||\
     ( ((!A_ETYPE) && (!B_ETYPE)) &&\
      ((A_COST) < (B_COST)) ) ||\
     ( ((A_ETYPE) && (B_ETYPE)) &&\
      ( ((A_TYPE2COST) < (B_TYPE2COST)) ||\
       (((A_TYPE2COST) == (B_TYPE2COST)) && ((A_COST) < (B_COST))))))

#define ASE_COST_EQUAL(A_ETYPE,A_COST,A_TYPE2COST,B_ETYPE,B_COST,B_TYPE2COST)\
	( ((A_ETYPE) == (B_ETYPE)) &&\
	  ((A_COST) == (B_COST)) &&\
	  ((A_TYPE2COST) == (B_TYPE2COST)) )

/* Queue general hdrs */
#define NEXT 0
#define LAST 1

struct Q {			/* an empty shell for general doubly-linked
				 * queueing */
    struct Q *ptr[2];		/* 0 is foward ptr, 1 is back ptr */
};

#define QNULL ((struct Q *)0)

/* Queue of LS_HDRs */

struct LS_HDRQ {
    struct LS_HDRQ *ptr[2];
    struct LS_HDR ls_hdr;
};

/**/
/* Ack list manipulation */

#define	ADD_ACK(qhp, db) \
	{ \
	    register u_short ack_age = ADV_AGE(db); \
	    struct LS_HDRQ *ack = (struct LS_HDRQ *) task_block_alloc(ospf_hdrq_index); \
	    ack->ls_hdr = DB_LS_HDR(db); \
	    ack->ls_hdr.ls_age = htons((u_int16) MIN(ack_age, MaxAge)); \
	    ADD_Q((qhp), ack); \
	}

#define	ADD_ACK_INTF(intf, db) \
	{ \
	    ADD_ACK(&intf->acks, db); \
	    intf->ack_cnt++; \
	}

#define	ACK_QUEUE_FULL(intf) \
	((size_t) (OSPF_HDR_SIZE + (intf->ack_cnt + 1) * ACK_PIECE_SIZE) > INTF_MTU(intf))

/**/
/* LSDB Access */

#define Xkey1(A) ((A)->ls_hdr.ls_id)
#define Xkey2(A) ((A)->ls_hdr.adv_rtr)
#define Xtype(A) ((A)->ls_hdr.ls_type)
#define XXhash(A) (XHASH(Xkey1(A),Xkey1(A)))

#define XThash(K1,K2,T) (XHASH(K1,K1))

#define XAddLSA(DB,Area,X) addLSA(DB, Area, Xkey1(X), Xkey2(X), (u_int) Xtype(X))

#define AddLSA(DB,Area,K1,K2,Typ) addLSA(DB, Area, K1, K2, (u_int) Typ)

#define XFindLSA(Area,X)\
     	findLSA(&Area->htbl[Xtype(X)][XXhash(X)],\
		Xkey1(X), Xkey2(X), (u_int) Xtype(X))

#define FindLSA(Area,K1,K2,Typ)\
  	findLSA(&Area->htbl[Typ][XThash(K1,K2,Typ)], K1, K2, (u_int) Typ)

/*
 *	Remove from a neighbors retransmit Q
 */
#define	REM_DB_PTR(nbr, lp)	{ DEL_Q(lp, ospf_lsdblist_index); (nbr)->rtcnt--; }

/*
 * Add Q to the queue
 */
#define ADD_Q(PREV,Q) {\
	if ((PREV)->ptr[NEXT]) {\
	    (PREV)->ptr[NEXT]->ptr[LAST] = (Q);\
	}\
	(Q)->ptr[NEXT] = (PREV)->ptr[NEXT];\
	(PREV)->ptr[NEXT] = (Q);\
	(Q)->ptr[LAST] = (PREV);}

/*
 * Remove Q from the queue
 */
#define DEL_Q(Q,TYPE) {\
	if ((Q)->ptr[LAST]) (Q)->ptr[LAST]->ptr[NEXT] = (Q)->ptr[NEXT];\
	if ((Q)->ptr[NEXT]) (Q)->ptr[NEXT]->ptr[LAST] = (Q)->ptr[LAST];\
	(Q)->ptr[NEXT] = ((Q)->ptr[LAST] = 0);\
	task_block_free((TYPE), (void_t) (Q));}

/*
 * These queue macros assume that the queue doesn't have a head
 */
#define EN_Q(QHP,Q) {\
	if ((!QHP)){\
		(QHP) = (Q); (Q)->ptr[LAST] = 0;\
	} else ADD_Q((QHP),(Q));}

#define REM_Q(QHP,Q,TYPE) {\
            if ((QHP) == (Q)) {\
                (QHP) = (Q)->ptr[NEXT];\
                if (QHP) (QHP)->ptr[LAST] = 0;\
                if (TYPE) { \
		    task_block_free((TYPE), (void_t) (Q));\
                } else { \
		    (Q)->ptr[NEXT] = ((Q)->ptr[LAST] = 0);\
		} \
            } else DEL_Q((Q), TYPE);}

/*
 *
 */
#define	DB_ADDQUE(head, db) INSQUE(db, head.q_back)

#define	DB_INSQUE(db, prev) INSQUE(db, prev)

#define	DB_REMQUE(db) \
    do { \
	if ((db)->lsdb_forw) { \
	    REMQUE(db); \
	    (db)->lsdb_forw = (db)->lsdb_back = (struct LSDB *) 0; \
	} \
    } while (0)

#define	DB_FIRSTQ(head)	((struct LSDB *) (head).q_forw)

#define	DB_EMPTYQ(head) ((head).q_forw == &(head))

#define	DB_INITQ(head)	(head).q_forw = (head).q_back = &(head)

#define	DB_MOVEQ(from, to) \
    do { \
	(to) = (from);	/* struct copy */ \
	(to).q_forw->q_back = (to).q_back->q_forw = (to); \
	DB_INITQ(from); \
    } while (0)

#define	DB_RUNQUE(head, db) \
    do { \
	register struct LSDB *Xdb; \
	for ((db) = (struct LSDB *) (head).q_forw; \
	     ((db) != (struct LSDB *) &(head)) && (Xdb = (db)->lsdb_forw); \
	     (db) = Xdb)
#define	DB_RUNQUE_END(head, db) \
    } while (0)

/* Function prototypes */
PROTOTYPE(addLSA,
	  extern int,
	  (struct LSDB **,
	   struct AREA *,
	   u_int32,
	   u_int32,
	   u_int));
PROTOTYPE(ospf_add_stub_lsa,
	  extern int,
	  (struct LSDB **,
	   struct AREA *,
	   u_int32,
	   u_int32,
	   u_int32));
PROTOTYPE(findLSA,
	  extern struct LSDB *,
	  (struct LSDB_HEAD *,
	   u_int32,
	   u_int32,
	   u_int));
PROTOTYPE(db_free,
	  extern void,
	  (struct LSDB *,
	   int));
PROTOTYPE(nbr_rem_req,
	  extern int,
	  (struct NBR *,
	   union LSA_PTR));
