#ident	"@(#)filter.h	1.2"
#ident	"$Header$"
/*      @(#) filter.h,v 1.1.1.1 1994/09/28 22:52:19 naren Exp */
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1995 Legent Corporation
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
/*      SCCS IDENTIFICATION        */
/*
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @(#) /proj/tcp/gis/lcvs/usr/src/cmd/net/pppd/filter.h,v 1.1.1.1 1994/09/28 22:52:19 naren Exp (LBL)
 */

#ifdef __GNUC__
#define inline __inline
#else
#define inline
#endif

#ifndef __STDC__
extern char *malloc();
extern char *calloc();
#endif

#define min(a,b) ((a)>(b)?(b):(a))
#define max(a,b) ((b)>(a)?(b):(a))

/* 
 * The default snapshot length.  This value allows most printers to print
 * useful information while keeping the amount of unwanted data down.
 * In particular, it allows for an ethernet header, tcp/ip header, and
 * 14 bytes of data (assuming no ip options).
 */
#define DEFAULT_SNAPLEN 68

#ifndef BIG_ENDIAN
#define BIG_ENDIAN 4321
#define LITTLE_ENDIAN 1234
#endif

#ifdef TCPDUMP_ALIGN
#if BYTEORDER == LITTLE_ENDIAN
#define EXTRACT_SHORT(p)\
	((u_short)\
		((u_short)*((u_char *)p+1)<<8|\
		 (u_short)*((u_char *)p+0)<<0))
#define EXTRACT_LONG(p)\
		((u_long)*((u_char *)p+3)<<24|\
		 (u_long)*((u_char *)p+2)<<16|\
		 (u_long)*((u_char *)p+1)<<8|\
		 (u_long)*((u_char *)p+0)<<0)
#else
#define EXTRACT_SHORT(p)\
	((u_short)\
		((u_short)*((u_char *)p+0)<<8|\
		 (u_short)*((u_char *)p+1)<<0))
#define EXTRACT_LONG(p)\
		((u_long)*((u_char *)p+0)<<24|\
		 (u_long)*((u_char *)p+1)<<16|\
		 (u_long)*((u_char *)p+2)<<8|\
		 (u_long)*((u_char *)p+3)<<0)
#endif
#else
#define EXTRACT_SHORT(p)	((u_short)ntohs(*(u_short *)p))
#define EXTRACT_LONG(p)		(ntohl(*(u_long *)p))
#endif


/*
 * If a protocol is unknown, PROTO_UNDEF is returned.
 * Also, s_nametoport() returns the protocol along with the port number.
 * If there are ambiguous entried in /etc/services (i.e. domain
 * can be either tcp or udp) PROTO_UNDEF is returned.
 */
#define PROTO_UNDEF		-1

/* Address qualifers. */

#define Q_HOST		1
#define Q_NET		2
#define Q_PORT		3
#define Q_GATEWAY	4
#define Q_PROTO		5

/* Protocol qualifiers. */

#define Q_LINK		1
#define Q_IP		2
#define Q_ARP		3
#define Q_RARP		4
#define Q_TCP		5
#define Q_UDP		6
#define Q_ICMP		7

/* Directional qualifers. */

#define Q_SRC		1
#define Q_DST		2
#define Q_OR		3
#define Q_AND		4

#define Q_DEFAULT	0
#define Q_UNDEF		255

struct stmt {
	int code;
	long k;
};

struct slist {
	struct stmt s;
	struct slist *next;
};

/* 
 * A bit vector to represent definition sets.  We assume TOT_REGISTERS
 * is smaller than 8*sizeof(atomset).
 */
typedef u_long atomset;
#define ATOMMASK(n) (1 << (n))
#define ATOMELEM(d, n) (d & ATOMMASK(n))

/*
 * An unbounded set.
 */
typedef u_long *uset;

/*
 * Total number of atomic entities, including accumulator (A) and index (X).
 * We treat all these guys similarly during flow analysis.
 */
#define N_ATOMS (BPF_MEMWORDS+2)

struct edge {
	int id;
	int code;
	uset edom;
	struct block *succ;
	struct block *pred;
	struct edge *next;	/* link list of incoming edges for a node */
};

struct block {
	int id;
	struct slist *stmts;	/* side effect stmts */
	struct stmt s;		/* branch stmt */
	int mark;
	int level;
	int offset;
	int sense;
	struct edge et;
	struct edge ef;
	struct block *head;
	struct block *link;	/* link field used by optimizer */
	uset dom;
	uset closure;
	struct edge *in_edges;
	atomset def, kill;
	atomset in_use;
	atomset out_use;
	long oval;
	long val[N_ATOMS];
};

struct arth {
	struct block *b;	/* protocol checks */
	struct slist *s;	/* stmt list */
	int regno;		/* virtual register number of result */
};

struct qual {
	unsigned char addr;
	unsigned char proto;
	unsigned char dir;
	unsigned char pad;
};

/* XXX */
#define JT(b)  ((b)->et.succ)
#define JF(b)  ((b)->ef.succ)

#ifdef __STDC__
# define	P(s) s
#else
# define P(s) ()
#endif

/* gencode.c */
struct arth *gen_loadi P((int val));
struct arth *gen_load P((int proto, struct arth *index, int size));
struct arth *gen_loadlen P((void));
struct arth *gen_neg P((struct arth *a));
struct arth *gen_arth P((int code, struct arth *a0, struct arth *a1));

void gen_and P((struct block *b0, struct block *b1));
void gen_or P((struct block *b0, struct block *b1));
void gen_not P((struct block *b0));

struct block *gen_scode P((char *name, struct qual q));
struct block *gen_ncode P((u_long v, struct qual q));
struct block *gen_proto_abbrev P((int proto));
struct block *gen_relation P((int code, struct arth *a0, struct arth *a1, int reversed));
struct block *gen_less P((int n));
struct block *gen_greater P((int n));
struct block *gen_byteop P((int op, int idx, int val));
struct block *gen_multicast P((int proto));
struct block *gen_broadcast P((int proto));
void finish_parse P((struct block *p));
u_long **s_nametoaddr P((char *name));
u_long s_nametonetaddr P((char *name));
int s_nametoport P((char *name, int *port, int *proto));
int s_nametoproto P((char *str));
void init_linktype P((int type));
int alloc_reg P((void ));
void free_reg P((int n ));

/* optimize.c */
void opt_init P((struct block *root));
void opt_cleanup P((void));
void make_marks P((struct block *p));
void make_code P((struct block *p));
void intern_blocks P((struct block *root));
int eq_slist P((struct slist *x, struct slist *y));
void optimize P(( struct block **rootp));
struct bpf_insn *icode_to_fcode P((struct block *root, int *lenp));
#undef P
