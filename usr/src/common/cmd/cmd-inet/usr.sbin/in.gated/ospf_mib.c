#ident	"@(#)ospf_mib.c	1.3"
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


#define	INCLUDE_ISODE_SNMP
#include "include.h"
#include "inet.h"
#include "ospf.h"
#include "snmp_isode.h"


PROTOTYPE(o_ospf,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));
PROTOTYPE(o_area,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));
PROTOTYPE(o_stub,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));
PROTOTYPE(o_lsdb,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));
PROTOTYPE(o_range,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));
PROTOTYPE(o_host,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));
PROTOTYPE(o_intf,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));
PROTOTYPE(o_metric,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));
PROTOTYPE(o_vintf,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));
PROTOTYPE(o_nbr,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));
PROTOTYPE(o_vnbr,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));
PROTOTYPE(o_ase,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));
PROTOTYPE(o_aggr,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));

static struct object_table ospf_objects[] = {
#define	IospfRouterId		1
#define	IospfAdminStat		2
#define	IospfVersionNumber	3
#define	IospfAreaBdrRtrStatus	4
#define	IospfASBdrRtrStatus	5
#define	IospfExternLsaCount	6
#define	IospfExternLsaCksumSum	7
#define	IospfTOSSupport		8
#define	IospfOriginateNewLsas	9
#define	IospfRxNewLsas		10
#define	IospfExtLsdbLimit	11
#define	IospfMulticastExtensions	12
    OTE(ospfRouterId, o_ospf, NULL),
    OTE(ospfAdminStat, o_ospf, NULL),
    OTE(ospfVersionNumber, o_ospf, NULL),
    OTE(ospfAreaBdrRtrStatus, o_ospf, NULL),
    OTE(ospfASBdrRtrStatus, o_ospf, NULL),
    OTE(ospfExternLsaCount, o_ospf, NULL),
    OTE(ospfExternLsaCksumSum, o_ospf, NULL),
    OTE(ospfTOSSupport, o_ospf, NULL),
    OTE(ospfOriginateNewLsas, o_ospf, NULL),
    OTE(ospfRxNewLsas, o_ospf, NULL),
    OTE(ospfExtLsdbLimit, o_ospf, NULL),
    OTE(ospfMulticastExtensions, o_ospf, NULL),
#define	MIB_ME_INTRA	0x01
#define	MIB_ME_INTER	0x02
#define	MIB_ME_EXTERN	0x04
    
#define	IospfAreaId		13
#define	IospfAuthType		14
#define	IospfImportAsExtern	15
#define	IospfSpfRuns		16
#define	IospfAreaBdrRtrCount	17
#define	IospfAsBdrRtrCount	18
#define	IospfAreaLsaCount	19
#define	IospfAreaLsaCksumSum	20
#define	IospfAreaSummary	21
#define	IospfAreaStatus		22
    OTE(ospfAreaId, o_area, NULL),
    OTE(ospfAuthType, o_area, NULL),
    OTE(ospfImportAsExtern, o_area, NULL),
    OTE(ospfSpfRuns, o_area, NULL),
    OTE(ospfAreaBdrRtrCount, o_area, NULL),
    OTE(ospfAsBdrRtrCount, o_area, NULL),
    OTE(ospfAreaLsaCount, o_area, NULL),
    OTE(ospfAreaLsaCksumSum, o_area, NULL),
    OTE(ospfAreaSummary, o_area, NULL),
#define	MIB_NOAREASSUMMARY	1
#define	MIB_SENDAREASUMMARY	2
    OTE(ospfAreaStatus, o_area, NULL),

    
#define	IospfStubAreaId		23
#define	IospfStubTOS		24
#define	IospfStubMetric		25
#define	IospfStubStatus		26
#define	IospfStubMetricType	27
    OTE(ospfStubAreaId, o_stub, NULL),
    OTE(ospfStubTOS, o_stub, NULL),
    OTE(ospfStubMetric, o_stub, NULL),
    OTE(ospfStubStatus, o_stub, NULL),
    OTE(ospfStubMetricType, o_stub, NULL),
#define	MIB_OSPFMETRIC		1
#define	MIB_COMPARABLECOST	2
#define	MIB_NONCOMPARABLE	3

    
#define	IospfLsdbAreaId		28
#define	IospfLsdbType		29
#define	IospfLsdbLsid		30
#define	IospfLsdbRouterId	31
#define	IospfLsdbSequence	32
#define	IospfLsdbAge		33
#define	IospfLsdbChecksum	34
#define	IospfLsdbAdvertisement	35
    OTE(ospfLsdbAreaId, o_lsdb, NULL),
    OTE(ospfLsdbType, o_lsdb, NULL),
    OTE(ospfLsdbLsid, o_lsdb, NULL),
    OTE(ospfLsdbRouterId, o_lsdb, NULL),
    OTE(ospfLsdbSequence, o_lsdb, NULL),
    OTE(ospfLsdbAge, o_lsdb, NULL),
    OTE(ospfLsdbChecksum, o_lsdb, NULL),
    OTE(ospfLsdbAdvertisement, o_lsdb, NULL),


#define	IospfAreaRangeAreaId	36
#define	IospfAreaRangeNet	37
#define	IospfAreaRangeMask	38
#define	IospfAreaRangeStatus	39
#define	IospfAreaRangeEffect	40
    OTE(ospfAreaRangeAreaId, o_range, NULL),
    OTE(ospfAreaRangeNet, o_range, NULL),
    OTE(ospfAreaRangeMask, o_range, NULL),
    OTE(ospfAreaRangeStatus, o_range, NULL),
    OTE(ospfAreaRangeEffect, o_range, NULL),
#define	MIB_ADVERTISE_MATCHING		1
#define	MIB_DONOT_ADVERTISE_MATCHING	2

    
#define	IospfHostIpAddress	41
#define	IospfHostTOS		42
#define	IospfHostMetric		43
#define	IospfHostStatus		44
#define	IospfHostAreaID		45
    OTE(ospfHostIpAddress, o_host, NULL),
    OTE(ospfHostTOS, o_host, NULL),
    OTE(ospfHostMetric, o_host, NULL),
    OTE(ospfHostStatus, o_host, NULL),
    OTE(ospfHostAreaID, o_host, NULL),

    
#define	IospfIfIpAddress		46
#define	IospfAddressLessIf		47
#define	IospfIfAreaId			48
#define	IospfIfType			49
#define	IospfIfAdminStat		50
#define	IospfIfRtrPriority		51
#define	IospfIfTransitDelay		52
#define	IospfIfRetransInterval		53
#define	IospfIfHelloInterval		54
#define	IospfIfRtrDeadInterval		55
#define	IospfIfPollInterval		56
#define	IospfIfState			57
#define	IospfIfDesignatedRouter		58
#define	IospfIfBackupDesignatedRouter	59
#define	IospfIfEvents			60
#define	IospfIfAuthKey			61
#define	IospfIfStatus			62
#define	IospfIfMulticastForwarding	63
    OTE(ospfIfIpAddress, o_intf, NULL),
    OTE(ospfAddressLessIf, o_intf, NULL),
    OTE(ospfIfAreaId, o_intf, NULL),
    OTE(ospfIfType, o_intf, NULL),
    OTE(ospfIfAdminStat, o_intf, NULL),
    OTE(ospfIfRtrPriority, o_intf, NULL),
    OTE(ospfIfTransitDelay, o_intf, NULL),
    OTE(ospfIfRetransInterval, o_intf, NULL),
    OTE(ospfIfHelloInterval, o_intf, NULL),
    OTE(ospfIfRtrDeadInterval, o_intf, NULL),
    OTE(ospfIfPollInterval, o_intf, NULL),
    OTE(ospfIfState, o_intf, NULL),
#define	I_STATE_DOWN		1
#define	I_STATE_LOOPBACK	2
#define	I_STATE_WAITING		3
#define	I_STATE_P2P		4
#define	I_STATE_DR		5
#define	I_STATE_BDR		6
#define	I_STATE_DROTHER		7
    OTE(ospfIfDesignatedRouter, o_intf, NULL),
    OTE(ospfIfBackupDesignatedRouter, o_intf, NULL),
    OTE(ospfIfEvents, o_intf, NULL),
    OTE(ospfIfAuthKey, o_intf, NULL),
    OTE(ospfIfStatus, o_intf, NULL),
    OTE(ospfIfMulticastForwarding, o_intf, NULL),
#define	MIB_BLOCKED	1
#define	MIB_MULTICAST	2
#define	MIB_UNICAST	3


#define	IospfIfMetricIpAddress		64
#define	IospfIfMetricAddressLessIf	65
#define	IospfIfMetricTOS		66
#define	IospfIfMetricValue		67
#define	IospfIfMetricStatus		68
    OTE(ospfIfMetricIpAddress, o_metric, NULL),
    OTE(ospfIfMetricAddressLessIf, o_metric, NULL),
    OTE(ospfIfMetricTOS, o_metric, NULL),
    OTE(ospfIfMetricValue, o_metric, NULL),
    OTE(ospfIfMetricStatus, o_metric, NULL),


#define	IospfVirtIfAreaId		69
#define	IospfVirtIfNeighbor		70
#define	IospfVirtIfTransitDelay		71
#define	IospfVirtIfRetransInterval	72
#define	IospfVirtIfHelloInterval	73
#define	IospfVirtIfRtrDeadInterval	74
#define	IospfVirtIfState		75
#define	IospfVirtIfEvents		76
#define	IospfVirtIfAuthKey		77
#define	IospfVirtIfStatus		78
    OTE(ospfVirtIfAreaId, o_vintf, NULL),
    OTE(ospfVirtIfNeighbor, o_vintf, NULL),
    OTE(ospfVirtIfTransitDelay, o_vintf, NULL),
    OTE(ospfVirtIfRetransInterval, o_vintf, NULL),
    OTE(ospfVirtIfHelloInterval, o_vintf, NULL),
    OTE(ospfVirtIfRtrDeadInterval, o_vintf, NULL),
    OTE(ospfVirtIfState, o_vintf, NULL),
    OTE(ospfVirtIfEvents, o_vintf, NULL),
    OTE(ospfVirtIfAuthKey, o_vintf, NULL),
    OTE(ospfVirtIfStatus, o_vintf, NULL),


#define	IospfNbrIpAddr			79
#define	IospfNbrAddressLessIndex	80
#define	IospfNbrRtrId			81
#define	IospfNbrOptions			82
#define	IospfNbrPriority		83
#define	IospfNbrState			84
#define	IospfNbrEvents			85
#define	IospfNbrLsRetransQLen		86
#define	IospfNbmaNbrStatus		87
#define	IospfNbmaNbrPermanence		88
    OTE(ospfNbrIpAddr, o_nbr, NULL),
    OTE(ospfNbrAddressLessIndex, o_nbr, NULL),
    OTE(ospfNbrRtrId, o_nbr, NULL),
    OTE(ospfNbrOptions, o_nbr, NULL),
#define	MIB_BIT_TOS	0x01
#define	MIB_BIT_ASE	0x02
#define	MIB_BIT_MULTI	0x04
#define	MIB_BIT_NSSA	0x08
    OTE(ospfNbrPriority, o_nbr, NULL),
    OTE(ospfNbrState, o_nbr, NULL),
#define	N_STATE_DOWN		1
#define	N_STATE_ATTEMPT		2
#define	N_STATE_INIT		3
#define	N_STATE_2WAY		4
#define	N_STATE_EXSTART		5
#define	N_STATE_EXCHANGE	6
#define	N_STATE_LOADING		7
#define	N_STATE_FULL		8
    OTE(ospfNbrEvents, o_nbr, NULL),
    OTE(ospfNbrLsRetransQLen, o_nbr, NULL),
    OTE(ospfNbmaNbrStatus, o_nbr, NULL),
    OTE(ospfNbmaNbrPermanence, o_nbr, NULL),
#define	N_PERMANENCE_DYNAMIC	1
#define	N_PERMANENCE_PERMANENT	2


#define	IospfVirtNbrArea		89
#define	IospfVirtNbrRtrId		90
#define	IospfVirtNbrIpAddr		91
#define	IospfVirtNbrOptions		92
#define	IospfVirtNbrState		93
#define	IospfVirtNbrEvents		94
#define	IospfVirtNbrLsRetransQLen	95
    OTE(ospfVirtNbrArea, o_vnbr, NULL),
    OTE(ospfVirtNbrRtrId, o_vnbr, NULL),
    OTE(ospfVirtNbrIpAddr, o_vnbr, NULL),
    OTE(ospfVirtNbrOptions, o_vnbr, NULL),
    OTE(ospfVirtNbrState, o_vnbr, NULL),
    OTE(ospfVirtNbrEvents, o_vnbr, NULL),
    OTE(ospfVirtNbrLsRetransQLen, o_vnbr, NULL),


#define	IospfExtLsdbType		96
#define	IospfExtLsdbLsid		97
#define	IospfExtLsdbRouterId		98
#define	IospfExtLsdbSequence		99
#define	IospfExtLsdbAge			100
#define	IospfExtLsdbChecksum		101
#define	IospfExtLsdbAdvertisement	102
    OTE(ospfExtLsdbType, o_ase, NULL),
    OTE(ospfExtLsdbLsid, o_ase, NULL),
    OTE(ospfExtLsdbRouterId, o_ase, NULL),
    OTE(ospfExtLsdbSequence, o_ase, NULL),
    OTE(ospfExtLsdbAge, o_ase, NULL),
    OTE(ospfExtLsdbChecksum, o_ase, NULL),
    OTE(ospfExtLsdbAdvertisement, o_ase, NULL),


#define	IospfAreaAggregateAreaID	103
#define	IospfAreaAggregateLsdbType	104
#define	IospfAreaAggregateNet		105
#define	IospfAreaAggregateMask		106
#define	IospfAreaAggregateStatus	107
#define	IospfAreaAggregateEffect	108
    OTE(ospfAreaAggregateAreaID, o_aggr, NULL),
    OTE(ospfAreaAggregateLsdbType, o_aggr, NULL),
#define	MIB_AGGR_SUMMARYLINK		3
#define	MIB_AGGR_NSSAEXTERNALLINK	7
    OTE(ospfAreaAggregateNet, o_aggr, NULL),
    OTE(ospfAreaAggregateMask, o_aggr, NULL),
    OTE(ospfAreaAggregateStatus, o_aggr, NULL),
    OTE(ospfAreaAggregateEffect, o_aggr, NULL),
    
