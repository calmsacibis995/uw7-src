#ident "@(#)hrstorage.c	1.2"
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
** Source file:   hrstorage.c
**
** Description:   Module to realize hrStorage group of Host Resources MIB
**                (RFC 1514)
**
** Contained functions:
**                      hrStorageInit();
**                      hrStorageObj()
**
** Author:   Cheng Yang
**
** Date Created:  December 1993.
**
** COPYRIGHT STATEMENT: (C) COPYRIGHT 1993 by Novell, Inc.
**                      Property of Novell, Inc.  All Rights Reserved.
**
****************************************************************************/

/* including system include files */
#include <stdio.h>
#include <string.h>

#include <sys/types.h>           /* types for swap.h */
#include <sys/fcntl.h>
#include <sys/unistd.h>
#include <macros.h>
#include <sys/swap.h>            /* for swap information */

#include <sys/mnttab.h>          /* for mount table informatio */
#include <sys/statvfs.h>         /* for statvfs call */
#include <sys/immu.h>
#include "hostmibd.h"            

/* including isode and snmp header files */
#include "snmp.h"
#include "objects.h"

/* Include NetWare for Unix headers */

/* External Variables */

/* Local External Variables */
static int memsize=0;

typedef struct hrstorage_entry
{
  int index;
  OID type;
  char descr[80];
  int alloc_unit;
  int size;
  int used;
  int alloc_failure;
  struct hrstorage_entry * next;
} hrstorage_entry_t;

static hrstorage_entry_t *StorageTable=NULL;
static hrstorage_entry_t *StorageTableEnd=NULL;
static time_t StorageTableLastUpdate=0;

/* Forward References */

void  hrStorageInit(void);

static int hrStorageObj(OI oi, 
			struct type_SNMP_VarBind *v, 
			int offset);

static int hrMemoryObj(OI oi, 
		       struct type_SNMP_VarBind *v, 
		       int offset);

static void init_storage_table(void);    /* Initialize the storage table */

static void free_storage_table(void);    /* free storage table */
static int getmemsize(void);             /* get system ram size */


/* Defines */

#define hrStorageTypes                  21
#define hrStorageOther                  211
#define hrStorageRam                    212
#define hrStorageVirtualMemory          213
#define hrStorageFixedDisk              214
#define hrStorageRemovableDisk          215
#define hrStorageFloppyDisk             216

#define hrMemorySize                    22

#define hrStorageTable                  23
#define hrStorageIndex                  231
#define hrStorageType                   232
#define hrStorageDescr                  233
#define hrStorageAllocationUnits        234
#define hrStorageSize                   235
#define hrStorageUsed                   236
#define hrStorageAllocationFailures     237

void hrStorageInit(void) 
{
  OT ot;

  if(ot = text2obj("hrMemorySize"))
    ot->ot_getfnx = hrMemoryObj, 
    ot->ot_info = (caddr_t) hrMemorySize;

  if(ot = text2obj("hrStorageIndex"))
    ot->ot_getfnx = hrStorageObj, 
    ot->ot_info = (caddr_t) hrStorageIndex;

  if(ot = text2obj("hrStorageType"))
    ot->ot_getfnx = hrStorageObj, 
    ot->ot_info = (caddr_t) hrStorageType;

  if(ot = text2obj("hrStorageDescr"))
    ot->ot_getfnx = hrStorageObj, 
    ot->ot_info = (caddr_t) hrStorageDescr;

  if(ot = text2obj("hrStorageAllocationUnits"))
    ot->ot_getfnx = hrStorageObj, 
    ot->ot_info = (caddr_t) hrStorageAllocationUnits;

  if(ot = text2obj("hrStorageSize"))
    ot->ot_getfnx = hrStorageObj, 
    ot->ot_info = (caddr_t) hrStorageSize;

  if(ot = text2obj("hrStorageUsed"))
    ot->ot_getfnx = hrStorageObj, 
    ot->ot_info = (caddr_t) hrStorageUsed;

  if(ot = text2obj("hrStorageAllocationFailures"))
    ot->ot_getfnx = hrStorageObj, 
    ot->ot_info = (caddr_t) hrStorageAllocationFailures;

  init_storage_table();
}

static int hrMemoryObj(OI oi, 
		       struct type_SNMP_VarBind *v, 
		       int offset)
{
  int ifvar;
  OID oid = oi->oi_name;
  OT ot = oi->oi_type;

  ifvar = (int) ot->ot_info;

  switch(offset) 
    {
    case SMUX__PDUs_get__request:
      if(oid->oid_nelem != ot->ot_name->oid_nelem + 1
	 || oid->oid_elements[oid->oid_nelem - 1] != 0) 
	{
	  return error__status_noSuchName;
	}
      break;

    case SMUX__PDUs_get__next__request:
      if(oid->oid_nelem == ot->ot_name->oid_nelem)
	{
	  OID   new;

	  if((new = oid_extend(oid, 1)) == NULLOID)
	    return NOTOK;
	  new->oid_elements[new->oid_nelem - 1] = 0;
	  
	  if(v->name)
	    free_SNMP_ObjectName(v->name);

	  v->name = new;
	}
      else
	return NOTOK;
      break;

    default:
      return error__status_genErr;
    }

  switch(ifvar)
    {
    case hrMemorySize:  /* Same algorithm as memsize */
      if (memsize==0)
	memsize=getmemsize();
      return o_integer(oi, v, memsize);

    default:
      return error__status_noSuchName;
    }
}

