#ident	"@(#)hostmibd.h	1.2"
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

#ifndef SNMP_HOSTMIBD_H
#define SNMP_HOSTMIBD_H

/* Update information every 5 seconds */
#define HOSTMIBD_UPDATE_INTERVAL   5   

/* The path of Host Resources MIB file */
#define HOSTMIBD_MIB_FILE "/etc/netmgt/unixwared.defs"

/* The path of commands, hostmibd uses */

#define GETDEVCMD           "/bin/getdev"
#define GETDGRPCMD          "/bin/getdgrp"
#define DEVATTRCMD          "/bin/devattr"

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/mman.h>
#include <mas.h>
#include <metreg.h>
#include <signal.h>


struct met 
{
  uint32 met;      /* the previous value */
  caddr_t met_p;	 /* the pointer to the current value */
  double intv;     /* difference */
  double cooked;   /* difference / time interval*/
};

struct dblmet 
{
  dl_t met;
  caddr_t met_p;	
  double intv;
  double cooked;
};

/*
 *	per-processor metrics
 */
struct met *idl_time;

/*
 *	global metrics supplied by system
 */
struct dblmet *freemem;
struct dblmet *freeswp;

#endif