    { NULL }
};


#define	MIB_ENABLED	1
#define	MIB_DISABLED	2

#define	MIB_TRUE	1
#define	MIB_FALSE	2

#define	MIB_VALID	1
#define	MIB_INVALID	2

static struct snmp_tree ospf_mib_tree = {
    NULL, NULL,
    "ospf",
    NULLOID,
    readOnly,
    ospf_objects,
    0
};


/* General group */
static int
o_ospf __PF3(oi, OI,
	     v, register struct type_SNMP_VarBind *,
	     offset, int)
{
    register OID    oid = oi->oi_name;
    register OT	    ot = oi->oi_type;

    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
	if (oid->oid_nelem != ot->ot_name->oid_nelem + 1
	    || oid->oid_elements[oid->oid_nelem - 1]) {
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
	if (oid->oid_nelem == ot->ot_name->oid_nelem) {
	    OID new;

	    if ((new = oid_extend(oid, 1)) == NULLOID) {
		return int_SNMP_error__status_genErr;
	    }
	    new->oid_elements[new->oid_nelem - 1] = 0;

	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    return NOTOK;
	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }

    switch (ot2object(ot)->ot_info) {
    case IospfRouterId:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(ospf.router_id,
				  (int *) 0));
	
    case IospfAdminStat:
	return o_integer(oi, v, ospf.ospf_admin_stat == OSPF_ENABLED ? MIB_ENABLED : MIB_DISABLED);
	
    case IospfVersionNumber:
	return o_integer(oi, v, OSPF_VERSION);

    case IospfAreaBdrRtrStatus:
	return o_integer(oi, v, IAmBorderRtr ? MIB_TRUE : MIB_FALSE);

    case IospfASBdrRtrStatus:
	return o_integer(oi, v, ospf.asbr ? MIB_TRUE : MIB_FALSE);
	
    case IospfExternLsaCount:
	return o_integer(oi, v, ospf.db_ase_cnt);
	
    case IospfExternLsaCksumSum:
	return o_integer(oi, v, ospf.db_chksumsum);

    case IospfTOSSupport:
	return o_integer(oi, v, MIB_FALSE);

    case IospfOriginateNewLsas:
	return o_integer(oi, v, ospf.orig_new_lsa);

    case IospfRxNewLsas:
	return o_integer(oi, v, ospf.rx_new_lsa);
	
    case IospfExtLsdbLimit:
	return o_integer(oi, v, -1);	/* Not supported */

    case IospfMulticastExtensions:
	return o_integer(oi, v, 0);	/* Not supported */
    }

    return int_SNMP_error__status_noSuchName;
}


/* Area group */


static struct AREA *
o_area_lookup __PF3(ip, register unsigned int *,
		    len, u_int,
		    isnext, int)
{
    static unsigned int *last;
    static struct AREA *last_area;

    if (snmp_last_match(&last, ip, len, isnext)) {
	return last_area;
    }

    if (len) {
	u_int32 area_id;
	register struct AREA *area;
	
	oid2ipaddr(ip, &area_id);

	GNTOHL(area_id);

	AREA_LIST(area) {
	    register u_int32 cur_id = ntohl(area->area_id);
	    
	    if (cur_id == area_id) {
		if (!isnext) {
		    return last_area = area;
		}
	    } else if (cur_id > area_id){
		return last_area = isnext ? area : (struct AREA *) 0;
	    }
	} AREA_LIST_END(area) ;

	return last_area = (struct AREA *) 0;
    }
    
    return last_area = ospf.area.area_forw;
}


static int
o_area __PF3 (oi, OI,
	      v, register struct type_SNMP_VarBind *,
	      offset, int)
{
    register int    i;
    register unsigned int *ip,
			  *jp;
    register struct AREA *area;
    register OID    oid = oi->oi_name;
    register OT	    ot = oi->oi_type;
    OID		    new;

    /* INDEX { ospfAreaID } */
#define	NDX_SIZE	(sizeof (area->area_id))
    
    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
	if (oid->oid_nelem != ot->ot_name->oid_nelem + NDX_SIZE) {
		return int_SNMP_error__status_noSuchName;
	    }
	area = o_area_lookup(oid->oid_elements + oid->oid_nelem - NDX_SIZE,
			    NDX_SIZE,
			    0);
	if (!area) {
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
	/* next request with incomplete instance? */
	if ((i = oid->oid_nelem - ot->ot_name->oid_nelem) != 0 && i < NDX_SIZE) {
	    for (jp = (ip = oid->oid_elements + 
		       ot->ot_name->oid_nelem - 1) + i;
		 jp > ip;
		 jp--) {
		if (*jp != 0) {
		    break;
		}
	    }
	    if (jp == ip) {
		oid->oid_nelem = ot->ot_name->oid_nelem;
	    } else {
		if ((new = oid_normalize(oid, NDX_SIZE - i, 256)) == NULLOID) {
			return NOTOK;
		    }
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    }
	}

	/* next request with no instance? */
	if (oid->oid_nelem == ot->ot_name->oid_nelem) {
	    area = o_area_lookup((unsigned int *) 0,
				 0,
				 TRUE);
	    if (!area) {
		return NOTOK;
	    }

	    if ((new = oid_extend(oid, NDX_SIZE)) == NULLOID) {
		return int_SNMP_error__status_genErr;
	    }

	    ip = new->oid_elements + new->oid_nelem - NDX_SIZE;
	    STR_OID(ip, &area->area_id, sizeof (area->area_id));
		
	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    int j;

	    area = o_area_lookup(ip = oid->oid_elements + ot->ot_name->oid_nelem,
				     (u_int) (j = oid->oid_nelem - ot->ot_name->oid_nelem),
				     TRUE);
	    if (!area) {
		return NOTOK;
	    }

	    if ((i = j - NDX_SIZE) < 0) {
		if ((new = oid_extend(oid, -i)) == NULLOID) {
		    return int_SNMP_error__status_genErr;
		}
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    } else if (i > 0) {
		oid->oid_nelem -= i;
	    }
		
	    ip = oid->oid_elements + ot->ot_name->oid_nelem;
	    STR_OID(ip, &area->area_id, sizeof (area->area_id));
	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }
#undef	NDX_SIZE

    switch (ot2object(ot)->ot_info) {
    case IospfAreaId:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(sockbuild_in(0, area->area_id),
				  (int *) 0));

    case IospfAuthType:
	return o_integer(oi, v, area->authtype);

    case IospfImportAsExtern:
	return o_integer(oi, v, BIT_TEST(area->area_flags, OSPF_AREAF_STUB) ? MIB_FALSE : MIB_TRUE);

    case IospfSpfRuns:
	return o_integer(oi, v, area->spfcnt);

    case IospfAreaBdrRtrCount:
	return o_integer(oi, v, area->abr_cnt);

    case IospfAsBdrRtrCount:
	return o_integer(oi, v, area->asbr_cnt);

    case IospfAreaLsaCount:
	return o_integer(oi, v, area->db_int_cnt);	

    case IospfAreaLsaCksumSum:
	return o_integer(oi, v, area->db_chksumsum);

    case IospfAreaSummary:
	return o_integer(oi, v, MIB_SENDAREASUMMARY);

    case IospfAreaStatus:
	return o_integer(oi, v, MIB_VALID);
    }
    
    return int_SNMP_error__status_noSuchName;
}


/* Stub area group */

/* INDEX { ospfStubAreaId, ospfStubTOS } */
#define	NDX_SIZE	(sizeof (struct in_addr) + 1)

static struct AREA *
o_stub_lookup __PF3(ip, register unsigned int *,
		    len, u_int,
		    isnext, int)
{
    u_int32 area_id = 0;
    register struct AREA *area;
    static unsigned int *last;
    static struct AREA *last_area;

    if (snmp_last_match(&last, ip, len, isnext)) {
	return last_area;
    }

    if (len) {
	oid2ipaddr(ip, &area_id);
	GNTOHL(area_id);

	if (ip[sizeof (struct in_addr)]) {
	    /* We don't support TOS */
	    return last_area = (struct AREA *) 0;
	}
    }

    AREA_LIST(area) {
	register u_int32 cur_id;

	if (!BIT_TEST(area->area_flags, OSPF_AREAF_STUB)) {
	    /* Not a stub area */
	    continue;
	}

	cur_id = ntohl(area->area_id);

	if (cur_id == area_id) {
	    if (!isnext) {
		return last_area = area;
	    }
	} else if (cur_id > area_id){
	    return last_area = isnext ? area : (struct AREA *) 0;
	}
    } AREA_LIST_END(area) ;

    return last_area = (struct AREA *) 0;
}


