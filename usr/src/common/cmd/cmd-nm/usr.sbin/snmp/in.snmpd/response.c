#ident	"@(#)response.c	1.3"
#ident	"$Header$"

/*
 * STREAMware TCP
 * Copyright 1987, 1993 Lachman Technology, Inc.
 * All Rights Reserved.
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

#ifndef lint
static char SNMPID[] = "@(#)response.c 4.1 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

/*
 * Revision History:
 *
 * L000		jont	02oct97
 *	- fixed incorrect duplication of pointers leading to problems when
 *	  structures are later freed.
 */

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <syslog.h>
#include <unistd.h>

#include "snmp.h"
#include "snmpuser.h"
#include "snmpd.h"
#include "peer.h"

#define OK       0
#define NOTOK   -1

VarEntry *find_var(VarEntry *var_entry_ptr, VarBindList *vb_ptr, 
		   int search_flag);

Pdu *make_error_pdu();
extern VarEntry *var_list_root;
extern int log_level;

static VarBindList temp_vb;

Pdu *make_response_pdu(Pdu *in_pdu_ptr)
   {
   Pdu *out_pdu_ptr;
   VarBindList *old_vb_ptr, *new_vb_ptr = NULL;
   VarEntry *var;
   OID oid_ptr = NULL;
   int type_search;
   int var_counter;
   int status;

   if(!((in_pdu_ptr->type == GET_REQUEST_TYPE) ||
         (in_pdu_ptr->type == GET_NEXT_REQUEST_TYPE) ||
         (in_pdu_ptr->type == SET_REQUEST_TYPE))) 
      {
      syslog(LOG_WARNING, 
            gettxt(":97", "Bad type (%d) for pdu in make_response_pdu().\n"),
            in_pdu_ptr->type);
      return(NULL);
      }

    /* allocate the pdu to use if successful */
   if((out_pdu_ptr = make_pdu(GET_RESPONSE_TYPE,
         in_pdu_ptr->u.normpdu.request_id, NO_ERROR,
         0, NULL, NULL, 0L, 0L, 0L)) == NULL) 
      return(NULL);

   if(in_pdu_ptr->type == SET_REQUEST_TYPE)
	{
								/* L000 */
   	out_pdu_ptr->var_bind_list = varbind_cpy(in_pdu_ptr->var_bind_list);
   	return(out_pdu_ptr);
	}

   if(in_pdu_ptr->type == GET_NEXT_REQUEST_TYPE)
      type_search = NEXT;
   else  /* GET_REQUEST_TYPE  or SET_REQUEST_TYPE */
      type_search = EXACT;

   old_vb_ptr = in_pdu_ptr->var_bind_list;

   var_counter = 1;

   while(old_vb_ptr != NULL) 
      {
      if((var = find_var(var_list_root, old_vb_ptr, type_search)) == NULL) 
         {
no_name: ;
         free_varbind_list(new_vb_ptr); 
         NULLIT(new_vb_ptr);
         free_pdu(out_pdu_ptr); 
         out_pdu_ptr = NULL;
         snmpstat->outgetresponses--; /* incremented earlier */
         return(make_error_pdu(GET_RESPONSE_TYPE,
               in_pdu_ptr->u.normpdu.request_id,
               NO_SUCH_NAME_ERROR, var_counter, in_pdu_ptr));
         }

      if(type_search == NEXT) 
         {
         if((var->funct_get == NULL) && (var->smux == NULL))
            goto get_next;
         }
      else if((var->funct_get == NULL) && (var->smux == NULL))
         goto no_name;

      if((oid_ptr = oid_cpy(old_vb_ptr->vb_ptr->name)) == NULL) 
         {
         syslog(LOG_WARNING, gettxt(":91", "Not enough memory.\n"));
         snmpstat->outgetresponses--; /* incremented earlier */
         return(make_error_pdu(GET_RESPONSE_TYPE,
              in_pdu_ptr->u.normpdu.request_id, 
               GEN_ERROR, var_counter, in_pdu_ptr));
         }

try_again: ;
      if(log_level)
         printf(gettxt(":98", "VAR requested: %s.\n"), sprintoid(oid_ptr));

      if(var->smux) 
         {
         free_varbind_list(new_vb_ptr); 
         NULLIT(new_vb_ptr);
         new_vb_ptr = make_varbind(oid_ptr, NULL_TYPE, 0L, 0L, NULL, NULL);
         NULLIT(oid_ptr);
       	status = smux_get_method(var,
                               	((struct smuxTree *)var->smux)->tb_peer,
                               	new_vb_ptr, type_search, 
				in_pdu_ptr->u.normpdu.request_id);

         switch(status) 
            {
            case NOTOK:
              free_varbind_list(new_vb_ptr); 
              NULLIT(new_vb_ptr);
              free_oid(oid_ptr); 
              NULLIT(oid_ptr);

              if(var->smux) 
                 {
                 int level = var->class_ptr->length;

                 while(var->next && var->next->class_ptr->length > level)
                    var = var->next;
                 }
get_next: ;
              for(;;) 
                 {
                 if((var = var->next) == NULL)
                    goto no_name;

                 if((var->funct_get) || (var->smux)) 
                    {
                    if((oid_ptr = oid_cpy(var->class_ptr)) == NULL) 
                       {
                       syslog(LOG_WARNING, gettxt(":91", "Not enough memory.\n"));

            /* incremented earlier */
                       snmpstat->outgetresponses--;

                       return(make_error_pdu(GET_RESPONSE_TYPE,
                             in_pdu_ptr->u.normpdu.request_id,
                             GEN_ERROR, var_counter, in_pdu_ptr));
                       }
                    goto try_again;
                    }
                 }

            case OK:
               link_varbind(out_pdu_ptr, new_vb_ptr);
               NULLIT(new_vb_ptr);
            break;

            default:
              free_varbind_list(new_vb_ptr); 
              NULLIT(new_vb_ptr);
              free_oid(oid_ptr); 
              NULLIT(oid_ptr);
              free_pdu(out_pdu_ptr); 
              out_pdu_ptr = NULL;
              snmpstat->outgetresponses--; /* incremented earlier */
            return(make_error_pdu(GET_RESPONSE_TYPE,
                  in_pdu_ptr->u.normpdu.request_id,
                  status, var_counter, in_pdu_ptr));
            }
         }
      else 
         {
         free_oid(oid_ptr); 
         NULLIT(oid_ptr);

         if((new_vb_ptr = (*var->funct_get) (var->class_ptr,
               old_vb_ptr->vb_ptr->name, /*old_vb_ptr->name,*/ var->arg,
               var->next, type_search)) == NULL)
            goto no_name;

         link_varbind(out_pdu_ptr, new_vb_ptr);
         NULLIT(new_vb_ptr);
         }

      old_vb_ptr = old_vb_ptr->next;/*old_vb_ptr->next_var;*/
      var_counter++;
      } /* while */

   if((in_pdu_ptr->type == GET_REQUEST_TYPE) ||
         (in_pdu_ptr->type == GET_NEXT_REQUEST_TYPE))
      snmpstat->intotalreqvars += (var_counter - 1);

   return(out_pdu_ptr);
   } /* end of make_response_pdu() */

