#ident	"@(#)v_smux.c	1.2"
#ident	"$Header$"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/* v_smux.c - SMUX group */

/*
 *
 * Contributed by NYSERNet Inc. This work was partially supported by
 * the U.S. Defense Advanced Research Projects Agency and the Rome
 * Air Development Center of the U.S. Air Force Systems Command under
 * contract number F30602-88-C-0016.
 *
 */

/*
 * All contributors disclaim all warranties with regard to this
 * software, including all implied warranties of mechantibility
 * and fitness. In no event shall any contributor be liable for
 * any special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in action of contract, negligence or other tortuous action,
 * arising out of or in connection with, the use or performance
 * of this software.
 */

/*
 * As used above, "contributor" includes, but not limited to:
 * NYSERNet, Inc.
 * Marshall T. Rose
 */

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "snmp.h" /*"../lib/snmp.h"*/
#include "snmpuser.h" /*"../lib/snmpuser.h"*/
#include "snmpd.h"
#include "peer.h"

#define SMUX
#define FALSE   0
#define TRUE    1

/*   SMUX GROUP */

#ifdef	SMUX
#define	smuxPindex	1
#define	smuxPidentity	2
#define	smuxPdescription 3
#define	smuxPstatus	4

#define PB_VALID        1               /* smuxPstatus */
#define PB_INVALID      2               /*   .. */
#define PB_CONNECTING   3               /*   .. */

VarBindList *get_next_class();
int elem_cmp();

extern  struct smuxPeer *PHead;
extern  struct smuxTree *THead;
extern	int log_level;