static int
o_stub __PF3(oi, OI,
	     v, register struct type_SNMP_VarBind *,
	     offset, int)
{
    register int    i;
    register unsigned int *ip,
			  *jp;
    register struct AREA *area;
    register OID    oid = oi->oi_name;
    register OT	    ot = oi->oi_type;
    OID		    new;

    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
	if (oid->oid_nelem != ot->ot_name->oid_nelem + NDX_SIZE) {
		return int_SNMP_error__status_noSuchName;
	    }
	area = o_stub_lookup(oid->oid_elements + oid->oid_nelem - NDX_SIZE,
			     NDX_SIZE,
			     0);
	if (!area) {
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
	/* next request with incomplete instance? */
	if ((i = oid->oid_nelem - ot->ot_name->oid_nelem) != 0 && i < NDX_SIZE) {
	    for (jp = (ip = oid->oid_elements + 
		       ot->ot_name->oid_nelem - 1) + i;
		 jp > ip;
		 jp--) {
		if (*jp != 0) {
		    break;
		}
	    }
	    if (jp == ip) {
		oid->oid_nelem = ot->ot_name->oid_nelem;
	    } else {
		if ((new = oid_normalize(oid, NDX_SIZE - i, 256)) == NULLOID) {
			return NOTOK;
		    }
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    }
	}

	/* next request with no instance? */
	if (oid->oid_nelem == ot->ot_name->oid_nelem) {
	    area = o_stub_lookup((unsigned int *) 0,
				 0,
				 TRUE);
	    if (!area) {
		return NOTOK;
	    }

	    if ((new = oid_extend(oid, NDX_SIZE)) == NULLOID) {
		return int_SNMP_error__status_genErr;
	    }

	    ip = new->oid_elements + new->oid_nelem - NDX_SIZE;
	    STR_OID(ip, &area->area_id, sizeof (area->area_id));
		
	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    int j;

	    area = o_stub_lookup(ip = oid->oid_elements + ot->ot_name->oid_nelem,
				 (u_int) (j = oid->oid_nelem - ot->ot_name->oid_nelem),
				 TRUE);
	    if (!area) {
		return NOTOK;
	    }

	    if ((i = j - NDX_SIZE) < 0) {
		if ((new = oid_extend(oid, -i)) == NULLOID) {
		    return int_SNMP_error__status_genErr;
		}
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    } else if (i > 0) {
		oid->oid_nelem -= i;
	    }
		
	    ip = oid->oid_elements + ot->ot_name->oid_nelem;
	    STR_OID(ip, &area->area_id, sizeof (area->area_id));
	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }
#undef	NDX_SIZE

    switch (ot2object(ot)->ot_info) {
    case IospfStubAreaId:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(sockbuild_in(0, area->area_id),
				  (int *) 0));

    case IospfStubTOS:
	return o_integer(oi, v, 0);

    case IospfStubMetric:
	return o_integer(oi, v, BIT_TEST(area->area_flags, OSPF_AREAF_STUB_DEFAULT) ? area->dflt_metric : (unsigned) -1);

    case IospfStubStatus:
	return o_integer(oi, v, MIB_VALID);

    case IospfStubMetricType:
	return o_integer(oi, v, MIB_OSPFMETRIC);
    }

    return int_SNMP_error__status_noSuchName;
}


/* Link state database group */

/* INDEX { ospfLsdbAreaId, ospfLsdbType, ospfLsdbLsid, ospfLsdbRouterId } */
#define	NDX_SIZE	(sizeof (struct in_addr) + 1 + sizeof (struct in_addr) + sizeof (struct in_addr))

#define	MLSDB_LIST(list, lpp)		for ((lpp) = (list); *(lpp); (lpp)++)
#define	MLSDB_LIST_END(list, lpp)	

static int
o_lsdb_compare __PF2(le1, const VOID_T,
		     le2, const VOID_T)
{
    register u_int32 key1_0 = ntohl(LS_ID(*((struct LSDB * const *) le1)));
    register u_int32 key1_1 = ntohl(ADV_RTR(*((struct LSDB * const *) le1)));
    register u_int32 key2_0 = ntohl(LS_ID(*((struct LSDB * const *) le2)));
    register u_int32 key2_1 = ntohl(ADV_RTR(*((struct LSDB * const *) le2)));

    if (key1_0 > key2_0) {
	return 1;
    } else if (key1_0 < key2_0
	       || key1_1 < key2_1) {
	return -1;
    } else if (key1_1 > key2_1) {
	return 1;
    }

    return 0;
}


static void
o_lsdb_get __PF5(lsdb_cnt, u_int,
		 list_head, struct LSDB_HEAD *,
		 list, struct LSDB ***,
		 size, u_int *,
		 cnt, u_int *)
{
    register struct LSDB **lsdbp;
    register struct LSDB_HEAD *hp;

    if (*size < lsdb_cnt) {
	if (*list) {
	    lsdbp = *list;

	    task_block_reclaim((size_t) ((*size + 1) * sizeof (struct LSDB *)), (void_t) *list);
	}

	*size = lsdb_cnt;

	*list = (struct LSDB **) task_block_malloc((size_t) ((*size + 1) * sizeof (struct LSDB *)));
    }

    lsdbp = *list;
    *cnt = 0;
    
    LSDB_HEAD_LIST(list_head, hp, 0, HTBLSIZE) {
	register struct LSDB *db;

	LSDB_LIST(hp, db) {
	    if (!DB_FREEME(db)) {
		*lsdbp++ = db;
		(*cnt)++;		     
	    }
	} LSDB_LIST_END(hp, db) ;
    } LSDB_HEAD_LIST_END(list_head, hp, 0, HTBLSIZE) ;

    *lsdbp++ = (struct LSDB *) 0;

    qsort(*list,
	  *cnt,
	  sizeof (struct LSDB **),
	  o_lsdb_compare);
}


static struct LSDB *
o_lsdb_lookup __PF3(ip, register unsigned int *,
		    len, u_int,
		    isnext, int)
{
    static struct LSDB *last_lsdb;
    static unsigned int *last;
    static int last_quantum;

    if (last_quantum != snmp_quantum) {
	int changed = 0;
	register struct AREA *area;

	last_quantum = snmp_quantum;

	AREA_LIST(area) {
	    if (area->db_chksumsum != area->mib_chksumsum) {
		u_int type;
		
		/* Time to rebuild the sorted lists */

		changed++;

		for (type = LS_RTR; type < LS_ASE; type++) {
		    o_lsdb_get(area->db_int_cnt,
			       area->htbl[type],
			       &area->mib_lsdb_list[type],
			       &area->mib_lsdb_size[type],
			       &area->mib_lsdb_cnt[type]);
		}

		area->mib_chksumsum = area->db_chksumsum;
	    }
	} AREA_LIST_END(area);

	if (changed && last) {
	    task_mem_free((task *) 0, (void_t) last);
	    last = (unsigned int *) 0;
	}
    }

    if (snmp_last_match(&last, ip, len, isnext)) {
	return last_lsdb;
    }

    if (len) {
	u_int32 area_id, key0, key1;
	int type_id;
	register struct AREA *ap;

	oid2ipaddr(ip, &area_id);
	ip += sizeof (struct in_addr);
	GNTOHL(area_id);

	type_id = *ip++;

	oid2ipaddr(ip, &key0);
	ip += sizeof (struct in_addr);
	GNTOHL(key0);

	oid2ipaddr(ip, &key1);
	GNTOHL(key1);

	if (isnext) {
	    register u_int next = 0;

	    AREA_LIST(ap) {
		register u_int type;

		if (!next && area_id > ntohl(ap->area_id)) {
		    continue;
		}
		
		for (type = type_id; type < LS_ASE; type++) {
		    register struct LSDB **lpp;

		    MLSDB_LIST(ap->mib_lsdb_list[type], lpp) {
			register struct LSDB *lp = *lpp;
			register u_int32 cur_key0 = ntohl(LS_ID(lp));
			register u_int32 cur_key1 = ntohl(ADV_RTR(lp));

			if (next
			    || cur_key0 > key0
			    || (cur_key0 == key0
				&& cur_key1 > key1)) {
			    return last_lsdb = lp;
			}
			if (cur_key0 == key0
			    && cur_key1 == key1) {
			    next++;
			}
		    } MLSDB_LIST_END(ap->mib_lsdb_list[type], lpp) ;

		    next++;
		}

		type_id = LS_RTR;
	    } AREA_LIST_END(ap) ;
	} else {
	    AREA_LIST(ap) {
		register struct LSDB **lpp;
		register u_int32 cur_area_id = ntohl(ap->area_id);

		if (area_id > cur_area_id) {
		    continue;
		} else if (area_id < cur_area_id) {
		    break;
		}

		/* XXX - binary search */

		MLSDB_LIST(ap->mib_lsdb_list[type_id], lpp) {
		    register struct LSDB *lp = *lpp;
		    register u_int32 cur_key0 = ntohl(LS_ID(lp));
		    register u_int32 cur_key1 = ntohl(ADV_RTR(lp));

		    if (key0 > cur_key0) {
			continue;
		    } else if (key0 == cur_key0) {
			if (key1 > cur_key1) {
			    continue;
			} else if (key1 == cur_key1) {
			    return last_lsdb = lp;
			}
		    }
		    return last_lsdb = (struct LSDB *) 0;
		} MLSDB_LIST_END(ap->mib_lsdb_list[type], lpp);
	    } AREA_LIST_END(ap) ;
	}
    } else {
	register struct AREA *ap;

	/* Find first lsdb */

	AREA_LIST(ap) {
	    register u_int type;

	    for (type = LS_RTR; type < LS_ASE; type++) {
		register struct LSDB **list = (struct LSDB **) ap->mib_lsdb_list[type];

		if (*list) {
		    return last_lsdb = *list;
		}
	    }
	} AREA_LIST_END(ap) ;
    }
		
    return last_lsdb = (struct LSDB *) 0;
}

static int
o_lsdb __PF3(oi, OI,
	     v, register struct type_SNMP_VarBind *,
	     offset, int)
{
    register int    i;
    register unsigned int *ip,
			  *jp;
    register OID    oid = oi->oi_name;
    register OT	    ot = oi->oi_type;
    OID		    new;
    register struct LSDB *lsdb;

    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
	if (oid->oid_nelem - ot->ot_name->oid_nelem != NDX_SIZE) {
	    return int_SNMP_error__status_noSuchName;
	}
	lsdb = o_lsdb_lookup(oid->oid_elements + oid->oid_nelem - NDX_SIZE,
			     NDX_SIZE,
			     0);
	if (!lsdb) {
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
	/* next request with incomplete instance? */
	if ((i = oid->oid_nelem - ot->ot_name->oid_nelem) != 0 && i < NDX_SIZE) {
	    for (jp = (ip = oid->oid_elements + 
		       ot->ot_name->oid_nelem - 1) + i;
		 jp > ip;
		 jp--) {
		if (*jp != 0) {
		    break;
		}
	    }
	    if (jp == ip) {
		oid->oid_nelem = ot->ot_name->oid_nelem;
	    } else {
		if ((new = oid_normalize(oid, NDX_SIZE - i, 256)) == NULLOID) {
			return NOTOK;
		    }
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    }
	}

	/* next request with no instance? */
	if (oid->oid_nelem == ot->ot_name->oid_nelem) {
	    lsdb = o_lsdb_lookup((unsigned int *) 0,
				 0,
				 TRUE);
	    if (!lsdb) {
		return NOTOK;
	    }
	    
	    if ((new = oid_extend(oid, NDX_SIZE)) == NULLOID) {
		return int_SNMP_error__status_genErr;
	    }

	    ip = new->oid_elements + new->oid_nelem - NDX_SIZE;
	    STR_OID(ip, &lsdb->lsdb_area->area_id, sizeof (lsdb->lsdb_area->area_id));
	    INT_OID(ip, LS_TYPE(lsdb));
	    STR_OID(ip, &LS_ID(lsdb), sizeof (LS_ID(lsdb)));
	    STR_OID(ip, &ADV_RTR(lsdb), sizeof (ADV_RTR(lsdb)));
		
	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    int j;

	    lsdb = o_lsdb_lookup(ip = oid->oid_elements + ot->ot_name->oid_nelem,
				 (u_int) (j = oid->oid_nelem - ot->ot_name->oid_nelem),
				 TRUE);
	    if (!lsdb) {
		return NOTOK;
	    }

	    if ((i = j - NDX_SIZE) < 0) {
		if ((new = oid_extend(oid, -i)) == NULLOID) {
		    return int_SNMP_error__status_genErr;
		}
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    } else if (i > 0) {
		oid->oid_nelem -= i;
	    }
		
	    ip = oid->oid_elements + ot->ot_name->oid_nelem;
	    STR_OID(ip, &lsdb->lsdb_area->area_id, sizeof (lsdb->lsdb_area->area_id));
	    INT_OID(ip, LS_TYPE(lsdb));
	    STR_OID(ip, &LS_ID(lsdb), sizeof (LS_ID(lsdb)));
	    STR_OID(ip, &ADV_RTR(lsdb), sizeof (ADV_RTR(lsdb)));
	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }
#undef	NDX_SIZE
    
    switch (ot2object(ot)->ot_info) {
    case IospfLsdbAreaId:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(sockbuild_in(0, lsdb->lsdb_area->area_id),
				  (int *) 0));

    case IospfLsdbType:
	return o_integer(oi, v, LS_TYPE(lsdb));

    case IospfLsdbLsid:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(sockbuild_in(0, LS_ID(lsdb)),
				  (int *) 0));

    case IospfLsdbRouterId:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(sockbuild_in(0, ADV_RTR(lsdb)),
				  (int *) 0));

    case IospfLsdbSequence:
	return o_integer(oi, v, LS_SEQ(lsdb));

    case IospfLsdbAge:
	return o_integer(oi, v, MIN(ADV_AGE(lsdb), MaxAge));

    case IospfLsdbChecksum:
	return o_integer(oi, v, LS_CKS(lsdb));

    case IospfLsdbAdvertisement:
	return o_string(oi, v, (caddr_t) DB_RTR(lsdb), ntohs(LS_LEN(lsdb)));
    }

    return int_SNMP_error__status_noSuchName;
}

