#ident	"@(#)ospf_rxmon.c	1.3"
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


#define	INCLUDE_TIME
#define	INCLUDE_FILE
#include "include.h"
#include "krt.h"
#include "inet.h"
#include "ospf.h"


static const char *nbr_modes[] = {
    "None",
    "Slave",
    "Master",
    "Null",
    "Hold"
};

static const char *if_types[] = {
    "Bcast",
    "NBMA",
    "PtoP",
    "Virt"
};

static const char *paths[] = {
    "Stub",
    "Rtr",
    "Net",
    "SNet",
    "ASB ",
    "ASE",
    "GM",
    "NSSA"
};

const char *ospf_con_types[] = {
    "",
    "Router",
    "TransNet",
    "StubNet",
    "Virtual"
};

static const char *ospf_where_types[] = {
    "Uninitialized",
    "Clist",
    "SpfTree",
    "SumAsb List",
    "SumNet List",
    "Inter List",
    "Sum Infinity",
    "Ase List",
    "Ase Infinity"
};


static char *
ospf_print_dest_mask __PF2(addr, sockaddr_un *,
			   mask, sockaddr_un *)
{
    static char buf[19];
    u_int prefix = inet_prefix_mask(mask);
    byte *ap = (byte *) &sock2in(addr);

#ifdef	notdef
    if (prefix
	&& (prefix % 8
	    || ap[(prefix >> 3) - 1] != 0xff)) {
	(void) sprintf(buf, "%A/%u",
		       addr,
		       prefix);
    } else {
	(void) sprintf(buf, "%A",
		       addr);
    }
#endif	/* notdef */

    switch (prefix) {
    case 0:
	(void) sprintf(buf, "%A",
		       addr);
	break;

    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
	(void) sprintf(buf, "%u/%u",
		       ap[0],
		       prefix);
	break;
	
    case 8:
	(void) sprintf(buf, "%u",
		       ap[0]);
	break;

    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
	(void) sprintf(buf, "%u.%u/%u",
		       ap[0], ap[1],
		       prefix);
	break;
	
    case 16:
	(void) sprintf(buf, "%u.%u",
		       ap[0], ap[1]);
	break;

    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
	(void) sprintf(buf, "%u.%u.%u/%u",
		       ap[0], ap[1], ap[2],
		       prefix);
	break;
	
    case 24:
	(void) sprintf(buf, "%u.%u.%u",
		       ap[0], ap[1], ap[2]);
	break;

    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
	(void) sprintf(buf, "%u.%u.%u.%u/%u",
		       ap[0], ap[1], ap[2], ap[3],
		       prefix);
	break;
	
    case 32:
	(void) sprintf(buf, "%u.%u.%u.%u",
		       ap[0], ap[1], ap[2], ap[3]);
	break;

    default:
	(void) sprintf(buf, "%A/%u",
		       addr,
		       prefix);
	break;
    }
    
    return buf;
}


/*
 * Print the retran list of lsdbs held by all nbrs associated with this intf
 */
static void
ospf_print_nbr_retrans __PF2(fp, FILE *,
			intf, struct INTF *)
{
    struct NBR *n;

    NBRS_LIST(n, intf) {
	if (n->rtcnt) {
	    int hash = OSPF_HASH_QUEUE;
	    
	    (void) fprintf(fp,"Nbr %A retrans list:\n",
			   n->nbr_addr);

	    while (hash--) {
		struct ospf_lsdb_list *ll;
	    
		/* remove from all nbrs' lists */
		for (ll = (struct ospf_lsdb_list *) n->retrans[hash].ptr[NEXT];
		     ll;
		     ll = ll->ptr[NEXT]) {
		    (void) fprintf(fp,"  type %10s  %A %A\n",
				   trace_state(ospf_ls_type_bits, LS_TYPE(ll->lsdb)),
				   sockbuild_in(0, LS_ID(ll->lsdb)),
				   sockbuild_in(0, ADV_RTR(ll->lsdb)));
		}
	    }
	}
    } NBRS_LIST_END(n, intf) ;
}

/*
 * Print the retran list of nbrs held by this lsdb structure
 */
static void
ospf_print_db_retrans __PF2(fp, FILE *,
		       db, struct LSDB *)
{
    struct NBR_LIST *nl;

    if (db->lsdb_retrans) {
	(void) fprintf(fp,"retrans list:\n");
	for (nl = db->lsdb_retrans; nl != NLNULL; nl = nl->ptr[NEXT]) {
	    (void) fprintf(fp,"       nbr: %A\n",
			   nl->nbr->nbr_addr);
	}
    }
}


/*
 * Show the link-state data base for this area
 */