VarBindList
*var_smuxPeer_get (var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
OID var_name_ptr;
OID in_name_ptr;
int arg;
VarEntry *var_next_ptr;
int type_search;
{
    int		ifnum = 0;
    char	buffer[256];
    OID		new;
    OID		oidvalue_ptr;
    OctetString *os_ptr;
    VarBindList	*vb_ptr;
    register struct smuxPeer *pb;

    /* An exact search has only one sub-id field for smuxPIndex */
    if ((type_search == EXACT) &&
	(in_name_ptr->length != (var_name_ptr->length + 1)))
	return (NULL);

    /* Now find out which peer they are interested in */
    if (in_name_ptr->length > var_name_ptr->length)
	ifnum = in_name_ptr->oid_ptr[var_name_ptr->length];

    if (type_search == NEXT)
	ifnum ++;

    for (pb = PHead->pb_forw; pb != PHead; pb = pb->pb_forw) {
	if (pb->pb_index == ifnum)
	    break;
	if ((pb->pb_index > ifnum) && (type_search == NEXT))
	    break;
    }

    if ((pb == PHead) && (type_search == EXACT))
	return (NULL);

    if ((type_search == NEXT) && (pb == PHead))
	return (get_next_class(var_next_ptr));

    if (((arg == smuxPidentity) || (arg == smuxPdescription))
	&& (pb->pb_identity == NULL)) {
	if (type_search == NEXT)
	    return (get_next_class(var_next_ptr));
	return (NULL);
    }
    ifnum = pb->pb_index;

    switch (arg) {
	case 1:
	    sprintf(buffer,"smuxPindex.%d", ifnum);
	    new = make_obj_id_from_dot((unsigned char *)buffer);
	    vb_ptr = make_varbind(new, INTEGER_TYPE,
				  0, pb->pb_index, NULL, NULL);
	    new = NULL;
	    break;
	case 2:
	    sprintf(buffer,"smuxPidentity.%d", ifnum);
	    new = make_obj_id_from_dot((unsigned char *)buffer);
	    if ((oidvalue_ptr = oid_cpy (pb->pb_identity)) == NULL)
		return (NULL);
	    vb_ptr = make_varbind(new, OBJECT_ID_TYPE,
				  0, 0, NULL, oidvalue_ptr);
	    new = NULL;
	    oidvalue_ptr = NULL;
	    break;
	case 3:
	    sprintf(buffer,"smuxPdescription.%d", ifnum);
	    new = make_obj_id_from_dot((unsigned char *)buffer);
	    os_ptr = make_octet_from_text((unsigned char *)pb->pb_description);
	    vb_ptr = make_varbind(new, DisplayString,
				  0, 0, os_ptr, NULL);
	    new = NULL;
	    os_ptr = NULL;
	    break;
	case 4:
	    sprintf(buffer,"smuxPstatus.%d", ifnum);
	    new = make_obj_id_from_dot((unsigned char *)buffer);
	    vb_ptr = make_varbind(new, INTEGER_TYPE,
				  0, pb->pb_identity ? PB_VALID
				  : PB_CONNECTING, NULL, NULL);
	    new = NULL;
	    break;
	default:
	    return (NULL);
	    break;
    }
    return (vb_ptr);

}

/*  */

int
var_smuxPeer_test (var_name_ptr, in_name_ptr, arg, value)
OID var_name_ptr;
OID in_name_ptr;
int arg;
ObjectSyntax *value;
{
    int		ifnum = 0;
    register struct smuxPeer *pb;

    if (in_name_ptr->length != (var_name_ptr->length + 1))
	return (FALSE);

    /* Now find out which peer they are interested in */
    ifnum = in_name_ptr->oid_ptr[var_name_ptr->length];

    for (pb = PHead->pb_forw; pb != PHead; pb = pb->pb_forw)
	if (pb->pb_index == ifnum)
	    break;

    if (pb == PHead)
	return (FALSE);

    switch (arg) {
	case 4:
	    pb->pb_newstatus = value->sl_value;
	    if ((value->sl_value != 1) && (value->sl_value != 2))
		return (FALSE);
	    if ((value->sl_value == 1) && (!pb->pb_identity))
		return (FALSE);
	    break;

	default:
	    return (FALSE);
    }
    return (TRUE);

}

/*  */

int
var_smuxPeer_set (var_name_ptr, in_name_ptr, arg, value)
OID var_name_ptr;
OID in_name_ptr;
int arg;
ObjectSyntax *value;
{
    int		ifnum = 0;
    register struct smuxPeer *pb;

    if (in_name_ptr->length != (var_name_ptr->length + 1))
	return (FALSE);

    /* Now find out which peer they are interested in */
    ifnum = in_name_ptr->oid_ptr[var_name_ptr->length];

    for (pb = PHead->pb_forw; pb != PHead; pb = pb->pb_forw)
	if (pb->pb_index == ifnum)
	    break;

    if (pb == PHead)
	return (FALSE);

    switch (arg) {
	case 4:
	    if (pb->pb_newstatus == 2)
		pb->pb_invalid = 1;
	    break;

	default:
	    return (FALSE);
    }
    return (TRUE);

}

/*  */

#define	smuxTsubtree	1
#define	smuxTpriority	2
#define	smuxTindex	3
#define	smuxTstatus	4

#define	TB_VALID	1		/* smuxTstatus */
#define	TB_INVALID	2		/*   .. */

struct smuxTree *get_tbent ();


VarBindList
*var_smuxTree_get (var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
OID var_name_ptr;
OID in_name_ptr;
int  arg;
VarEntry *var_next_ptr;
int type_search;
{
    OID tmp_oid;
    OID	new;
    OID	oidvalue_ptr;
    VarBindList	*vb_ptr;
    register struct smuxTree *tb;

    register int    i, j, k, l;
    register unsigned int *ip;
    register unsigned int *jp;

    tmp_oid = NULL;
    new = NULL;
    oidvalue_ptr = NULL;
    vb_ptr = (VarBindList *)NULL;

    i = in_name_ptr->length;
    j = var_name_ptr->length;
    k = i - j;

    if (type_search == EXACT) {
	if (i <= j)
	    return (NULL);

	if ((tb = get_tbent(in_name_ptr->oid_ptr + j, k, 0)) == NULL)
	    return (NULL);

	if ((new = oid_cpy (in_name_ptr)) == NULL) 
	    return (NULL);

    }
    else { /* NEXT */
	if (i <= j) {
	    if ((tb = THead->tb_forw) == THead)
		return (get_next_class(var_next_ptr));

	    if ((new = oid_extend (var_name_ptr, tb->tb_insize)) == NULL)
		return (get_next_class(var_next_ptr));

	    ip = new->oid_ptr + new->length - tb->tb_insize;
	}
	else {
	    tmp_oid = in_name_ptr;
	    if ((tb = get_tbent(tmp_oid->oid_ptr + j, k, 1)) == NULL)
		return (get_next_class(var_next_ptr));

	    if ((l = k - tb->tb_insize) < 0) {
		if ((new = oid_extend (in_name_ptr, -l)) == NULL)
		    return (get_next_class(var_next_ptr));
	    }
	    else {
		    if ((new = oid_cpy (in_name_ptr)) == NULL) 
			return (get_next_class(var_next_ptr));
		    new->length = in_name_ptr->length - l;
	    }

	    ip = new->oid_ptr + var_name_ptr->length;
	}
	jp = tb->tb_instance;
	for (i = tb->tb_insize; i > 0; i--)
	    *ip++ = (long)*jp++;

    }

    switch (arg) {
	case 1:
	    if ((oidvalue_ptr = oid_cpy (tb->tb_subtree->class_ptr)) == NULL)
		return (NULL);
	    vb_ptr = make_varbind(new, OBJECT_ID_TYPE,
				  0, 0, NULL, oidvalue_ptr);
	    new = NULL;
	    oidvalue_ptr = NULL;
	    break;
	case 2:
	    vb_ptr = make_varbind(new, INTEGER_TYPE,
				  0, tb->tb_priority, NULL, NULL);
	    new = NULL;
	    break;
	case 3:
	    vb_ptr = make_varbind(new, INTEGER_TYPE,
				  0, tb->tb_peer->pb_index, NULL, NULL);
	    new = NULL;
	    break;
	case 4:
	    vb_ptr = make_varbind(new, INTEGER_TYPE,
				  0, TB_VALID , NULL, NULL);
	    new = NULL;
	    break;
	default:
	    return (NULL);
	    break;
    }
    return (vb_ptr);

}

/*  */

int
var_smuxTree_test (var_name_ptr, in_name_ptr, arg, value)
OID var_name_ptr;
OID in_name_ptr;
int arg;
ObjectSyntax *value;
{
    register struct smuxTree *tb;
    register int    i, j, k;

    i = in_name_ptr->length;
    j = var_name_ptr->length;
    k = i - j;

    if (i <= j)
	return (FALSE);

    if ((tb = get_tbent(in_name_ptr->oid_ptr + j, k, 0)) == NULL)
	return (FALSE);

    switch (arg) {
	case 4:
	    tb->tb_newstatus = value->sl_value;
	    if ((tb->tb_newstatus != 1) && (tb->tb_newstatus != 2))
		return (FALSE);
	    break;

	default:
	    return (FALSE);
    }
    return (TRUE);

}
/*  */

int
var_smuxTree_set (var_name_ptr, in_name_ptr, arg, value)
OID var_name_ptr;
OID in_name_ptr;
int arg;
ObjectSyntax *value;
{
    register struct smuxTree *tb;
    register int    i, j, k;

    i = in_name_ptr->length;
    j = var_name_ptr->length;
    k = i - j;

    if (i <= j)
	return (FALSE);

    if ((tb = get_tbent(in_name_ptr->oid_ptr + j, k, 0)) == NULL)
	return (FALSE);

    switch (arg) {
	case 4:
	    if (tb->tb_newstatus == TB_INVALID)
		tb->tb_invalid = 1;
	    break;

	default:
	    return (FALSE);
    }
    return (TRUE);

}

/*  */

static struct smuxTree *get_tbent (ip, len, isnext)
register unsigned int *ip;
int	len;
int	isnext;
{
    register struct smuxTree *tb;

    for (tb = THead->tb_forw; tb != THead; tb = tb->tb_forw)
	switch (elem_cmp (tb->tb_instance, tb->tb_insize, ip, len)) {
	    case 0:
	        if (!isnext)
		    return tb;
		if ((tb = tb->tb_forw) == THead)
		    return NULL;
		/* else fall... */

	    case 1:
		return (isnext ? tb : NULL);
	}

    return NULL;
}

#endif