/**/

static struct LSDB *
o_ase_lookup __PF3(ip, register unsigned int *,
		    len, u_int,
		    isnext, int)
{
    static struct LSDB *last_lsdb;
    static unsigned int *last;
    static int last_quantum;

    if (last_quantum != snmp_quantum) {

	last_quantum = snmp_quantum;

	if (ospf.db_chksumsum != ospf.mib_ase_chksumsum) {
	    o_lsdb_get(ospf.db_ase_cnt,
		       ospf.ase,
		       &ospf.mib_ase_list,
		       &ospf.mib_ase_size,
		       &ospf.mib_ase_cnt);

	    ospf.mib_ase_chksumsum = ospf.db_chksumsum;

	    if (last) {
		task_mem_free((task *) 0, (void_t) last);
		last = (unsigned int *) 0;
	    }
	}
    }

    if (snmp_last_match(&last, ip, len, isnext)) {
	return last_lsdb;
    }

    if (!ospf.mib_ase_cnt) {
	return last_lsdb = (struct LSDB *) 0;
    }
    
    if (len) {
	u_int type_id;
	u_int32 key0, key1;
	register struct LSDB **lpp;

	type_id = *ip++;
	if (type_id != LS_ASE) {
	    return last_lsdb = (struct LSDB *) 0;
	}
	oid2ipaddr(ip, &key0);
	GNTOHL(key0);
	ip += sizeof (struct in_addr);

	oid2ipaddr(ip, &key1);
	GNTOHL(key1);

	if (isnext) {
	    MLSDB_LIST(ospf.mib_ase_list, lpp) {
		register struct LSDB *lp = *lpp;
		register u_int32 cur_key0 = ntohl(LS_ID(lp));
		register u_int32 cur_key1 = ntohl(ADV_RTR(lp));

		if (cur_key0 > key0
		    || (cur_key0 == key0
			&& cur_key1 > key1)) {
		    return last_lsdb = lp;
		}
	    } MLSDB_LIST_END(ospf.mib_ase_list, lpp) ;
	} else {
	    /* XXX - binary search */

	    MLSDB_LIST(ospf.mib_ase_list, lpp) {
		register struct LSDB *lp = *lpp;
		register u_int32 cur_key0 = ntohl(LS_ID(lp));
		register u_int32 cur_key1 = ntohl(ADV_RTR(lp));

		if (key0 < cur_key0) {
		    continue;
		} else if (key0 == cur_key0) {
		    if (key1 < cur_key1) {
			continue;
		    } else if (key1 == cur_key1) {
			return last_lsdb = lp;
		    }
		}
		return last_lsdb = (struct LSDB *) 0;
	    } MLSDB_LIST_END(ospf.mib_ase_list, lpp);
	}
    } else {

	return last_lsdb = *ospf.mib_ase_list;
    }
		
    return last_lsdb = (struct LSDB *) 0;
}


static int
o_ase __PF3(oi, OI,
	     v, register struct type_SNMP_VarBind *,
	     offset, int)
{
    register int    i;
    register unsigned int *ip,
			  *jp;
    register OID    oid = oi->oi_name;
    register OT	    ot = oi->oi_type;
    OID		    new;
    register struct LSDB *lsdb;
/* INDEX { ospfExtLsdbType, ospfExtLsdbLsid, ospfExtLsdbRouterId } */
#define	NDX_SIZE	(1 + sizeof (struct in_addr) + sizeof (struct in_addr))

    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
	if (oid->oid_nelem - ot->ot_name->oid_nelem != NDX_SIZE) {
	    return int_SNMP_error__status_noSuchName;
	}
	lsdb = o_ase_lookup(oid->oid_elements + oid->oid_nelem - NDX_SIZE,
			    NDX_SIZE,
			    0);
	if (!lsdb) {
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
	/* next request with incomplete instance? */
	if ((i = oid->oid_nelem - ot->ot_name->oid_nelem) != 0 && i < NDX_SIZE) {
	    for (jp = (ip = oid->oid_elements + 
		       ot->ot_name->oid_nelem - 1) + i;
		 jp > ip;
		 jp--) {
		if (*jp != 0) {
		    break;
		}
	    }
	    if (jp == ip) {
		oid->oid_nelem = ot->ot_name->oid_nelem;
	    } else {
		if ((new = oid_normalize(oid, NDX_SIZE - i, 256)) == NULLOID) {
			return NOTOK;
		    }
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    }
	}

	/* next request with no instance? */
	if (oid->oid_nelem == ot->ot_name->oid_nelem) {
	    lsdb = o_ase_lookup((unsigned int *) 0,
				0,
				TRUE);
	    if (!lsdb) {
		return NOTOK;
	    }
	    
	    if ((new = oid_extend(oid, NDX_SIZE)) == NULLOID) {
		return int_SNMP_error__status_genErr;
	    }

	    ip = new->oid_elements + new->oid_nelem - NDX_SIZE;
	    INT_OID(ip, LS_TYPE(lsdb));
	    STR_OID(ip, &LS_ID(lsdb), sizeof (LS_ID(lsdb)));
	    STR_OID(ip, &ADV_RTR(lsdb), sizeof (ADV_RTR(lsdb)));
		
	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    int j;

	    lsdb = o_ase_lookup(ip = oid->oid_elements + ot->ot_name->oid_nelem,
				(u_int) (j = oid->oid_nelem - ot->ot_name->oid_nelem),
				TRUE);
	    if (!lsdb) {
		return NOTOK;
	    }

	    if ((i = j - NDX_SIZE) < 0) {
		if ((new = oid_extend(oid, -i)) == NULLOID) {
		    return int_SNMP_error__status_genErr;
		}
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    } else if (i > 0) {
		oid->oid_nelem -= i;
	    }
		
	    ip = oid->oid_elements + ot->ot_name->oid_nelem;
	    INT_OID(ip, LS_TYPE(lsdb));
	    STR_OID(ip, &LS_ID(lsdb), sizeof (LS_ID(lsdb)));
	    STR_OID(ip, &ADV_RTR(lsdb), sizeof (ADV_RTR(lsdb)));
	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }
#undef	NDX_SIZE
    
    switch (ot2object(ot)->ot_info) {
    case IospfExtLsdbType:
	return o_integer(oi, v, LS_TYPE(lsdb));

    case IospfExtLsdbLsid:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(sockbuild_in(0, LS_ID(lsdb)),
				  (int *) 0));

    case IospfExtLsdbRouterId:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(sockbuild_in(0, ADV_RTR(lsdb)),
				  (int *) 0));

    case IospfExtLsdbSequence:
	return o_integer(oi, v, LS_SEQ(lsdb));

    case IospfExtLsdbAge:
	return o_integer(oi, v, MIN(ADV_AGE(lsdb), MaxAge));

    case IospfExtLsdbChecksum:
	return o_integer(oi, v, LS_CKS(lsdb));

    case IospfExtLsdbAdvertisement:
	return o_string(oi, v, (caddr_t) DB_RTR(lsdb), ntohs(LS_LEN(lsdb)));
    }

    return int_SNMP_error__status_noSuchName;
}

/**/

/* Network range group */
static struct NET_RANGE *
o_range_lookup __PF4(ip, register unsigned int *,
		     len, u_int,
		     isnext, int,
		     return_area, struct AREA **)
{
    int next = FALSE;
    u_int32 nr_net = (u_int32) 0;
    u_int32 area_id = (u_int32) 0;
    register struct AREA *area;
    register struct NET_RANGE *nr;
    static struct NET_RANGE *last_nr;
    static struct AREA *last_area;
    static unsigned int *last;

    if (snmp_last_match(&last, ip, len, isnext)) {
	*return_area = last_area;
	return last_nr;
    }

    if (len) {
	oid2ipaddr(ip, &area_id);
	GNTOHL(area_id);

	oid2ipaddr(ip + sizeof (struct in_addr), &nr_net);
	GNTOHL(nr_net);
    }

    AREA_LIST(area) {
	register u_int32 cur_id = ntohl(area->area_id);
	
	if (cur_id < area_id) {
	    continue;
	}

	RANGE_LIST(nr, area) {
	    u_int32 cur_net = ntohl(nr->net);

	    if (next) {
		goto got_it;
	    }
	    
	    if (cur_net == nr_net) {
		if (isnext) {
		    next = TRUE;
		} else {
		    goto got_it;
		}
	    } else if (cur_net > nr_net) {
		if (!isnext) {
		    nr = (struct NET_RANGE *) 0;
		}
		goto got_it;
	    }
	} RANGE_LIST_END(nr, area) ;
    } AREA_LIST_END(area) ;

    nr = (struct NET_RANGE *) 0;
    area = (struct AREA *) 0;

 got_it:
    *return_area = last_area = area;
    return last_nr = nr;
}


static int
o_range __PF3(oi, OI,
	      v, register struct type_SNMP_VarBind *,
	      offset, int)
{
    register int    i;
    register unsigned int *ip,
			  *jp;
    register OID    oid = oi->oi_name;
    register OT	    ot = oi->oi_type;
    OID		    new;
    struct AREA *area;
    struct NET_RANGE *range;

    /* INDEX { ospfAreaRangeAreaId, ospfAreaRangeNet } */
#define	NDX_SIZE	(sizeof (struct in_addr) + sizeof (struct in_addr))

    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
	if (oid->oid_nelem - ot->ot_name->oid_nelem != NDX_SIZE) {
	    return int_SNMP_error__status_noSuchName;
	}
	range = o_range_lookup(oid->oid_elements + oid->oid_nelem - NDX_SIZE,
			       NDX_SIZE,
			       0,
			       &area);
	if (!range) {
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
	/* next request with incomplete instance? */
	if ((i = oid->oid_nelem - ot->ot_name->oid_nelem) != 0 && i < NDX_SIZE) {
	    for (jp = (ip = oid->oid_elements + 
		       ot->ot_name->oid_nelem - 1) + i;
		 jp > ip;
		 jp--) {
		if (*jp != 0) {
		    break;
		}
	    }
	    if (jp == ip) {
		oid->oid_nelem = ot->ot_name->oid_nelem;
	    } else {
		if ((new = oid_normalize(oid, NDX_SIZE - i, 256)) == NULLOID) {
			return NOTOK;
		    }
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    }
	}

	/* next request with no instance? */
	if (oid->oid_nelem == ot->ot_name->oid_nelem) {
	    range = o_range_lookup((unsigned int *) 0,
				   0,
				   TRUE,
				   &area);
	    if (!range) {
		return NOTOK;
	    }
	    
	    if ((new = oid_extend(oid, NDX_SIZE)) == NULLOID) {
		return int_SNMP_error__status_genErr;
	    }

	    ip = new->oid_elements + new->oid_nelem - NDX_SIZE;
	    STR_OID(ip, &area->area_id, sizeof (area->area_id));
	    STR_OID(ip, &range->net, sizeof (range->net));
		
	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    int j;

	    range = o_range_lookup(ip = oid->oid_elements + ot->ot_name->oid_nelem,
				   (u_int) (j = oid->oid_nelem - ot->ot_name->oid_nelem),
				   TRUE,
				   &area);
	    if (!range) {
		return NOTOK;
	    }

	    if ((i = j - NDX_SIZE) < 0) {
		if ((new = oid_extend(oid, -i)) == NULLOID) {
		    return int_SNMP_error__status_genErr;
		}
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    } else if (i > 0) {
		oid->oid_nelem -= i;
	    }
		
	    ip = oid->oid_elements + ot->ot_name->oid_nelem;
	    STR_OID(ip, &area->area_id, sizeof (area->area_id));
	    STR_OID(ip, &range->net, sizeof (range->net));
	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }
#undef	NDX_SIZE

    switch (ot2object(ot)->ot_info) {
    case IospfAreaRangeAreaId:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(sockbuild_in(0, area->area_id),
				  (int *) 0));

    case IospfAreaRangeNet:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(sockbuild_in(0, range->net),
				  (int *) 0));

    case IospfAreaRangeMask:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(sockbuild_in(0, range->mask),
				  (int *) 0));

    case IospfAreaRangeStatus:
	return o_integer(oi, v, MIB_VALID);

    case IospfAreaRangeEffect:
	return o_integer(oi,
			 v,
			 (range->status == Advertise) ? MIB_ADVERTISE_MATCHING : MIB_DONOT_ADVERTISE_MATCHING);
			 
    }

    return int_SNMP_error__status_noSuchName;
}

