/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)Space.c	1.8"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/proc.h>
#include <sys/ws/ws.h>
#include <config.h>

#ifndef KD_CPUBIND
#define KD_CPUBIND 0
#endif

/*
 * Change the cpu field in the kd driver sdevice entry to change this value
 * note that all multiconsole drivers _MUST_ be bound to the same cpu
 */
processorid_t ws_processor = KD_CPUBIND;

int ws_maxminor = WS_MAXSTATION * 15;		/* 15 == WS_MAXCHAN */
int i8042_timeout_spin = 100000;

#define MAX_CALLOUTS 16			/* Max number multiconsole devices */
int ws_max_callouts = MAX_CALLOUTS;