VarEntry *find_var(VarEntry *var_entry_ptr, 
		   VarBindList *vb_ptr, 
		   int search_flag)
{
  int    i;

  if(var_entry_ptr == NULL)
    return(NULL);

  i = cmp_oid_class(vb_ptr->vb_ptr->name,/*vb_ptr->name,*/ 
		    var_entry_ptr->class_ptr);

  if(search_flag == EXACT)
    goto exact;

  if(i == 0) 
    {
      if(var_entry_ptr->smux)
	{
	  if(cmp_oid_total(vb_ptr->vb_ptr->name, var_entry_ptr->class_ptr) 
	     == 0)
            return(var_entry_ptr);
	  
	  if(var_entry_ptr->sibling)
            return(find_var(var_entry_ptr->sibling, vb_ptr, search_flag));
	  
	  return(var_entry_ptr);
	}
      
      if(var_entry_ptr->type == Aggregate) 
	{
	  if(cmp_oid_total(vb_ptr->vb_ptr->name, var_entry_ptr->class_ptr) 
	     == 0)
            return(var_entry_ptr);

	  if(var_entry_ptr->child)
            return(find_var(var_entry_ptr->child, vb_ptr, search_flag));
	  else
            return(var_entry_ptr);
	}
      else
	return(var_entry_ptr);
    }

  else 
    {
      if(i < 0) 
	{
	  if(var_entry_ptr->sibling)
            return(find_var(var_entry_ptr->sibling, vb_ptr, search_flag));

	  return(var_entry_ptr);
	}
      else 
	{  /* (i > 0) */
	  if(var_entry_ptr->child)
            return(find_var(var_entry_ptr->child, vb_ptr, search_flag));
	  
	  return(var_entry_ptr->next);
	}
    }

exact: ;

  if(i == 0) 
    {
      if(var_entry_ptr->smux)
	return(var_entry_ptr);

      if(var_entry_ptr->type == Aggregate) 
	{
	  if(cmp_oid_total(vb_ptr->vb_ptr->name,
			   var_entry_ptr->class_ptr) == 0)
            return(NULL);

	  return(find_var(var_entry_ptr->child, vb_ptr, search_flag));
	}
      else
	return(var_entry_ptr);
    }
  else 
    {
      if(i < 0) 
	return(find_var(var_entry_ptr->sibling, vb_ptr, search_flag));
      else  /* (i > 0) */
	return(find_var(var_entry_ptr->child, vb_ptr,   search_flag));
    }
}