static int hrStorageObj(OI oi, 
		       struct type_SNMP_VarBind *v, 
		       int offset)
{
  int   ifvar;
  OID oid = oi->oi_name;
  OT ot = oi->oi_type;

  OID   new;

  int Index;
  hrstorage_entry_t *tmp_ptr;  

  ifvar = (int) ot->ot_info;


  /* Refresh the storage table if it's too old */
  {
    time_t now;
    time(&now);
    if (difftime(now, StorageTableLastUpdate)>=HOSTMIBD_UPDATE_INTERVAL)
      {
	free_storage_table();
	init_storage_table();
      }
  }

  switch(offset) 
    {
    case SMUX__PDUs_get__request:

      if(oid->oid_nelem != ot->ot_name->oid_nelem + 1)
	
	return error__status_noSuchName;

      Index=oid->oid_elements[oid->oid_nelem-1];
      break;

    case SMUX__PDUs_get__next__request:
      if (oid->oid_nelem > ot->ot_name -> oid_nelem)
	{
	  int i=ot->ot_name->oid_nelem;
	  oid->oid_elements[i]++;
	  if (oid->oid_elements[i]>StorageTableEnd->index)
	    return NOTOK;
	}

      if(oid->oid_nelem == ot->ot_name->oid_nelem)
	{
	  Index = 0;

	  if((new = oid_extend(oid, 1)) == NULLOID)
	    return NOTOK;
	  
	  new->oid_elements[new->oid_nelem - 1] = Index;
	  
	  if(v->name)
	    free_SNMP_ObjectName(v->name);

	  v->name = new;

	  oid = new;  /* for try_again ... */
	}
      else
	{
	  Index=oid->oid_elements[ot->ot_name->oid_nelem];
	}
      break;
	      
    default:
      return error__status_genErr;
    }

  /* jump to the right structure */
  {
    int i;
    for (i=0, tmp_ptr=StorageTable;i<Index;i++,tmp_ptr=tmp_ptr->next)    
      ;
  }

  switch(ifvar)
    {
    case hrStorageIndex:
      return o_integer(oi, v, Index);

    case hrStorageType:
      return o_specific(oi, v, (caddr_t)tmp_ptr->type);

    case hrStorageDescr:
      return o_string(oi, v, tmp_ptr->descr, strlen(tmp_ptr->descr));

    case hrStorageAllocationUnits:
      return o_integer(oi, v, tmp_ptr->alloc_unit);

    case hrStorageSize:
      return o_integer(oi, v, tmp_ptr->size);

    case hrStorageUsed:
      return o_integer(oi, v, tmp_ptr->used);

    case hrStorageAllocationFailures:
      return o_integer(oi, v, tmp_ptr->alloc_failure);

    default:
      return error__status_noSuchName;
    }
}


/*****************************************************************************/

