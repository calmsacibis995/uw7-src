#ident	"@(#)hrdevice.c	1.2"
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
** Source file:   hrdevice.c
**
** Description:   Module to realize hrDevice group of Host Resources MIB
**                (RFC 1514)
**
** Contained functions:
**                      hrDeviceInit();
****                    hrDeviceObj()
**
** Author:   Cheng Yang
**
** Date Created:  December 1993.
**
** COPYRIGHT STATEMENT: (C) COPYRIGHT 1993 by Novell, Inc.
**                      Property of Novell, Inc.  All Rights Reserved.
**
****************************************************************************/

/*
*	MODIFICATION HISTORY
*
* 	L000	townsend 11 Nov 97	
*	- made hrProcessor, hrNetwork,hrPrinter, and hrDiskStorage
*         sparse tables - in order that index entries refer back to
*         corresponding hrDevice table entries
*       - corrected diskette size calculation
*       - saved boot device index for hrSystemInitialLoadDevice
*
*/

/* including system include files */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mnttab.h>
#include <dirent.h>
/* including isode and snmp header files */
#include "snmp.h"
#include "objects.h"

#include "hostmibd.h"
/* External Variables */

/* Local External Variables */

/* Device Table Structures */
typedef struct hrdevice_entry
{
  int index;
  OID type;
  char descr[80];
  OID deviceid;
  int status;
  int errors;
  struct hrdevice_entry * next;
} hrdevice_entry_t;

static hrdevice_entry_t *DeviceTable=NULL;
static hrdevice_entry_t *DeviceTableEnd=NULL;
static time_t DeviceTableLastUpdate=0;

/* Processor Table Structures */
typedef struct hrprocessor_entry
{
  int index;
  OID frwid;
  struct hrprocessor_entry * next;
} hrprocessor_entry_t;

static hrprocessor_entry_t *ProcessorTable=NULL;
static hrprocessor_entry_t *ProcessorTableEnd=NULL;
static time_t ProcessorTableLastUpdate=0;

/* Network Table Structures */
typedef struct hrnetwork_entry
{
  int ifindex;
  struct hrnetwork_entry * next;
} hrnetwork_entry_t;

static hrnetwork_entry_t *NetworkTable=NULL;
static hrnetwork_entry_t *NetworkTableEnd=NULL;
static time_t NetworkTableLastUpdate=0;

/* Printer Table Structures */
typedef struct hrprinter_entry
{
  int index;
  int status;
  char detectederrorstate[1];
  struct hrprinter_entry * next;
} hrprinter_entry_t;

static hrprinter_entry_t *PrinterTable=NULL;
static hrprinter_entry_t *PrinterTableEnd=NULL;
static time_t PrinterTableLastUpdate=0;

/* Disk Storage Table Structures */
typedef struct hrdisk_entry
{
  int index;
  int access;
  int media;
  int removeble;
  int capacity;
  struct hrdisk_entry * next;
} hrdisk_entry_t;

static hrdisk_entry_t *DiskTable=NULL;
static hrdisk_entry_t *DiskTableEnd=NULL;
static time_t DiskTableLastUpdate=0;

/* Partition Table Structures */
typedef struct hrpartition_entry
{
  int index;
  char label[1024];
  char id[1024];
  int size;
  int fsindex;
  struct hrpartition_entry * next;
} hrpartition_entry_t;

static hrpartition_entry_t *PartitionTable=NULL;
static hrpartition_entry_t *PartitionTableEnd=NULL;
static time_t PartitionTableLastUpdate=0;

/* File System Structures */
typedef struct hrfs_entry
{
  int index;
  char mountpoint[1024];
  char remotemountpoint[1024];
  OID type;
  int access;
  int bootable;                /* only bfs bootable */
  int storageindex;
  int lastfullbackupdate;                /* unknown on UNIX */
  int lastpartialbackupdate;             /* unknown on UNIX */
  struct hrfs_entry * next;
} hrfs_entry_t;

static hrfs_entry_t *FSTable=NULL;
static hrfs_entry_t *FSTableEnd=NULL;
static time_t FSTableLastUpdate=0;

/* Forward References */

void  hrDeviceInit(void);

static void get_device_table(void);
static void free_device_table(void);

static void get_processor_table(void);
static void free_processor_table(void);

static void get_network_table(void);
static void free_network_table(void);

static void get_printer_table(void);
static void free_printer_table(void);

static void get_disk_table(void);
static void free_disk_table(void);

static void get_partition_table(void);
static void free_partition_table(void);

static void get_file_system_table(void);
static void free_file_system_table(void);

static int hrDeviceObj(OI oi, 
		       struct type_SNMP_VarBind *v, 
		       int offset);

static int hrProcessorObj(OI oi, 
			  struct type_SNMP_VarBind *v, 
			  int offset);

static int hrNetworkObj(OI oi, 
			struct type_SNMP_VarBind *v, 
			int offset);

static int hrPrinterObj(OI oi, 
			struct type_SNMP_VarBind *v, 
			int offset);

static int hrDiskStorageObj(OI oi, 
			    struct type_SNMP_VarBind *v, 
			    int offset);

static int hrPartitionObj(OI oi, 
			  struct type_SNMP_VarBind *v, 
			  int offset);

static int hrFSObj(OI oi, 
		   struct type_SNMP_VarBind *v, 
		   int offset);

/* L000 vvv */
static int processorTablePos;
static int networkTablePos;
static int printerTablePos;
static int diskTablePos;
int bootDiskIndex = 1;
/* L000 ^^^ */

/* Defines */

#define hrDeviceTypes                  31
#define hrDeviceOther                  311
#define hrDeviceUnknown                312
#define hrDeviceProcessor              313
#define hrDeviceNetwork                314
#define hrDevicePrinter                315
#define hrDeviceDiskStorage            316
#define hrDeviceVideo                  317
#define hrDeviceAudio                  318
#define hrDeviceCoprocessor            319
#define hrDeviceKeyboard               3110
#define hrDeviceModem                  3111
#define hrDeviceParallelPort           3112
#define hrDevicePointing               3113
#define hrDeviceSerialPort             3114
#define hrDeviceTape                   3115
#define hrDeviceClock                  3116
#define hrDeviceVolatileMemory         3117
#define hrDeviceNonVolatileMemory      3118

#define hrDeviceTable                  32
#define hrDeviceIndex                  321
#define hrDeviceType                   322
#define hrDeviceDescr                  323
#define hrDeviceID                     324
#define hrDeviceStatus                 325
#define hrDeviceErrors                 326

#define hrProcessorTable               33
#define hrProcessorFrwID               331
#define hrProcessorLoad                332

#define hrNetworkTable                 34
#define hrNetworkIfIndex               341

#define hrPrinterTable                 35
#define hrPrinterStatus                351
#define hrPrinterDetectedErrorState    352

#define hrDiskStorageTable             36
#define hrDiskStorageAccess            361
#define hrDiskStorageMedia             362
#define hrDiskStorageRemoveble         363
#define hrDiskStorageCapacity          364

#define hrPartitionTable               37
#define hrPartitionIndex               371
#define hrPartitionLabel               372
#define hrPartitionID                  373
#define hrPartitionSize                374
#define hrPartitionFSIndex             375

#define hrFSTable                      38
#define hrFSIndex                      381
#define hrFSMountPoint                 382
#define hrFSRemoteMountPoint           383
#define hrFSType                       384
#define hrFSAccess                     385
#define hrFSBootable                   386
#define hrFSStorageIndex               387
#define hrFSLastFullBackupDate         388
#define hrFSLastPartialBackupDate      389