static void
ospf_mon_dump_lsdb __PF2(fp, FILE *,
			 retrans, long)
{
    struct AREA *a;
    int type;

    fprintf(fp,"LS Data Base:");

    AREA_LIST(a) {

	fprintf(fp, "\nArea: %A\n",
		sockbuild_in(0, a->area_id));
	fprintf(fp,
		"Type LinkState ID    AdvRouter        Age Len Sequence Metric Where\n");
	fprintf(fp,
		"-------------------------------------------------------------------\n");

	
	for (type = LS_STUB; type < LS_ASE; type++) {
	    register struct LSDB_HEAD *hp;

	    LSDB_HEAD_LIST(a->htbl[type], hp, 0, HTBLSIZE) {
		register struct LSDB *e;

		LSDB_LIST(hp, e) {
		    fprintf(fp,"%-4s %-15A %-15A %4d %-3d %-8x %6d %s\n",
			    paths[type],
			    sockbuild_in(0, LS_ID(e)),
			    sockbuild_in(0, ADV_RTR(e)),
			    ADV_AGE(e) < MaxAge ? ADV_AGE(e) : -1,
			    ntohs(LS_LEN(e)),
			    ntohl(LS_SEQ(e)),
			    LS_TYPE(e) < LS_SUM_NET ? 0 : BIG_METRIC(e),
			    ospf_where_types[DB_WHERE(e)]);
		    if (retrans) {
			ospf_print_db_retrans(fp, e);
		    }
		} LSDB_LIST_END(hp, e) ;
	    } LSDB_HEAD_LIST_END(a->htbl[type], hp, 0, HTBLSIZE) ;
	}
    } AREA_LIST_END(a) ;
}


/*
 * Show the link-state data base for this area
 */
static void
ospf_mon_dump_ase __PF2(fp, FILE *,
	      retrans, long)
{
    struct AREA *a = ospf.area.area_forw;
    register struct LSDB_HEAD *hp;

    fprintf(fp,"AS External Data Base:\n");

    fprintf(fp,
	    "Destination        AdvRouter       Forward Addr     Age Len Sequence T Metric\n");
    fprintf(fp,
	    "-----------------------------------------------------------------------------\n");

    LSDB_HEAD_LIST(a->htbl[LS_ASE], hp, 0, HTBLSIZE) {
	register struct LSDB *e;

	LSDB_LIST(hp, e) {
	    fprintf(fp,"%-18s %-15A %-15A %4d %-3d %-8x %1d %6d\n",
		    ospf_print_dest_mask(sockbuild_in(0, LS_ID(e)),
					 inet_mask_locate(DB_ASE(e)->net_mask)),
		    sockbuild_in(0, ADV_RTR(e)),
		    sockbuild_in(0, DB_ASE(e)->tos0.ForwardAddr),
		    ADV_AGE(e) < MaxAge ? ADV_AGE(e) : -1,
		    ntohs(LS_LEN(e)),
		    ntohl(LS_SEQ(e)),
		    (ASE_TYPE1(e) ? 1 : 2),
		    BIG_METRIC(e));
	    if (retrans) {
		ospf_print_db_retrans(fp, e);
	    }
	} LSDB_LIST_END(hp, e) ;
    } LSDB_HEAD_LIST_END(a->htbl[LS_ASE], hp, 0, HTBLSIZE) ;

    fprintf(fp, "\n\tCount %d, checksum sum %X\n",
	    ospf.db_ase_cnt,
	    ospf.db_chksumsum);
}


/*
 * Print configured interfaces and their status
 */
static void
ospf_mon_dump_ifs __PF1(fp, FILE *)
{
    struct AREA *a;
    struct INTF *intf;

    AREA_LIST(a) {
	fprintf(fp,
		"\nArea: %-15A\n",
		sockbuild_in(0, a->area_id));
	fprintf(fp,
		"IP Address      Type  State    Cost Pri DR             BDR            \n");
	fprintf(fp,
		"----------------------------------------------------------------------\n");

	INTF_LIST(intf, a) {
	    fprintf(fp, "%-15A %-5s %-8s %-4d %-3d %-15A %A\n",
		    intf->ifap->ifa_addr,
		    if_types[intf->type - 1],
		    ospf_intf_states[intf->state],
		    intf->cost,
		    intf->pri,
		    intf->dr ? intf->dr->nbr_addr : sockbuild_str("None"),
		    intf->bdr ? intf->bdr->nbr_addr : sockbuild_str("None"));
	} INTF_LIST_END(intf, a) ;
    } AREA_LIST_END(a) ;

    /* print virtual links */
    if (IAmBorderRtr && ospf.vcnt) {
	fprintf(fp, "\nVirtual Links:\n");
	fprintf(fp,
		"Transit Area    Router ID       Remote IP       Local IP        Type  State    Cost\n");
	fprintf(fp,
		"-----------------------------------------------------------------------------------\n");

	VINTF_LIST(intf) {
	    struct NBR *n = FirstNbr(intf);

	    fprintf(fp, "%-15A %-15A %-15A %-15A %-5s %-8s %-4d\n",
		    sockbuild_in(0, intf->trans_area->area_id),
		    n->nbr_id,
		    n->nbr_addr ? n->nbr_addr : sockbuild_str("N/A"),
		    intf->ifap ? intf->ifap->ifa_addr_local : sockbuild_str("N/A"),
		    if_types[intf->type - 1],
		    ospf_intf_states[intf->state],
		    intf->cost);
	} VINTF_LIST_END(intf) ;
    }
}