/**/

/* Network range group */
static struct NET_RANGE *
o_aggr_lookup __PF4(ip, register unsigned int *,
		     len, u_int,
		     isnext, int,
		     return_area, struct AREA **)
{
    int next = FALSE;
    u_int32 nr_net = (u_int32) 0;
    u_int32 nr_mask = (u_int32) 0;
    u_int32 area_id = (u_int32) 0;
    u_int type;
    register struct AREA *area;
    register struct NET_RANGE *nr;
    static struct NET_RANGE *last_nr;
    static struct AREA *last_area;
    static unsigned int *last;

    if (snmp_last_match(&last, ip, len, isnext)) {
	*return_area = last_area;
	return last_nr;
    }

    if (len) {
	oid2ipaddr(ip, &area_id);
	GNTOHL(area_id);
	ip += sizeof (struct in_addr);

	type = *ip++;
	if (type != MIB_AGGR_SUMMARYLINK) {
	    nr = (struct NET_RANGE *) 0;
	    area = (struct AREA *) 0;
	    goto got_it;
	}

	oid2ipaddr(ip, &nr_net);
	GNTOHL(nr_net);
	ip += sizeof (struct in_addr);

	oid2ipaddr(ip, &nr_mask);
	GNTOHL(nr_mask);	
    }

    AREA_LIST(area) {
	register u_int32 cur_id = ntohl(area->area_id);
	
	if (cur_id < area_id) {
	    continue;
	}

	RANGE_LIST(nr, area) {
	    u_int32 cur_net = ntohl(nr->net);
	    u_int32 cur_mask = ntohl(nr->mask);

	    if (next) {
		goto got_it;
	    }

	    if (cur_net == nr_net
		&& cur_mask == nr_mask) {
		if (isnext) {
		    next = TRUE;
		} else {
		    goto got_it;
		}
	    } else if (cur_net > nr_net
		       || (cur_net == nr_net
			   && cur_mask > nr_mask)) {
		if (!isnext) {
		    nr = (struct NET_RANGE *) 0;
		}
		goto got_it;
	    }
	} RANGE_LIST_END(nr, area) ;
    } AREA_LIST_END(area) ;

    nr = (struct NET_RANGE *) 0;
    area = (struct AREA *) 0;

 got_it:
    *return_area = last_area = area;
    return last_nr = nr;
}


static int
o_aggr __PF3(oi, OI,
	      v, register struct type_SNMP_VarBind *,
	      offset, int)
{
    register int    i;
    register unsigned int *ip,
			  *jp;
    register OID    oid = oi->oi_name;
    register OT	    ot = oi->oi_type;
    OID		    new;
    struct AREA *area;
    struct NET_RANGE *range;

    /* INDEX { ospfAreaAggregateAreaID, ospfAreaAggregateLsdbType ospfAreaAggregateNet ospfAreaAggregateMask } */
#define	NDX_SIZE	(sizeof (struct in_addr) + 1 + sizeof (struct in_addr) + sizeof (struct in_addr))

    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
	if (oid->oid_nelem - ot->ot_name->oid_nelem != NDX_SIZE) {
	    return int_SNMP_error__status_noSuchName;
	}
	range = o_aggr_lookup(oid->oid_elements + oid->oid_nelem - NDX_SIZE,
			       NDX_SIZE,
			       0,
			       &area);
	if (!range) {
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
	/* next request with incomplete instance? */
	if ((i = oid->oid_nelem - ot->ot_name->oid_nelem) != 0 && i < NDX_SIZE) {
	    for (jp = (ip = oid->oid_elements + 
		       ot->ot_name->oid_nelem - 1) + i;
		 jp > ip;
		 jp--) {
		if (*jp != 0) {
		    break;
		}
	    }
	    if (jp == ip) {
		oid->oid_nelem = ot->ot_name->oid_nelem;
	    } else {
		if ((new = oid_normalize(oid, NDX_SIZE - i, 256)) == NULLOID) {
			return NOTOK;
		    }
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    }
	}

	/* next request with no instance? */
	if (oid->oid_nelem == ot->ot_name->oid_nelem) {
	    range = o_aggr_lookup((unsigned int *) 0,
				   0,
				   TRUE,
				   &area);
	    if (!range) {
		return NOTOK;
	    }
	    
	    if ((new = oid_extend(oid, NDX_SIZE)) == NULLOID) {
		return int_SNMP_error__status_genErr;
	    }

	    ip = new->oid_elements + new->oid_nelem - NDX_SIZE;
	    STR_OID(ip, &area->area_id, sizeof (area->area_id));
	    INT_OID(ip, MIB_AGGR_SUMMARYLINK);
	    STR_OID(ip, &range->net, sizeof (range->net));
	    STR_OID(ip, &range->mask, sizeof (range->mask));
		
	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    int j;

	    range = o_aggr_lookup(ip = oid->oid_elements + ot->ot_name->oid_nelem,
				   (u_int) (j = oid->oid_nelem - ot->ot_name->oid_nelem),
				   TRUE,
				   &area);
	    if (!range) {
		return NOTOK;
	    }

	    if ((i = j - NDX_SIZE) < 0) {
		if ((new = oid_extend(oid, -i)) == NULLOID) {
		    return int_SNMP_error__status_genErr;
		}
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    } else if (i > 0) {
		oid->oid_nelem -= i;
	    }
		
	    ip = oid->oid_elements + ot->ot_name->oid_nelem;
	    STR_OID(ip, &area->area_id, sizeof (area->area_id));
	    INT_OID(ip, MIB_AGGR_SUMMARYLINK);
	    STR_OID(ip, &range->net, sizeof (range->net));
	    STR_OID(ip, &range->mask, sizeof (range->mask));
	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }
#undef	NDX_SIZE

    switch (ot2object(ot)->ot_info) {
    case IospfAreaAggregateAreaID:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(sockbuild_in(0, area->area_id),
				  (int *) 0));

    case IospfAreaAggregateLsdbType:
	return o_integer(oi,
			 v,
			 MIB_AGGR_SUMMARYLINK);
			 
    case IospfAreaAggregateNet:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(sockbuild_in(0, range->net),
				  (int *) 0));

    case IospfAreaAggregateMask:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(sockbuild_in(0, range->mask),
				  (int *) 0));

    case IospfAreaAggregateStatus:
	return o_integer(oi, v, MIB_VALID);

    case IospfAreaAggregateEffect:
	return o_integer(oi,
			 v,
			 (range->status == Advertise) ? MIB_ADVERTISE_MATCHING : MIB_DONOT_ADVERTISE_MATCHING);
			 
    }

    return int_SNMP_error__status_noSuchName;
}

/**/

/* Host group */
static struct OSPF_HOSTS *
o_host_lookup __PF3(ip, register unsigned int *,
		    len, u_int,
		    isnext, int)
{
    u_int32 host_addr = (u_int32) 0;
    register struct OSPF_HOSTS *host;
    register struct AREA *area;
    static unsigned int *last;
    static struct OSPF_HOSTS *last_host;

    if (snmp_last_match(&last, ip, len, isnext)) {
	return last_host;
    }

    if (len) {
	oid2ipaddr(ip, &host_addr);

	if (ip[sizeof (struct in_addr)]) {
	    /* We don't support TOS */
	    return last_host = (struct OSPF_HOSTS *) 0;
	}
    }

    if (isnext) {
	register struct OSPF_HOSTS *new = (struct OSPF_HOSTS *) 0;
	u_int32 new_addr = 0;

	GNTOHL(host_addr);

	AREA_LIST(area) {

	    if (area->hostcnt) {
		host = &area->hosts;

		while ((host = host->ptr[NEXT])) {
		    u_int32 c_addr = ntohl(host->host_if_addr);

		    if (c_addr > host_addr &&
			(!new || c_addr < new_addr)) {
			new = host;
			new_addr = c_addr;
		    }
		}
	    }
	} AREA_LIST_END(area) ;

	return last_host = new;
    } else {

	AREA_LIST(area) {

	    if (area->hostcnt) {
		host = &area->hosts;

		while ((host = host->ptr[NEXT])) {
		    if (host_addr == host->host_if_addr) {
			return last_host = host;
		    }
		}
	    }
	} AREA_LIST_END(area) ;

	return last_host = (struct OSPF_HOSTS *) 0;
    }
}


static int
o_host __PF3(oi, OI,
	     v, register struct type_SNMP_VarBind *,
	     offset, int)
{
    register int    i;
    register unsigned int *ip,
			  *jp;
    register OID    oid = oi->oi_name;
    register OT	    ot = oi->oi_type;
    OID		    new;
    struct OSPF_HOSTS *host;

    /* INDEX { ospfHostIpAddress, ospfHostTOS } */
#define	NDX_SIZE	(sizeof (struct in_addr) + 1)

    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
	if (oid->oid_nelem - ot->ot_name->oid_nelem != NDX_SIZE) {
	    return int_SNMP_error__status_noSuchName;
	}
	host = o_host_lookup(oid->oid_elements + oid->oid_nelem - NDX_SIZE,
			     NDX_SIZE,
			     0);
	if (!host) {
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
	/* next request with incomplete instance? */
	if ((i = oid->oid_nelem - ot->ot_name->oid_nelem) != 0 && i < NDX_SIZE) {
	    for (jp = (ip = oid->oid_elements + 
		       ot->ot_name->oid_nelem - 1) + i;
		 jp > ip;
		 jp--) {
		if (*jp != 0) {
		    break;
		}
	    }
	    if (jp == ip) {
		oid->oid_nelem = ot->ot_name->oid_nelem;
	    } else {
		if ((new = oid_normalize(oid, NDX_SIZE - i, 256)) == NULLOID) {
			return NOTOK;
		    }
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    }
	}

	/* next request with no instance? */
	if (oid->oid_nelem == ot->ot_name->oid_nelem) {
	    host = o_host_lookup((unsigned int *) 0,
				 0,
				 TRUE);
	    if (!host) {
		return NOTOK;
	    }
	    
	    if ((new = oid_extend(oid, NDX_SIZE)) == NULLOID) {
		return int_SNMP_error__status_genErr;
	    }

	    ip = new->oid_elements + new->oid_nelem - NDX_SIZE;
	    STR_OID(ip, &host->host_if_addr, sizeof (host->host_if_addr));
	    /* No support for TOS */
	    INT_OID(ip, 0);
		
	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    int j;

	    host = o_host_lookup(ip = oid->oid_elements + ot->ot_name->oid_nelem,
				 (u_int) (j = oid->oid_nelem - ot->ot_name->oid_nelem),
				 TRUE);
	    if (!host) {
		return NOTOK;
	    }

	    if ((i = j - NDX_SIZE) < 0) {
		if ((new = oid_extend(oid, -i)) == NULLOID) {
		    return int_SNMP_error__status_genErr;
		}
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    } else if (i > 0) {
		oid->oid_nelem -= i;
	    }
		
	    ip = oid->oid_elements + ot->ot_name->oid_nelem;
	    STR_OID(ip, &host->host_if_addr, sizeof (host->host_if_addr));
	    /* No support for TOS */
	    INT_OID(ip, 0);
	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }
#undef	NDX_SIZE
    
    switch (ot2object(ot)->ot_info) {
    case IospfHostIpAddress:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(sockbuild_in(0, host->host_if_addr),
				  (int *) 0));

    case IospfHostTOS:
	/* No support for TOS */
	return o_integer(oi, v, 0);

    case IospfHostMetric:
	return o_integer(oi, v, ntohl(host->host_cost));

    case IospfHostStatus:
	return o_integer(oi, v, MIB_VALID);
    }
    
    return int_SNMP_error__status_noSuchName;
}


/* Interface group */

struct intf_entry {
    struct intf_entry *forw;
    struct intf_entry *back;
    u_int32 addr;
    int index;
    struct INTF *intf;
};

static struct intf_entry o_intf_list = {&o_intf_list, &o_intf_list };
static int o_intf_cnt;
static block_t o_intf_index;
static unsigned int *o_intf_last;

