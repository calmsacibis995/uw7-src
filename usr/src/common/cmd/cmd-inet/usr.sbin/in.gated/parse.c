#ident	"@(#)parse.c	1.4"
#ident	"$Header$"

/*
 * Public Release 3
 * 
 * $Id$
 */

/*
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1996, 1997 The Regents of the University of Michigan
 * All Rights Reserved
 *  
 * Royalty-free licenses to redistribute GateD Release
 * 3 in whole or in part may be obtained by writing to:
 * 
 * 	Merit GateDaemon Project
 * 	4251 Plymouth Road, Suite C
 * 	Ann Arbor, MI 48105
 *  
 * THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS OF THE
 * UNIVERSITY OF MICHIGAN AND MERIT DO NOT WARRANT THAT THE
 * FUNCTIONS CONTAINED IN THE SOFTWARE WILL MEET LICENSEE'S REQUIREMENTS OR
 * THAT OPERATION WILL BE UNINTERRUPTED OR ERROR FREE. The Regents of the
 * University of Michigan and Merit shall not be liable for
 * any special, indirect, incidental or consequential damages with respect
 * to any claim by Licensee or any third party arising from use of the
 * software. GateDaemon was originated and developed through release 3.0
 * by Cornell University and its collaborators.
 * 
 * Please forward bug fixes, enhancements and questions to the
 * gated mailing list: gated-people@gated.merit.edu.
 * 
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1990,1991,1992,1993,1994,1995 by Cornell University.
 *     All rights reserved.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT
 * LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * GateD is based on Kirton's EGP, UC Berkeley's routing
 * daemon	 (routed), and DCN's HELLO routing Protocol.
 * Development of GateD has been supported in part by the
 * National Science Foundation.
 * 
 * ------------------------------------------------------------------------
 * 
 * Portions of this software may fall under the following
 * copyrights:
 * 
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms are
 * permitted provided that the above copyright notice and
 * this paragraph are duplicated in all such forms and that
 * any documentation, advertising materials, and other
 * materials related to such distribution and use
 * acknowledge that the software was developed by the
 * University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote
 * products derived from this software without specific
 * prior written permission.  THIS SOFTWARE IS PROVIDED
 * ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */


#define	INCLUDE_DIRENT
#define	INCLUDE_ROUTE
#include "include.h"
#include "parse.h"
#include "krt.h"
#ifdef	PROTO_RIP
#include "rip.h"
#endif	/* PROTO_RIP */
#ifdef	PROTO_HELLO
#include "hello.h"
#endif	/* PROTO_HELLO */
#ifdef	PROTO_OSPF
#include "ospf.h"
#endif	/* PROTO_OSPF */
#ifdef	PROTO_EGP
#include "egp.h"
#endif	/* PROTO_EGP */
#ifdef	PROTO_BGP
#include "bgp.h"
#endif	/* PROTO_BGP */
#ifdef	PROTO_SLSP
#include "slsp.h"
#endif	/* PROTO_SLSP */
#ifdef	PROTO_ISIS
#include "isis.h"
#endif	/* PROTO_ISIS */
#ifdef	PROTO_DVMRP
#include "dvmrp.h"
#endif	/* PROTO_DVMRP */
#include "parser.h"

/*
 * Patricia search node for keywords.  This is a bit wasteful and
 * could be reduced by four bytes by not keeping the length of
 * the keyword in there, but the length provides an easy check
 * for mismatches.  I'll leave it for now.
 */
typedef struct _parse_key_token {
    struct _parse_key_token *pkw_left;		/* node when bit is zero */
    struct _parse_key_token *pkw_right;		/* node when bit is one */
    const char *pkw_keyword;			/* the keyword */
    int pkw_token;				/* parser token to return */
#if	defined(ultrix) && defined(vax) && !defined(__GNUC__) && !defined(__STDC__)
    u_int pkw_bit;				/* bit to test */
    u_int pkw_len;				/* length of kw in bit format */
#else	/* defined(ultrix) && defined(vax) && !defined(__GNUC__) && !defined(__STDC__) */
    u_int16 pkw_bit;				/* bit to test */
    u_int16 pkw_len;				/* length of kw in bit format */
#endif	/* defined(ultrix) && defined(vax) && !defined(__GNUC__) && !defined(__STDC__) */
} parse_key_token;

/*
 * The `bit' in the keyword structure is formatted as a byte offset
 * in the upper 8 bits, and a bit to test in a byte in the lower 8
 * bits.  You hence do tests as (string[bit >> 8] & (char)bit).
 * The order of bit tests in a byte is such that the value of bit
 * increases as you go further down the tree, i.e. the low order bit
 * in a byte is tested first.
 *
 * This implementation results in the following:
 */
#define	PKW_MAX_LEN	254		/* maximum length of keyword */

#define	PKW_BITTEST(cp, bit) \
    ((((cp)[(bit) >> 8]) & ((char)(bit))) != 0)

#define	PKW_MAKEBIT(offset, bit_in_byte) \
    ((((offset) & 0xff) << 8) | (bit_in_byte & 0xff))

#define	PKW_NOBIT	0		/* bit # when no external node */


/*
 * Value of the token we return when none found.
 */
#define	NO_TOKEN	0

/*
 * A table to use to return the first 1 bit in a byte, counting from
 * the low order end.  I.e. 0x1 returns 0x1, 0x3 returns 0x1 and 0x6
 * returns 0x2.
 */
static const byte parse_low_bit_set[256] = {
    0x01, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x10, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x20, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x10, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x40, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x10, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x20, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x10, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x80, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x10, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x20, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x10, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x40, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x10, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x20, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x10, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01
};


#ifndef	CASE_SENSITIVE
/*
 * XXX This table is used to make case-insensitive keywork comparisons
 * by converting to lower case.  This is obviously dependent on 7-bit ASCII,
 * which means it may break on some systems.  This could be avoided by
 * using system routines, but this is used in a spot in the code which
 * is speed-critical when running with large configurations and the system
 * routines may add unwanted code.
 *
 * If this screws up on a machine you are interested in, try compiling
 * the keyword lookups for case sensitivity.  This is still not perfect,
 * but it should be more widely portable.
 */