#define hrFSTypes                      39
#define hrFSOther                      391
#define hrFSUnknown                    392
#define hrFSBerkeleyFFS                393
#define hrFSSys5FS                     394   
#define hrFSFat                        395   /* DOS */
#define hrFSHPFS                       396   /* OS/2 */
#define hrFSHFS                        397   /* Mac Hierarchical File System */
#define hrFSMFS                        398   /* Macintosh  */
#define hrFSNTFS                       399   /* Windows NT */
#define hrFSVNode                      3910
#define hrFSJournaled                  3911
#define hrFSiso9660                    3912  /* CD ROM */
#define hrFSRockRidge                  3913  /* ? */
#define hrFSNFS                        3914 
#define hrFSNetware                    3915
#define hrFSAFS                        3916
#define hrFSDFS                        3917  /* OSF DCE Distributred FS */
#define hrFSAppleshare                 3918 
#define hrFSRFS                        3919
#define hrFSDGCFS                      3920  /* Data General FS */
#define hrFSBFS                        3921  /* SVR4/UnixWare Boot FS */


void hrDeviceInit(void) 
{
  OT ot;

  if(ot = text2obj("hrDeviceIndex"))
    ot->ot_getfnx = hrDeviceObj, 
    ot->ot_info = (caddr_t) hrDeviceIndex;

  if(ot = text2obj("hrDeviceType"))
    ot->ot_getfnx = hrDeviceObj, 
    ot->ot_info = (caddr_t) hrDeviceType;

  if(ot = text2obj("hrDeviceDescr"))
    ot->ot_getfnx = hrDeviceObj, 
    ot->ot_info = (caddr_t) hrDeviceDescr;

  if(ot = text2obj("hrDeviceID"))
    ot->ot_getfnx = hrDeviceObj, 
    ot->ot_info = (caddr_t) hrDeviceID;

  if(ot = text2obj("hrDeviceStatus"))
    ot->ot_getfnx = hrDeviceObj, 
    ot->ot_info = (caddr_t) hrDeviceStatus;

  if(ot = text2obj("hrDeviceErrors"))
    ot->ot_getfnx = hrDeviceObj, 
    ot->ot_info = (caddr_t) hrDeviceErrors;

  if(ot = text2obj("hrProcessorFrwID"))
    ot->ot_getfnx = hrProcessorObj, 
    ot->ot_info = (caddr_t) hrProcessorFrwID;

  if(ot = text2obj("hrProcessorLoad"))
    ot->ot_getfnx = hrProcessorObj, 
    ot->ot_info = (caddr_t) hrProcessorLoad;

  if(ot = text2obj("hrNetworkIfIndex"))
    ot->ot_getfnx = hrNetworkObj, 
    ot->ot_info = (caddr_t) hrNetworkIfIndex;

  if(ot = text2obj("hrPrinterStatus"))
    ot->ot_getfnx = hrPrinterObj, 
    ot->ot_info = (caddr_t) hrPrinterStatus;

  if(ot = text2obj("hrPrinterDetectedErrorState"))
    ot->ot_getfnx = hrPrinterObj, 
    ot->ot_info = (caddr_t) hrPrinterDetectedErrorState;

  if(ot = text2obj("hrDiskStorageAccess"))
    ot->ot_getfnx = hrDiskStorageObj, 
    ot->ot_info = (caddr_t) hrDiskStorageAccess;

  if(ot = text2obj("hrDiskStorageMedia"))
    ot->ot_getfnx = hrDiskStorageObj, 
    ot->ot_info = (caddr_t) hrDiskStorageMedia;

  if(ot = text2obj("hrDiskStorageRemoveble"))
    ot->ot_getfnx = hrDiskStorageObj, 
    ot->ot_info = (caddr_t) hrDiskStorageRemoveble;

  if(ot = text2obj("hrDiskStorageCapacity"))
    ot->ot_getfnx = hrDiskStorageObj, 
    ot->ot_info = (caddr_t) hrDiskStorageCapacity;

  if(ot = text2obj("hrPartitionIndex"))
    ot->ot_getfnx = hrPartitionObj, 
    ot->ot_info = (caddr_t) hrPartitionIndex;

  if(ot = text2obj("hrPartitionLabel"))
    ot->ot_getfnx = hrPartitionObj, 
    ot->ot_info = (caddr_t) hrPartitionLabel;

  if(ot = text2obj("hrPartitionID"))
    ot->ot_getfnx = hrPartitionObj, 
    ot->ot_info = (caddr_t) hrPartitionID;

  if(ot = text2obj("hrPartitionSize"))
    ot->ot_getfnx = hrPartitionObj, 
    ot->ot_info = (caddr_t) hrPartitionSize;

  if(ot = text2obj("hrPartitionFSIndex"))
    ot->ot_getfnx = hrPartitionObj, 
    ot->ot_info = (caddr_t) hrPartitionFSIndex;

  if(ot = text2obj("hrFSIndex"))
    ot->ot_getfnx = hrFSObj, 
    ot->ot_info = (caddr_t) hrFSIndex;

  if(ot = text2obj("hrFSMountPoint"))
    ot->ot_getfnx = hrFSObj, 
    ot->ot_info = (caddr_t) hrFSMountPoint;

  if(ot = text2obj("hrFSRemoteMountPoint"))
    ot->ot_getfnx = hrFSObj, 
    ot->ot_info = (caddr_t) hrFSRemoteMountPoint;

  if(ot = text2obj("hrFSType"))
    ot->ot_getfnx = hrFSObj, 
    ot->ot_info = (caddr_t) hrFSType;

  if(ot = text2obj("hrFSAccess"))
    ot->ot_getfnx = hrFSObj, 
    ot->ot_info = (caddr_t) hrFSAccess;

  if(ot = text2obj("hrFSBootable"))
    ot->ot_getfnx = hrFSObj, 
    ot->ot_info = (caddr_t) hrFSBootable;

  if(ot = text2obj("hrFSStorageIndex"))
    ot->ot_getfnx = hrFSObj, 
    ot->ot_info = (caddr_t) hrFSStorageIndex;

  if(ot = text2obj("hrFSLastFullBackupDate"))
    ot->ot_getfnx = hrFSObj, 
    ot->ot_info = (caddr_t) hrFSLastFullBackupDate;

  if(ot = text2obj("hrFSLastPartialBackupDate"))
    ot->ot_getfnx = hrFSObj, 
    ot->ot_info = (caddr_t) hrFSLastPartialBackupDate;

  get_device_table();
  get_processor_table();
  get_network_table();
  get_printer_table();
  get_disk_table();
  get_partition_table();
  get_file_system_table();
}

/*****************************************************************************/
static int hrDeviceObj(OI oi, 
		         struct type_SNMP_VarBind *v, 
		       int offset)
{
  int   ifvar;
  OID oid = oi->oi_name;
  OT ot = oi->oi_type;

  OID   new;

  int Index;
  hrdevice_entry_t *tmp_ptr;  

  ifvar = (int) ot->ot_info;

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
	  if (oid->oid_elements[i]>DeviceTableEnd->index)
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
    for (i=0, tmp_ptr=DeviceTable;i<Index;i++,tmp_ptr=tmp_ptr->next)
      ;
  }
  switch(ifvar)
    {
    case hrDeviceIndex:
      return o_integer(oi, v, Index);

    case hrDeviceType:
      return o_specific(oi, v, (caddr_t)tmp_ptr->type);

    case hrDeviceDescr:
      return o_string(oi, v, tmp_ptr->descr, strlen(tmp_ptr->descr));

    case hrDeviceID:
      return o_specific(oi, v, (caddr_t)tmp_ptr->deviceid);

    case hrDeviceStatus:
      return o_integer(oi, v, tmp_ptr->status);

    case hrDeviceErrors:
      return o_integer(oi, v, tmp_ptr->errors);

    default:
      return error__status_noSuchName;
    }
}