/*
 * Show current neigbors and their status
 */
static void
ospf_mon_dump_nbrs __PF2(fp, FILE *,
	       retrans, int)
{
    struct AREA *a;
    struct INTF *intf;
    struct NBR *n;

    AREA_LIST(a) {
	INTF_LIST(intf, a) {
	    fprintf(fp,
		    "\nInterface: %-15A Area: %-15A\n",
		    intf->ifap->ifa_addr,
		    sockbuild_in(0, a->area_id));
	    fprintf(fp,
		    "Router Id       Nbr IP Addr     State      Mode   Prio\n");
	    fprintf(fp,
		    "------------------------------------------------------\n");
	    NBRS_LIST(n, intf) {
		fprintf(fp, "%-15A %-15A %-10s %-6s %d\n",
			n->nbr_id,
			n->nbr_addr,
			ospf_nbr_states[n->state],
			nbr_modes[n->mode],
			n->pri);
	    } NBRS_LIST_END(n, intf) ;
	    if (retrans) {
		ospf_print_nbr_retrans(fp, intf);
	    }
	} INTF_LIST_END(intf, a) ;
    } AREA_LIST_END(a) ;

    if (IAmBorderRtr && ospf.vcnt) {
	fprintf(fp,
		"\nVirtual links:\n");
	fprintf(fp,
		"Interface       Router Id       Nbr IP Addr     State      Mode   Prio\n");
	fprintf(fp,
		"----------------------------------------------------------------------\n");
	VINTF_LIST(intf) {
	    NBRS_LIST(n, intf) {
		fprintf(fp, "%-15A %-15A %-15A %-10s %-6s %d\n",
			intf->ifap ? intf->ifap->ifa_addr : (sockaddr_un *) 0,
			n->nbr_id,
			n->nbr_addr,
			ospf_nbr_states[n->state],
			nbr_modes[n->mode],
			n->pri);
	    } NBRS_LIST_END(n, intf) ;
	    if (retrans) {
		ospf_print_nbr_retrans(fp, intf);
	    }
	} VINTF_LIST_END(intf) ;
    }
}