#define	MINTF_LIST(intfp)	for (intfp = o_intf_list.forw; intfp != &o_intf_list; intfp = intfp->forw)
#define	MINTF_LIST_END(intfp)	

void
o_intf_get __PF0(void)
{
    register struct intf_entry *intfp;
    register struct AREA *area;

    /* Free the old list */
    MINTF_LIST(intfp) {
	register struct intf_entry *intfp2 = intfp->back;

	REMQUE(intfp);
	task_block_free(o_intf_index, (void_t) intfp);

	intfp = intfp2;
    } MINTF_LIST_END(intfp) ;

    snmp_last_free(&o_intf_last);
    o_intf_cnt = 0;

    AREA_LIST(area) {
	register struct INTF *intf;

	INTF_LIST(intf, area) {
	    register u_int32 intf_addr = ntohl(INTF_LCLADDR(intf));
	    register int intf_index;

	    if (BIT_TEST(intf->ifap->ifa_state, IFS_POINTOPOINT)
		&& intf->ifap->ifa_addrent_local->ifae_n_if > 1) {
		/* Set addressless index */

		intf_index = intf->ifap->ifa_link->ifl_index;
	    } else {
		intf_index = 0;
	    }
	    
	    MINTF_LIST(intfp) {
		if (intf_addr < intfp->addr
		    || (intf_addr == intfp->addr
			&& intf_index < intfp->index)) {
		    break;
		}
	    } MINTF_LIST_END(intfp) ;

	    INSQUE(task_block_alloc(o_intf_index), intfp->back);
	    intfp->back->intf = intf;
	    intfp->back->addr = intf_addr;
	    intfp->back->index = intf_index;
	    o_intf_cnt++;
	} INTF_LIST_END(intf, area) ;
    } AREA_LIST_END(area) ;

}


static struct intf_entry *
o_intf_lookup __PF3(ip, register unsigned int *,
		    len, u_int,
		    isnext, int)
{
    static struct intf_entry *last_intfp;

    if (snmp_last_match(&o_intf_last, ip, len, isnext)) {
	return last_intfp;
    }
    
    if (!o_intf_cnt) {
	return last_intfp = (struct intf_entry *) 0;
    }
    
    if (len) {
	u_int32 intf_addr = (u_int32) 0;
	int intf_index;
	register struct intf_entry *intfp;

	oid2ipaddr(ip, &intf_addr);
	GNTOHL(intf_addr);

	intf_index = ip[sizeof (struct in_addr)];

	MINTF_LIST(intfp) {
	    if (intfp->addr == intf_addr
		&& intfp->index == intf_index) {
		if (!isnext) {
		    return last_intfp = intfp;
		}
	    } else if (intfp->addr > intf_addr
		       || (intfp->addr == intf_addr
			   && intfp->index > intf_index)) {
		return last_intfp = isnext ? intfp : (struct intf_entry *) 0;
	    }
	} MINTF_LIST_END(ip) ;

	return last_intfp = (struct intf_entry *) 0;
    }

    return last_intfp = o_intf_list.forw;
}


static inline int
o_intf_state __PF1(s_intf, struct INTF *)
{
    int state;

    switch (s_intf->state) {
    case IDOWN:
	state = I_STATE_DOWN;
	break;
	
    case ILOOPBACK:
	state = I_STATE_LOOPBACK;
	break;
	
    case IWAITING:
	state = I_STATE_WAITING;
	break;
	
    case IPOINT_TO_POINT:
	state = I_STATE_P2P;
	break;
	
    case IDr:
	state = I_STATE_DR;
	break;
	
    case IBACKUP:
	state = I_STATE_BDR;
	break;
	
    case IDrOTHER:
	state = I_STATE_DROTHER;
	break;

    default:
	state = -1;
    }

    return state;
}


static int
o_intf __PF3(oi, OI,
	     v, register struct type_SNMP_VarBind *,
	     offset, int)
{
    register int    i;
    register unsigned int *ip,
			  *jp;
    struct intf_entry *intfp;
    register OID    oid = oi->oi_name;
    register OT	    ot = oi->oi_type;
    OID		    new;

    /* INDEX { ospfIfIpAddress, ospfAddressLessIf } */
#define	NDX_SIZE	(sizeof (struct in_addr) + 1)

    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
	if (oid->oid_nelem - ot->ot_name->oid_nelem != NDX_SIZE) {
	    return int_SNMP_error__status_noSuchName;
	}
	intfp = o_intf_lookup(oid->oid_elements + oid->oid_nelem - NDX_SIZE,
			      NDX_SIZE,
			      0);
	if (!intfp) {
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
	/* next request with incomplete instance? */
	if ((i = oid->oid_nelem - ot->ot_name->oid_nelem) != 0 && i < NDX_SIZE) {
	    for (jp = (ip = oid->oid_elements + 
		       ot->ot_name->oid_nelem - 1) + i;
		 jp > ip;
		 jp--) {
		if (*jp != 0) {
		    break;
		}
	    }
	    if (jp == ip) {
		oid->oid_nelem = ot->ot_name->oid_nelem;
	    } else {
		if ((new = oid_normalize(oid, NDX_SIZE - i, 256)) == NULLOID) {
			return NOTOK;
		    }
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    }
	}

	/* next request with no instance? */
	if (oid->oid_nelem == ot->ot_name->oid_nelem) {
	    intfp = o_intf_lookup((unsigned int *) 0,
				  0,
				  TRUE);
	    if (!intfp) {
		return NOTOK;
	    }
	    
	    if ((new = oid_extend(oid, NDX_SIZE)) == NULLOID) {
		return int_SNMP_error__status_genErr;
	    }

	    ip = new->oid_elements + new->oid_nelem - NDX_SIZE;
	    STR_OID(ip, &sock2ip(intfp->intf->ifap->ifa_addr_local),
		    sizeof (sock2ip(intfp->intf->ifap->ifa_addr_local)));
	    INT_OID(ip, intfp->index);
		
	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    int j;

	    intfp = o_intf_lookup(ip = oid->oid_elements + ot->ot_name->oid_nelem,
				  (u_int) (j = oid->oid_nelem - ot->ot_name->oid_nelem),
				  TRUE);
	    if (!intfp) {
		return NOTOK;
	    }

	    if ((i = j - NDX_SIZE) < 0) {
		if ((new = oid_extend(oid, -i)) == NULLOID) {
		    return int_SNMP_error__status_genErr;
		}
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    } else if (i > 0) {
		oid->oid_nelem -= i;
	    }
		
	    ip = oid->oid_elements + ot->ot_name->oid_nelem;
	    STR_OID(ip, &sock2ip(intfp->intf->ifap->ifa_addr_local),
		    sizeof (sock2ip(intfp->intf->ifap->ifa_addr_local)));
	    INT_OID(ip, intfp->index);
	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }
#undef	NDX_SIZE

    switch (ot2object(ot)->ot_info) {
    case IospfIfIpAddress:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(intfp->intf->ifap->ifa_addr_local,
				  (int *) 0));

    case IospfAddressLessIf:
	/* All interfaces have addresses */
	return o_integer(oi, v, intfp->index);

    case IospfIfAreaId:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(sockbuild_in(0, intfp->intf->area->area_id),
				  (int *) 0));

    case IospfIfType:
	return o_integer(oi, v, intfp->intf->type);

    case IospfIfAdminStat:
	return o_integer(oi, v, BIT_TEST(intfp->intf->flags, OSPF_INTFF_ENABLE) ? MIB_ENABLED : MIB_DISABLED);

    case IospfIfRtrPriority:
	return o_integer(oi, v, intfp->intf->nbr.pri);

    case IospfIfTransitDelay:
	return o_integer(oi, v, intfp->intf->transdly);

    case IospfIfRetransInterval:
	return o_integer(oi, v, intfp->intf->retrans_timer);

    case IospfIfHelloInterval:
	return o_integer(oi, v, intfp->intf->hello_timer);

    case IospfIfRtrDeadInterval:
	return o_integer(oi, v, intfp->intf->dead_timer);

    case IospfIfPollInterval:
	return o_integer(oi, v, intfp->intf->poll_timer);

    case IospfIfState:
	return o_integer(oi, v, o_intf_state(intfp->intf));

    case IospfIfDesignatedRouter:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(intfp->intf->dr ?
			intfp->intf->dr->nbr_addr : inet_addr_default,
				  (int *) 0));

    case IospfIfBackupDesignatedRouter:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(intfp->intf->bdr ?
			intfp->intf->bdr->nbr_addr : inet_addr_default,
				  (int *) 0));

    case IospfIfEvents:
	return o_integer(oi, v, intfp->intf->events);

    case IospfIfAuthKey:
	/* When read, ospfIfAuthKey always returns an Octet String of length zero. */
	return o_string(oi, v, (caddr_t) 0, 0);

    case IospfIfStatus:
	return o_integer(oi, v, MIB_VALID);

    case IospfIfMulticastForwarding:
	return o_integer(oi, v, MIB_BLOCKED);
    }
    
    return int_SNMP_error__status_noSuchName;
}


/* Metric group */
static struct intf_entry *
o_metric_lookup __PF3(ip, register unsigned int *,
		      len, u_int,
		      isnext, int)
{

    if (len && ip[sizeof (struct in_addr) + 1]) {
	/* We dont' support TOS */
	return (struct intf_entry *) 0;
    }

    return o_intf_lookup(ip, len, isnext);
}


static int
o_metric __PF3(oi, OI,
	       v, register struct type_SNMP_VarBind *,
	       offset, int)
{
    register int    i;
    register unsigned int *ip,
			  *jp;
    register OID    oid = oi->oi_name;
    register OT	    ot = oi->oi_type;
    OID		    new;
    struct intf_entry *intfp;

    /* INDEX { ospfIfMetricIpAddress, ospfIfMetricAddressLessIf, ospfIfMetricTOS } */
#define	NDX_SIZE	(sizeof (struct in_addr) + 1 + 1)
    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
	if (oid->oid_nelem - ot->ot_name->oid_nelem != NDX_SIZE) {
	    return int_SNMP_error__status_noSuchName;
	}
	intfp = o_metric_lookup(oid->oid_elements + oid->oid_nelem - NDX_SIZE,
				NDX_SIZE,
				0);
	if (!intfp) {
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
	/* next request with incomplete instance? */
	if ((i = oid->oid_nelem - ot->ot_name->oid_nelem) != 0 && i < NDX_SIZE) {
	    for (jp = (ip = oid->oid_elements + 
		       ot->ot_name->oid_nelem - 1) + i;
		 jp > ip;
		 jp--) {
		if (*jp != 0) {
		    break;
		}
	    }
	    if (jp == ip) {
		oid->oid_nelem = ot->ot_name->oid_nelem;
	    } else {
		if ((new = oid_normalize(oid, NDX_SIZE - i, 256)) == NULLOID) {
			return NOTOK;
		    }
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    }
	}

	/* next request with no instance? */
	if (oid->oid_nelem == ot->ot_name->oid_nelem) {
	    intfp = o_metric_lookup((unsigned int *) 0,
				    0,
				    TRUE);
	    if (!intfp) {
		return NOTOK;
	    }
	    
	    if ((new = oid_extend(oid, NDX_SIZE)) == NULLOID) {
		return int_SNMP_error__status_genErr;
	    }

	    ip = new->oid_elements + new->oid_nelem - NDX_SIZE;
	    STR_OID(ip, &sock2ip(intfp->intf->ifap->ifa_addr_local),
		    sizeof (sock2ip(intfp->intf->ifap->ifa_addr_local)));
	    INT_OID(ip, intfp->index);
	    /* No support for TOS */
	    INT_OID(ip, 0);
		
	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    int j;

	    intfp = o_metric_lookup(ip = oid->oid_elements + ot->ot_name->oid_nelem,
				    (u_int) (j = oid->oid_nelem - ot->ot_name->oid_nelem),
				    TRUE);
	    if (!intfp) {
		return NOTOK;
	    }

	    if ((i = j - NDX_SIZE) < 0) {
		if ((new = oid_extend(oid, -i)) == NULLOID) {
		    return int_SNMP_error__status_genErr;
		}
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    } else if (i > 0) {
		oid->oid_nelem -= i;
	    }
		
	    ip = oid->oid_elements + ot->ot_name->oid_nelem;
	    STR_OID(ip, &sock2ip(intfp->intf->ifap->ifa_addr_local),
		    sizeof (sock2ip(intfp->intf->ifap->ifa_addr_local)));
	    INT_OID(ip, intfp->index);
	    /* No support for TOS */
	    INT_OID(ip, 0);
	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }
#undef	NDX_SIZE

    switch (ot2object(ot)->ot_info) {
    case IospfIfMetricIpAddress:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(intfp->intf->ifap->ifa_addr_local,
				  (int *) 0));

    case IospfIfMetricAddressLessIf:
	/* All interfaces have addresses */
	return o_integer(oi, v, intfp->index);

    case IospfIfMetricTOS:
	/* No support for TOS */
	return o_integer(oi, v, 0);

    case IospfIfMetricValue:
	return o_integer(oi, v, intfp->intf->cost);

    case IospfIfMetricStatus:
	return o_integer(oi, v, MIB_VALID);
    }
    
    return int_SNMP_error__status_noSuchName;
}