/*****************************************************************************/

static int hrProcessorObj(OI oi, 
			    struct type_SNMP_VarBind *v, 
			  int offset)
{
  int   ifvar;
  OID oid = oi->oi_name;
  OT ot = oi->oi_type;

  OID   new;

  int Index;
  hrprocessor_entry_t *tmp_ptr;  

  ifvar = (int) ot->ot_info;

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
	  if (oid->oid_elements[i]>ProcessorTableEnd->index)
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
    for (i=0, tmp_ptr=ProcessorTable;i<Index;i++,tmp_ptr=tmp_ptr->next)
      ;
  }
   /* check for valid entry - L000 vvv */
  while(tmp_ptr->index == -1 ) {
  	switch(offset) 
    {
    case SMUX__PDUs_get__request:
    	return error__status_noSuchName;
    	
    case SMUX__PDUs_get__next__request:
    	oid->oid_elements[ot->ot_name->oid_nelem]++;
		if (oid->oid_elements[ot->ot_name->oid_nelem]>ProcessorTableEnd->index)
	    	return NOTOK;
	    tmp_ptr = tmp_ptr->next;
	    break;
	default:
		break;
    }
   }
    /* check for valid entry - L000 ^^^ */
  switch(ifvar)
    {
    case hrProcessorFrwID:
      return o_specific(oi, v, (caddr_t)tmp_ptr->frwid);

    case hrProcessorLoad:
      {
	time_t now;
	time(&now);
	if (difftime(now, ProcessorTableLastUpdate) > 60)
	  {
	    ProcessorTableLastUpdate=now;
	    snap_mets();
	  }
	return o_integer(oi, v, 100-(int)(idl_time[Index].cooked));
      }
    default:
      return error__status_noSuchName;
    }
}

/*****************************************************************************/

static int hrNetworkObj(OI oi, 
			struct type_SNMP_VarBind *v, 
		       int offset)
{
  int   ifvar;
  OID oid = oi->oi_name;
  OT ot = oi->oi_type;

  OID   new;

  int Index;
  hrnetwork_entry_t *tmp_ptr;  

  ifvar = (int) ot->ot_info;

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
	  if (oid->oid_elements[i]>NetworkTableEnd->ifindex)
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
    for (i=0, tmp_ptr=NetworkTable;i<Index;i++,tmp_ptr=tmp_ptr->next)
      ;
  }
   /* check for valid entry - L000 vvv */
  while(tmp_ptr->ifindex == -1 ) {
  	switch(offset) 
    {
    case SMUX__PDUs_get__request:
    	return error__status_noSuchName;
    	
    case SMUX__PDUs_get__next__request:
    	oid->oid_elements[ot->ot_name->oid_nelem]++;
		if (oid->oid_elements[ot->ot_name->oid_nelem]>NetworkTableEnd->ifindex)
	    	return NOTOK;
	    tmp_ptr = tmp_ptr->next;
	    break;
	default:
		break;
    }
   }
    /* check for valid entry - L000 ^^^ */
  switch(ifvar)
    {
    case hrNetworkIfIndex:
      return o_integer(oi, v, tmp_ptr->ifindex);

    default:
      return error__status_noSuchName;
    }
}

/*****************************************************************************/

static int hrPrinterObj(OI oi, 
			  struct type_SNMP_VarBind *v, 
			int offset)
{
  int   ifvar;
    OID oid = oi->oi_name;
    OT ot = oi->oi_type;

  OID   new;

  int Index;
  hrprinter_entry_t *tmp_ptr;  

  ifvar = (int) ot->ot_info;

  if (PrinterTable==NULL)   /* if there is no printer, return nosuchname */
    return NOTOK;

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
	  if (oid->oid_elements[i]>PrinterTableEnd->index)
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
    for (i=0, tmp_ptr=PrinterTable;i<Index;i++,tmp_ptr=tmp_ptr->next)
      ;
  }
  
 /* check for valid entry - L000 vvv */
  while(tmp_ptr->index == -1 ) {
  	switch(offset) 
    {
    case SMUX__PDUs_get__request:
    	return error__status_noSuchName;
    	
    case SMUX__PDUs_get__next__request:
    	oid->oid_elements[ot->ot_name->oid_nelem]++;
		if (oid->oid_elements[ot->ot_name->oid_nelem]>PrinterTableEnd->index)
	    	return NOTOK;
	    tmp_ptr = tmp_ptr->next;
	    break;
	default:
		break;
    }
   }
    /* check for valid entry - L000 ^^^ */
  switch(ifvar)
    {
    case hrPrinterStatus:
      return o_integer(oi, v, tmp_ptr->status);

    case hrPrinterDetectedErrorState:
      return o_string(oi, v, tmp_ptr->detectederrorstate, 1);

    default:
      return error__status_noSuchName;
    }
}


/*****************************************************************************/

static int hrDiskStorageObj(OI oi, 
			      struct type_SNMP_VarBind *v, 
			    int offset)
{
  int   ifvar;
  OID oid = oi->oi_name;
  OT ot = oi->oi_type;

  OID   new;

  int Index;
  hrdisk_entry_t *tmp_ptr;  

  ifvar = (int) ot->ot_info;

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
	  if (oid->oid_elements[i]>DiskTableEnd->index)
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
    for (i=0, tmp_ptr=DiskTable;i<Index;i++,tmp_ptr=tmp_ptr->next)
      ;
  }
  /* check for valid entry - L000 vvv */
  while(tmp_ptr->index == -1 ) {
  	switch(offset) 
    {
    case SMUX__PDUs_get__request:
    	return error__status_noSuchName;
    	
    case SMUX__PDUs_get__next__request:
    	oid->oid_elements[ot->ot_name->oid_nelem]++;
		if (oid->oid_elements[ot->ot_name->oid_nelem]>DiskTableEnd->index)
	    	return NOTOK;
	    tmp_ptr = tmp_ptr->next;
	    break;
	default:
		break;
    }
   }
    /* check for valid entry - L000 ^^^ */
  switch(ifvar)
    {
    case hrDiskStorageAccess:
      return o_integer(oi, v, tmp_ptr->access);

    case hrDiskStorageMedia:
      return o_integer(oi, v, tmp_ptr->media);

    case hrDiskStorageRemoveble:
      return o_integer(oi, v, tmp_ptr->removeble);

    case hrDiskStorageCapacity:
      return o_integer(oi, v, tmp_ptr->capacity);

    default:
      return error__status_noSuchName;
    }
}


/*****************************************************************************/

static int hrPartitionObj(OI oi, 
			    struct type_SNMP_VarBind *v, 
			  int offset)
{
  int   ifvar;
  OID oid = oi->oi_name;
  OT ot = oi->oi_type;

  OID   new;

  int Index;
  hrpartition_entry_t *tmp_ptr;  

  ifvar = (int) ot->ot_info;

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
	  if (oid->oid_elements[i]>PartitionTableEnd->index)
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
    for (i=0, tmp_ptr=PartitionTable;i<Index;i++,tmp_ptr=tmp_ptr->next)
      ;
  }
  switch(ifvar)
    {
    case hrPartitionIndex:
      return o_integer(oi, v, Index);

    case hrPartitionLabel:
      return o_string(oi, v, (char *)tmp_ptr->label, 
		      strlen((char *)(tmp_ptr->label)));

    case hrPartitionID:
      return o_string(oi, v, (char *)tmp_ptr->id, strlen(tmp_ptr->id));

    case hrPartitionSize:
      return o_integer(oi, v, tmp_ptr->size);

    case hrPartitionFSIndex:
      return o_integer(oi, v, tmp_ptr->fsindex);

    default:
      return error__status_noSuchName;
    }
}