static void
ospf_mon_dump_lsa __PF5(fp, FILE *,
		area_id, u_int32,
		type, u_int,
		ls_id, u_int32,
		adv_rtr, u_int32)
{
    struct LSDB *db;
    struct AREA *area;

    if (type < 1 || type > 5) {
	fprintf(fp, "Invalid type: %d\n",
		type);
	return;
    }

    AREA_LIST(area) {
	if (area->area_id == area_id) {
	    goto found;
	} else if (area->area_id > area_id) {
	    break;
	}
    } AREA_LIST_END(a) ;

    /* Not found */
    fprintf(fp, "Unknown area: %A\n",
	    sockbuild_in(0, area_id));
    return;

 found:
    db = (struct LSDB *) FindLSA(area, ls_id, adv_rtr, type);
    if (db) {
	int cnt, i;
	struct NET_LA_PIECES *att_rtr;
	struct RTR_LA_PIECES *linkp;
	union LSA_PTR adv;

	adv.rtr = (struct RTR_LA_HDR *) DB_RTR(db);

        {
	    struct LS_HDR *ls_hdr = (struct LS_HDR *) DB_RTR(db);

	    fprintf(fp, "LSA  type: %s ls id: %A adv rtr: %A age: %d\n",
		    trace_state(ospf_ls_type_bits, ls_hdr->ls_type),
		    sockbuild_in(0, ls_hdr->ls_id),
		    sockbuild_in(0, ls_hdr->adv_rtr),
		    ls_hdr->ls_age);

	    fprintf(fp, "     len: %d seq #: %x cksum: 0x%x\n",
		    ntohs(ls_hdr->length),
		    ls_hdr->ls_seq,
		    ls_hdr->ls_chksum);
	}

	switch (adv.rtr->ls_hdr.ls_type) {
	case LS_RTR:
	    fprintf(fp, "     Capabilities: As Border: %s Area Border: %s\n",
		    (ntohs(adv.rtr->E_B) & bit_E) ? "On" : "Off",
		    (ntohs(adv.rtr->E_B) & bit_B) ? "On" : "Off");

	    fprintf(fp, "Link count: %d\n",
		    adv.rtr->lnk_cnt);

	    for (cnt = ntohs(adv.rtr->lnk_cnt),
		 i = 0,
		 linkp = (struct RTR_LA_PIECES *) &adv.rtr->link;
		 i < cnt;
		 linkp = (struct RTR_LA_PIECES *) ((long) linkp +
						   RTR_LA_PIECES_SIZE +
						   ((linkp->metric_cnt) * RTR_LA_METRIC_SIZE)),
		 i++) {
		fprintf(fp,
			"     link id: %-12A data: %-12A type: %-12s metric: %d\n",
			sockbuild_in(0, linkp->lnk_id),
			sockbuild_in(0, linkp->lnk_data),
			ospf_con_types[linkp->con_type],
			ntohs(linkp->tos0_metric));
	    }
	    break;

	case LS_NET:
	    fprintf(fp,
		    "    Net mask: %A\n",
		    sockbuild_in(0, adv.net->net_mask));

	    cnt = ntohs(adv.net->ls_hdr.length) - NET_LA_HDR_SIZE;
	    for (att_rtr = &adv.net->att_rtr, i = 0;
		 i < cnt;
		 att_rtr++, i += 4) {
		fprintf(fp,
			"Attached router: %A\n",
			sockbuild_in(0, att_rtr->lnk_id));
	    }
	    break;

	case LS_SUM_NET:
	    fprintf(fp, "    Net mask: %A",
		    sockbuild_in(0, adv.sum->net_mask));
	    /* Fall through */

	case LS_SUM_ASB:
	    fprintf(fp, " Tos 0 metric: %d\n",
		    ntohl(adv.sum->tos0.tos_metric));
	    break;

	case LS_ASE:
	    fprintf(fp,
		    "     Net mask: %A Tos 0 metric %d E type %d\n",
		    sockbuild_in(0, adv.ase->net_mask),
		    ADV_BIG_METRIC(adv.ase),
		    (ADV_ASE_TYPE2(adv.ase) ? 2 : 1));

	    fprintf(fp,
		    "     Forwarding Address %A Tag %x\n",
		    sockbuild_in(0, adv.ase->tos0.ForwardAddr),
		    ntohl(adv.ase->tos0.ExtRtTag));
	    break;
	}
    } else {
	/* Not found */

	fprintf(fp, "LSDB %s %A %A not found\n",
		paths[type],
		sockbuild_in(0, ls_id),
		sockbuild_in(0, adv_rtr));
    }
}


/*
 * Print the asb rtr and ab rtr part of the ospf routing table
 */