static void init_storage_table(void)                
{

  if (StorageTable==NULL)
    {
      StorageTable=(hrstorage_entry_t *)malloc(sizeof(hrstorage_entry_t));
      StorageTableEnd=StorageTable;
    }

  /* check RAM */
  StorageTable->index=0;
  StorageTable->type=text2oid("hrStorageRam");
  strcpy(StorageTable->descr, "System RAM");
  StorageTable->alloc_unit=1;      /* Allocation Unit is 1 byte.*/
  if (memsize==0)
    memsize=getmemsize();
  StorageTable->size=memsize*1024;
  snap_mets();				/* Update freemem */
  StorageTable->used=StorageTable->size-(int)(freemem->cooked*NBPP*100);
  StorageTable->alloc_failure=0;
  StorageTable->next=NULL;


  /* check swap space */
  {
#define MSFILES 16
    hrstorage_entry_t * tempentry;

    static struct SwapTab
      {
	int             swt_n;
	struct swapent  swt_ent[MSFILES];
      } swapTable;

    static int  swapInited = 0;
    int         ret;
    int         i;
    long        swapMax;
    long        swapFree;

    swapTable.swt_n = MSFILES;

    for (i = 0; i < swapTable.swt_n; i++)
      swapTable.swt_ent[i].ste_path = (char *) malloc (MAXPATHLEN);
    
    /* Get the swapTable using swapctl call */
    if ((ret = swapctl (SC_LIST, &swapTable)) <= 0)
      swapMax = swapFree = 99999;   /* Unknown */
    else
      {
	swapMax = swapFree = 0;

	/* Sum up the space */
	for (i = 0; i < ret; i++)
	  {
	    swapMax  += swapTable.swt_ent[i].ste_pages;
	    swapFree += swapTable.swt_ent[i].ste_free;
	  }
	
        swapMax  = swapMax*NBPP;   /* Convert page to bytes */
	swapFree = swapFree*NBPP;  /* Convert page to bytes */
      }

    tempentry=(hrstorage_entry_t *)malloc(sizeof(hrstorage_entry_t));

    tempentry->index=StorageTableEnd->index+1;
    tempentry->type=text2oid("hrStorageVirtualMemory");
    strcpy(tempentry->descr, "System Swap Memory");
    tempentry->alloc_unit=1;      /* Allocation Unit is 1 byte.*/
    tempentry->size=swapMax;
    tempentry->used=swapMax-swapFree;
    tempentry->alloc_failure=0;
    tempentry->next=NULL;
    StorageTableEnd->next=tempentry;
    StorageTableEnd=StorageTableEnd->next;

    for (i = 0; i < swapTable.swt_n; i++)  /* Don't forget to free */
      free(swapTable.swt_ent[i].ste_path);
  }

  /* Check all the mounted file systems */
  /* Same algorithm as df.c */
  {
    FILE *fi;
    struct mnttab Mp;
    struct statvfs Fs_info;
    int physblks;
    int tfree;
    int kbytes;
    int avail;
    hrstorage_entry_t * tempentry;

    fi = fopen(MNTTAB, "r");

    while (!getmntent(fi, &Mp)) 
      {
	statvfs(Mp.mnt_mountp, &Fs_info);

	/* Remote file systems shouldn't be here. */
	if (strcmp(Mp.mnt_fstype, "nfs")==0)
	  continue;
	if (strcmp(Mp.mnt_fstype, "dfs")==0)
	  continue;

	physblks = Fs_info.f_frsize / DEV_BSIZE;
	tfree    = Fs_info.f_bavail * physblks;
	kbytes = Fs_info.f_blocks * Fs_info.f_frsize / 1024;
	if (DEV_BSIZE <= 1024) 
	  {
	    avail = tfree / (1024 / DEV_BSIZE);
	  } 
	else 
	  {
	    avail = tfree * (DEV_BSIZE / 1024);
	  }
        
	
	tempentry=(hrstorage_entry_t *)malloc(sizeof(hrstorage_entry_t));

	tempentry->index=StorageTableEnd->index+1;

	if (strcmp(Mp.mnt_fstype, "bfs")==0)
	  tempentry->type=text2oid("hrStorageFixedDisk");
	else if (strcmp(Mp.mnt_fstype, "vxfs")==0)
	  tempentry->type=text2oid("hrStorageFixedDisk");
	else if (strcmp(Mp.mnt_fstype, "ufs")==0)
	  tempentry->type=text2oid("hrStorageFixedDisk");
	else if (strcmp(Mp.mnt_fstype, "sfs")==0)
	  tempentry->type=text2oid("hrStorageFixedDisk");
	else if (strcmp(Mp.mnt_fstype, "s5")==0)
	  tempentry->type=text2oid("hrStorageFixedDisk");
	else if (strcmp(Mp.mnt_fstype, "cdfs")==0)
	  tempentry->type=text2oid("hrStorageCompactDisk");
	else if (strcmp(Mp.mnt_fstype, "fdfs")==0)
	  tempentry->type=text2oid("hrStorageFloppyDisk");
	else
	  tempentry->type=text2oid("hrStorageOther");
	strcpy(tempentry->descr, Mp.mnt_special);
	tempentry->alloc_unit=1024;      /* Allocation Unit is 1K byte.*/
	tempentry->size=kbytes;
	tempentry->used=kbytes-avail;
	tempentry->alloc_failure=0;
	tempentry->next=NULL;
	StorageTableEnd->next=tempentry;
	StorageTableEnd=StorageTableEnd->next;
      }
    fclose(fi);
  }
  time(&StorageTableLastUpdate);
}

static int getmemsize(void)
{
  long num_pages;
  int memsize;

  /* Get memory size in units of _SC_PAGE_SIZE */

  num_pages = sysconf (_SC_TOTAL_MEMORY);

  /*
   * Return memsize in Kb.  Since we are using units of 1K, an int should be
   * sufficient.  This will need to be revised for Gemini 64.
   */

  memsize = (int) (num_pages * (sysconf (_SC_PAGE_SIZE) / 1024));
      
  return memsize;
} 

static void free_storage_table(void)
{
  hrstorage_entry_t * tmp_ptr;

  if (StorageTable!=NULL)
    {
      for (tmp_ptr=StorageTable;tmp_ptr!=StorageTableEnd;tmp_ptr=StorageTable)
	{
	  StorageTable=StorageTable->next;  /* Move the Storage table head */
	  free_oid(tmp_ptr->type);     /* free up OID */
	  free(tmp_ptr);               /* free up the struct*/
	}
      free_oid(tmp_ptr->type);
      free(tmp_ptr);                   /* free the last one */
      StorageTable=NULL;
      StorageTableEnd=NULL;
    }
}