/*****************************************************************************/

static int hrFSObj(OI oi, 
		   struct type_SNMP_VarBind *v, 
		   int offset)
{
  int   ifvar;
  OID oid = oi->oi_name;
  OT ot = oi->oi_type;

  OID   new;

  int Index;
  hrfs_entry_t *tmp_ptr;  
  
  /* Refresh the file system table if it's too old */
  {
    time_t now;
    time(&now);
    if (difftime(now, FSTableLastUpdate)>=HOSTMIBD_UPDATE_INTERVAL)
      {
	free_file_system_table();
	get_file_system_table();
      }
  }

  ifvar = (int) ot->ot_info;

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
	  if (oid->oid_elements[i]>FSTableEnd->index)
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
    for (i=0, tmp_ptr=FSTable;i<Index;i++,tmp_ptr=tmp_ptr->next)
      ;
  }
  switch(ifvar)
    {
    case hrFSIndex:
      return o_integer(oi, v, Index);

    case hrFSMountPoint:
      return o_string(oi, v, tmp_ptr->mountpoint, 
		      strlen(tmp_ptr->mountpoint));

    case hrFSRemoteMountPoint:
      return o_string(oi, v, tmp_ptr->remotemountpoint, 
		      strlen(tmp_ptr->remotemountpoint));

    case hrFSType:
      return o_specific(oi, v, (caddr_t)tmp_ptr->type);

    case hrFSAccess:
      return o_integer(oi, v, tmp_ptr->access);

    case hrFSBootable:
      return o_integer(oi, v, tmp_ptr->bootable);

    case hrFSStorageIndex:
      return o_integer(oi, v, tmp_ptr->storageindex);

    case hrFSLastFullBackupDate:        
    case hrFSLastPartialBackupDate:
      {
	char date[8]={0x00,0x00,0x01,0x01,0x00,0x00,0x00,0x00};
	return o_string(oi, v, date, 8);
      }

    default:
      return error__status_noSuchName;
    }
}