static void
ospf_mon_dump_rtab __PF2(fp, FILE *,
			 which, int)
{
    rt_entry *rt;
    struct OSPF_ROUTE *r;
    struct AREA *a;
    int count = 0;
    int count_net = 0;
    int count_sum = 0;
    int count_ase = 0;

    if (!which
	|| BIT_TEST(which, 0x01)) {
	
	/* As border rtrs */

	fprintf(fp, "AS Border Routes:\n");
	fprintf(fp,
		"Router          Cost AdvRouter       NextHop(s)\n");
	fprintf(fp,
		"----------------------------------------------------\n");
	count = 0;
	AREA_LIST(a) {
	    (void) fprintf(fp, "Area %A:\n",
			   sockbuild_in(0, a->area_id));
	    for (r = a->asbrtab.ptr[NEXT]; r; r = RRT_NEXT(r)) {
		count++;
		fprintf(fp, "%-15A %4d %-15A",
			sockbuild_in(0, RRT_DEST(r)),
			RRT_COST(r),
			sockbuild_in(0, RRT_ADVRTR(r)));
		if (RRT_NH_CNT(r)) {
		    u_int i;
		
		    for (i = 0; i < RRT_NH_CNT(r); i++) {
			fprintf(fp, "%s %-15A\n",
				i ? "                                    " : "",
				sockbuild_in(0, RRT_NH(r)[i]->nh_addr));
		    }
		} else {
		    (void) fprintf(fp, "\n");
		}
	    }
	    fprintf(fp, "\n");
	} AREA_LIST_END(a) ;
	if (count) {
	    fprintf(fp, "Total AS Border routes: %d\n",
		    count);
	}
	fprintf(fp, "\n");
    }

    if (!which
	|| BIT_TEST(which, 0x02)) {
	
	/* Area border rtrs */

	fprintf(fp,"Area Border Routes:\n");
	fprintf(fp,
		"Router          Cost AdvRouter       NextHop(s)\n");
	fprintf(fp,
		"----------------------------------------------------\n");

	count = 0;
	AREA_LIST(a) {
	    (void) fprintf(fp, "Area %A:\n",
			   sockbuild_in(0, a->area_id));
	    for (r = a->abrtab.ptr[NEXT]; r; r = RRT_NEXT(r)) {
		count++;
		fprintf(fp,"%-15A %4d %-15A",
			sockbuild_in(0, RRT_DEST(r)),
			RRT_COST(r),
			sockbuild_in(0, RRT_ADVRTR(r)));
		if (RRT_NH_CNT(r)) {
		    u_int i;
		
		    for (i = 0; i < RRT_NH_CNT(r); i++) {
			fprintf(fp, "%s %-15A\n",
				i ? "                                    " : "",
				sockbuild_in(0, RRT_NH(r)[i]->nh_addr));
		    }
		} else {
		    (void) fprintf(fp, "\n");
		}
	    }
	    fprintf(fp, "\n");
	} AREA_LIST_END(a) ;
	if (count) {
	    fprintf(fp, "Total Area Border Routes: %d\n",
		    count);
	}
	fprintf(fp, "\n");
    }

    if (!which
	|| BIT_TEST(which, 0x04)) {
	/* Summary asb routes */

	fprintf(fp,"Summary AS Border Routes:\n");
	fprintf(fp,
		"Router          Cost AdvRouter       NextHop(s)\n");
	fprintf(fp,
		"---------------------------------------------------\n");

	count = 0;
	for (r = ospf.sum_asb_rtab.ptr[NEXT]; r; r = RRT_NEXT(r)) {
	    count++;
	    fprintf(fp, "%-15A %4d %-15A",
		    sockbuild_in(0, RRT_DEST(r)),
		    RRT_COST(r),
		    sockbuild_in(0, RRT_ADVRTR(r)));
	    if (RRT_NH_CNT(r)) {
		u_int i;
		
		for (i = 0; i < RRT_NH_CNT(r); i++) {
		    fprintf(fp, "%s %-15A\n",
			    i ? "                                                    " : "",
			    sockbuild_in(0, RRT_NH(r)[i]->nh_addr));
		}
	    } else {
		(void) fprintf(fp, "\n");
	    }
	}
	if (count) {
	    fprintf(fp, "Total Summary AS Border Routes: %d\n",
		    count);
	}
	fprintf(fp, "\n");
    }

    if (!which
	|| BIT_TEST(which, 0x38)) {
	/* Networks and Summaries */

	if (!which
	    || BIT_TEST(which, 0x18)) {
	    fprintf(fp, "Networks:\n");
	    fprintf(fp,
		    "Destination        Area            Cost Type NextHop         AdvRouter\n");
	    fprintf(fp,
		    "----------------------------------------------------------------------------\n");
	}
	
	count = 0;
	RTQ_LIST(&ospf.gwp->gw_rtq, rt) {
	    register int i;
	
	    count++;
	    if (ORT_PTYPE(rt) == LS_NET) {
		count_net++;
		if (which
		    && !BIT_TEST(which, 0x08)) {
		    continue;
		}
	    } else {
		count_sum++;
		if (which
		    && !BIT_TEST(which, 0x10)) {
		    continue;
		}
	    }

	    fprintf(fp, "%-18s %-15A %4d %-4s %-15A %-15A\n",
		    ospf_print_dest_mask(rt->rt_dest, rt->rt_dest_mask),
		    sockbuild_in(0, ORT_AREA(rt) ? ORT_AREA(rt)->area_id : OSPF_BACKBONE),
		    ORT_COST(rt),
		    paths[ORT_PTYPE(rt)],
		    RT_ROUTER(rt),
		    sockbuild_in(0, ORT_ADVRTR(rt)));

	    for (i = 0; i < rt->rt_n_gw; i++) {
		if (i != rt->rt_gw_sel) {
		    fprintf(fp, "                                             %-15A\n",
			    rt->rt_routers[i]);
		}
	    }
	} RTQ_LIST_END(&ospf.gwp->gw_rtq, rt) ;

	/* ASEs */
	if (!which
	    || BIT_TEST(which, 0x20)) {

	    fprintf(fp, "ASEs:\n");
	    fprintf(fp,
		    "Destination        Cost E      Tag NextHop         AdvRotuer\n");
	    fprintf(fp,
		    "-----------------------------------------------------------------------------\n");
	}

	RTQ_LIST(&ospf.gwp_ase->gw_rtq, rt) {
	    register int i;

	    count_ase++;

	    if (!which
		|| BIT_TEST(which, 0x20)) {
		fprintf(fp, "%-18s %3d %3d %8x %-15A %A\n",
			ospf_print_dest_mask(rt->rt_dest, rt->rt_dest_mask),
			ORT_COST(rt),
			rt->rt_metric,
			rt->rt_tag,
			RT_ROUTER(rt),
			sockbuild_in(0, ORT_ADVRTR(rt)));

		for (i = 1; i < rt->rt_n_gw; i++) {
		    if (i != rt->rt_gw_sel) {
			fprintf(fp, "                                      %-15A\n",
				rt->rt_routers[i]);
		    }
		}
	    }
	} RTQ_LIST_END(&ospf.gwp_ase->gw_rtq, rt) ;

	fprintf(fp, "Total nets: %d\n\tIntra Area: %d\tInter Area: %d\tASE: %d\n",
		count,
		count_net,
		count_sum,
		count_ase);
    }
}