Pdu *make_error_pdu(int type, int req_id, int status, int counter, Pdu *in_pdu_ptr)
   {
   Pdu *out_pdu_ptr;

   VarBindList *old_vb_ptr;
   VarBindList *tmp_vb_ptr;
   OID name_oid;
   OID value_oid;

   OctetString *value_os;

   out_pdu_ptr = make_pdu(type, req_id, status, counter, NULL, NULL, 0L, 0L, 0L);

   old_vb_ptr = in_pdu_ptr->var_bind_list;

   /* now COPY in the vb list from in_pdu_ptr */
   while(old_vb_ptr != NULL) 
      {
      name_oid = make_oid(old_vb_ptr->vb_ptr->name->oid_ptr, old_vb_ptr->vb_ptr->name->length);

      if(old_vb_ptr->vb_ptr->value->type == OBJECT_ID_TYPE)
         value_oid = make_oid(old_vb_ptr->vb_ptr->value->oid_value->oid_ptr, 
                              old_vb_ptr->vb_ptr->value->oid_value->length);
      else
         value_oid = NULL;

      if((old_vb_ptr->vb_ptr->value->type == OCTET_PRIM_TYPE) || 
            (old_vb_ptr->vb_ptr->value->type == IP_ADDR_PRIM_TYPE) ||
            (old_vb_ptr->vb_ptr->value->type == OPAQUE_PRIM_TYPE) ||
            (old_vb_ptr->vb_ptr->value->type == OCTET_CONSTRUCT_TYPE) || 
            (old_vb_ptr->vb_ptr->value->type == IP_ADDR_CONSTRUCT_TYPE) ||
            (old_vb_ptr->vb_ptr->value->type == OPAQUE_CONSTRUCT_TYPE))

         value_os = make_octetstring(old_vb_ptr->vb_ptr->value->os_value->octet_ptr,
                                 old_vb_ptr->vb_ptr->value->os_value->length);
      else
         value_os = NULL;

      tmp_vb_ptr = make_varbind(name_oid, old_vb_ptr->vb_ptr->value->type, 
                                 old_vb_ptr->vb_ptr->value->ul_value,
                                 old_vb_ptr->vb_ptr->value->sl_value,
                                 value_os, value_oid);

      link_varbind(out_pdu_ptr, tmp_vb_ptr);
      old_vb_ptr = old_vb_ptr->next;
      }

   if(status == TOO_BIG_ERROR)
      snmpstat->outtoobigs++;
   else if(status == NO_SUCH_NAME_ERROR)
      snmpstat->outnosuchnames++;
   else if(status == BAD_VALUE_ERROR)
      snmpstat->outbadvalues++;
   else if(status == READ_ONLY_ERROR)
      snmpstat->outreadonlys++;
   else if(status == GEN_ERROR)
      snmpstat->outgenerrs++;

   return(out_pdu_ptr);
   } /* end of make_error_pdu */
