#ident	"@(#)sets.c	1.3"
#ident	"$Header$"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1989, 1990 INTERACTIVE Systems Corporation
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
 * Revision History:
 *
 * L000		jont		02oct97
 *	- fixed incorrect handling of varbind lists.
 *
 */

#ifndef lint
static char SNMPID[] = "@(#)sets.c	3.2 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "snmp.h"
#include "snmpuser.h"
#include "snmpd.h"
#include "peer.h"

extern struct smuxPeer *PHead;
extern struct smuxTree *THead;

#define OK       0
#define NOTOK   -1

#define TRUE  1
#define FALSE 0

#define SET        0
#define COMMIT     1
#define ROLLBACK   2

extern VarEntry *var_list_root;
extern VarEntry *find_var();
extern Pdu *make_error_pdu();


Pdu *do_sets(Pdu *in_pdu_ptr)
{
  VarBindList *old_vb_ptr;
  VarEntry *var;
  int var_counter, var_counter2;
  int commit;

  old_vb_ptr = in_pdu_ptr->var_bind_list;
  
  commit = 1;
  var_counter = 1;
  while (old_vb_ptr != NULL) 
    {
      if ((var = find_var (var_list_root, old_vb_ptr, EXACT)) == NULL)
	goto no_name;

      if ((var->smux == NULL) && ((var->funct_set == NULL) ||
				  (var->funct_test_set == NULL)))
	goto no_name;

      if (var->rw_flag != READ_WRITE) /* it's not read_write */
	{
	  snmpstat->inbadcommunityuses++;
no_name: ;
	  return (make_error_pdu (GET_RESPONSE_TYPE,
				  in_pdu_ptr->u.normpdu.request_id, 
				  NO_SUCH_NAME_ERROR,
				  var_counter, in_pdu_ptr));
	}

      if ((var->type != old_vb_ptr->vb_ptr->value->type) 
	  && (old_vb_ptr->vb_ptr->value->type != NULL_TYPE)
	  && (var->smux == NULL))
	return (make_error_pdu (GET_RESPONSE_TYPE,
				in_pdu_ptr->u.normpdu.request_id, 
				BAD_VALUE_ERROR,
				var_counter, in_pdu_ptr));

      if (var->smux) 
	{
	  VarBindList *tmp_vb_ptr;
	  OID name, oid_value; 
	  OctetString *os_value;
	  u_long 	ul_value = old_vb_ptr->vb_ptr->value->ul_value;
	  long        sl_value = old_vb_ptr->vb_ptr->value->sl_value;
	  short       type = old_vb_ptr->vb_ptr->value->type;    

	  /* allocate a temp. var_bind for this */
	  name = (OID ) oid_cpy(old_vb_ptr->vb_ptr->name);
	  oid_value = (OID ) oid_cpy(old_vb_ptr->vb_ptr->value->oid_value);
	  os_value = (OctetString *) 
	    os_cpy(old_vb_ptr->vb_ptr->value->os_value);

	  tmp_vb_ptr = make_varbind (name, type, ul_value, sl_value,
				     os_value, oid_value);

	  if (smux_set_method (var,
			       ((struct smuxTree *) var->smux)->tb_peer,
			       tmp_vb_ptr, SET, in_pdu_ptr->u.normpdu.request_id) != OK) 
	    commit = 0;
	  /*
	   * now you can get rid of it. We do not need to free the
	   * individual parts as they are built into the varbind list
	   * and will be freed automatically (L000)
	   */
	  free_varbind_list(tmp_vb_ptr);
	}
      else
	if ((*var->funct_test_set) (var->class_ptr, 
				    old_vb_ptr->vb_ptr->name, var->arg,
				    old_vb_ptr->vb_ptr->value) == FALSE) 
	  {
	    return (make_error_pdu (GET_RESPONSE_TYPE,
				    in_pdu_ptr->u.normpdu.request_id, 
				    GEN_ERROR,
				    var_counter, in_pdu_ptr));
	    break;
	  }

      if (!commit)
	break;
      old_vb_ptr = old_vb_ptr->next;
      var_counter++;
    }

  var_counter2 = 1;
  old_vb_ptr = in_pdu_ptr->var_bind_list;
  if (!commit) 
    {
      while ((old_vb_ptr != NULL) && (var_counter2 < var_counter)) 
	{
	  if ((var = find_var (var_list_root, old_vb_ptr, EXACT)) == NULL)
	    goto no_name;

	  if (var->smux) 
	    smux_set_method (var,
			     ((struct smuxTree *) var->smux)->tb_peer,
			     old_vb_ptr, ROLLBACK, in_pdu_ptr->u.normpdu.request_id);

	  old_vb_ptr = old_vb_ptr->next;
	  var_counter2++;
	}
      return (make_error_pdu (GET_RESPONSE_TYPE,
			      in_pdu_ptr->u.normpdu.request_id, GEN_ERROR,
			      var_counter, in_pdu_ptr));

    }
    else 
      {
	/* OK, now actually do the set */
	while ((old_vb_ptr != NULL) && (var_counter2 < var_counter)) 
	  {
	    if ((var = find_var (var_list_root, old_vb_ptr, EXACT)) == NULL)
	      goto no_name;

	    if (var->smux)  
	      smux_set_method (var,
			       ((struct smuxTree *) var->smux)->tb_peer,
			       old_vb_ptr, COMMIT, in_pdu_ptr->u.normpdu.request_id);
	    else
	      (*var->funct_set) (var->class_ptr, old_vb_ptr->vb_ptr->name,
				 var->arg, old_vb_ptr->vb_ptr->value);
	    
	    old_vb_ptr = old_vb_ptr->next;
	    var_counter2++;
	  }
      }

  snmpstat->intotalsetvars += (var_counter2 - 1);
  
  (void) peer_resets();
  return(NULL);
}


int peer_resets(void)
{
  register struct smuxPeer *pb, *qb;
  register struct smuxTree *tb, *ub;

  for (pb = PHead->pb_forw; pb != PHead; pb = qb) 
    {
      qb = pb->pb_forw;

      if (pb->pb_invalid)
	pb_free (pb);
    }

  for (tb = THead->tb_forw; tb != THead; tb = ub) 
    {
      ub = tb->tb_forw;
      if (tb->tb_invalid)
	tb_free (tb);
    }
}


