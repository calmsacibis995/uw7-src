#ident	"@(#)hostmibd.c	1.2"
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#if !defined(NO_SCCS_ID) && !defined(lint) && !defined(SABER)
static char rcsid[] = "@(#)$Header$";
#endif

/****************************************************************************
** Source file:  hostmibd.c
**
** Description:  Network Management Daemon process running on UnixWare.
**               It realizes Host Resources MIB (RFC 1514).
**               It interfaces the SNMP Agent running on UnixWare with
**               SMUX protocol.
**
** Contained functions:
**
** Author:   Cheng Yang
**
** Date Created:  January 25, 1993
**
** COPYRIGHT STATEMENT: (C) COPYRIGHT 1993 by Novell, Inc.
**                      Property of Novell, Inc.  All Rights Reserved.
**
****************************************************************************/

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/resource.h>

#include <sys/types.h>
#include <sys/stat.h>                 /* for file stat call */

#include <sys/metrics.h>              /* for performance metrics */
#include "snmp.h"
#include "objects.h"

/* Constants */
#include "hostmibd.h"

/* Forward References */
static void envinit(void);
static void mibinit(void);

static void start_smux(void);
static void doit_smux(void);
static void get_smux(register struct type_SNMP_GetRequest__PDU *pdu, 
		     int offset);
static void set_smux(struct type_SNMP_SMUX__PDUs *event);

void hrSystemInit(void);             /* init hrSystem group */
void hrStorageInit(void);            /* init hrStorage group */
void hrDeviceInit(void);             /* init hrDevice group */
/*void hrSWRunInit(void);*/              /* init hrSWRun group */
/*void hrSWRunPerfInit(void);*/          /* init hrSWRunPerf group */
/*void hrSWInstalledInit(void); */       /* init hrSWInstalled group */


/* External Variables */
int debug = 0;

/* DATA */
int nbits = FD_SETSIZE;

static int smux_fd = NOTOK;   /* File descriptor for SMUX interface */
static int got_at_least_one = 0;
static int rock_and_roll = 0;
static int dont_bother_anymore = 0;

static struct smuxEntry *se = NULL;

static fd_set ifds;
static fd_set ofds;

static int nwTrapTime=5;    /* default NetWare Trap Time is 5 seconds */

static struct subgroup
{
  char *t_tree;
  OID t_name;
  int t_access;
  void (*t_init)(void);
  IFP t_sync;
} subgroups[] = {
  "hrSystem", NULL, readWrite, hrSystemInit, NULL,
  "hrStorage", NULL, readOnly, hrStorageInit, NULL,
  "hrDevice", NULL, readOnly, hrDeviceInit, NULL,

/* The following three groups are not supported in UnixWare 2.0 */
/*"hrSWRun", NULL, readOnly, hrSWRunInit, NULL,
  "hrSWRunPerf", NULL, readOnly, hrSWRunPerfInit, NULL,
  "hrSWInstalled", NULL, readOnly, hrSWInstalledInit, NULL,*/

  NULL
  };

static struct subgroup * tc;

/* Main */

void main(int argc, char **argv)
{
  int nfds;

  if (argc == 2)
    {
      if (strcmp(argv[1], "-v")==0)
	debug=1;
    }

#ifdef SNMPD_PID_FILE
   /* Check if SNMP agent is up */
   {
     struct stat tmpbuff;
     int errorcount=0;

     while ((stat(SNMPD_PID_FILE, &tmpbuff)==-1) && (errorcount<20))
       {
#ifdef DEBUG
	 fprintf(stderr, "\nSNMP Daemon is not up.  Retrying. %d", errorcount);
#endif
	 sleep(5);
	 errorcount++;
       }
     if (errorcount>=20)
       {
#ifdef DEBUG
	 fprintf(stderr, "\nSNMP Daemon is not up.  hostmibd gave up.");
#endif
	 exit(0);
       }
   }
#endif

  open_mets();                /* Open performance metrics */
  snap_mets();
  envinit();

  FD_ZERO(&ifds);
  FD_ZERO(&ofds);
  nfds = 0;

  for(;;)
    {
      int n;
      int secs;
      fd_set rfds;
      fd_set wfds;

      secs = nwTrapTime;
      rfds = ifds;   /* struct copy */
      wfds = ofds;   /*   .. */

      if(smux_fd == NOTOK && !dont_bother_anymore)
	secs = 5 * 60L;
      else 
	if(rock_and_roll)
	  FD_SET(smux_fd, &rfds);
	else
	  FD_SET(smux_fd, &wfds);

      if(smux_fd >= nfds)
	nfds = smux_fd + 1;

      if((n = xselect(nfds, &rfds, &wfds, NULL, secs)) == NOTOK)
	fprintf(stderr, "\nhostmibd: xselect error.\n");

      /* check fd's for other purposes here... */

      if(smux_fd == NOTOK && !dont_bother_anymore) 
	{
	  if(n == 0)
            {
	      if((smux_fd = smux_init(debug)) == NOTOK)
		fprintf(stderr, "\nhostmibd: smux_init error.\n");
	      else
		rock_and_roll = 0;
            }
	}
      else 
	if(rock_and_roll)
	  {
	    if(FD_ISSET(smux_fd, &rfds)) 
	      doit_smux();
	  }
	else 
	  if(FD_ISSET(smux_fd, &wfds))
	    start_smux();
    }
}

