#ifndef _PPP_PARAM
#define _PPP_PARAM

#ident	"@(#)ppp_param.h	1.6"

/*#define PROTO_BACP		0x8071*/
#define PROTO_CCP		0x80FD
#define PROTO_ILCCP		0x80FB
#define PROTO_CCPDG		0x00FD
#define PROTO_ILCCPDG		0x00FB

/*
 * Link types (as defined in the BACP draft)
 */
#define LNK_ISDN	0x01
#define LNK_X25		0x02
#define LNK_ANALOG	0x04
#define LNK_SWCHDIG	0x08
#define LNK_ISDNVOC	0x10

#define LNK_STATIC	0x1000	/* Not defined in BACP draft */
#define LNK_TCP		0x1001	/* Not defined in BACP draft */

#define LNK_ANY		0x2000	/* Not defined in BACP draft ..
				   allow any service */
				   
/*
 * These are the flow control values
 */
#define FLOW_NONE	0
#define FLOW_HARD	1
#define	FLOW_SOFT	2

#define MAXID 40	/* Max length of a definition name */
#define MAXDEV 255
#define MAXUIDLEN 10
#define MAXCALLID 20 

#ifdef MAXHOSTNAMELEN
#undef MAXHOSTNAMELEN
#endif
#define MAXHOSTNAMELEN 255

/*
 * Bundle styles
 */
#define BT_NONE		0x00	/* Bundle is disabled */
#define BT_IN		0x01	/* Incoming calls only */
#define BT_OUT		0x02	/* Outgoing calls only */
#define BT_INOUT 	(BT_IN | BT_OUT)	/* Incoming and outgoing*/

/*
 * BOD types
 */
#define BOD_IN 0
#define BOD_OUT 1
#define BOD_ANY 2
#define BOD_NONE 3

/*
 * Debug levels
 */
#define DBG_NONE	0
#define DBG_LOW		1
#define DBG_MED		2
#define DBG_HIGH	3
#define DBG_WIRE	4
#define DBG_DEBUG	5

/*
 * Definition types
 */
#define DEF_GLOBAL	0
#define DEF_AUTH	1
#define DEF_ALG		2
#define	DEF_PROTO	3
#define DEF_LINK	4
#define	DEF_BUNDLE	5

/* The number of definition types */
#define DEF_MAX		6

/*
 * OP_LCP / OP_NCP administation options 
 */
#define CP_UP 		1	/* Control protocol UP */
#define CP_DOWN		2	/* Control protocol DOWN */
#define CP_STATUS	3	/* Obtain Status for the Control Protocol */

/*
 * Default values
 */
#define DEFAULT_MRRU 1500 /* Is this a good default to use ??? */
#define DEFAULT_SSN 0
#define DEFAULT_MAXLINKS 1
#define DEFAULT_MINLINKS 1

/* Need better defaults for these times */
#define DEFAULT_THRASH 10 /* Seconds */

#define DEFAULT_ED 1	/* enabled */
#define DEFAULT_MINFRAG 100
#define DEFAULT_MAXFRAGS 10 /* This is per member links */
#define DEFAULT_MLIDLE 5
#define DEFAULT_MLNULLS 1
#define DEFAULT_ADDLOAD 60
#define DEFAULT_DROPLOAD 20
#define DEFAULT_ADDSAMPLE 10
#define DEFAULT_DROPSAMPLE 30
#define DEFAULT_THRASHTIME 60
#define DEFAULT_MAXIDLE 0
#define DEFAULT_BOD BOD_OUT

#define DEFAULT_PAP 0 /* We don't require PAP by default */
#define DEFAULT_CHAP 0 /* We don't require CHAP by default */
#define DEFAULT_INC_PAP 1 /* We  require PAP on incoming */
#define DEFAULT_INC_CHAP 1 /* We require CHAP on incoming */
#define DEFAULT_AUTHTMOUT 60 /* Default time for authentication */
#define DEFAULT_BRINGUP OUT_AUTOMATIC
#define DEFAULT_BTYPE BT_INOUT

/* Maximum length of Endpoint Discriminator */
#define MAX_ED_SIZE 20

/*
 * The maximum number of Network Control Protocols Supported
 */
#define MAX_CP 15

/*
 * CHAP Algorithms
 */
#define CHAP_MD5	0x05
#define CHAP_MS		0x80	/* Microsoft */

#include "assert.h"

#define ASSERT(a) assert(a)

#define MUTEX_LOCKED(x) ((&(x)->m_lmutex)->lock)

#define HZ 100

/*
 * For the Phase FSM
 */
#define PHASE_DEAD	0
#define PHASE_ESTAB	1
#define PHASE_AUTH	2
#define PHASE_NETWORK	3
#define PHASE_TERM	4
#define NUM_PHASE	5

#endif /* _PPP_PARAM */