static void get_device_table(void)
{
  hrdevice_entry_t * tempentry=NULL;
   int count = 0;	/* L000 */
  int cpus = 0;		/* L000 */
  int i;			/* L000 */

  /* Processors */
  processorTablePos = count;	/* L000 */
  cpus=get_ncpu();
  for (i=0; i<cpus; i++)
    {
      if (i==0) /* this is the first one */
	{
	  tempentry= (hrdevice_entry_t *)malloc
	    (sizeof(hrdevice_entry_t));
	  DeviceTable=tempentry;
	  DeviceTableEnd=tempentry;
	}
      else
	{
	  tempentry= (hrdevice_entry_t *)malloc
	    (sizeof(hrdevice_entry_t));
	  DeviceTableEnd->next=tempentry;
	  DeviceTableEnd=tempentry;
	}
	tempentry->index=i;
  	tempentry->type=text2oid("hrDeviceProcessor");
  	strcpy(tempentry->descr, "Intel i386 Compatible");
  	tempentry->deviceid=text2oid("0.0");
  	tempentry->status=2; /* running */
  	tempentry->errors=0;
  	tempentry->next=NULL;
  	count++;	/* L000 */
    }
    
  /* Network */
  networkTablePos = count;	/* L000 */
  {
    FILE * iifp;
    char buf[1024];
    if (iifp = fopen("/etc/confnet.d/inet/interface", "r"))
    {
      while ((fgets(buf, sizeof(buf), iifp)) != NULL) 
	{
	  /* skip the comment and empty lines */
	if (buf[0] == '#' || buf[0] == '\0')
	  continue;

	tempentry=(hrdevice_entry_t *)malloc(sizeof(hrdevice_entry_t));
	tempentry->index=DeviceTableEnd->index+1;
	tempentry->type=text2oid("hrDeviceNetwork");

	/* Generate a suitable description string */
	if( strncmp( buf,"lo:",strlen("lo:")) == 0 )
	 strcpy(tempentry->descr, "Network Loopback");
	else
	 strcpy(tempentry->descr, "Network Interface Card");

	tempentry->deviceid=text2oid("0.0");
	tempentry->status=2;
	tempentry->errors=0;
	tempentry->next=NULL;
	count++;	/* L000 */
	DeviceTableEnd->next=tempentry;
	DeviceTableEnd=DeviceTableEnd->next;
        }
      fclose( iifp );
    }
  }

  printerTablePos = count;	/* L000 */
  /* Printers */
  {
    DIR *dp;
    struct dirent *dirp;

    if ((dp=opendir("/etc/lp/printers"))!= NULL)
      while ((dirp= readdir(dp))!=NULL)
	{
	  if (dirp->d_name[0]=='.')
	    continue;
	  else
	    {
	      FILE * fpin;
	      char cmdstr[1024];
	      char output[1024];
	      /* get the path of the configuration file */
	      strcpy(cmdstr, "/bin/grep Remote: /etc/lp/printers/");
	      strcat(cmdstr, dirp->d_name);
	      strcat(cmdstr, "/configuration");

	      /* do a grep Remote */
	      fpin=popen(cmdstr, "r");
	      
	      /* if there is no output, this is a local printer. */
	      if (fgets(output, 1024, fpin)==NULL)
		{
		  tempentry=(hrdevice_entry_t *)
		    malloc(sizeof(hrdevice_entry_t));
		  tempentry->index=DeviceTableEnd->index+1;
		  tempentry->type=text2oid("hrDevicePrinter");
		  strcpy(tempentry->descr, "Printer");
		  tempentry->deviceid=text2oid("0.0");
		  tempentry->status=1;  /* unknown */
		  tempentry->errors=0;
		  count++;	/* L000 */
		  tempentry->next=NULL;
		  DeviceTableEnd->next=tempentry;
		  DeviceTableEnd=DeviceTableEnd->next;
		}
	      /* if there is output, this is a remote printer, ignore */
	      pclose(fpin);
	    }
	}
    closedir(dp);
  }
  diskTablePos = count;	/* L000 */

  /* FLOPPY DISKETTE Storage */
  {
    FILE * fpin;
    char cmdstr[1024];
    char output[1024];
    sprintf(cmdstr, "%s type=\"diskette\"", GETDEVCMD);
    fpin=popen(cmdstr, "r");
    for(;;)
      {
	if(fgets(output, 1024, fpin)==NULL)
	  break;
	tempentry=(hrdevice_entry_t *)malloc(sizeof(hrdevice_entry_t));
	tempentry->index=DeviceTableEnd->index+1;
	tempentry->type=text2oid("hrDeviceDiskStorage");
	output[strlen(output)-1]=NULL;  /* take out the last garbage char */
	sprintf(tempentry->descr, "Floppy Diskette (%s)", output);
	tempentry->deviceid=text2oid("0.0");
	tempentry->status=2;
	tempentry->errors=0;
	tempentry->next=NULL;
	count++;	/* L000 */
	DeviceTableEnd->next=tempentry;
	DeviceTableEnd=DeviceTableEnd->next;  
      }
    pclose(fpin);
  }

  /* CD-ROM Storage */
  {
    FILE * fpin;
    char cmdstr[1024];
    char output[1024];
    sprintf(cmdstr, "%s type=\"cdrom\"", GETDEVCMD);
    fpin=popen(cmdstr, "r");
    for(;;)
      {
	if(fgets(output, 1024, fpin)==NULL)
	  break;
	tempentry=(hrdevice_entry_t *)malloc(sizeof(hrdevice_entry_t));
	tempentry->index=DeviceTableEnd->index+1;
	tempentry->type=text2oid("hrDeviceDiskStorage");
	output[strlen(output)-1]=NULL;  /* take out the last garbage char */
	sprintf(tempentry->descr, "CD ROM (%s)", output);
	tempentry->deviceid=text2oid("0.0");
	tempentry->status=2;
	tempentry->errors=0;
	tempentry->next=NULL;
	count++;	/* L000 */
	DeviceTableEnd->next=tempentry;
	DeviceTableEnd=DeviceTableEnd->next;  
      }
    pclose(fpin);
  }

  /* Hard Disk Storage */
  {
    FILE * fpin;
    char cmdstr[1024];
    char output[1024];
    sprintf(cmdstr, "%s type=\"disk\"", GETDEVCMD);
    fpin=popen(cmdstr, "r");
    for(;;)
      {
	if(fgets(output, 1024, fpin)==NULL)
	  break;
	if(strcmp("disk1", output) != 0)
		bootDiskIndex = count;	/* L000 */
	tempentry=(hrdevice_entry_t *)malloc(sizeof(hrdevice_entry_t));
	tempentry->index=DeviceTableEnd->index+1;
	tempentry->type=text2oid("hrDeviceDiskStorage");
	output[strlen(output)-1]=NULL;  /* take out the last garbage char */
	sprintf(tempentry->descr, "Hard Disk (%s)", output);
	tempentry->deviceid=text2oid("0.0");
	tempentry->status=2;
	tempentry->errors=0;
	tempentry->next=NULL;
	count++;	/* L000 */
	DeviceTableEnd->next=tempentry;
	DeviceTableEnd=DeviceTableEnd->next;  
      }
    pclose(fpin);
  }

  /* Video */
  tempentry=(hrdevice_entry_t *)malloc(sizeof(hrdevice_entry_t));
  tempentry->index=DeviceTableEnd->index+1;
  tempentry->type=text2oid("hrDeviceVideo");
  strcpy(tempentry->descr, "Video Subsystem");
  tempentry->deviceid=text2oid("0.0");
  tempentry->status=2;
  tempentry->errors=0;
  tempentry->next=NULL;
  DeviceTableEnd->next=tempentry;
  DeviceTableEnd=DeviceTableEnd->next;  

  /* Audio */
  /* This in not very good but, since we do not have an api into the audio */
  /* system, check its config file. If it is not present then we have no */
  /* audio system. If it is present then we may have an audio card */
  {
    FILE * acfp;
    char buf[1024];
    char *mod_name;
    if (acfp = fopen("/usr/lib/audio/audioconfig/audioconfig.cfg", "r"))
    {
      while ((fgets(buf, sizeof(buf), acfp)) != NULL) 
      {
	buf[strlen(buf)-1]=NULL;  /* take out the last garbage char */
        if( (mod_name = strstr( buf, "model=")) == NULL )
	  continue;

	/* Skip over "model=" */
	mod_name += strlen("model=");

        tempentry=(hrdevice_entry_t *)malloc(sizeof(hrdevice_entry_t));
        tempentry->index=DeviceTableEnd->index+1;
        tempentry->type=text2oid("hrDeviceAudio");

	/* Build up device name string */
        sprintf(tempentry->descr, "Audio Device (%s)", mod_name);

        tempentry->deviceid=text2oid("0.0");
        tempentry->status=2;
        tempentry->errors=0;
        tempentry->next=NULL;
        DeviceTableEnd->next=tempentry;
        DeviceTableEnd=DeviceTableEnd->next; 
      }
      fclose( acfp );
    }
  }

  /* Coprocessor */
  tempentry=(hrdevice_entry_t *)malloc(sizeof(hrdevice_entry_t));
  tempentry->index=DeviceTableEnd->index+1;
  tempentry->type=text2oid("hrDeviceCoprocessor");
  strcpy(tempentry->descr, "Coprocessor");
  tempentry->deviceid=text2oid("0.0");
  tempentry->status=2;
  tempentry->errors=0;
  tempentry->next=NULL;
  DeviceTableEnd->next=tempentry;
  DeviceTableEnd=DeviceTableEnd->next;

  /* Keyboard */
  tempentry=(hrdevice_entry_t *)malloc(sizeof(hrdevice_entry_t));
  tempentry->index=DeviceTableEnd->index+1;
  tempentry->type=text2oid("hrDeviceKeyboard");
  strcpy(tempentry->descr, "Keyboard");
  tempentry->deviceid=text2oid("0.0");
  tempentry->status=2;
  tempentry->errors=0;
  tempentry->next=NULL;
  DeviceTableEnd->next=tempentry;
  DeviceTableEnd=DeviceTableEnd->next;

  /* Modem */
  /* 
  tempentry=(hrdevice_entry_t *)malloc(sizeof(hrdevice_entry_t));
  tempentry->index=DeviceTableEnd->index+1;
  tempentry->type=text2oid("hrDeviceModem");
  strcpy(tempentry->descr, "Modem");
  tempentry->deviceid=text2oid("0.0");
  tempentry->status=2;
  tempentry->errors=0;
  tempentry->next=NULL;
  DeviceTableEnd->next=tempentry;
  DeviceTableEnd=DeviceTableEnd->next;
  */
  /* Parallel Port */
  tempentry=(hrdevice_entry_t *)malloc(sizeof(hrdevice_entry_t));
  tempentry->index=DeviceTableEnd->index+1;
  tempentry->type=text2oid("hrDeviceParallelPort");
  strcpy(tempentry->descr, "Parallel Port");
  tempentry->deviceid=text2oid("0.0");
  tempentry->status=2;
  tempentry->errors=0;
  tempentry->next=NULL;
  DeviceTableEnd->next=tempentry;
  DeviceTableEnd=DeviceTableEnd->next;

  /* Pointing Port*/  /* Code derived from mouseadmin.c */
  {
    FILE *tabf;
    char dname[64], mname[64];

    if ((tabf = fopen("/usr/lib/mousetab", "r")) != NULL)
      {
	while (fscanf(tabf, "%s %s", dname, mname) > 0) 
	  {
	    tempentry=(hrdevice_entry_t *)malloc(sizeof(hrdevice_entry_t));
	    tempentry->index=DeviceTableEnd->index+1;
	    tempentry->type=text2oid("hrDevicePointing");
	    strcpy(tempentry->descr, "Pointing Device");
	    tempentry->deviceid=text2oid("0.0");
	    tempentry->status=2;
	    tempentry->errors=0;
	    tempentry->next=NULL;
	    DeviceTableEnd->next=tempentry;
	    DeviceTableEnd=DeviceTableEnd->next;

	    if (strncmp(mname, "m320", 4) == 0)
	      strcat(tempentry->descr, " (PS2 Mouse)");
	    else 
	      if (strncmp(mname, "bmse", 4) == 0)
		strcat(tempentry->descr, " (BUS Mouse)");
	      else
		strcat(tempentry->descr, " (Serial Mouse)");
	  }
      }
    fclose(tabf);
  }

  /* Serial Port */
  tempentry=(hrdevice_entry_t *)malloc(sizeof(hrdevice_entry_t));
  tempentry->index=DeviceTableEnd->index+1;
  tempentry->type=text2oid("hrDeviceSerialPort");
  strcpy(tempentry->descr, "Serial Port");
  tempentry->deviceid=text2oid("0.0");
  tempentry->status=2;
  tempentry->errors=0;
  tempentry->next=NULL;
  DeviceTableEnd->next=tempentry;
  DeviceTableEnd=DeviceTableEnd->next;

  /* Tape */
  {
    FILE * fpin;
    char cmdstr[1024];
    char output[1024];
    sprintf(cmdstr, "%s type=\"qtape\"", GETDEVCMD);
    fpin=popen(cmdstr, "r");
    for(;;)
      {
	if(fgets(output, 1024, fpin)==NULL)
	  break;
	tempentry=(hrdevice_entry_t *)malloc(sizeof(hrdevice_entry_t));
	tempentry->index=DeviceTableEnd->index+1;
	tempentry->type=text2oid("hrDeviceTape");
	output[strlen(output)-1]=NULL;  /* take out the last garbage char */
	sprintf(tempentry->descr, "Tape Drive (%s)", output);
	tempentry->deviceid=text2oid("0.0");
	tempentry->status=2;
	tempentry->errors=0;
	tempentry->next=NULL;
	DeviceTableEnd->next=tempentry;
	DeviceTableEnd=DeviceTableEnd->next;  
      }
    pclose(fpin);
  }

  /* Clock */
  tempentry=(hrdevice_entry_t *)malloc(sizeof(hrdevice_entry_t));
  tempentry->index=DeviceTableEnd->index+1;
  tempentry->type=text2oid("hrDeviceClock");
  strcpy(tempentry->descr, "System Clock");
  tempentry->deviceid=text2oid("0.0");
  tempentry->status=2;
  tempentry->errors=0;
  tempentry->next=NULL;
  DeviceTableEnd->next=tempentry;
  DeviceTableEnd=DeviceTableEnd->next;

  /* Volatile Memory */
  tempentry=(hrdevice_entry_t *)malloc(sizeof(hrdevice_entry_t));
  tempentry->index=DeviceTableEnd->index+1;
  tempentry->type=text2oid("hrDeviceVolatileMemory");
  strcpy(tempentry->descr, "System RAM");
  tempentry->deviceid=text2oid("0.0");
  tempentry->status=2;
  tempentry->errors=0;
  tempentry->next=NULL;
  DeviceTableEnd->next=tempentry;
  DeviceTableEnd=DeviceTableEnd->next;

  /* Non Volatile Memory */
  tempentry=(hrdevice_entry_t *)malloc(sizeof(hrdevice_entry_t));
  tempentry->index=DeviceTableEnd->index+1;
  tempentry->type=text2oid("hrDeviceVolatileMemory");
  strcpy(tempentry->descr, "System ROM");
  tempentry->deviceid=text2oid("0.0");
  tempentry->status=2;
  tempentry->errors=0;
  tempentry->next=NULL;
  DeviceTableEnd->next=tempentry;
  DeviceTableEnd=DeviceTableEnd->next;

  /* Update the time stamp */
  time(&DeviceTableLastUpdate);
}