static void
ospf_mon_dump_errs __PF1(fp, FILE *)
{
    register int type = 0;

    fprintf(fp,"Packets Received:\n");
    do {
	(void) fprintf(fp, "%4d: %-32s%s",
		       ospf_cumlog[type],
		       trace_state(ospf_logtype, type),
		       (type & 1) ? "\n" : "  ");
    } while (++type < 6) ;

    fprintf(fp,"\nPackets Sent:\n");
    do {
	(void) fprintf(fp, "%4d: %-32s%s",
		       ospf_cumlog[type],
		       trace_state(ospf_logtype, type),
		       (type & 1) ? "\n" : "  ");
    } while (++type < 12) ;

    fprintf(fp,"\nErrors:\n");
    do {
	(void) fprintf(fp, "%4d: %-32s%s",
		       ospf_cumlog[type],
		       trace_state(ospf_logtype, type),
		       (type & 1) ? "\n" : "  ");
    } while (++type < OSPF_ERR_LAST) ;
#if	OSPF_ERR_LAST & 1
    (void) fprintf(fp, "\n");
#endif
}


static void
ospf_mon_dump_io __PF1(fp, FILE *)
{
    int i;
    struct AREA *area;
    int count_net = 0;
    int count_sum = 0;
    rt_entry *rt;

    fprintf(fp,
	    "IO stats\n");
    fprintf(fp,
	    "\tInput  Output  Type\n");
    for (i = 0; i < 6; i++) {
	fprintf(fp,
		"\t%6d  %6d  %s\n",
		ospf_cumlog[i],
		ospf_cumlog[i+6],
		trace_state(ospf_logtype, i));
    }

    fprintf(fp, "\tASE: %d checksum sum %X\n",
	    ospf.db_ase_cnt,
	    ospf.db_chksumsum);

    /* LSA counts */
    (void) fprintf(fp, "\n\tLSAs originated: %u\treceived: %u\n\t\t",
		   ospf.orig_new_lsa,
		   ospf.rx_new_lsa);
    for (i = 1; i < LS_MAX; i++) {
	if (ospf.orig_lsa_cnt[i]) {
	    (void) fprintf(fp, "%s%s: %u",
			   i == 1 ? "" : "  ",
			   trace_state(ospf_ls_type_bits, i),
			   ospf.orig_lsa_cnt[i]);
	}
    }
    (void) fprintf(fp, "\n");

    /* Area information */
    AREA_LIST(area) {
	fprintf(fp, "\n\tArea %A:\n",
		sockbuild_in(0, area->area_id));

	fprintf(fp, "\t\tNeighbors: %d\tInterfaces: %d\n",
		area->nbrEcnt,
		area->ifcnt);
	fprintf(fp, "\t\tSpf: %d\tChecksum sum %X\n",
		area->spfcnt,
		area->db_chksumsum);
	if (area->spfsched) {
	    fprintf(fp, "\t\t\tScheduled: %s\n",
		    trace_bits(ospf_sched_bits, (flag_t)
			       area->spfsched));
	}

	fprintf(fp, "\t\tDB: rtr: %d net: %d sumasb: %d sumnet: %d\n",
		area->db_cnts[LS_RTR],
		area->db_cnts[LS_NET],
		area->db_cnts[LS_SUM_ASB],
		area->db_cnts[LS_SUM_NET]);
    } AREA_LIST_END(area) ;
    fprintf(fp, "\n");

    /* Routing table information */
    fprintf(fp, "Routing Table:\n");
    RTQ_LIST(&ospf.gwp->gw_rtq, rt) {
	if (ORT_PTYPE(rt) == LS_NET) {
	    count_net++;
	} else {
	    count_sum++;
	}
    } RTQ_LIST_END(&ospf.gwp->gw_rtq, rt) ;

    fprintf(fp, "\tIntra Area: %d\tInter Area: %d\tASE: %d\n",
	    count_net,
	    count_sum,
	    ospf.gwp_ase->gw_n_routes);
}

