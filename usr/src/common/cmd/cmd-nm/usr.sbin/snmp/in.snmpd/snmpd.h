#ident	"@(#)snmpd.h	1.3"
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
 */

/*      @(#)snmpd.h	1.3

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

/* search type (i.e., get or get-next) */
#define NEXT       1
#define EXACT      2

/* read flags (access and variable) */
#define NONE       0
#define READ_ONLY  1
#define READ_WRITE 2

#define MAXINTERFACES	20	/* size of array to hold ifLastChange */

typedef struct _var_entry {
  OID class_ptr;
  unsigned int type;
  unsigned rw_flag;
  unsigned int arg;
  char *smux;
  struct _var_entry *child;
  struct _var_entry *sibling;
  VarBindList *(*funct_get)();
  int (*funct_test_set)();
  int (*funct_set)();
  struct _var_entry *next;
} VarEntry;

#if defined(SVR3) || defined(SVR4)
#define N_IPSTAT          0
#define N_IFNET           1
#define N_RTHOST          2
#define N_RTNET           3
#define N_ICMPSTAT        4
#define N_RTSTAT          5
#define N_RTHASHSIZE      6
#define N_ARPTAB          7
#define N_ARPTAB_SIZE     8
#define N_NETTOREGTABLE   9
#define N_REGROUTETABLE  10
#define N_REGHASHSIZE    11
#define N_UDPSTAT        12
#define N_TCPSTAT        13
#define N_TCB            14
#define N_PROVIDER       15
#define N_LASTPROV       16
#define N_NTCP           17
#define N_IPFORWARDING   18
#define N_UDB            19
#define N_TCPMINREXMT    20
#define N_TCPMAXREXMT    21
#define N_IPQ_TTL        22
#define N_IP_TTL         23
#endif

/* add this to files that need it */
#ifndef SNMPD
extern struct nlist nl[];
extern char *system_name;
extern char *kmemf;
extern int kmem;
#endif

/* This is for printing TLI error messages */

extern char *t_errmsg();

#define GFNULL  (VarBindList *(*) ()) NULL
#define TFNULL  (int (*) ()) NULL
#define CFNULL  (int (*) ()) NULL
#define SFNULL  (int (*) ()) NULL

#define FALSE 0
#define TRUE  1

#ifdef NEW_MIB

#include <sys/socket.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>

struct arptab {
	struct in_addr at_iaddr;
	union {
		ether_addr_t atu_enaddr;
		long atu_tvsec;
	} at_union;
#define at_enaddr at_union.atu_enaddr
#define at_tvsec at_union.atu_tvsec
	unsigned char at_expire;
	unsigned char at_flags;
	void *at_hold;
};
#endif /* NEW_MIB */