static void free_device_table(void)
{
  hrdevice_entry_t * tmp_ptr;
  
  if (DeviceTable!=NULL)
    {
      for (tmp_ptr=DeviceTable;tmp_ptr!=DeviceTableEnd;tmp_ptr=DeviceTable)
	{
	  DeviceTable=tmp_ptr->next;       /* Move the device table head */
	  free_oid(tmp_ptr->type);
	  free_oid(tmp_ptr->deviceid);
	  free(tmp_ptr);               /* free up */
	}
      free_oid(tmp_ptr->type);
      free_oid(tmp_ptr->deviceid);
      free(tmp_ptr);                   /* free the last one */
      DeviceTable=NULL;
      DeviceTableEnd=NULL;
    }
} 


static void get_processor_table(void)
{
  hrprocessor_entry_t * tempentry=NULL;

  /* Processors */
  int cpus=0;
  int i=0;
  int count = 0;	/* L000 */
  
  /* L000 vvv */
  /* 'null' entries to bring table index into line with hrDeviceTable */
  for(;count < processorTablePos;count++) {
  	tempentry=(hrprocessor_entry_t *)malloc(sizeof(hrprocessor_entry_t));
    tempentry->index=-1;
    tempentry->frwid=text2oid("0.0");                                 
    tempentry->next=NULL;
      if (ProcessorTableEnd==NULL)
	     ProcessorTable=ProcessorTableEnd=tempentry;
      else
	{
	  ProcessorTableEnd->next=tempentry;
	  ProcessorTableEnd=ProcessorTableEnd->next;  
	} 
  }
  cpus=get_ncpu();
  for (i=processorTablePos; i<(cpus + processorTablePos); i++)
    {
      if (i==processorTablePos) /* this is the first one */
	{
	  tempentry= (hrprocessor_entry_t *)malloc
	    (sizeof(hrprocessor_entry_t));
	  ProcessorTable=tempentry;
	  ProcessorTableEnd=tempentry;
	}
      else
	{
	  tempentry= (hrprocessor_entry_t *)malloc
	    (sizeof(hrprocessor_entry_t));
	  ProcessorTableEnd->next=tempentry;
	  ProcessorTableEnd=tempentry;
	}
      tempentry->index=i;
      tempentry->frwid=text2oid("0.0");
      tempentry->next=NULL;
    }
/* L000 ^^^ */
  snap_mets();
  time(&ProcessorTableLastUpdate); 
}

static void free_processor_table(void)
{
  hrprocessor_entry_t * tmp_ptr;
  
  if (ProcessorTable!=NULL)
    {
      for (tmp_ptr=ProcessorTable;tmp_ptr!=ProcessorTableEnd;
	   tmp_ptr=ProcessorTable)
	{
	  ProcessorTable=tmp_ptr->next;       /* Move the device table head */
	  free(tmp_ptr->frwid);        /* free the OID */
	  free(tmp_ptr);               /* free up */
	}
      free(tmp_ptr);                   /* free the last one */
      ProcessorTable=NULL;
      ProcessorTableEnd=NULL;
    }
} 

static void get_network_table(void)
{
  hrnetwork_entry_t * tempentry=NULL;
  FILE * iifp;
  char buf[1024];
  int ifindex=0;
  int count = 0;	/* L000 */

  /* Network */

  /* L000 vvv */
/* 'null entries to bring table index into line with hrDeviceTable */
  for(;count < networkTablePos;count++) {
  	tempentry=(hrnetwork_entry_t *)malloc(sizeof(hrnetwork_entry_t));
    tempentry->ifindex=-1;      
    tempentry->next=NULL;
      if (NetworkTableEnd==NULL)
	     NetworkTable=NetworkTableEnd=tempentry;
      else
	{
	  NetworkTableEnd->next=tempentry;
	  NetworkTableEnd=NetworkTableEnd->next;  
	} 
  }
  /* L000 ^^^ */
  if (!(iifp = fopen("/etc/confnet.d/inet/interface", "r")))
    {
      NetworkTable=NetworkTableEnd=NULL;
      return;
    }
	
  while ((fgets(buf, sizeof(buf), iifp)) != NULL) 
    {
      /* skip the comment and empty lines */
      if (buf[0] == '#' || buf[0] == '\0')
	continue;

      tempentry= (hrnetwork_entry_t *)malloc(sizeof(hrnetwork_entry_t));
      tempentry->ifindex=ifindex + networkTablePos;
      ifindex++;
      tempentry->next=NULL;
      
      if (NetworkTable==NULL)  /* This is the first one */
	{
	  NetworkTable=tempentry;
	  NetworkTableEnd=tempentry;
	}
      else
	{
	  NetworkTableEnd->next=tempentry;
	  NetworkTableEnd=NetworkTableEnd->next;
	}

    }
  time(&NetworkTableLastUpdate);   /* Update the time stamp */
}