/* Init */

static void envinit(void)
{
  pid_t pid=0;
  
#ifdef SVR4
  getrlimit(RLIMIT_NOFILE, (struct rlimit *)&nbits);
#else
  nbits = getdtablesize();
#endif
  
  /* daemonize here */
  if(debug == 0)
    {
      pid_t forkvalue=0;

      forkvalue=fork();

      switch(forkvalue)
	{
	case NOTOK: 
	  exit(-1);
	  
	case OK: 
	  if (setpgrp()==-1)
	    exit(-1);
	  
	  if ((pid=fork())<0)
	    exit(-1);
	  else 
	    if (pid>0)
	      exit(-1);
	  break;
	  
	default: 
	  _exit(0);
	}
    }

  mibinit();

}

/* MIB Init */

static void mibinit(void) 
{
  OT   ot;

  if((se = getsmuxEntrybyname("hostmibd")) == NULL)
    {
      fprintf(stderr, "\nhostmibd: getsmuxEntrybyname error.\n");
    }      
  
  if(readobjects(HOSTMIBD_MIB_FILE) == NOTOK)
    {
      fprintf(stderr, "\nhostmibd: readobjects error.\n");
    }

  for (tc=subgroups; tc->t_tree; tc++)
    if(ot = text2obj(tc->t_tree))
      {
	tc->t_name=ot->ot_name;
	(void)(*tc->t_init)();
      }
    else
      {
	fprintf(stderr, "\nhostmibd: text2obj error\n");
      }

  if((smux_fd = smux_init(debug)) == NOTOK)
    {
      fprintf(stderr, "\nhostmibd: smux_init error.\n");
    }
  else
    {
      rock_and_roll = 0;
    }
}

/* SMUX */

static void start_smux(void) 
{
  if(smux_simple_open(&se->se_identity, "hostmibd",
			se->se_password, strlen(se->se_password)) == NOTOK)
    {
      if(smux_errno == inProgress)
	return;
#ifdef DEBUG
      fprintf(stderr, "\nhostmibd: smux_simple_open.\n");
#endif /* DEBUG */
losing: 
      ;
      smux_fd = NOTOK;
      return;
    }
#ifdef DEBUG
  fprintf(stderr, "\nhostmibd: smux_simple_open, %s %s\n",
	  oid2ode(&se->se_identity), se->se_name);
#endif /* DEBUG */
  rock_and_roll = 1;
  
  for (tc=subgroups; tc->t_tree; tc++)
    if (tc->t_name)
      {
	/* send register request */
	if(smux_register(tc->t_name, -1, tc->t_access) == NOTOK) 
	  {
#ifdef DEBUG
	    fprintf(stderr, "\nhostmibd: smux_register, %s %s\n",
		    smux_error(smux_errno), smux_info);
#endif /* DEBUG */
	    break;
	  }
	
	/* Waiting for answer */
	{
	  struct type_SNMP_SMUX__PDUs *event;
	  
	  /* wait for at most 5 minutes for register response */
	  smux_wait(&event, 300);
	  if (event->offset== SMUX__PDUs_registerResponse)
	      {
		struct type_SNMP_RRspPDU *rsp = event->un.registerResponse;
		if(rsp->parm == RRspPDU_failure) 
		  fprintf(stderr, "\nhostmibd: smux_register failed.%s\n",
			  tc->t_tree);
#ifdef DEBUG
		else
		  fprintf(stderr, "\nhostmibd: smux_register %s.\n",
			  tc->t_tree);
#endif
	      }
	  else
	    fprintf(stderr, "\nhostmibd: smux_register failed %s.\n",
		    tc->t_tree);
	}
      }
  if(smux_trap(trap_coldStart,
		 0, (struct type_SNMP_VarBindList *) 0) == NOTOK)
    fprintf(stderr, "\nhostmibd: smux_trap %s, %s.\n",
	    smux_error(smux_errno), smux_info);
}

/*****************************************************************************/