static void
ospf_mon_dump_nexthops __PF1(fp, FILE *)
{

    (void) fprintf(fp, "Next hops:\n\n");
    (void) fprintf(fp, "Address          Type      Refcount  Interface\n");
    (void) fprintf(fp, "-------------------------------------------------------------\n");

    if (ospf.nh_list.nh_forw != &ospf.nh_list) {
	register struct NH_BLOCK *np;

	NH_LIST(np) {
	    (void) fprintf(fp, "%-15A  %-8s  %8d  %-15A %s\n",
			   sockbuild_in(0, np->nh_addr),
			   trace_state(ospf_nh_bits, np->nh_type),
			   np->nh_refcount,
			   np->nh_ifap->ifa_addr,
			   np->nh_ifap->ifa_link->ifl_name);
	} NH_LIST_END(np) ;
    }

}


static void
ospf_mon_dump_version __PF1(fp, FILE *)
{
    (void) fprintf(fp, "\n\t%s %s built %s\n\tpid %u  started %s\n\n\tsystem %s\n",
		   task_progname,
		   gated_version,
		   build_date,
		   task_mpid,
		   task_time_start.gt_ctime,
		   task_hostname);
    if (krt_version_kernel) {
	(void) fprintf(fp, "\tversion %s\n", krt_version_kernel);
    }
    (void) fprintf(fp, "\n");
}

/**/

static block_t ospf_mon_block;


static void
ospf_do_mon __PF1(tp, task *)
{
    FILE *fp = fdopen(tp->task_socket, "w");
    struct MON_HDR *mreq = (struct MON_HDR *) tp->task_data;

    if (!fp) {
	trace_log_tp(tp,
		     0,
		     LOG_ERR,
		     ("ospf_do_mon: fdopen: %m"));
	return;
    }

    trace_tp(tp,
	     TR_NORMAL,
	     0,
	     ("ospf_do_mon: processing '%c' %#A -> %#A",
	      mreq->req,
	      task_get_addr_local(tp),
	      task_get_addr_remote(tp)));
    
    switch (mreq->req) {
    case 'a':
	ospf_mon_dump_lsa(fp,
			  mreq->p[0],
			  ntohl(mreq->p[1]),
			  mreq->p[2],
			  mreq->p[3]);
	break;

    case 'c':
	ospf_mon_dump_io(fp);
	break;

    case 'e':
	ospf_mon_dump_errs(fp);
	break;

    case 'h':
	ospf_mon_dump_nexthops(fp);
	break;
	
    case 'l':
	ospf_mon_dump_lsdb(fp,
			   (long) ntohl(mreq->p[0]));
	break;

    case 'A':
	ospf_mon_dump_ase(fp,
			  (long) ntohl(mreq->p[0]));
	break;

    case 'o':
	ospf_mon_dump_rtab(fp,
			   (int) ntohl(mreq->p[0]));
	break;

    case 'I':
	ospf_mon_dump_ifs(fp);
	break;

    case 'N':
	ospf_mon_dump_nbrs(fp,
			   (int) ntohl(mreq->p[0]));
	break;

    case 'V':
	ospf_mon_dump_version(fp);
	break;
	
    default:
	fprintf(fp,
		"Unknown command: '%c'\n\n",
		mreq->req);
    }

    /* Close the stream which closes the socket */
    if (ferror(fp)) {
	trace_tp(tp,
		 TR_ALL,
		 LOG_INFO,
		 ("ospf_do_mon: Error on %#A",
		  tp->task_addr));
    }
	
    if (fprintf(fp, "done\n") == EOF) {
	trace_tp(tp,
		 TR_ALL,
		 LOG_INFO,
		 ("ospf_do_mon: Error writing to %#A",
		  tp->task_addr));
    }
    if (fclose(fp) == EOF) {
	trace_tp(tp,
		 TR_ALL,
		 LOG_INFO,
		 ("ospf_do_mon: Error closing %#A",
		  tp->task_addr));
    }
	
    task_reset_socket(tp);
}


static void
ospf_mon_connect __PF1(tp, task *)
{
    sockaddr_un *addr;

    addr = task_get_addr_remote(tp);
    if (addr
	&& sockaddrcmp_in(addr, tp->task_addr)) {
	/* Connect succeeded */

	task_set_connect(tp, 0);
	BIT_RESET(tp->task_flags, TASKF_CONNECT);
	task_set_socket(tp, tp->task_socket);

	/* Process monitor request */
	ospf_do_mon(tp);

	/* Free the data */
	task_block_free(ospf_mon_block, tp->task_data);
    } else {
	/* Connect failed */

	trace_tp(tp,
		 TR_ALL,
		 0,
		 ("ospf_mon_connect: Unable to connect to %#A",
		  tp->task_addr));
    }
    
    task_delete(tp);
}