static void free_network_table(void)
{
  hrnetwork_entry_t * tmp_ptr;
  
  if (NetworkTable!=NULL)
    {
      for (tmp_ptr=NetworkTable;tmp_ptr!=NetworkTableEnd;
	   tmp_ptr=NetworkTable)
	{
	  NetworkTable=tmp_ptr->next;       /* Move the device table head */
	  free(tmp_ptr);               /* free up */
	}
      free(tmp_ptr);                   /* free the last one */
      NetworkTable=NULL;
      NetworkTableEnd=NULL;
    }
} 

static void get_printer_table(void)
{
  DIR *dp;
  struct dirent *dirp;
  int count=0;
  int nprinters = 0;	/* L000 */
  hrprinter_entry_t * tempentry=NULL;	/* L000 */
  
/* L000 vvv */
/* 'null entries to bring table index into line with hrDeviceTable */
  for(;count < printerTablePos;count++) {
  	tempentry=(hrprinter_entry_t *)malloc(sizeof(hrprinter_entry_t));
    tempentry->index=-1;
    tempentry->status=2; /* unknown */
	tempentry->detectederrorstate[0]=NULL;         
    tempentry->next=NULL;
      if (PrinterTableEnd==NULL)
	     PrinterTable=PrinterTableEnd=tempentry;
      else
	{
	  PrinterTableEnd->next=tempentry;
	  PrinterTableEnd=PrinterTableEnd->next;  
	} 
  }
  nprinters = 0;
  /* L000 ^^^ */
  if ((dp=opendir("/etc/lp/printers"))!= NULL)
    while ((dirp= readdir(dp))!=NULL)
      {
	if (dirp->d_name[0]=='.')
	  continue;
	else
	  {
	    FILE * fpin;
	    char cmdstr[1024];
	    char output[1024];
	    /* get the path of the configuration file */
	    strcpy(cmdstr, "/bin/grep Remote: /etc/lp/printers/");
	    strcat(cmdstr, dirp->d_name);
	    strcat(cmdstr, "/configuration");

	    /* do a grep Remote */
	    fpin=popen(cmdstr, "r");

	    /* if there is no output, this is a local printer. */
	    if (fgets(output, 1024, fpin)==NULL)
	      {
		hrprinter_entry_t * tempentry =
		  (hrprinter_entry_t *)malloc(sizeof(hrprinter_entry_t));
		tempentry->index=count;
		tempentry->status=2; /* unknown */
		tempentry->detectederrorstate[0]=NULL;
		tempentry->next=NULL;
		count++;
		nprinters++;
		if (PrinterTable==NULL)  /* This is the first one */
		  {
		    PrinterTable=tempentry;
		    PrinterTableEnd=tempentry;
		  }
		else
		  {
		    PrinterTableEnd->next=tempentry;
		    PrinterTableEnd=PrinterTableEnd->next;
		  }
	      }
	    /* if there is output, this is a remote printer, ignore */
	    pclose(fpin);
	  }
      }
  closedir(dp);
  /* L000 vvv */
  if(nprinters == 0)	/* no printers, so null out the printer table */
	PrinterTable = PrinterTableEnd = NULL;
  /* L000 ^^^ */
  time(&PrinterTableLastUpdate);   /* Update the time stamp */
}

static void free_printer_table(void)
{
  hrprinter_entry_t * tmp_ptr;
  
  if (PrinterTable!=NULL)
    {
      for (tmp_ptr=PrinterTable;tmp_ptr!=PrinterTableEnd;
	   tmp_ptr=PrinterTable)
	{
	  PrinterTable=tmp_ptr->next;       /* Move the device table head */
	  free(tmp_ptr);               /* free up */
	}
      free(tmp_ptr);                   /* free the last one */
      PrinterTable=NULL;
      PrinterTableEnd=NULL;
    }
} 

static void get_disk_table(void)
{
  hrdisk_entry_t *tempentry;
  FILE * fpin;
  char cmdstr[1024];
  char output[1024];
  int count=0;

  /* L000 vvv */
/* 'null entries to bring table index into line with hrDeviceTable */
  for(;count < diskTablePos;count++) {
  	tempentry=(hrdisk_entry_t *)malloc(sizeof(hrdisk_entry_t));
    tempentry->index=-1;
    tempentry->access=-1;                   
    tempentry->media=0;                    
    tempentry->removeble=0;                
    tempentry->next=NULL;
      if (DiskTableEnd==NULL)
	     DiskTable=DiskTableEnd=tempentry;
      else
	{
	  DiskTableEnd->next=tempentry;
	  DiskTableEnd=DiskTableEnd->next;  
	} 
  }
  /* L000 ^^^ */
  /* FLOPPY DISKETTE Storage */

  sprintf(cmdstr, "%s type=\"diskette\"", GETDEVCMD);
  fpin=popen(cmdstr, "r");
  for(;;)
    {
      if(fgets(output, 1024, fpin)==NULL)
	break;
      tempentry=(hrdisk_entry_t *)malloc(sizeof(hrdisk_entry_t));
      tempentry->index=count;
      tempentry->access=1;                   /* readWrite */
      tempentry->media=4;                    /* floppy */
      tempentry->removeble=1;                /* yes */
      {
	FILE *fpin;
	char cmdstr[1024];
	output[strlen(output)-1]=NULL;
	sprintf(cmdstr, "%s %s capacity", DEVATTRCMD, output);
	fpin=popen(cmdstr, "r");
	fscanf(fpin, "%d", &(tempentry->capacity));  /* capacity */
	/* L000 vvv */
	tempentry->capacity /= 2;       /* blocks to kbytes */
    /* L000 ^^^ */
	pclose(fpin);
      }
      tempentry->next=NULL;
      if (DiskTableEnd==NULL)
	DiskTable=DiskTableEnd=tempentry;
      else
	{
	  DiskTableEnd->next=tempentry;
	  DiskTableEnd=DiskTableEnd->next;  
	}
      count++;
    }
  pclose(fpin);

  /* CD-ROM Storage */
  sprintf(cmdstr, "%s type=\"cdrom\"", GETDEVCMD);
  fpin=popen(cmdstr, "r");
  for(;;)
    {
      if(fgets(output, 1024, fpin)==NULL)
	break;
      tempentry=(hrdisk_entry_t *)malloc(sizeof(hrdisk_entry_t));
      tempentry->index=count;
      tempentry->access=2;                   /* readOnly */
      tempentry->media=5;                    /* opticalDiskROM */
      tempentry->removeble=1;                /* yes */
      {
	FILE *fpin;
	char cmdstr[1024];
	output[strlen(output)-1]=NULL;
	sprintf(cmdstr, "%s %s capacity", DEVATTRCMD, output);
	fpin=popen(cmdstr, "r");
	fscanf(fpin, "%d", &(tempentry->capacity));  /* capacity */
	pclose(fpin);
      }
      tempentry->next=NULL;
      if (DiskTableEnd==NULL)
	DiskTable=DiskTableEnd=tempentry;
      else
	{
	  DiskTableEnd->next=tempentry;
	  DiskTableEnd=DiskTableEnd->next;  
	}
      count++;
    }
  pclose(fpin);

  /* Hard Disk Storage */
  sprintf(cmdstr, "%s type=\"disk\"", GETDEVCMD);
  fpin=popen(cmdstr, "r");
  for(;;)
    {
      if(fgets(output, 1024, fpin)==NULL)
	break;
      tempentry=(hrdisk_entry_t *)malloc(sizeof(hrdisk_entry_t));
      tempentry->index=count;
      tempentry->access=1;                   /* readWrite */
      tempentry->media=3;                    /* hard disk */
      tempentry->removeble=2;                /* NO */
      {
	FILE *fpin;
	char cmdstr[1024];
	output[strlen(output)-1]=NULL;
	sprintf(cmdstr, "%s %s capacity", DEVATTRCMD, output);
	fpin=popen(cmdstr, "r");
	fscanf(fpin, "%d", &(tempentry->capacity));  /* capacity */
	pclose(fpin);
      }
      tempentry->next=NULL;
      if (DiskTableEnd==NULL)
	DiskTable=DiskTableEnd=tempentry;
      else
	{
	  DiskTableEnd->next=tempentry;
	  DiskTableEnd=DiskTableEnd->next;  
	}
      count++;
    }
  pclose(fpin);


  time(&DiskTableLastUpdate);   /* Update the time stamp */
}