/* Virtual Interfaces */

/* INDEX { ospfVirtIfAreaId, ospfVirtIfNeighbor } */
#define	NDX_SIZE	(sizeof (struct in_addr) + sizeof (struct in_addr))

static struct intf_entry o_vintf_list = {&o_vintf_list, &o_vintf_list };
static int o_vintf_cnt;
static unsigned int *o_vintf_last;

#define	MVINTF_LIST(intfp)	for (intfp = o_vintf_list.forw; intfp != &o_vintf_list; intfp = intfp->forw)
#define	MVINTF_LIST_END(ip)	

void
o_vintf_get __PF0(void)
{
    register struct intf_entry *intfp;
    register struct INTF *intf;

    /* Free the old list */
    MVINTF_LIST(intfp) {
	register struct intf_entry *intfp2 = intfp->back;

	REMQUE(intfp);
	task_block_free(o_intf_index, (void_t) intfp);

	intfp = intfp2;
    } MVINTF_LIST_END(intfp) ;

    o_vintf_cnt = 0;
    snmp_last_free(&o_vintf_last);

    VINTF_LIST(intf) {
	u_int32 intf_area = ntohl(intf->trans_area->area_id);
	u_int32 nbr_id = ntohl(NBR_ID(&intf->nbr));
	    
	MVINTF_LIST(intfp) {
	    if (intf_area <= ntohl(intfp->intf->trans_area->area_id) &
		nbr_id > ntohl(NBR_ID(&intfp->intf->nbr))) {
		break;
	    }
	} MVINTF_LIST_END(intfp) ;

	INSQUE(task_block_alloc(o_intf_index), intfp);
	intfp->forw->intf = intf;
	o_vintf_cnt++;
    } VINTF_LIST_END(intfp) ;
}


static struct INTF *
o_vintf_lookup __PF3(ip, register unsigned int *,
		     len, u_int,
		     isnext, int)
{
    static struct INTF *last_intf;

    if (snmp_last_match(&o_vintf_last, ip, len, isnext)) {
	return last_intf;
    }
    
    if (!o_vintf_cnt) {
	return last_intf = (struct INTF *) 0;
    }
    
    if (len) {
	u_int32 area_id, nbr_id;
	register struct intf_entry *intfp;

	oid2ipaddr(ip, &area_id);
	GNTOHL(area_id);
	
	oid2ipaddr(ip, &nbr_id);
	GNTOHL(nbr_id);

	if (ip[sizeof (struct in_addr)]) {
	    /* We don't support address less interfaces */
	    return last_intf = (struct INTF *) 0;
	}

	MINTF_LIST(intfp) {
	    register u_int32 cur_area = ntohl(intfp->intf->trans_area->area_id);
	    register u_int32 cur_nbr = ntohl(NBR_ID(&intfp->intf->nbr));

	    if (cur_area == area_id) {
		if (nbr_id == cur_nbr) {
		    if (!isnext) {
			return last_intf = intfp->intf;
		    }
		} else if (nbr_id > cur_nbr) {
		    continue;
		}
	    } if (cur_area < area_id) {
		continue;
	    }

	    return last_intf = isnext ? intfp->intf : (struct INTF *) 0;
	} MINTF_LIST_END(ip) ;

	return last_intf = (struct INTF *) 0;
    }

    return last_intf = o_vintf_list.forw->intf;
}


/* Virtual Interface group */
static int
o_vintf __PF3(oi, OI,
	      v, register struct type_SNMP_VarBind *,
	      offset, int)
{
    register int    i;
    register unsigned int *ip,
			  *jp;
    struct INTF *intf;
    register OID    oid = oi->oi_name;
    register OT	    ot = oi->oi_type;
    OID		    new;

    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
	if (oid->oid_nelem - ot->ot_name->oid_nelem != NDX_SIZE) {
	    return int_SNMP_error__status_noSuchName;
	}
	intf = o_vintf_lookup(oid->oid_elements + oid->oid_nelem - NDX_SIZE,
			      NDX_SIZE,
			      0);
	if (!intf) {
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
	/* next request with incomplete instance? */
	if ((i = oid->oid_nelem - ot->ot_name->oid_nelem) != 0 && i < NDX_SIZE) {
	    for (jp = (ip = oid->oid_elements + 
		       ot->ot_name->oid_nelem - 1) + i;
		 jp > ip;
		 jp--) {
		if (*jp != 0) {
		    break;
		}
	    }
	    if (jp == ip) {
		oid->oid_nelem = ot->ot_name->oid_nelem;
	    } else {
		if ((new = oid_normalize(oid, NDX_SIZE - i, 256)) == NULLOID) {
			return NOTOK;
		    }
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    }
	}

	/* next request with no instance? */
	if (oid->oid_nelem == ot->ot_name->oid_nelem) {
	    intf = o_vintf_lookup((unsigned int *) 0,
				  0,
				  TRUE);
	    if (!intf) {
		return NOTOK;
	    }
	    
	    if ((new = oid_extend(oid, NDX_SIZE)) == NULLOID) {
		return int_SNMP_error__status_genErr;
	    }

	    ip = new->oid_elements + new->oid_nelem - NDX_SIZE;
	    STR_OID(ip, &intf->trans_area->area_id, sizeof (intf->trans_area->area_id));
	    STR_OID(ip, &NBR_ID(&intf->nbr), sizeof (NBR_ID(&intf->nbr)));
		
	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    int j;

	    intf = o_vintf_lookup(ip = oid->oid_elements + ot->ot_name->oid_nelem,
				  (u_int) (j = oid->oid_nelem - ot->ot_name->oid_nelem),
				  TRUE);
	    if (!intf) {
		return NOTOK;
	    }

	    if ((i = j - NDX_SIZE) < 0) {
		if ((new = oid_extend(oid, -i)) == NULLOID) {
		    return int_SNMP_error__status_genErr;
		}
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    } else if (i > 0) {
		oid->oid_nelem -= i;
	    }
		
	    ip = oid->oid_elements + ot->ot_name->oid_nelem;
	    STR_OID(ip, &intf->trans_area->area_id, sizeof (intf->trans_area->area_id));
	    STR_OID(ip, &NBR_ID(&intf->nbr), sizeof (NBR_ID(&intf->nbr)));
	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }
#undef	NDX_SIZE

    switch (ot2object(ot)->ot_info) {
    case IospfVirtIfAreaId:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(sockbuild_in(0, intf->trans_area->area_id),
				  (int *) 0));

    case IospfVirtIfNeighbor:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(intf->nbr.nbr_id,
				  (int *) 0));

    case IospfVirtIfTransitDelay:
	return o_integer(oi, v, intf->transdly);

    case IospfVirtIfRetransInterval:
	return o_integer(oi, v, intf->retrans_timer);

    case IospfVirtIfHelloInterval:
	return o_integer(oi, v, intf->hello_timer);

    case IospfVirtIfRtrDeadInterval:
	return o_integer(oi, v, intf->dead_timer);

    case IospfVirtIfState:
	return o_integer(oi, v, o_intf_state(intf));

    case IospfVirtIfEvents:
	return o_integer(oi, v, intf->events);

    case IospfVirtIfAuthKey:
	/* When read, ospfIfAuthKey always returns an Octet String of length zero. */
	return o_string(oi, v, (caddr_t) 0, 0);

    case IospfVirtIfStatus:
	return o_integer(oi, v, MIB_VALID);
    }
    
    return int_SNMP_error__status_noSuchName;
}


/* Neighbor group */

/* INDEX { ospfNbrIpAddr, ospfNbrAddressLessIndex } */
#define	NDX_SIZE	(sizeof (struct in_addr) + 1)


struct nbr_entry {
    struct NBR *nbr;
    u_int32 addr;
    u_int index;
};

static struct nbr_entry *o_nbr_list;
static u_int o_nbr_cnt;
static u_int o_nbr_size;

static int
o_nbr_compare __PF2(p1, const VOID_T,
		    p2, const VOID_T)
{
    register const struct nbr_entry *nbrp1 = (const struct nbr_entry *) p1;
    register const struct nbr_entry *nbrp2 = (const struct nbr_entry *) p2;

    if (nbrp1->addr == nbrp2->addr
	&& nbrp1->index == nbrp2->index) {
	return 0;
    } else if (nbrp1->addr < nbrp2->addr
	       || (nbrp1->addr == nbrp2->addr
		   && nbrp1->index < nbrp2->index)) {
	return -1;
    }

    return 1;
}


static void
o_nbr_get __PF0(void)
{
    register struct nbr_entry *nbrp;
    register struct intf_entry *intfp;

    if (o_nbr_size < ospf.nbrcnt + ospf.nintf) {
	if (o_nbr_list) {
	    task_block_reclaim((size_t) (o_nbr_size * sizeof (*o_nbr_list)), (void_t) o_nbr_list);
	}

	o_nbr_size = ospf.nbrcnt + ospf.nintf;
	o_nbr_list = (struct nbr_entry *) task_block_malloc((size_t) (o_nbr_size * sizeof (*o_nbr_list)));
    }

    nbrp = o_nbr_list;
    o_nbr_cnt = 0;
    
    MINTF_LIST(intfp) {
	register struct NBR *nbr;

	NBRS_LIST(nbr, intfp->intf) {
	    assert(o_nbr_cnt <= o_nbr_size);
	    nbrp->nbr = nbr;
	    nbrp->addr = ntohl(NBR_ADDR(nbr));
	    if (BIT_TEST(nbr->intf->ifap->ifa_state, IFS_POINTOPOINT)
		&& nbr->intf->ifap->ifa_addrent_local->ifae_n_if > 1) {
		nbrp->index = nbr->intf->ifap->ifa_link->ifl_index;
	    } else {
		nbrp->index = 0;
	    }
	    nbrp++;
	} NBRS_LIST_END(nbr, intfp->intf) ;
    } MINTF_LIST_END(intfp) ;

    o_nbr_cnt = nbrp - o_nbr_list;

    qsort((caddr_t) o_nbr_list,
	  o_nbr_cnt,
	  sizeof (struct nbr_entry), 
	  o_nbr_compare);
}


static struct nbr_entry *
o_nbr_lookup __PF3(ip, register unsigned int *,
		   len, u_int,
		   isnext, int)
{
    static struct nbr_entry *last_nbrp;
    static unsigned int *last;
    static int last_quantum;

    if (last_quantum != snmp_quantum) {
	last_quantum = snmp_quantum;

	o_nbr_get();

	if (last) {
	    task_mem_free((task *) 0, (void_t) last);
	    last = (unsigned int *) 0;
	}
    }

    if (snmp_last_match(&last, ip, len, isnext)) {
	return last_nbrp;
    }

    if (!o_nbr_cnt) {
	return last_nbrp = (struct nbr_entry *) 0;
    }
    
    if (len) {
	u_int32 nbr_addr = (u_int32) 0;
	int nbr_index;
	register struct nbr_entry *nbrp = o_nbr_list;
	struct nbr_entry *lp = &o_nbr_list[o_nbr_cnt];
	
	oid2ipaddr(ip, &nbr_addr);
	GNTOHL(nbr_addr);

	nbr_index = ip[sizeof (struct in_addr)];

	do {
	    if (nbrp->addr == nbr_addr
		&& nbrp->index == nbr_index) {
		if (!isnext) {
		    return last_nbrp = nbrp;
		}
	    } else if (nbrp->addr > nbr_addr
		       || (nbrp->addr == nbr_addr
			   && nbrp->index == nbr_index)) {
		return last_nbrp = isnext ? nbrp : (struct nbr_entry *) 0;
	    }
	} while (++nbrp < lp) ;

	return last_nbrp = (struct nbr_entry *) 0;
    }

    return last_nbrp = o_nbr_list;
}