static void
ospf_mon_dump __PF2(tp, task *,
		    fp, FILE *)
{
    struct MON_HDR *mreq = (struct MON_HDR *) tp->task_data;

    (void) fprintf(fp, "\tAddress: %#A\tRequest: '%c' local: %d p: %x %x %x %x\n",
		   tp->task_addr,
		   mreq->req,
		   ntohs(mreq->local),
		   ntohl(mreq->p[0]),
		   ntohl(mreq->p[1]),
		   htonl(mreq->p[2]),
		   htonl(mreq->p[3]));
}

#ifdef SCO_UW21
/* re-call connect() to establish connection on non-blocking socket
 */
static void
ospf_do_connect __PF1(tp, task *)
{
    if (task_connect(tp)) {
	switch (errno) {
	case EINPROGRESS:	/* FALLTHRU */
	case EALREADY:		/* better luck next time */
		break ;
	case EISCONN:
		ospf_mon_connect(tp);
		break ;
	default:
		trace_log_tp(tp,
			0,
			LOG_ERR,
			("ospf_do_connect: can not connect to %#A: %m",
			tp->task_addr));
		task_delete(tp);
		break ;
	}
    }
}
#endif /* SCO_UW21 */


int
ospf_rx_mon __PF5(mreq, struct MON_HDR *,
		  intf, struct INTF *,
		  src, sockaddr_un *,
		  router_id, sockaddr_un *,
		  olen, size_t)
{
    task *tp;

    tp = task_alloc("OSPF_Monitor",
		    TASKPRI_PROTO,
		    ospf.trace_options);
    task_set_dump(tp, ospf_mon_dump);

    if (mreq->type != MREQUEST) {
	trace_tp(tp,
		 TR_ALL,
		 0,
		 ("Bad mrequest type"));
	return GOOD_RX;

    }

    if (mreq->local) {
	task *tp2;

	/* Open a socket to the specified port */

	/* Get the address */
	tp->task_addr = sockdup(src);
	sock2port(tp->task_addr) = mreq->port;

	if ((tp2 = task_locate("OSPF_Monitor", tp->task_addr))) {
	    /* If another request from same addr/port, delete the first one */

	    task_delete(tp2);
	}
	
	if ((tp->task_socket = task_get_socket(tp, AF_INET, SOCK_STREAM, 0)) < 0) {
	    trace_log_tp(tp,
			 0,
			 LOG_ERR,
			 ("mon_open: can not get socket for OSPF monitor response"));
	    return GOOD_RX;
	}
#ifdef	SCO_UW21
	task_set_connect(tp, ospf_do_connect);
#else	/* SCO_UW21 */
	task_set_connect(tp, ospf_mon_connect);
#endif	/* SCO_UW21 */
	BIT_SET(tp->task_flags, TASKF_CONNECT);
	if (!task_create(tp)) {
	    task_quit(EINVAL);
	}

	/* Make a copy of the data area */
	if (!ospf_mon_block) {
	    ospf_mon_block = task_block_init(sizeof *mreq, "ospf_mreq");
	}
	tp->task_data = task_block_alloc(ospf_mon_block);
	bcopy((caddr_t) mreq, (void_t) tp->task_data, sizeof *mreq);

	/* Set non-blocking */
	if (task_set_option(tp,
			    TASKOPTION_NONBLOCKING,
			    TRUE) < 0) {
	    task_delete(tp);
	    return GOOD_RX;
	}

	/* Set the buffer as high as possible */
	if (task_set_option(tp,
			    TASKOPTION_SENDBUF,
			    task_maxpacket) < 0) {
	    task_quit(errno);
	}
	
	if (task_connect(tp)) {
	    switch (errno) {
	    case EINPROGRESS:
		/* We will be notified when it completes */
		return GOOD_RX;

	    default:
		/* Opps */
		trace_log_tp(tp,
			     0,
			     LOG_ERR,
			     ("mon_open: can not connect to %#A: %m",
			      src));
		task_delete(tp);
		return GOOD_RX;
	    }
	} else {
	    ospf_mon_connect(tp);
	}

    } else {
	tp->task_socket = open(_PATH_DEVNULL, O_WRONLY);
	if (tp->task_socket < 0) {
	    trace_log_tp(tp,
			 0,
			 LOG_ERR,
			 ("mon_open: can not open %s: %m",
			  _PATH_DEVNULL));
	    task_delete(tp);
	    return GOOD_RX;
	}
	tp->task_data = (void_t) mreq;
	ospf_do_mon(tp);
	task_delete(tp);
    }

    return GOOD_RX;
}