static void free_disk_table(void)
{
  hrdisk_entry_t * tmp_ptr;
  
  if (DiskTable!=NULL)
    {
      for (tmp_ptr=DiskTable;tmp_ptr!=DiskTableEnd;
	   tmp_ptr=DiskTable)
	{
	  DiskTable=tmp_ptr->next;       /* Move the  table head */
	  free(tmp_ptr);               /* free up */
	}
      free(tmp_ptr);                   /* free the last one */
      DiskTable=NULL;
      DiskTableEnd=NULL;
    }
} 

static void get_partition_table(void)
{
  hrpartition_entry_t *tempentry;
  FILE * fpin;
  char cmdstr[1024];
  char output[1024];  

  sprintf(cmdstr, "%s type=\"dpart\"", GETDEVCMD);
  fpin=popen(cmdstr, "r");
  for(;;)
    {
      if(fgets(output, 1024, fpin)==NULL)
	break;
      tempentry=(hrpartition_entry_t *)malloc(sizeof(hrpartition_entry_t));
      if (PartitionTableEnd==NULL)   /* if this is the first entry */
	tempentry->index=0;
      else
	tempentry->index=PartitionTableEnd->index+1;
      output[strlen(output)-1]=NULL;
      tempentry->fsindex=tempentry->index;
      {
	FILE *fpin;
	char cmdstr[1024];
	sprintf(cmdstr, "%s %s desc bdevice capacity", DEVATTRCMD, output);
	fpin=popen(cmdstr, "r");
	fgets(tempentry->label, 1024, fpin);
	fgets(tempentry->id, 1024, fpin);
	fscanf(fpin, "%d", &(tempentry->size));  /* capacity */
	tempentry->size=tempentry->size / 2;     /* it was in 512-byte */
	pclose(fpin);
      }
      tempentry->next=NULL;
      if (PartitionTableEnd==NULL)
	PartitionTable=PartitionTableEnd=tempentry;
      else
	{
	  PartitionTableEnd->next=tempentry;
	  PartitionTableEnd=PartitionTableEnd->next;  
	}
    }
  pclose(fpin);
  time(&PartitionTableLastUpdate);   /* Update the time stamp */
}

static void free_partition_table(void)
{
  hrpartition_entry_t * tmp_ptr;
  
  if (PartitionTable!=NULL)
    {
      for (tmp_ptr=PartitionTable;tmp_ptr!=PartitionTableEnd;
	   tmp_ptr=PartitionTable)
	{
	  PartitionTable=tmp_ptr->next;       /* Move the device table head */
	  free(tmp_ptr);               /* free up */
	}
      free(tmp_ptr);                   /* free the last one */
      PartitionTable=NULL;
      PartitionTableEnd=NULL;
    }
} 

static void get_file_system_table(void)
{
  
  FILE *fi;
  struct mnttab Mp;
  hrfs_entry_t * tempentry;
  int localindex=0;

  fi = fopen(MNTTAB, "r");

  while (!getmntent(fi, &Mp))
    {
      if (FSTable==NULL)  /* if this is the first one */
	{
	  tempentry= (hrfs_entry_t *)malloc(sizeof(hrfs_entry_t));
	  FSTable=tempentry;
	  FSTableEnd=tempentry;
	}
      else                /* if this is not the first one */
	{
	  tempentry=(hrfs_entry_t *)malloc(sizeof(hrfs_entry_t));
	  FSTableEnd->next=tempentry;
	  FSTableEnd=FSTableEnd->next;
	}
	    
      /* set hrFSIndex */
      tempentry->index=localindex;
      localindex++;

      /* set hrFSMountPoint */
      strncpy(tempentry->mountpoint, Mp.mnt_mountp, 1024);
      
      /* set remote mount point */
      if (strcmp(Mp.mnt_fstype, "nfs")==0)
	strncpy(tempentry->remotemountpoint, Mp.mnt_special, 1024);
      else
	tempentry->remotemountpoint[0]=0;
      
      /* set hrFSType */
      if (strcmp (Mp.mnt_fstype, "ufs")==0)
	tempentry->type=text2oid("hrFSBerkeleyFFS");

      else if (strcmp (Mp.mnt_fstype, "s5")==0)
	tempentry->type=text2oid("hrFSSys5FS");

      else if (strcmp(Mp.mnt_fstype, "vxfs")==0)
        tempentry->type=text2oid("hrFSVNode");

      else if (strcmp (Mp.mnt_fstype, "nfs")==0)
	tempentry->type=text2oid("hrFSNFS");

      else if (strcmp (Mp.mnt_fstype, "rfs")==0)
	tempentry->type=text2oid("hrFSRFS");

      else if (strcmp (Mp.mnt_fstype, "nxfs")==0)
	tempentry->type=text2oid("hrFSNetware");

      else if (strcmp (Mp.mnt_fstype, "npfs")==0)
	tempentry->type=text2oid("hrFSNetware");

      else if (strcmp (Mp.mnt_fstype, "nucfs")==0)
	tempentry->type=text2oid("hrFSNetware");

      else if (strcmp (Mp.mnt_fstype, "cdfs")==0)
	tempentry->type=text2oid("hrFSiso9660");

      else if (strcmp (Mp.mnt_fstype, "bfs")==0)
	tempentry->type=text2oid("hrFSBFS");

      else 
	tempentry->type=text2oid("hrFSUnknown");
      
      /* set access */
      tempentry->access=1;
      if (strstr(Mp.mnt_mntopts,"ro"))
	tempentry->access=2;
      else if (strstr(Mp.mnt_mntopts,"rw"))
	tempentry->access=1;
      
      /* set bootable */
      if (strcmp (Mp.mnt_fstype, "bfs")==0)
	tempentry->bootable=1;
      else
	tempentry->bootable=2;

      /* set storage index */
      tempentry->storageindex=0;

      tempentry->next=NULL;
    } 
  fclose(fi);
  time(&FSTableLastUpdate);
}


static void free_file_system_table(void)
{
  hrfs_entry_t * tmp_ptr;
  
  if (FSTable!=NULL)
    {
      for (tmp_ptr=FSTable;tmp_ptr!=FSTableEnd;tmp_ptr=FSTable)
	{
	  FSTable=tmp_ptr->next;  /* Move the file system table head */
	  free_oid(tmp_ptr->type); /* don't forget free up OID */
	  free(tmp_ptr);               /* free up */
	}
      free_oid(tmp_ptr->type); /* don't forget free up OID */
      free(tmp_ptr);                   /* free the last one */
      FSTable=NULL;
      FSTableEnd=NULL;
    }
}