static void doit_smux(void)
{
  struct type_SNMP_SMUX__PDUs *event;

  if(smux_wait(&event, NOTOK) == NOTOK)
    {
      if(smux_errno == inProgress)
	return;
      fprintf(stderr, "\nhostmibd: smux_wait %s %s\n",
	      smux_error(smux_errno), smux_info);
losing: 
      ;
      smux_fd = NOTOK;
      return;
    }

  switch(event->offset)
    {
    case SMUX__PDUs_get__request:
    case SMUX__PDUs_get__next__request:
      get_smux(event->un.get__request, event->offset);
      break;

    case SMUX__PDUs_close:
      fprintf(stderr, "\nhostmibd: smux_close %s.\n",
	      smux_error(event->un.close->parm));
      goto losing;
      
    case SMUX__PDUs_registerResponse:
    case SMUX__PDUs_simple:
    case SMUX__PDUs_registerRequest:
    case SMUX__PDUs_get__response:
    case SMUX__PDUs_trap:
unexpected: ;
      fprintf(stderr, "\nhostmibd: smux unexpected operations. %d\n",
	      event->offset);
      (void) smux_close(protocolError);
      goto losing;
      
    case SMUX__PDUs_set__request:
    case SMUX__PDUs_commitOrRollback:
      set_smux(event);
      break;

    default:
      fprintf(stderr, "\nhostmibd: smux unexpected operations. %d\n",
	      event->offset);
      (void) smux_close(protocolError);
      goto losing;
    }
}

/***************************************************************************/

static void get_smux(register struct type_SNMP_GetRequest__PDU *pdu, 
		     int offset)
{
  int   idx;
  int   status;

  object_instance ois;
  register struct type_SNMP_VarBindList *vp;
  
  idx = 0;

  for(vp = pdu->variable__bindings; vp; vp = vp->next)
    {
      register OI oi;
      register OT ot;
      register struct type_SNMP_VarBind *v = vp->VarBind;

      idx++;

      if(offset == SMUX__PDUs_get__next__request) 
	{
	  if((oi = name2inst(v->name)) == NULLOI
	     && (oi = next2inst(v->name)) == NULLOI)
            goto no_name;

	  if((ot = oi->oi_type)->ot_getfnx == NULLIFP)
            goto get_next;
	}
      else if((oi = name2inst(v->name)) == NULLOI
	      || (ot = oi->oi_type)->ot_getfnx == NULLIFP)
	{
no_name: 
	  ;
	  pdu->error__status = error__status_noSuchName;
	  goto out;
	}

try_again: 
      ;
      switch(ot->ot_access) 
	{
	case OT_NONE:
	  if(offset == SMUX__PDUs_get__next__request)
	    goto get_next;
	  goto no_name;
	  
	case OT_RDONLY:
	  if(offset == SMUX__PDUs_set__request)
	    {
	      pdu->error__status = error__status_noSuchName;
	      goto out;
	    }
	  break;

	case OT_RDWRITE:
	  break;
	}
      
      switch(status = (*ot->ot_getfnx) (oi, v, offset)) 
	{
	case NOTOK:       /* get-next wants a bump */
get_next: 
	  ;
	  oi = &ois;
	  for(;;) 
	    {
	      if((ot = ot->ot_next) == NULLOT) 
		{
                  pdu->error__status = error__status_noSuchName;
                  goto out;
		}

	      oi->oi_name = (oi->oi_type = ot)->ot_name;
	      
	      if(ot->ot_getfnx)
		goto try_again;
	    }
  	case error__status_noError:
	  break;

	default:
	  pdu->error__status = status;
	  goto out;
	}
    }
  
  idx = 0;

out: 
  ;
  pdu->error__index = idx;
  
  if(smux_response(pdu) == NOTOK) 
    {
      fprintf(stderr, "\nhostmibd: smux_response not ok: %s %s\n",
	      smux_error(smux_errno), smux_info);
      smux_fd = NOTOK;
    }
}

/*****************************************************************************/

static void set_smux(struct type_SNMP_SMUX__PDUs *event)
{
  struct type_SNMP_VarBindList *vp;
  register struct type_SNMP_GetRequest__PDU *pdu = 
    event->un.get__response;

  for (vp=event->un.set__request->variable__bindings; vp; vp=vp->next)
    {
      switch(event->offset) 
	{
	case SMUX__PDUs_set__request:
	  {
	    OI oi;
	    OT ot;
	    if ((oi = name2inst (vp->vb_ptr->name))==NULLOI)
	      pdu->error__status = error__status_noSuchName;
	    else
	      {
		ot=oi->oi_type;
		if (ot->ot_setfnx)
		  pdu->error__status = 
		    (ot->ot_setfnx)(oi, vp->vb_ptr, event->offset);
		else
		  pdu->error__status = error__status_noSuchName;
	      }
	    /*pdu->error__index = pdu->variable__bindings ? 1 : 0;*/
	    if(smux_response(pdu) == NOTOK)
	      {
		fprintf(stderr, "\nhostmibd: smux_response %s %s\n",
			smux_error(smux_errno), smux_info);
		smux_fd = NOTOK;
	      }
	  }
	  break;

	case SMUX__PDUs_commitOrRollback:
	  {
	    struct type_SNMP_SOutPDU *cor = event->un.commitOrRollback;
	    if(cor->parm == SOutPDU_commit)
	      {
	      }
	  }
	  break;
	}
    }
}
