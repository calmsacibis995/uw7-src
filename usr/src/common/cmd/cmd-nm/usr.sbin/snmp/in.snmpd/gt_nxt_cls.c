#ident	"@(#)gt_nxt_cls.c	1.2"
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

#ifndef lint
static char SNMPID[] = "@(#)gt_nxt_cls.c	4.1 INTERACTIVE SNMP source";
#endif /* lint */
/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
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

#define OK	 0
#define NOTOK	-1

extern int log_level;

static VarBindList temp_vb;

VarBindList *get_next_class(VarEntry *var)
{
  VarBindList *vb_ptr;
  VarBindList *new_vb_ptr = NULL;
  OID oid_ptr = NULL;

  int status;
  
try_again: ;
  if (var == NULL)
    return(NULL);

  if (var->smux != NULL) 
    {
      if ((oid_ptr = oid_cpy (var->class_ptr)) == NULL) 
	{
	  syslog(LOG_WARNING, gettxt(":91", "Not enough memory.\n"));
	  return (NULL);
	}
      new_vb_ptr = make_varbind(oid_ptr, NULL_TYPE, 0L, 0L, NULL, NULL);

      if (log_level) 
	{
	  printf (gettxt(":92", "gt_nxt_cls: Variable is with a peer.\n"));
	  printf (gettxt(":93", "Variable is: %s.\n"), sprintoid(oid_ptr));
	}

      status = smux_get_method (var,
				((struct smuxTree *) var->smux)->tb_peer,
				new_vb_ptr, NEXT, 0);

      switch (status) 
	{
	case NOTOK:
	  free_varbind_list (new_vb_ptr); NULLIT(new_vb_ptr);
	  if (var->smux) 
	    {
	      int level = var->class_ptr->length;
	      
	      while (var->next && var->next->class_ptr->length > level)
		var = var->next;
	    }
get_next: ;
	  if (log_level)
	    printf (gettxt(":94", "gt_nxt_cls: get-next got bumped.\n"));

	  for (;;) 
	    {
	      if ((var = var->next) == NULL) 
		return NULL;
	      if ((var->funct_get) || (var->smux))
		goto try_again;
	    }

	case OK:
	  vb_ptr = new_vb_ptr;
	  break;

	default:
	  free_varbind_list (new_vb_ptr); NULLIT(new_vb_ptr);
	  return NULL;
	}
    }
  else 
    {
      if (var->funct_get == NULL) 
	{
	  var = var->next;
	  goto try_again;
	}

      if (log_level)
	printf (gettxt(":93", "Variable is: %s.\n"), sprintoid(var->class_ptr));

      vb_ptr = (var->funct_get) (var->class_ptr, var->class_ptr,
				 var->arg, var->next, NEXT);
      /* we're doing get-next */
    }
  return(vb_ptr);
}