static inline int
o_nbr_state __PF1(s_nbr, struct NBR *)
{
    int state;
    
    switch (s_nbr->state) {
    case NDOWN:
	state = N_STATE_DOWN;
	break;

    case NATTEMPT:
	state = N_STATE_ATTEMPT;
	break;
	
    case NINIT:
	state = N_STATE_INIT;
	break;
	
    case N2WAY:
	state = N_STATE_2WAY;
	break;
	
    case NEXSTART:
	state = N_STATE_EXSTART;
	break;
	
    case NEXCHANGE:
	state = N_STATE_EXCHANGE;
	break;
	
    case NLOADING:
	state = N_STATE_LOADING;
	break;

    case NFULL:
	state = N_STATE_FULL;
	break;

    default:
	state = -1;
	break;
    }

    return state;
}


static int
o_nbr __PF3(oi, OI,
	    v, register struct type_SNMP_VarBind *,
	    offset, int)
{
    register int    i;
    register unsigned int *ip,
			  *jp;
    register OID    oid = oi->oi_name;
    register OT	    ot = oi->oi_type;
    OID		    new;
    struct nbr_entry *nbrp;

    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
	if (oid->oid_nelem - ot->ot_name->oid_nelem != NDX_SIZE) {
	    return int_SNMP_error__status_noSuchName;
	}
	nbrp = o_nbr_lookup(oid->oid_elements + oid->oid_nelem - NDX_SIZE,
			    NDX_SIZE,
			    0);
	if (!nbrp) {
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
	/* next request with incomplete instance? */
	if ((i = oid->oid_nelem - ot->ot_name->oid_nelem) != 0 && i < NDX_SIZE) {
	    for (jp = (ip = oid->oid_elements + 
		       ot->ot_name->oid_nelem - 1) + i;
		 jp > ip;
		 jp--) {
		if (*jp != 0) {
		    break;
		}
	    }
	    if (jp == ip) {
		oid->oid_nelem = ot->ot_name->oid_nelem;
	    } else {
		if ((new = oid_normalize(oid, NDX_SIZE - i, 256)) == NULLOID) {
			return NOTOK;
		    }
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    }
	}

	/* next request with no instance? */
	if (oid->oid_nelem == ot->ot_name->oid_nelem) {
	    nbrp = o_nbr_lookup((unsigned int *) 0,
				0,
				TRUE);
	    if (!nbrp) {
		return NOTOK;
	    }
	    
	    if ((new = oid_extend(oid, NDX_SIZE)) == NULLOID) {
		return int_SNMP_error__status_genErr;
	    }

	    ip = new->oid_elements + new->oid_nelem - NDX_SIZE;
	    STR_OID(ip, &sock2ip(nbrp->nbr->nbr_addr), sizeof (sock2ip(nbrp->nbr->nbr_addr)));
	    INT_OID(ip, nbrp->index);
		
	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    int j;

	    nbrp = o_nbr_lookup(ip = oid->oid_elements + ot->ot_name->oid_nelem,
				(u_int) (j = oid->oid_nelem - ot->ot_name->oid_nelem),
				TRUE);
	    if (!nbrp) {
		return NOTOK;
	    }

	    if ((i = j - NDX_SIZE) < 0) {
		if ((new = oid_extend(oid, -i)) == NULLOID) {
		    return int_SNMP_error__status_genErr;
		}
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    } else if (i > 0) {
		oid->oid_nelem -= i;
	    }
		
	    ip = oid->oid_elements + ot->ot_name->oid_nelem;
	    STR_OID(ip, &sock2ip(nbrp->nbr->nbr_addr), sizeof (sock2ip(nbrp->nbr->nbr_addr)));
	    INT_OID(ip, nbrp->index);
	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }
#undef	NDX_SIZE
    
    switch (ot2object(ot)->ot_info) {
    case IospfNbrIpAddr:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(nbrp->nbr->nbr_addr,
				  (int *) 0));

    case IospfNbrAddressLessIndex:
	return o_integer(oi, v, nbrp->index);

    case IospfNbrRtrId:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(nbrp->nbr->nbr_id ?
			nbrp->nbr->nbr_id : inet_addr_default,
				  (int *) 0));

    case IospfNbrOptions:
	i = 0;

	if (!BIT_TEST(nbrp->nbr->intf->area->area_flags, OSPF_AREAF_STUB)) {
	    BIT_SET(i, MIB_BIT_ASE);
	}

	/* TOS not supported */
	return o_integer(oi, v, i);

    case IospfNbrPriority:
	return o_integer(oi, v, nbrp->nbr->pri);

    case IospfNbrState:
	return o_integer(oi, v, o_nbr_state(nbrp->nbr));

    case IospfNbrEvents:
	return o_integer(oi, v, nbrp->nbr->events);

    case IospfNbrLsRetransQLen:
	return o_integer(oi, v, nbrp->nbr->rtcnt);

    case IospfNbmaNbrStatus:
	switch (nbrp->nbr->intf->type) {
	case NONBROADCAST:
	    return o_integer(oi, v, MIB_VALID);

	default:
	    return NOTOK;
	}

    case IospfNbmaNbrPermanence:
	return o_integer(oi, v,
			 nbrp->nbr->intf->type == NONBROADCAST ? N_PERMANENCE_PERMANENT : N_PERMANENCE_DYNAMIC);	/* XXX - ??? */
    }
    
    return int_SNMP_error__status_noSuchName;
}


/* Virtual neighbor group */
static int
o_vnbr __PF3(oi, OI,
	     v, register struct type_SNMP_VarBind *,
	     offset, int)
{
    register int    i;
    register unsigned int *ip,
			  *jp;
    register OID    oid = oi->oi_name;
    register OT	    ot = oi->oi_type;
    OID		    new;
    struct INTF *intf;

    /* INDEX { ospfVirtNbrArea, ospfVirtNbrRtrId } */
#define	NDX_SIZE	(sizeof (struct in_addr) + sizeof (struct in_addr))

    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
	if (oid->oid_nelem - ot->ot_name->oid_nelem != NDX_SIZE) {
	    return int_SNMP_error__status_noSuchName;
	}
	intf = o_vintf_lookup(oid->oid_elements + oid->oid_nelem - NDX_SIZE,
			      NDX_SIZE,
			      0);
	if (!intf) {
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
	/* next request with incomplete instance? */
	if ((i = oid->oid_nelem - ot->ot_name->oid_nelem) != 0 && i < NDX_SIZE) {
	    for (jp = (ip = oid->oid_elements + 
		       ot->ot_name->oid_nelem - 1) + i;
		 jp > ip;
		 jp--) {
		if (*jp != 0) {
		    break;
		}
	    }
	    if (jp == ip) {
		oid->oid_nelem = ot->ot_name->oid_nelem;
	    } else {
		if ((new = oid_normalize(oid, NDX_SIZE - i, 256)) == NULLOID) {
			return NOTOK;
		    }
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    }
	}

	/* next request with no instance? */
	if (oid->oid_nelem == ot->ot_name->oid_nelem) {
	    intf = o_vintf_lookup((unsigned int *) 0,
				  0,
				  TRUE);
	    if (!intf) {
		return NOTOK;
	    }
	    
	    if ((new = oid_extend(oid, NDX_SIZE)) == NULLOID) {
		return int_SNMP_error__status_genErr;
	    }

	    ip = new->oid_elements + new->oid_nelem - NDX_SIZE;
	    STR_OID(ip, &intf->trans_area->area_id, sizeof (intf->trans_area->area_id));
	    STR_OID(ip, &NBR_ID(&intf->nbr), sizeof (NBR_ID(&intf->nbr)));
		
	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    int j;

	    intf = o_vintf_lookup(ip = oid->oid_elements + ot->ot_name->oid_nelem,
				  (u_int) (j = oid->oid_nelem - ot->ot_name->oid_nelem),
				  TRUE);
	    if (!intf) {
		return NOTOK;
	    }

	    if ((i = j - NDX_SIZE) < 0) {
		if ((new = oid_extend(oid, -i)) == NULLOID) {
		    return int_SNMP_error__status_genErr;
		}
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    } else if (i > 0) {
		oid->oid_nelem -= i;
	    }
		
	    ip = oid->oid_elements + ot->ot_name->oid_nelem;
	    STR_OID(ip, &intf->trans_area->area_id, sizeof (intf->trans_area->area_id));
	    STR_OID(ip, &NBR_ID(&intf->nbr), sizeof (NBR_ID(&intf->nbr)));
	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }
#undef	NDX_SIZE
    
    switch (ot2object(ot)->ot_info) {
    case IospfVirtNbrArea:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(sockbuild_in(0, intf->trans_area->area_id),
				  (int *) 0));

    case IospfVirtNbrRtrId:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(intf->nbr.nbr_id ? intf->nbr.nbr_id : inet_addr_default,
				  (int *) 0));

    case IospfVirtNbrIpAddr:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(intf->nbr.nbr_id ? intf->nbr.nbr_id : inet_addr_default,
				  (int *) 0));

    case IospfVirtNbrOptions:
	i = 0;
	
	if (!BIT_TEST(intf->area->area_flags, OSPF_AREAF_STUB)) {
	    BIT_SET(i, MIB_BIT_ASE);
	}

	/* TOS not supported */
	return o_integer(oi, v, i);

    case IospfVirtNbrState:
	return o_integer(oi, v, o_nbr_state(&intf->nbr));

    case IospfVirtNbrEvents:
	return o_integer(oi, v, intf->nbr.events);

    case IospfVirtNbrLsRetransQLen:
	return o_integer(oi, v, intf->nbr.dbcnt + intf->nbr.reqcnt + intf->nbr.rtcnt);

    }
    
    return int_SNMP_error__status_noSuchName;
}


void
ospf_init_mib(enabled)
int enabled;
{
    if (enabled) {
	if (!o_intf_index) {
	    o_intf_index = task_block_init(sizeof (struct intf_entry), "ospf_intf_entry");
	}
	snmp_tree_register(&ospf_mib_tree);

    } else {
	struct AREA *area;
	struct intf_entry *intfp;
	
	snmp_tree_unregister(&ospf_mib_tree);

	/* Free LSDB memory */
	AREA_LIST(area) {
	    int type;

	    for (type = LS_RTR; type < LS_ASE; type++) {
		if (area->mib_lsdb_size[type]) {
		    task_block_reclaim((size_t) ((area->mib_lsdb_size[type] + 1) * sizeof (struct LSDB *)),
				       (void_t) area->mib_lsdb_list[type]);
		    area->mib_lsdb_list[type] = (struct LSDB **) 0;
		    area->mib_lsdb_cnt[type] = area->mib_lsdb_size[type] = 0;
		}
	    }
	} AREA_LIST_END(area) ;

	if (ospf.mib_ase_size) {
	    task_block_reclaim((size_t) ((ospf.mib_ase_size + 1) * sizeof (struct LSDB *)),
			       (void_t) ospf.mib_ase_list);
	    ospf.mib_ase_cnt = ospf.mib_ase_size = 0;
	}

	/* Free Interface memory */
	MINTF_LIST(intfp) {
	    register struct intf_entry *intfp2 = intfp->back;

	    REMQUE(intfp);
	    task_block_free(o_intf_index, (void_t) intfp);

	    intfp = intfp2;
	} MINTF_LIST_END(intfp) ;

	MVINTF_LIST(intfp) {
	    register struct intf_entry *intfp2 = intfp->back;

	    REMQUE(intfp);
	    task_block_free(o_intf_index, (void_t) intfp);

	    intfp = intfp2;
	} MVINTF_LIST_END(intfp) ;

	/* Free neighbor memory */
	if (o_nbr_size) {
	    if (o_nbr_list) {
		task_block_reclaim((size_t) (o_nbr_size * sizeof (*o_nbr_list)), (void_t) o_nbr_list);
	    }

	    o_nbr_size = o_nbr_cnt = 0;
	    o_nbr_list = (struct nbr_entry *) 0;
	}

    }
}