static const u_char parse_tolower[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
     ' ',  '!',  '"',  '#',  '$',  '%',  '&', 0x27,
     '(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
     '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
     '8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
     '@',  'a',  'b',  'c',  'd',  'e',  'f',  'g',
     'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
     'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
     'x',  'y',  'z',  '[', '\\',  ']',  '^',  '_',
     '`',  'a',  'b',  'c',  'd',  'e',  'f',  'g',
     'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
     'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
     'x',  'y',  'z',  '{',  '|',  '}',  '~', 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
    0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
    0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
    0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
#endif	/* !CASE_SENSITIVE */

/*
 * Table of keywords recognized.  The patricia tree is built at run time.
 * These are also sorted into token order at run time.
 */

#define	KWE(token, string) \
    { (parse_key_token *) 0, (parse_key_token *) 0, (string), (token), (u_short) PKW_NOBIT, (u_short) (sizeof (string)) }

static parse_key_token *kw_tree_head = (parse_key_token *) 0;
static u_int kw_max_len;

/*
 * N.B. Here are the rules.
 *
 * (1) Keywords must be all lower case.
 *
 * (2) Alternative keywords returning the same token must be listed together
 *     in the list.
 *
 * (3) If there are multiple keywords for a token, the particular version
 *     you want printed by error diagnostics (i.e. the one found when doing
 *     token->keyword mappings) should be listed first.
 *
 * (4) Don't put the same keyword in twice (when you do this you'll get an
 *     assertion failure below in parse_keyword_sort())
 */
static parse_key_token keywords[] =
{
/* Start Sort */
#ifdef	TISIS_BUILDLSP
    KWE(TISIS_BUILDLSP, "buildlsp"),
#endif	/* TISIS_BUILDLSP */
#ifdef	TISIS_CSNP
    KWE(TISIS_CSNP, "csnp"),
#endif	/* TISIS_CSNP */
#ifdef	TISIS_DUMPLSP
    KWE(TISIS_DUMPLSP, "dumplsp"),
#endif	/* TISIS_DUMPLSP */
#ifdef	TISIS_EVENTS
    KWE(TISIS_EVENTS, "events"),
#endif	/* TISIS_EVENTS */
#ifdef	TISIS_FLOODING
    KWE(TISIS_FLOODING, "flooding"),
#endif	/* TISIS_FLOODING */
#ifdef	TISIS_IIH
    KWE(TISIS_IIH, "iih"),
#endif	/* TISIS_IIH */
#ifdef	TISIS_LANADJ
    KWE(TISIS_LANADJ, "lanadj"),
#endif	/* TISIS_LANADJ */
#ifdef	TISIS_LSPCONTENT
    KWE(TISIS_LSPCONTENT, "lspcontent"),
#endif	/* TISIS_LSPCONTENT */
#ifdef	TISIS_LSPDB
    KWE(TISIS_LSPDB, "lspdb"),
#endif	/* TISIS_LSPDB */
#ifdef	TISIS_LSPINPUT
    KWE(TISIS_LSPINPUT, "lspinput"),
#endif	/* TISIS_LSPINPUT */
#ifdef	TISIS_P2PADJ
    KWE(TISIS_P2PADJ, "p2padj"),
#endif	/* TISIS_P2PADJ */
#ifdef	TISIS_PATHS
    KWE(TISIS_PATHS, "paths"),
#endif	/* TISIS_PATHS */
#ifdef	TISIS_PSNP
    KWE(TISIS_PSNP, "psnp"),
#endif	/* TISIS_PSNP */
#ifdef	TISIS_SUMMARY
    KWE(TISIS_SUMMARY, "summary"),
#endif	/* TISIS_SUMMARY */
#ifdef	T_ADDRESS
    KWE(T_ADDRESS, "address"),
#endif	/* T_ADDRESS */
#ifdef	T_ACK
    KWE(T_ACK, "ack"),
    KWE(T_ACK, "acknowledgement"),
#endif	/* T_ACK */
#ifdef	T_ACQUIRE
    KWE(T_ACQUIRE, "acquire"),
#endif	/* T_ACQUIRE */
#ifdef	T_ADV
    KWE(T_ADV,	"adv"),
    KWE(T_ADV,	"adv_entry"),
#endif	/* T_ADV */
#ifdef	T_ADVERTISE
    KWE(T_ADVERTISE, "advertise"),
#endif	/* T_ADVERTISE */
#ifdef	T_AGGREGATE
    KWE(T_AGGREGATE, "aggregate"),
#endif	/* T_AGGREGATE */
#ifdef	T_ALERT
    KWE(T_ALERT, "alert"),
#endif	/* T_ALERT */
#ifdef	T_ALL
    KWE(T_ALL, "all"),
#endif	/* T_ALL */
#ifdef	T_ALLOW
    KWE(T_ALLOW, "allow" ),
#endif	/* T_ALLOW */
#ifdef	T_ANALRETENTIVE
    KWE(T_ANALRETENTIVE, "anal-retentive" ),
    KWE(T_ANALRETENTIVE, "analretentive" ),
    KWE(T_ANALRETENTIVE, "show-warnings"),
    KWE(T_ANALRETENTIVE, "showwarnings"),
#endif	/* T_ANALRETENTIVE */
#ifdef	T_ANNOUNCE
    KWE(T_ANNOUNCE, "announce"),
#endif	/* T_ANNOUNCE */
#ifdef	T_ANY
    KWE(T_ANY,	"any"),
#endif	/* T_ANY */
#ifdef	T_AREA
    KWE(T_AREA, "area"),
#endif	/* T_AREA */
#ifdef	T_AS
    KWE(T_AS, "as"),
    KWE(T_AS, "autonomous-system"),
    KWE(T_AS, "autonomoussystem"),
#endif	/* T_AS */
#ifdef	T_ASPATH
    KWE(T_ASPATH,	"as-path"),
    KWE(T_ASPATH,	"aspath"),
#endif	/* T_ASPATH */
#ifdef	T_AUTH
    KWE(T_AUTH, "auth"),
    KWE(T_AUTH, "authentication"),
#endif	/* T_AUTH */
#ifdef	T_AUTHKEY
    KWE(T_AUTHKEY, "auth-key"),
    KWE(T_AUTHKEY, "authentication-key"),
    KWE(T_AUTHKEY, "authenticationkey"),
    KWE(T_AUTHKEY, "authkey"),
#endif	/* T_AUTHKEY */
#ifdef	T_AUTHTYPE
    KWE(T_AUTHTYPE, "auth-type"),
    KWE(T_AUTHTYPE, "authentication-type"),
    KWE(T_AUTHTYPE, "authenticationtype"),
    KWE(T_AUTHTYPE, "authtype"),
#endif	/* T_AUTHTYPE */
#ifdef	T_BACKBONE
    KWE(T_BACKBONE, "backbone"),
#endif	/* T_BACKBONE */
#ifdef	T_BACKGROUND
    KWE(T_BACKGROUND, "background"),
#endif	/* T_BACKGROUND */
#ifdef	T_BGP
    KWE(T_BGP, "bgp"),
#endif	/* T_BGP */
#ifdef	T_BLACKHOLE
    KWE(T_BLACKHOLE, "blackhole"),
#endif	/* T_BLACKHOLE */
#ifdef	T_BRIEF
    KWE(T_BRIEF, "brief"),
#endif	/* T_BRIEF */
#ifdef	T_BROADCAST
    KWE(T_BROADCAST, "broadcast"),
#endif	/* T_BROADCAST */
#ifdef	T_CIRCUIT
    KWE(T_CIRCUIT,	"circuit"),
#endif	/* T_CIRCUIT */
#ifdef	T_CLIENT
    KWE(T_CLIENT,	"client"),
#endif	/* T_CLIENT */
#ifdef	T_CRIT
    KWE(T_CRIT, "crit"),
#endif	/* T_CRIT */
#ifdef	T_DD
    KWE(T_DD, "dd"),
#endif	/* T_DD */
#ifdef	T_DEBUG
    KWE(T_DEBUG, "debug"),
#endif	/* T_DEBUG */
#ifdef	T_DEFAULT
    KWE(T_DEFAULT, "default"),
#endif	/* T_DEFAULT */
#ifdef	T_DEFAULTIN
    KWE(T_DEFAULTIN,	"importdefault"),
#endif	/* T_DEFAULTIN */
#ifdef	T_DEFAULTMETRIC
    KWE(T_DEFAULTMETRIC, "defaultmetric"),
#endif	/* T_DEFAULTMETRIC */
#ifdef	T_DEFAULTOUT
    KWE(T_DEFAULTOUT,	"exportdefault"),
#endif	/* T_DEFAULTOUT */
#ifdef	T_DEFAULTS
    KWE(T_DEFAULTS, "defaults"),
#endif	/* T_DEFAULTS */
#ifdef	T_DEFINE
    KWE(T_DEFINE,		"define" ),
#endif	/* T_DEFINE */
#ifdef	T_DETAIL
    KWE(T_DETAIL, "detail"),
#endif	/* T_DETAIL */
#ifdef	T_DIRECT
    KWE(T_DIRECT, "direct"),
#endif	/* T_DIRECT */
#ifdef	T_DISABLE
    KWE(T_DISABLE, "disable"),
    KWE(T_DISABLE, "disabled"),
#endif	/* T_DISABLE */
#ifdef	T_DOWN
    KWE(T_DOWN, "down"),
#endif	/* T_DOWN */
#ifdef	T_DUAL
    KWE(T_DUAL,	"dual"),
#endif	/* T_DUAL */
#ifdef	T_DVMRP
    KWE(T_DVMRP, "dvmrp"),
#endif	/* T_DVMRP */
#ifdef	T_EGP
    KWE(T_EGP, "egp"),
#endif	/* T_EGP */
#ifdef	T_ELIGIBLE
    KWE(T_ELIGIBLE, "eligible"),
#endif	/* T_ELIGIBLE */
#ifdef	T_EMERG
    KWE(T_EMERG, "emerg" ),
#endif	/* T_EMERG */
#ifdef	T_ENABLE
    KWE(T_ENABLE, "enable"),
    KWE(T_ENABLE, "enabled"),
#endif	/* T_ENABLE */
#ifdef	T_ERR
    KWE(T_ERR, "err"),
#endif	/* T_ERR */
#ifdef	T_ERROR
    KWE(T_ERROR, "error"),
#endif	/* T_ERROR */
#ifdef	T_EVERY
    KWE(T_EVERY, "every"),
#endif	/* T_EVERY */
#ifdef	T_EXACT
    KWE(T_EXACT, "exact"),
#endif	/* T_EXACT */
#ifdef	T_EXCEPT
    KWE(T_EXCEPT, "except"),
#endif	/* T_EXCEPT */
#ifdef	T_EXPORT
    KWE(T_EXPORT, "export"),
#endif	/* T_EXPORT */
#ifdef	T_EXPORTINTERVAL
    KWE(T_EXPORTINTERVAL, "exportinterval"),
    KWE(T_EXPORTINTERVAL, "export-interval"),
#endif	/* T_EXPORTINTERVAL */
#ifdef	T_EXPORTLIMIT
    KWE(T_EXPORTLIMIT, "exportlimit"),
    KWE(T_EXPORTLIMIT, "export-limit"),
#endif	/* T_EXPORTLIMIT */
#ifdef	T_EXTERNAL
    KWE(T_EXTERNAL, "external"),
#endif	/* T_EXTERNAL */
#ifdef	T_FILES
    KWE(T_FILES, "files"),
#endif	/* T_FILES */
#ifdef	T_FIRST
    KWE(T_FIRST, "first"),
#endif	/* T_FIRST */
#ifdef	T_FLASH
     KWE(T_FLASH, "flash"),
#endif	/* T_FLASH */
#ifdef	T_GATEWAY
    KWE(T_GATEWAY, "gateway"),
    KWE(T_GATEWAY, "gw"),
    KWE(T_GATEWAY, "router"),
    KWE(T_GATEWAY, "rtr"),
#endif	/* T_GATEWAY */
#ifdef	T_GENDEFAULT
    KWE(T_GENDEFAULT, "gendefault"),
    KWE(T_GENDEFAULT, "originate-default"),
    KWE(T_GENDEFAULT, "originatedefault"),
#endif	/* T_GENDEFAULT */
#ifdef	T_GENERAL
    KWE(T_GENERAL, "general"),
#endif	/* T_GENERAL */
#ifdef	T_GENERATE
    KWE(T_GENERATE,	"generate"),
#endif	/* T_GENERATE */
#ifdef	T_GROUP
    KWE(T_GROUP, "group"),
#endif	/* T_GROUP */
#ifdef	T_HELLO
    KWE(T_HELLO, "hello"),
#endif	/* T_HELLO */
#ifdef	T_HELLOIN
    KWE(T_HELLOIN, "helloin"),
#endif	/* T_HELLOIN */
#ifdef	T_HELLOOUT
    KWE(T_HELLOOUT, "helloout"),
#endif	/* T_HELLOOUT */
#ifdef	T_HELLOINTERVAL
    KWE(T_HELLOINTERVAL, "hello-interval"),
    KWE(T_HELLOINTERVAL, "hellointerval"),
#endif	/* T_HELLOINTERVAL */
#ifdef	T_HIGHER
    KWE(T_HIGHER, "higher"),
#endif	/* T_HIGHER */
#ifdef	T_HOLDTIME
    KWE(T_HOLDTIME, "holdtime"),
#endif	/* T_HOLDTIME */
#ifdef	T_HOST
    KWE(T_HOST, "host"),
#endif	/* T_HOST */
#ifdef	T_ICMP
    KWE(T_ICMP, "icmp"),
#endif	/* T_ICMP */
#ifdef	T_IDPR
    KWE(T_IDPR, "idpr"),
#endif	/* T_IDPR */
#ifdef	T_IFLIST
    KWE(T_IFLIST, "iflist"),
    KWE(T_IFLIST, "if-list"),
    KWE(T_IFLIST, "interfacelist"),
    KWE(T_IFLIST, "interface-list"),
#endif	/* T_IFLIST */
#ifdef	T_IGMP
    KWE(T_IGMP, "igmp"),
#endif	/* T_IGMP */
#ifdef	T_IGNORE
    KWE(T_IGNORE, "ignore"),
#endif	/* T_IGNORE */
#ifdef T_IGNOREFIRSTASHOP
	KWE(T_IGNOREFIRSTASHOP, "ignorefirstashop"),
#endif /* T_IGNOREFIRSTASHOP */
#ifdef	T_IGP
    KWE(T_IGP,	"igp"),
#endif	/* T_IGP */
#ifdef	T_IMPORT
    KWE(T_IMPORT, "import"),
#endif	/* T_IMPORT */
#ifdef	T_INCOMPLETE
    KWE(T_INCOMPLETE, "incomplete"),
#endif	/* T_INCOMPLETE */
#ifdef	T_INDELAY
    KWE(T_INDELAY, "indelay"),
    KWE(T_INDELAY, "in-delay"),
#endif	/* T_INDELAY */
#ifdef	T_INELIGIBLE
    KWE(T_INELIGIBLE, "ineligible"),
#endif	/* T_INELIGIBLE */
#ifdef	T_INET
    KWE(T_INET,	"inet"),
#endif	/* T_INET */
#ifdef	T_INFINITY
    KWE(T_INFINITY, "infinity"),
    KWE(T_INFINITY, "unreachable"),
#endif	/* T_INFINITY */
#ifdef	T_INFO
    KWE(T_INFO, "info"),
#endif	/* T_INFO */
#ifdef	T_INFTRANSDELAY
    KWE(T_INFTRANSDELAY, "inftransdelay"),
    KWE(T_INFTRANSDELAY, "transit-delay"),
    KWE(T_INFTRANSDELAY, "transitdelay"),
#endif	/* T_INFTRANSDELAY */
#ifdef	T_INSTANCE
    KWE(T_INSTANCE, "instance"),
#endif	/* T_INSTANCE */
#ifdef	T_INTDOMINFO
    KWE(T_INTDOMINFO,	"internaldomaininfo"),
    KWE(T_INTDOMINFO,	"internal-domain-info"),
#endif	/* T_INTDOMINFO */
#ifdef	T_INTERFACE
    KWE(T_INTERFACE, "interface"),
    KWE(T_INTERFACE, "intf"),
#endif	/* T_INTERFACE */
#ifdef	T_INTERFACES
    KWE(T_INTERFACES, "interfaces"),
    KWE(T_INTERFACES, "intfs"),
#endif	/* T_INTERFACES */
#ifdef	T_INTERIOR
    KWE(T_INTERIOR, "interior"),
#endif	/* T_INTERIOR */
#ifdef	T_INTERNAL
    KWE(T_INTERNAL, "internal"),
#endif	/* T_INTERNAL */
#ifdef	T_IP
    KWE(T_IP,	"ip"),
#endif	/* T_IP */
#ifdef	T_IPREACH
    KWE(T_IPREACH,	"ip-reachability"),
    KWE(T_IPREACH,	"ipreachability"),
#endif	/* T_IPREACH */
#ifdef	T_ISIS
    KWE(T_ISIS,	"is-is"),
    KWE(T_ISIS,	"isis"),
#endif	/* T_ISIS */
#ifdef	T_ISO
    KWE(T_ISO,	"iso"),
#endif	/* T_ISO */
#ifdef	T_K
    KWE(T_K, "k"),
#endif	/* T_K */
#ifdef	T_KEEPALIVE
    KWE(T_KEEPALIVE, "keepalive"),
#endif	/* T_KEEPALIVE */
#ifdef	T_KEEPALIVESALWAYS
    KWE(T_KEEPALIVESALWAYS, "keepalivesalways"),
#endif	/* T_KEEPALIVESALWAYS */
#ifdef	T_KEEP
    KWE(T_KEEP, "keep"),
#endif	/* T_KEEP */
#ifdef	T_KERNEL
    KWE(T_KERNEL, "kernel"),
#endif	/* T_KERNEL */
#ifdef	T_LCLADDR
    KWE(T_LCLADDR, "localaddress"),
    KWE(T_LCLADDR, "lcl-addr"),
    KWE(T_LCLADDR, "lcladdr"),
    KWE(T_LCLADDR, "local-address"),
#endif	/* T_LCLADDR */
#ifdef	T_LEVEL
    KWE(T_LEVEL,	"level"),
#endif	/* T_LEVEL */
#ifdef	T_LIFETIME
    KWE(T_LIFETIME,	"lifetime"),
#endif	/* T_LIFETIME */
#ifdef	T_LIMIT
     KWE(T_LIMIT, "limit"),
#endif	/* T_LIMIT */
#ifdef	T_LISTEN
    KWE(T_LISTEN, "listen"),
#endif	/* T_LISTEN */
#ifdef	T_LOCALAS
    KWE(T_LOCALAS, "localas" ),
    KWE(T_LOCALAS, "asout"),
#endif	/* T_LOCALAS */
#ifdef	T_LOGUPDOWN
    KWE(T_LOGUPDOWN, "logupdown"),
#endif	/* T_LOGUPDOWN */
#ifdef	T_LOOPS
    KWE(T_LOOPS, "loops"),
#endif	/* T_LOOPS */
#ifdef	T_LOWER
    KWE(T_LOWER, "lower"),
#endif	/* T_LOWER */
#ifdef	T_LSA_BLD
    KWE(T_LSA_BLD, "lsa-build"),
    KWE(T_LSA_BLD, "lsabuild"),
#endif	/* T_LSA_BLD */
#ifdef	T_LSA_RX
    KWE(T_LSA_RX, "lsa-receive"),
    KWE(T_LSA_RX, "lsa-rx"),
    KWE(T_LSA_RX, "lsareceive"),
    KWE(T_LSA_RX, "lsarx"),
#endif	/* T_LSA_RX */
#ifdef	T_LSA_TX
    KWE(T_LSA_TX, "lsa-transmit"),
    KWE(T_LSA_TX, "lsa-tx"),
    KWE(T_LSA_TX, "lsatransmit"),
    KWE(T_LSA_TX, "lsatx"),
#endif	/* T_LSA_TX */
#ifdef	T_M
    KWE(T_M, "m"),
#endif	/* T_M */
#ifdef	T_MARK
    KWE(T_MARK, "mark"),
#endif	/* T_MARK */
#ifdef	T_MARTIANS
    KWE(T_MARTIANS, "martians"),
#endif	/* T_MARTIANS */
#ifdef	T_MASK
    KWE(T_MASK, "mask"),
#endif	/* T_MASK */
#ifdef	T_MASKLEN
    KWE(T_MASKLEN, "masklen"),
    KWE(T_MASKLEN, "mask-len"),
    KWE(T_MASKLEN, "mask-length"),
    KWE(T_MASKLEN, "masklength"),
#endif	/* T_MASKLEN */
#ifdef	T_MAXUP
    KWE(T_MAXUP, "maxup"),
#endif	/* T_MAXUP */
#ifdef	T_MAXADVINTERVAL
    KWE(T_MAXADVINTERVAL, "maxadvertisementinterval"),
    KWE(T_MAXADVINTERVAL, "maxadvinterval"),
#endif	/* T_MAXADVINTERVAL */
#ifdef	T_MD5
    KWE(T_MD5, "md5"),
#endif	/* T_MD5 */
#ifdef	T_METRIC
    KWE(T_METRIC, "cost"),
    KWE(T_METRIC, "distance"),
    KWE(T_METRIC, "metric"),
#endif	/* T_METRIC */
#ifdef	T_METRICIN
    KWE(T_METRICIN, "metricin"),
#endif	/* T_METRICIN */
#ifdef	T_METRICOUT
    KWE(T_METRICOUT, "metricout"),
#endif	/* T_METRICOUT */
#ifdef	T_MINADVINTERVAL
    KWE(T_MINADVINTERVAL, "minadvertisementinterval"),
    KWE(T_MINADVINTERVAL, "minadvinterval"),
#endif	/* T_MINADVINTERVAL */
#ifdef	T_MODE
    KWE(T_MODE,	"mode"),
#endif	/* T_MODE */
#ifdef	T_MONITORAUTH
    KWE(T_MONITORAUTH,	"mon-auth"),
    KWE(T_MONITORAUTH,	"monauth"),
    KWE(T_MONITORAUTH,	"monitor-authentication"),
    KWE(T_MONITORAUTH,	"monitorauthentication"),
#endif	/* T_MONITORAUTH */
#ifdef	T_MONITORAUTHKEY
    KWE(T_MONITORAUTHKEY,	"mon-auth-key"),
    KWE(T_MONITORAUTHKEY,	"monauthkey"),
    KWE(T_MONITORAUTHKEY,	"monitor-authentication-key"),
    KWE(T_MONITORAUTHKEY,	"monitorauthenticationkey"),
#endif	/* T_MONITORAUTHKEY */
#ifdef	T_MULTICAST
    KWE(T_MULTICAST,	"multicast" ),
#endif	/* T_MULTICAST */
#ifdef	T_NEIGHBOR
    KWE(T_NEIGHBOR, "neighbor"),
    KWE(T_NEIGHBOR, "neighbour"),
    KWE(T_NEIGHBOR, "peer"),
#endif	/* T_NEIGHBOR */
#ifdef	T_NEIGHBORID
    KWE(T_NEIGHBORID, "neighborid"),
    KWE(T_NEIGHBORID, "neighbourid"),
    KWE(T_NEIGHBORID, "neighbor-id"),
    KWE(T_NEIGHBORID, "neighbour-id"),
#endif	/* T_NEIGHBORID */
#ifdef	T_NETMASK
    KWE(T_NETMASK,		"netmask" ),
    KWE(T_NETMASK,		"net-mask" ),
    KWE(T_NETMASK,		"subnetmask" ),
#endif	/* T_NETMASK */
#ifdef	T_NETWORKS
    KWE(T_NETWORKS, "networks"),
#endif	/* T_NETWORKS */
#ifdef	T_NOAGGRID
    KWE(T_NOAGGRID, "noaggregatorid"),
    KWE(T_NOAGGRID, "no-aggregator-id"),
#endif	/* T_NOAGGRID */
#ifdef	T_NOAUTHCHECK
    KWE(T_NOAUTHCHECK, "noauthcheck"),
    KWE(T_NOAUTHCHECK, "noauthenticationcheck"),
    KWE(T_NOAUTHCHECK, "no-authentication-check"),
#endif	/* T_NOAUTHCHECK */
#ifdef	T_NOCHANGE
    KWE(T_NOCHANGE, "nochange"),
    KWE(T_NOCHANGE, "deleteaddonly"),
#endif	/* T_NOCHANGE */
#ifdef	T_NOCHECKZERO
    KWE(T_NOCHECKZERO, "no-check-zero"),
    KWE(T_NOCHECKZERO, "nocheckzero"),
#endif	/* T_NOCHECKZERO */
#ifdef	T_NODE
    KWE(T_NODE, "node"),
#endif	/* T_NODE */
#ifdef	T_NODEMASK
    KWE(T_NODEMASK, "nodemask"),
    KWE(T_NODEMASK, "node-mask"),
#endif	/* T_NODEMASK */
#ifdef	T_NOFLUSHATEXIT
    KWE(T_NOFLUSHATEXIT, "noflushatexit"),
    KWE(T_NOFLUSHATEXIT, "noflush"),
#endif	/* T_NOFLUSHATEXIT */
#ifdef	T_NOGENDEFAULT
    KWE(T_NOGENDEFAULT, "nogendefault"),
#endif	/* T_NOGENDEFAULT */
#ifdef	T_NOHELLOIN
    KWE(T_NOHELLOIN, "nohelloin"),
#endif	/* T_NOHELLOIN */
#ifdef	T_NOHELLOOUT
    KWE(T_NOHELLOOUT, "nohelloout"),
#endif	/* T_NOHELLOOUT */
#ifdef	T_NOINSTALL
    KWE(T_NOINSTALL, "noinstall"),
#endif	/* T_NOINSTALL */
#ifdef	T_NOMULTICAST
    KWE(T_NOMULTICAST, "nomulticast"),
#endif	/* T_NOMULTICAST */
#ifdef	T_NONBROADCAST
    KWE(T_NONBROADCAST, "nbma"),
    KWE(T_NONBROADCAST, "no-broadcast"),
    KWE(T_NONBROADCAST, "nobroadcast"),
    KWE(T_NONBROADCAST, "non-broadcast"),
    KWE(T_NONBROADCAST, "nonbroadcast"),
#endif	/* T_NONBROADCAST */
#ifdef	T_NONE
    KWE(T_NONE, "none"),
#endif	/* T_NONE */
#ifdef	T_NOREDIRECTS
    KWE(T_NOREDIRECTS, "noredirects"),
#endif	/* T_NOREDIRECTS */
#ifdef	T_NORESOLV
    KWE(T_NORESOLV, "noresolv"),
    KWE(T_NORESOLV, "noresolve"),
#endif	/* T_NORESOLV */
#ifdef	T_NORIPIN
    KWE(T_NORIPIN, "noripin"),
#endif	/* T_NORIPIN */
#ifdef	T_NORIPOUT
    KWE(T_NORIPOUT, "noripout"),
#endif	/* T_NORIPOUT */
#ifdef	T_NORMAL
    KWE(T_NORMAL, "normal"),
#endif	/* T_NORMAL */
#ifdef	T_NOSEND
    KWE(T_NOSEND, "nosend"),
#endif	/* T_NOSEND */
#ifdef	T_NOSTAMP
    KWE(T_NOSTAMP, "nostamp"),
#endif	/* T_NOSTAMP */
#ifdef	T_NOTICE
    KWE(T_NOTICE, "notice"),
#endif	/* T_NOTICE */
#ifdef	T_NOV4ASLOOP
    KWE(T_NOV4ASLOOP, "nov4asloop"),
#endif	/* T_NOV4ASLOOP */
#ifdef	T_OFF
    KWE(T_OFF, "no"),
    KWE(T_OFF, "off"),
#endif	/* T_OFF */
#ifdef	T_ON
    KWE(T_ON, "on"),
    KWE(T_ON, "yes"),
#endif	/* T_ON */
#ifdef	T_OPEN
    KWE(T_OPEN, "open"),
#endif	/* T_OPEN */
#ifdef	T_OPTIONS
    KWE(T_OPTIONS, "options"),
#endif	/* T_OPTIONS */
#ifdef	T_ORIGIN
    KWE(T_ORIGIN,	"origin"),
#endif	/* T_ORIGIN */
#ifdef	T_OSPF
    KWE(T_OSPF, "ospf"),
#endif	/* T_OSPF */
#ifdef	T_OSPF_ASE
    KWE(T_OSPF_ASE, "ospf-ase"),
    KWE(T_OSPF_ASE, "ospfase"),
#endif	/* T_OSPF_ASE */
#ifdef	T_OTHER
    KWE(T_OTHER, "other"),
#endif	/* T_OTHER */
#ifdef	T_OUTDELAY
    KWE(T_OUTDELAY, "outdelay"),
    KWE(T_OUTDELAY, "out-delay"),
#endif	/* T_OUTDELAY */
#ifdef	T_P1
    KWE(T_P1, "minhello"),
    KWE(T_P1, "p1"),
#endif	/* T_P1 */
#ifdef	T_P2
    KWE(T_P2, "minpoll"),
    KWE(T_P2, "p2"),
#endif	/* T_P2 */
#ifdef	T_PACKETS
    KWE(T_PACKETS, "packets"),
#endif	/* T_PACKETS */
#ifdef	T_PARSE
    KWE(T_PARSE, "parse"),
#endif	/* T_PARSE */
#ifdef	T_PASSIVE
    KWE(T_PASSIVE, "passive"),
#endif	/* T_PASSIVE */
#ifdef	T_PEERAS
    KWE(T_PEERAS, "asin"),
    KWE(T_PEERAS, "peeras" ),
#endif	/* T_PEERAS */
#ifdef	T_PKTSIZE
    KWE(T_PKTSIZE, "packet-size"),
    KWE(T_PKTSIZE, "packetsize"),
#endif	/* T_PKTSIZE */
#ifdef	T_POINTOPOINT
    KWE(T_POINTOPOINT,	"p2p"),
    KWE(T_POINTOPOINT,	"point2point"),
    KWE(T_POINTOPOINT,	"pointopoint"),
    KWE(T_POINTOPOINT,	"pointtopoint"),
#endif	/* T_POINTOPOINT */
#ifdef	T_POLICY
    KWE(T_POLICY, "policy"),
#endif	/* T_POLICY */
#ifdef	T_POLLINTERVAL
    KWE(T_POLLINTERVAL, "poll-interval"),
    KWE(T_POLLINTERVAL, "pollinterval"),
#endif	/* T_POLLINTERVAL */
#ifdef	T_PORT
    KWE(T_PORT, "port"),
#endif	/* T_PORT */
#ifdef	T_PREFERENCE
    KWE(T_PREFERENCE, "pref"),
    KWE(T_PREFERENCE, "preference"),
#endif	/* T_PREFERENCE */
#ifdef	T_PREFERENCE2
    KWE(T_PREFERENCE2, "pref2"),
    KWE(T_PREFERENCE2, "preference2"),
#endif	/* T_PREFERENCE2 */
#ifdef	T_PREFIX
    KWE(T_PREFIX,	"prefix"),
#endif	/* T_PREFIX */
#ifdef	T_PRIORITY
    KWE(T_PRIORITY,	"priority"),
#endif	/* T_PRIORITY */
#ifdef	T_PROTO
    KWE(T_PROTO, "proto"),
    KWE(T_PROTO, "protocol"),
#endif	/* T_PROTO */
#ifdef	T_QUERY
    KWE(T_QUERY, "query"),
#endif	/* T_QUERY */
#ifdef	T_QUIET
    KWE(T_QUIET, "quiet"),
#endif	/* T_QUIET */
#ifdef	T_RECEIVE
    KWE(T_RECEIVE, "receive"),
    KWE(T_RECEIVE, "recv"),
    KWE(T_RECEIVE, "rx"),
#endif	/* T_RECEIVE */
#ifdef	T_RECVBUF
    KWE(T_RECVBUF, "recv-buffer"),
    KWE(T_RECVBUF, "recvbuf"),
    KWE(T_RECVBUF, "recvbuffer"),
#endif	/* T_RECVBUF */
#ifdef	T_REDIRECT
    KWE(T_REDIRECT, "redirect"),
#endif	/* T_REDIRECT */
#ifdef	T_REDIRECTS
    KWE(T_REDIRECTS, "redirects"),
#endif	/* T_REDIRECTS */
#ifdef	T_REFINE
    KWE(T_REFINE, "refine"),
    KWE(T_REFINE, "refines"),
#endif	/* T_REFINE */
#ifdef	T_REGISTER
    KWE(T_REGISTER, "register"),
#endif	/* T_REGISTER */
#ifdef	T_REJECT
    KWE(T_REJECT, "reject"),
#endif	/* T_REJECT */
#ifdef	T_REMNANTHOLDTIME
    KWE(T_REMNANTHOLDTIME, "remnantholdtime"),
#endif	/* T_REMNANTHOLDTIME */
#ifdef	T_REMNANTS
    KWE(T_REMNANTS, "remnants"),
#endif	/* T_REMNANTS */
#ifdef	T_REPLACE
    KWE(T_REPLACE, "replace"),
#endif	/* T_REPLACE */
#ifdef	T_REQUEST
    KWE(T_REQUEST, "req"),
    KWE(T_REQUEST, "request"),
#endif	/* T_REQUEST */
#ifdef	T_RESOLVE
    KWE(T_RESOLVE, "resolv"),
    KWE(T_RESOLVE, "resolve"),
#endif	/* T_RESOLVE */
#ifdef	T_RESPONSE
    KWE(T_RESPONSE, "response"),
#endif	/* T_RESPONSE */
#ifdef	T_RESTRICT
    KWE(T_RESTRICT, "restrict"),
#endif	/* T_RESTRICT */
#ifdef	T_RETAIN
    KWE(T_RETAIN, "retain"),
#endif	/* T_RETAIN */
#ifdef	T_RIP
    KWE(T_RIP, "rip"),
#endif	/* T_RIP */
#ifdef	T_RIPIN
    KWE(T_RIPIN, "ripin"),
#endif	/* T_RIPIN */
#ifdef	T_RIPOUT
    KWE(T_RIPOUT, "ripout"),
#endif	/* T_RIPOUT */
#ifdef	T_ROUTE
    KWE(T_ROUTE, "route"),
#endif	/* T_ROUTE */
#ifdef	T_ROUTES
    KWE(T_ROUTES, "routes"),
#endif	/* T_ROUTES */
#ifdef	T_ROUTING
    KWE(T_ROUTING, "routing"),
#endif	/* T_ROUTING */
#ifdef	T_ROUTERDEADINTERVAL
    KWE(T_ROUTERDEADINTERVAL, "router-dead-interval"),
    KWE(T_ROUTERDEADINTERVAL, "routerdeadinterval"),
#endif	/* T_ROUTERDEADINTERVAL */
#ifdef	T_ROUTERDISCOVERY
    KWE(T_ROUTERDISCOVERY, "router-discovery"),
    KWE(T_ROUTERDISCOVERY, "routerdiscovery"),
#endif	/* T_ROUTERDISCOVERY */
#ifdef	T_ROUTERID
    KWE(T_ROUTERID, "router-id"),
    KWE(T_ROUTERID, "routerid"),
#endif	/* T_ROUTERID */
#ifdef	T_ROUTERS
    KWE(T_ROUTERS, "routers"),
#endif	/* T_ROUTERS */
#ifdef	T_RXMITINTERVAL
    KWE(T_RXMITINTERVAL, "retransmit-interval"),
    KWE(T_RXMITINTERVAL, "retransmitinterval"),
    KWE(T_RXMITINTERVAL, "rxmtinterval"),
#endif	/* T_RXMITINTERVAL */
#ifdef	T_SCANINTERVAL
    KWE(T_SCANINTERVAL, "scan-interval"),
    KWE(T_SCANINTERVAL, "scaninterval"),
#endif	/* T_SCANINTERVAL */
#ifdef	T_SECONDARY
    KWE(T_SECONDARY, "secondary"),
#endif	/* T_SECONDARY */
#ifdef	T_SEND
    KWE(T_SEND, "send"),
#endif	/* T_SEND */
#ifdef	T_SENDBUF
    KWE(T_SENDBUF, "send-buffer"),
    KWE(T_SENDBUF, "sendbuf"),
    KWE(T_SENDBUF, "sendbuffer"),
#endif	/* T_SENDBUF */
#ifdef	T_SERVER
    KWE(T_SERVER, "server"),
#endif	/* T_SERVER */
#ifdef	T_SET
    KWE(T_SET,	"set"),
#endif	/* T_SET */
#ifdef	T_SETPREF
    KWE(T_SETPREF,	"setpref"),
#endif	/* T_SETPREF */
#ifdef	T_SIMPLE
    KWE(T_SIMPLE, "simple"),
    KWE(T_SIMPLE, "simple-password"),
    KWE(T_SIMPLE, "simplepassword"),
#endif	/* T_SIMPLE */
#ifdef	T_SIMPLEX
    KWE(T_SIMPLEX, "simplex"),
#endif	/* T_SIMPLEX */
#ifdef	T_SIZE
    KWE(T_SIZE, "size"),
#endif	/* T_SIZE */
#ifdef	T_SLSP
    KWE(T_SLSP, "slsp"),
#endif	/* T_SLSP */
#ifdef	T_SNMP
    KWE(T_SNMP, "snmp"),
#endif	/* T_SNMP */
#ifdef	T_SNPA
    KWE(T_SNPA,	"snpa"),
#endif	/* T_SNPA */
#ifdef	T_SOLICIT
    KWE(T_SOLICIT, "solicit"),
#endif	/* T_SOLICIT */
#ifdef	T_SOURCEGATEWAYS
    KWE(T_SOURCEGATEWAYS, "sourcegateways"),
#endif	/* T_SOURCEGATEWAYS */
#ifdef	T_SOURCENET
    KWE(T_SOURCENET, "sourcenet"),
#endif	/* T_SOURCENET */
#ifdef	T_SPF
    KWE(T_SPF, "spf"),
#endif	/* T_SPF */
#ifdef	T_STATE
    KWE(T_STATE, "state"),
#endif	/* T_STATE */
#ifdef	T_STATIC
    KWE(T_STATIC, "static"),
#endif	/* T_STATIC */
#ifdef	T_STRICTIFS
    KWE(T_STRICTIFS, "strict-interfaces"),
    KWE(T_STRICTIFS, "strict-intfs"),
    KWE(T_STRICTIFS, "strictinterfaces"),
    KWE(T_STRICTIFS, "strictintfs"),
#endif	/* T_STRICTIFS */
#ifdef	T_STUB
    KWE(T_STUB, "stub"),
#endif	/* T_STUB */
#ifdef	T_STUBHOSTS
    KWE(T_STUBHOSTS, "stub-hosts"),
    KWE(T_STUBHOSTS, "stubhosts"),
#endif	/* T_STUBHOSTS */
#ifdef	T_SYMBOLS
    KWE(T_SYMBOLS, "symbols"),
#endif	/* T_SYMBOLS */
#ifdef	T_SYSLOG
    KWE(T_SYSLOG, "syslog"),
#endif	/* T_SYSLOG */
#ifdef	T_SYSTEMID
    KWE(T_SYSTEMID,	"system-id"),
    KWE(T_SYSTEMID,	"systemid"),
#endif	/* T_SYSTEMID */
#ifdef	T_TAG
    KWE(T_TAG, "tag"),
#endif	/* T_TAG */
#ifdef	T_TASK
    KWE(T_TASK, "task"),
#endif	/* T_TASK */
#ifdef	T_TEST
    KWE(T_TEST, "test" ),
#endif	/* T_TEST */
#ifdef	T_THRESHOLD
    KWE(T_THRESHOLD, "threshold"),
#endif	/* T_THRESHOLD */
#ifdef	T_TIMER
    KWE(T_TIMER, "timer"),
#endif	/* T_TIMER */
#ifdef	T_TRACEOPTIONS
    KWE(T_TRACEOPTIONS, "traceoptions"),
#endif	/* T_TRACEOPTIONS */
#ifdef	T_TRANSITAREA
    KWE(T_TRANSITAREA, "transit-area"),
    KWE(T_TRANSITAREA, "transitarea"),
#endif	/* T_TRANSITAREA */
#ifdef	T_TRAP
    KWE(T_TRAP, "trap"),
#endif	/* T_TRAP */
#ifdef	T_TROLL
    KWE(T_TROLL,	"troll"),
#endif	/* T_TROLL */
#ifdef	T_TRUSTEDGATEWAYS
    KWE(T_TRUSTEDGATEWAYS, "trustedgateways"),
#endif	/* T_TRUSTEDGATEWAYS */
#ifdef	T_TTL
    KWE(T_TTL, "ttl"),
#endif	/* T_TTL */
#ifdef	T_TUNNEL
    KWE(T_TUNNEL, "tunnel"),
#endif	/* T_TUNNEL */
#ifdef	T_TYPE
    KWE(T_TYPE, "type"),
#endif	/* T_TYPE */
#ifdef	T_UNREACH
    KWE(T_UNREACH, "unreach"),
#endif	/* T_UNREACH */
#ifdef	T_UPDATE
    KWE(T_UPDATE, "update"),
#endif	/* T_UPDATE */
#ifdef	T_UPTO
    KWE(T_UPTO,	"upto"),
#endif	/* T_UPTO */
#ifdef	T_V3ASLOOPOKAY
    KWE(T_V3ASLOOPOKAY, "v3asloopokay"),
    KWE(T_V3ASLOOPOKAY, "v2asloopokay"),
#endif	/* T_V3ASLOOPOKAY */
#ifdef	T_VERSION
    KWE(T_VERSION, "version"),
#endif	/* T_VERSION */
#ifdef	T_VIRTUALLINK
    KWE(T_VIRTUALLINK, "virtual-link"),
    KWE(T_VIRTUALLINK, "virtuallink"),
#endif	/* T_VIRTUALLINK */
#ifdef	T_WARNING
    KWE(T_WARNING, "warning"),
#endif	/* T_WARNING */
/* End Sort */
#if	YYDEBUG != 0
    KWE(T_YYDEBUG, "yydebug"),
    KWE(T_YYQUIT, "yyquit"),
    KWE(T_YYSTATE, "yystate"),
#endif	/* YYDEBUG != 0 */
    KWE(0, "")
};


/*
 * We also do token-keyword lookups for error diagnostics.  We keep a separate
 * array of sorted pointers in token value order to allow binary
 * searches.  This array will hold the pointers.
 */
#define	N_KEYWORDS	(sizeof(keywords)/sizeof(parse_key_token) - 1)

static u_int n_kw_tokens;
static parse_key_token *keyword_tokens[N_KEYWORDS];





/*
 * parse_keyword_lookup - given a token, return the corresponding keyword
 *			  string.  Do a binary search to find it.
 */
const char *
parse_keyword_lookup __PF1(token, int)
{
    register int t = token;
    register int u, l, i;

    l = 0;
    u = n_kw_tokens - 1;
    do {
	i = (u + l) / 2;
	if (keyword_tokens[i]->pkw_token < t) {
	    l = i + 1;
	} else if (keyword_tokens[i]->pkw_token > t) {
	    u = i - 1;
	} else {
	    return (keyword_tokens[i]->pkw_keyword);
	}
    } while (u >= l);

    return ((const char *) 0);
}


/*
 * parse_keyword - given a string return the keyword token for it
 */
int
parse_keyword __PF2(str, char *,
		    slen, u_int)
{
    register char *cp;
    register parse_key_token *pkp;
    register u_short bit;
    register u_short my_bit;
    register const char *sp;

    /*
     * Fetch the length.  If doing this case-insensitive, convert
     * to lower case.
     */
#ifndef	CASE_SENSITIVE
    char buf[PKW_MAX_LEN+1];

    if (slen >= kw_max_len) {
	return NO_TOKEN;
    }

    sp = str;
    cp = buf;
    do {
	*cp = parse_tolower[(byte)(*sp++)];
    } while (*cp++);

    cp = buf;
#else	/* CASE_SENSITIVE */
    if (slen >= kw_max_len) {
	return NO_TOKEN;
    }
    cp = str;
#endif	/* CASE_SENSITIVE */

    /*
     * Okay, we have the start of the string.  Search the tree
     * for our candidate.  Note that if we get into parts of the
     * tree where the mask is larger than my_bit we won't find
     * anything.
     */
    my_bit = ((u_short)(slen + 1)) << 8;
    pkp = kw_tree_head;
    assert(pkp);
    bit = PKW_NOBIT;
    while (bit < pkp->pkw_bit) {
	bit = pkp->pkw_bit;
	if (bit > my_bit) {
	    return NO_TOKEN;
	}
	if (PKW_BITTEST(cp, bit)) {
	    pkp = pkp->pkw_right;
	} else {
	    pkp = pkp->pkw_left;
	}
    }

    /*
     * pkp now points at the only candidate in the tree which could be
     * our guy.  If their lengths don't match they don't match.  Otherwise
     * do a character-by-character comparison.
     */
    my_bit >>= 8;
    if (my_bit != pkp->pkw_len) {
	return NO_TOKEN;
    }

    sp = pkp->pkw_keyword;
    do {
	if (*cp++ != *sp++) {
	    return NO_TOKEN;
	}
    } while ((--my_bit));

    /*
     * Matched it.
     */
    return pkp->pkw_token;
}


/*
 * parse_token_sort_compare - comparison routine for ordering by token value
 */
static int
parse_token_sort_compare __PF2(p1, const VOID_T,
			       p2, const VOID_T)
{
    return ((*(parse_key_token * const *)p1)->pkw_token
	    - (*(parse_key_token * const *)p2)->pkw_token);
}


/*
 * parse_keyword_sort - prepare the keyword list for use.  We quicksort
 *			the token->keyword array and build a patricia tree
 *			for the keyword->token search.
 */
static void
parse_keyword_sort __PF0(void)
{
    register parse_key_token *pkp;

    /*
     * Only do this once
     */
    if (kw_tree_head) {
	return;
    }

    {
	register int otoken;
	register parse_key_token **pkpp = keyword_tokens;

	n_kw_tokens = 0;
	pkp = keywords;
	otoken = NO_TOKEN;
	while (pkp->pkw_token) {
	    if (pkp->pkw_token != otoken) {
		otoken = pkp->pkw_token;
		*pkpp++ = pkp;
		n_kw_tokens++;
	    }
	    pkp++;
	}
	assert(n_kw_tokens);

	qsort((caddr_t) keyword_tokens,
	      n_kw_tokens,
	      sizeof(parse_key_token *),
	      parse_token_sort_compare);
    }
    {
	register parse_key_token *pkp_prev, *pkp_curr;
	register u_short bit;
	register u_short len;
	register const char *cp, *sp;

	/*
	 * Just add the first keyword as our first root node.
	 */
	pkp = keywords;
	pkp->pkw_left = pkp->pkw_right = pkp;
	pkp->pkw_bit = PKW_NOBIT;
	kw_max_len = pkp->pkw_len;
	kw_tree_head = pkp;

	/*
	 * Now march down the list, adding each entry to the tree as
	 * we go.
	 */
	while ((++pkp)->pkw_token) {
	    /*
	     * We need the length to make sure we don't search past
	     * the end of the key.  Find it now.
	     */
	    len = pkp->pkw_len;
	    if (len > kw_max_len) {
		kw_max_len = len;
	    }
	    len <<= 8;

	    /*
	     * Search the tree to find out where this guy leads.  Assume
	     * each string is zero-terminated.
	     */
	    pkp_curr = kw_tree_head;
	    bit = PKW_NOBIT;
	    cp = pkp->pkw_keyword;
	    while (bit < pkp_curr->pkw_bit) {
		bit = pkp_curr->pkw_bit;
		if (bit < len && PKW_BITTEST(cp, bit)) {
		    pkp_curr = pkp_curr->pkw_right;
		} else {
		    pkp_curr = pkp_curr->pkw_left;
		}
	    }

	    /*
	     * This will have led us to a leaf.  Find the first bit
	     * which differs.
	     */
	    sp = pkp_curr->pkw_keyword;
	    while (*cp == *sp) {
		/*
		 * If you get this assertion failure you have two
		 * identical keywords in the tree.  Take a look at
		 * pkp->pkw_keyword and pkp_curr->pkw_keyword
		 */
		assert(*cp);
		cp++, sp++;
	    }
	    bit = PKW_MAKEBIT((cp - pkp->pkw_keyword),
			      parse_low_bit_set[(byte)((*cp) ^ (*sp))]);
	    pkp->pkw_bit = bit;

	    /*
	     * Here we have the bit number for the internal node
	     * we need to insert.  Search down the tree until we
	     * find either an end node (i.e. an upward link) or
	     * a node with a bit number larger than ours.  We insert
	     * our node above this, making one of the pointers point
	     * at the node itself.
	     */
	    pkp_prev = (parse_key_token *) 0;
	    pkp_curr = kw_tree_head;
	    cp = pkp->pkw_keyword;
	    len = PKW_NOBIT;
	    while (len < pkp_curr->pkw_bit) {
		len = pkp_curr->pkw_bit;
		if (bit <= len) {
		    break;
		}
		pkp_prev = pkp_curr;
		if (PKW_BITTEST(cp, len)) {
		    pkp_curr = pkp_curr->pkw_right;
		} else {
		    pkp_curr = pkp_curr->pkw_left;
		}
	    }

	    /*
	     * Got our guy.  Point one point at the current node, the
	     * other at ourselves.
	     */
	    if (PKW_BITTEST(cp, bit)) {
		pkp->pkw_right = pkp;
		pkp->pkw_left = pkp_curr;
	    } else {
		pkp->pkw_left = pkp;
		pkp->pkw_right = pkp_curr;
	    }

	    /*
	     * Point whatever pointer above used to point at the current
	     * node at the new node instead.
	     */
	    if (!pkp_prev) {
		/* New root node */
		kw_tree_head = pkp;
	    } else if (pkp_prev->pkw_right == pkp_curr) {
		pkp_prev->pkw_right = pkp;
	    } else {
		pkp_prev->pkw_left = pkp;
	    }
	}
    }
}


/*
 *	Front end for yyparse().  Exit if any errors
 */
int
parse_parse __PF1(file, const char *)
{
    int errors;
    static int first_parse = TRUE;

    if (first_parse) {
	parse_keyword_sort();		/* Sort the keyword table */

    }
    parse_directory = task_mem_strdup((task *) 0, task_path_start);

    sethostent(1);
    setnetent(1);

    if (parse_open(task_mem_strdup((task *) 0, file))) {
	/* Indicate problem opening file */
	
	errors = -1;
    } else {
	protos_seen = (rtbit_mask) 0;
	parse_state = PS_INITIAL;
	switch (errors = yyparse()) {
	case 0:
	    errors = yynerrs;
	    break;

	default:
	    errors = yynerrs ? yynerrs : 1;
	}
    }

    endhostent();
    endnetent();

    if (parse_directory) {
	task_mem_free((task *) 0, parse_directory);
	parse_directory = (char *) 0;
    }
	
    if (errors) {
	int errs = ABS(errors);
	
	trace_log_tf(trace_global,
		     TRC_NL_BEFORE|TRC_NL_AFTER,
		     LOG_ERR,
		     ("parse_parse: %d parse error%s",
		      errs,
		      errs > 1 ? "s" : ""));
    }
    first_parse = FALSE;

    return errors;
}


/*
 *	Format a message indicating the current line number and return
 *	a pointer to it.
 */
char *
parse_where __PF0(void)
{
    static char where[LINE_MAX];

    if (parse_filename) {
	(void) sprintf(where, "%s:%d",
		       parse_filename,
		       yylineno);
    } else {
	(void) sprintf(where, "%d",
		       yylineno);
    }

    return where;
}


/*
 *	Limit check a number
 */
int
parse_limit_check __PF4(type, const char *,
			value, u_int,
			lower, u_int,
			upper, u_int)
{
    if ((value < lower) || ((upper != (u_int) -1) && (value > upper))) {
	(void) sprintf(parse_error, "invalid %s value at '%u' not in range %u to %u",
		       type,
		       value,
		       lower,
		       upper);
	return 1;
    }
    trace_tf(trace_global,
	     TR_PARSE,
	     0,
	     ("parse: %s %s: %u",
	      parse_where(),
	      type,
	      value));
    return 0;
}


/*
 *	Look up a string as a host or network name, returning it's IP address
 *	in normalized network byte order.
 */
sockaddr_un *
parse_addr_hostname __PF2(hname, char *,
			  parse_errmsg, char *)
{
    struct hostent *hostent;
    const char *errmsg = 0;

    if (BIT_TEST(task_state, TASKS_NORESOLV)) {
	(void) sprintf(parse_errmsg,
		       "resolution of symbolic hostnames disabled at '%s'",
		       hname);
	return (sockaddr_un *) 0;
    }
    
    trace_tf(trace_global,
	     TR_PARSE,
	     0,
	     ("parse_addr_hostname: resolving %s",
	      hname));

    hostent = gethostbyname(hname);
    if (hostent) {
	switch (hostent->h_addrtype) {
#ifdef	PROTO_INET
	case AF_INET:
	{
	    struct in_addr addr;
#ifdef	h_addr
	    if (hostent->h_addr_list[1]) {
		/* XXX - For a gateway we could use just the address on our network if there was only one */
		(void) sprintf(parse_errmsg, "host has multiple addresses at '%s'",
			       hname);
		return (sockaddr_un *) 0;
	    }
#endif	/* h_addr */

	    bcopy(hostent->h_addr, (caddr_t) &addr.s_addr, (size_t) hostent->h_length);
	    return sockbuild_in(0, addr.s_addr);
	}
#endif	/* PROTO_INET */

	default:
	    /* XXX - Should we pretend it does not exist if it is not an INET name? */
	    (void) sprintf(parse_errmsg, "unsupported host family at '%s'",
			   hname);
	    return (sockaddr_un *) 0;
	}
    } else {
#ifdef	HOST_NOT_FOUND
	if (h_errno && (h_errno < h_nerr)) {
	    errmsg = h_errlist[h_errno];
	} else {
	    errmsg = "Unknown host";
	}
#else	/* HOST_NOT_FOUND */
	errmsg = "Unknown host";
#endif	/* HOST_NOT_FOUND */
    }
    (void) sprintf(parse_errmsg, "error resolving '%s': %s",
		   hname,
		   errmsg);
    return (sockaddr_un *) 0;
}


/*
 *	Look up a string as a host or network name, returning it's IP address
 *	in normalized network byte order.
 */
sockaddr_un *
parse_addr_netname __PF2(hname, char *,
			 parse_errmsg, char *)
{
    struct netent *netent;

    if (BIT_TEST(task_state, TASKS_NORESOLV)) {
	(void) sprintf(parse_errmsg,
		       "resolution of symbolic network names disabled at '%s'",
		       hname);
	return (sockaddr_un *) 0;
    }
    
    trace_tf(trace_global,
	     TR_PARSE,
	     0,
	     ("parse_addr_netname: resolving %s",
	      hname));

    netent = getnetbyname(hname);
    if (netent) {

	switch (netent->n_addrtype) {
#ifdef	PROTO_INET
	case AF_INET:
	{
	    register u_int32 net;

	    net = netent->n_net;
	    if (net) {
		while (!(net & 0xff000000)) {
		    net <<= 8;
		}
	    }
	    return sockbuild_in(0, htonl(net));
	}
#endif	/* PROTO_INET */

	default:
	    (void) sprintf(parse_errmsg, "unsupported network family at '%s'",
			   hname);
	    return (sockaddr_un *) 0;
	}
    } else {
	(void) sprintf(parse_errmsg, "error resolving '%s': network unknown",
		       hname);
    }
    return (sockaddr_un *) 0;
}


/*
 *	Append an advlist to another advlist
 */
int
parse_adv_append __PF2(list, adv_entry **,
		       new, adv_entry *)
{
    adv_entry *alo, *aln, *last = NULL;

    /* Add this network to the end of the list */
    if (*list) {
	ADV_LIST(*list, alo) {
	    if (!alo->adv_next) {
		last = alo;
	    }
	    /* Scan list for duplicates */
	    for (aln = new; aln; aln = aln->adv_next) {
		if (aln->adv_proto != alo->adv_proto
		    || (aln->adv_flag & ADVF_TYPE) != (alo->adv_flag & ADVF_TYPE)) {
		    continue;
		}
		switch (aln->adv_flag & ADVF_TYPE) {
		    case ADVFT_ANY:
			break;

		    case ADVFT_GW:
			if (aln->adv_gwp == alo->adv_gwp) {
			    (void) sprintf(parse_error, "duplicate gateway in list at '%A'",
					   aln->adv_gwp->gw_addr);
			    return TRUE;
			}
			break;

		    case ADVFT_IFN:
			if (aln->adv_ifn == alo->adv_ifae) {
			    (void) sprintf(parse_error, "duplicate interface in list at '%A'",
					   aln->adv_ifn->ifae_addr);
			    return TRUE;
			}
			break;

		    case ADVFT_IFAE:
			if (aln->adv_ifae == alo->adv_ifae) {
			    (void) sprintf(parse_error, "duplicate interface address in list at '%A'",
					   aln->adv_ifae->ifae_addr);
			    return TRUE;
			}
			break;

		    case ADVFT_AS:
			if (aln->adv_as == alo->adv_as) {
			    (void) sprintf(parse_error, "duplicate autonomous-system in list at '%u'",
					   aln->adv_as);
			    return TRUE;
			}
			break;

		    case ADVFT_DM:
			if (aln->adv_dm.dm_dest
			    && alo->adv_dm.dm_dest
			    && socktype(aln->adv_dm.dm_dest) != socktype(alo->adv_dm.dm_dest)) {
			    (void) sprintf(parse_error, "conflicting address family in dest/mask list at '%A mask %A'",
					   aln->adv_dm.dm_dest,
					   aln->adv_dm.dm_mask);
			    return TRUE;
			}
			if ((aln->adv_dm.dm_dest == alo->adv_dm.dm_dest
			     || sockaddrcmp(aln->adv_dm.dm_dest, alo->adv_dm.dm_dest))
			    && aln->adv_dm.dm_mask == alo->adv_dm.dm_mask) {
			    (void) sprintf(parse_error, "duplicate dest and mask in list at '%A mask %A'",
					   aln->adv_dm.dm_dest,
					   aln->adv_dm.dm_mask);
			    return TRUE;
			}
			break;

#ifdef	PROTO_ASPATHS
		    case ADVFT_TAG:
			if (aln->adv_tag == alo->adv_tag) {
			    (void) sprintf(parse_error, "duplicate tag in last at '%A'",
					   sockbuild_in(0, aln->adv_tag));
			    return TRUE;
			}
			break;
			
		    case ADVFT_ASPATH:
			if (aspath_adv_compare(alo->adv_aspath, aln->adv_aspath)) {
			    (void) sprintf(parse_error,
					   "duplicate AS path pattern in list");
			    return TRUE;
			}
			break;
#endif	/* PROTO_ASPATHS */

		    case ADVFT_PS:
			if (PS_FUNC_VALID(aln, ps_compare) && PS_FUNC(aln, ps_compare)(alo->adv_ps, aln->adv_ps)) {
			    (void) sprintf(parse_error, "duplicate protocol specific data in list");
			    if (PS_FUNC_VALID(aln, ps_print)) {
				(void) sprintf(&parse_error[strlen(parse_error)],
					       "at '%s'",
					       PS_FUNC(aln, ps_print)(aln->adv_ps, TRUE));
			    }
			    return TRUE;
			}
			break;
		}
	    }
	} ADV_LIST_END(*list, alo) ;
	last->adv_next = new;
    } else {
	*list = new;
    }
    return FALSE;
}


/*
 *	Set a flag in the gw structure for each element in a list
 */
int
parse_gw_flag __PF3(list, adv_entry *,
		    proto, proto_t,
		    flag, flag_t)
{
    int n = 0;
    adv_entry *adv;

    ADV_LIST(list, adv) {
	BIT_SET(adv->adv_gwp->gw_flags, flag);
	adv->adv_gwp->gw_proto = proto;
	n++;
    } ADV_LIST_END(list, adv) ;
    return n;
}


/*
 *	Switch to a new state if it is a valid progression from
 *	the current state
 */
int
parse_new_state __PF1(state, int)
{
    static const bits states[] = {
        { PS_INITIAL,	"initial" },
        { PS_OPTIONS,	"options" },
        { PS_INTERFACE,	"interface" },
	{ PS_DEFINE,	"define" },
	{ PS_PROTO,	"protocol" },
	{ PS_ROUTE,	"route" },
	{ PS_CONTROL,	"control" },
	{ 0, NULL }
    } ;

    assert(state >= PS_MIN && state <= PS_MAX);

    if (state < parse_state) {
	(void) sprintf(parse_error, "statement out of order");
	return TRUE;
    } else if (state > parse_state) {
	trace_tf(trace_global,
		 TR_PARSE,
		 0,
		 ("parse_new_state: %s old %s new %s",
		  parse_where(),
		  trace_state(states, parse_state),
		  trace_state(states, state)));
	parse_state = state;
    }

    return FALSE;
}


int
parse_metric_check __PF2(proto, proto_t,
			 metric, pmet_t *)
{
    int rc = 1;
    
    struct limit {
	proto_t proto;
	metric_t low;
	metric_t high;
	const char *desc;
    };
    static struct limit limits[] = {
#ifdef	PROTO_RIP
        { RTPROTO_RIP, RIP_LIMIT_METRIC, "RIP metric" },
#endif	/* PROTO_RIP */
#ifdef	PROTO_HELLO
        { RTPROTO_HELLO, HELLO_LIMIT_DELAY, "HELLO delay" },
#endif	/* PROTO_HELLO */
#ifdef	PROTO_OSPF
        { RTPROTO_OSPF, OSPF_LIMIT_COST, "OSPF cost" },
#endif	/* PROTO_OSPF */
#ifdef	PROTO_OSPF
        { RTPROTO_OSPF_ASE, OSPF_LIMIT_METRIC, "OSPF metric" },
#endif	/* PROTO_OSPF */
#ifdef	PROTO_EGP
        { RTPROTO_EGP, EGP_LIMIT_DISTANCE, "EGP distance" },
#endif	/* PROTO_EGP */
#ifdef	PROTO_BGP
        { RTPROTO_BGP, BGP_LIMIT_METRIC, "BGP metric" },
#endif	/* PROTO_BGP */
#ifdef	PROTO_SLSP
	{ RTPROTO_SLSP, SLSP_LIMIT_COST, "SLSP metric" },
#endif	/* PROTO_SLSP */
#ifdef  PROTO_ISIS
        { RTPROTO_ISIS, ISIS_LIMIT_METRIC, "IS-IS metric" },
#endif  /* PROTO_ISIS */
#ifdef	PROTO_DVMRP
        { RTPROTO_DVMRP, DVMRP_LIMIT_METRIC, "DVMRP metric" },
#endif  /* PROTO_DVMRP */
        { RTPROTO_DIRECT, IF_LIMIT_METRIC, "interface metric" },
        { 0, 0, 0, NULL }
    }, *limit = limits;

    for (limit = limits; limit->proto; limit++) {
	if (limit->proto == proto) {
	    break;
	}
    }

    if (limit->proto) {
	switch (metric->state) {
	    case PARSE_METRICS_UNSET:
		(void) sprintf(parse_error, "parse_metric_check: %s metric not set",
			       trace_state(rt_proto_bits, proto));
		break;

	    case PARSE_METRICS_INFINITY:
		PARSE_METRIC_SET(metric, limit->high);
		/* Fall Thru */

	    case PARSE_METRICS_SET:
		rc = parse_limit_check(limit->desc,
				       (u_int) metric->metric,
				       (u_int) limit->low,
				       (u_int) limit->high);
		break;

	    default:
		(void) sprintf(parse_error, "parse_metric_set: %s metric in unknown state: %d",
			       trace_state(rt_proto_bits, proto),
			       metric->state);
	}
    } else {
	(void) sprintf(parse_error, "parse_metric_check: invalid protocol %s (%x)",
		       trace_state(rt_proto_bits, proto),
		       proto);
    }

    return rc;
}


/*
 *	Set metric for each element in list that does not have one
 */
adv_entry *
parse_adv_propagate_metric __PF4(list, adv_entry *,
				 proto, proto_t,
				 metric, pmet_t *,
				 advlist, adv_entry *)
{
    register adv_entry *adv;
    int times = 0;

    ADV_LIST(list, adv) {
	adv->adv_proto = proto;

	/* Tack the dest/mask list to each of the entries */
	assert(!adv->adv_list);
	adv->adv_list = advlist;
	times++;

	if (PARSE_METRIC_ISRESTRICT(metric)) {
	    BIT_SET(adv->adv_flag, ADVF_NO);
	} else if (PARSE_METRIC_ISSET(metric)) {
	    BIT_SET(adv->adv_flag, ADVFOT_METRIC);
	    adv->adv_result.res_metric = metric->metric;
	}
    } ADV_LIST_END(list, adv) ;

    /* Update the reference counts on the dest/mask list */
    if (!--times) {
	ADV_LIST(advlist, adv) {
	    adv->adv_refcount += times;
	} ADV_LIST_END(advlist, adv) ;
    }

    return list;
}


/*
 *	Set metric for each element in list that does not have one
 */
adv_entry *
parse_adv_propagate_preference __PF4(list, adv_entry *,
				     proto, proto_t,
				     preference, pmet_t *,
				     advlist, adv_entry *)
{
    adv_entry *adv;
    int times = 0;

    ADV_LIST(list, adv) {
	adv->adv_proto = proto;
	/* Tack the dest/mask list to each of the entries */

	assert(!adv->adv_list);
	adv->adv_list = advlist;
	times++;

	if (PARSE_METRIC_ISRESTRICT(preference)) {
	    BIT_SET(adv->adv_flag, ADVF_NO);
	} else if (PARSE_METRIC_ISSET(preference)) {
	    BIT_SET(adv->adv_flag, ADVFOT_METRIC);
	    adv->adv_result.res_preference = preference->metric;
	}
    } ADV_LIST_END(list, adv) ;

    /* Update the reference counts on the dest/mask list */
    if (!--times) {
	ADV_LIST(advlist, adv) {
	    adv->adv_refcount += times;
	} ADV_LIST_END(advlist, adv) ;
    }

    return list;
}


/*
 *	Set config list for each element
 */
adv_entry *
parse_adv_propagate_config __PF3(list, adv_entry *,
				 config, config_list *,
				 proto, proto_t)
{
    adv_entry *adv;

    if (config) {
	ADV_LIST(list, adv) {
	    if (config) {
		BIT_SET(adv->adv_flag, ADVFOT_CONFIG);
		adv->adv_config = config;
		config->conflist_refcount++;
	    }
	    adv->adv_proto = proto;
	} ADV_LIST_END(list, adv) ;
    }

    return list;
}


/*
 *	Set preference for each elmit in list that does not have one
 */
void
parse_adv_preference __PF3(list, adv_entry *,
			   proto, proto_t,
			   preference, pref_t)
{
    ADV_LIST(list, list) {
	if ((list->adv_flag & ADVFO_TYPE) == ADVFOT_NONE) {
	    list->adv_proto = proto;
	    BIT_SET(list->adv_flag, ADVFOT_PREFERENCE);
	    list->adv_result.res_preference = preference;
	}
    } ADV_LIST_END(list, list) ;
}


/*
 *	Lookup the entry in the list for the exterior protocol and append this list to it.
 */
int
parse_adv_as __PF2(advlist, adv_entry **,
		   adv, adv_entry *)
{
    adv_entry *list;

    ADV_LIST(*advlist, list) {
	if (adv->adv_as == list->adv_as) {
	    break;
	}
    } ADV_LIST_END(*advlist, list) ;

    if (!list) {
	list = adv_alloc(ADVFT_AS, adv->adv_proto);
	list->adv_as = adv->adv_as;
	if (parse_adv_append(advlist, list)) {
	    return 0;
	}
    }
    if (parse_adv_append(&list->adv_list, adv)) {
	return 0;
    }
    return 1;
}


/*
 *	Parse the config file options
 */
int
parse_args __PF2(argc, int,
		 argv, char **)
{
    int arg_n, err_flag = 0;
    char *arg, *cp, *ap;
    char seen[MAXHOSTNAMELENGTH];

    bzero(seen, MAXHOSTNAMELENGTH);

    for (arg_n = 1; arg_n < argc; arg_n++) {
	arg = argv[arg_n];
	if (*arg == '-') {
	    cp = arg + 1;
	    if (index(seen, (int) *cp)) {
		trace_log_tf(trace_global,
			     0,
			     LOG_ERR,
			     ("%s: duplicate switch: %s\n",
			      task_name,
			      arg));
		err_flag++;
		continue;
	    }
	    seen[strlen(seen)] = *cp;
	    switch (*cp++) {
	    case 'C':
		/* Test configuration */
		trace_nosyslog = TRACE_LOG_TRACE;
		task_newstate(TASKS_TEST|TASKS_NOSEND|TASKS_NODUMP, 0);
		BIT_SET(trace_global->tr_control, TRC_NOSTAMP);
		break;

	    case 'c':
		/* Test configuration */
		trace_nosyslog = TRACE_LOG_NONE;
		task_newstate(TASKS_TEST|TASKS_NOSEND, 0);
		trace_global->tr_flags = TR_GENERAL;
		break;

	    case 'n':
		/* Don't install in kernel */
		BIT_SET(krt_options, KRT_OPT_NOINSTALL);
		break;

	    case 'N':
		/* Don't daemonize */
		task_newstate(TASKS_NODAEMON, 0);
		break;
		
	    case 't':
		/* Set trace flags */
		if (*cp) {
		    if (!(trace_global->tr_flags = trace_args(cp))) {
			err_flag++;
		    }
		} else {
		    trace_global->tr_flags = TR_GENERAL;
		}
		break;

	    case 'f':
		/* Specify config file */
		ap = arg + 2;
		if (!*ap) {
		    if (((arg_n + 1) < argc) && (*argv[arg_n + 1] != '-')) {
			ap = argv[++arg_n];
		    }
		}
		if (!*ap) {
		    trace_log_tf(trace_global,
				 0,
				 LOG_ERR,
				 ("%s: missing argument for switch: %s\n",
				  task_progname,
				  arg));
		    err_flag++;
		    break;
		}
		if (task_config_file) {
		    task_mem_free((task *) 0, task_config_file);
		}
		task_config_file = task_mem_strdup((task *) 0, ap);
		break;

	    default:
		trace_log_tf(trace_global,
			     0,
			     LOG_ERR,
			     ("%s: invalid switch: %s\n",
			      task_progname,
			      arg));
		err_flag++;
	    }
	} else if (!trace_global->tr_file->trf_file) {
	    trace_file_free(trace_global->tr_file);
	    trace_global->tr_file = trace_file_locate(task_mem_strdup((task *) 0, arg),
						      (off_t) 0,
						      (u_int) 0,
						      (flag_t) 0);
	    trace_on(trace_global->tr_file);
	} else {
	    trace_log_tf(trace_global,
			 0,
			 LOG_ERR,
			 ("%s: extraneous information on command line: %s\n",
			  task_progname,
			  arg));
	    err_flag++;
	}
    }

    if (err_flag) {
	trace_log_tf(trace_global,
		     0,
		     LOG_ERR,
		     ("Usage: %s [-c] [-C] [-n] [-N] [-t[flags]] [-f config-file] [trace-file]\n",
		      task_progname));
    }
    return err_flag;
}
