/*
 *	@(#)grafinfo.h	6.1	12/14/95	15:23:53
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Fri Feb 01 19:45:47 PST 1991	pavelr@sco.com
 *	- Created file.
 *	S001	Thu Feb 14 15:37:00 PST 1991	pavelr@sco.com
 *	- Added error definitions
 *	S002	Thu Apr 25 20:00:00 PST 1991	pavelr@sco.com
 *	- added some more defintions, errorcodes
 *	S003	Mon Aug 26 18:00:21 PDT 1991	pavelr@sco.com
 *	- Changes necessary for new grafinfo API.
 *	S004	Thu Sep 12 1991			pavelr@sco.com
 *	- Monitor parsing changes
 *	S005	Mon Sep 16 1991			pavelr@sco.com
 *	- grafGetFunction added
 *	S006	Tue Mar 24 1992			pavelr@sco.com
 *	- added new field to mem struct
 *	S007	Tue Mar 24 1992			pavelr@sco.com
 *	- added grafGetMem macro
 *	S008	Tue Mar 24 1992			pavelr@sco.com
 *      - added function declaration for grafMemManage
 *	S009	Tue Mar 24 1992			pavelr@sco.com
 *      - added opcodes for read & write
 *	S010	Tue Mar 24 1992			pavelr@sco.com
 *      - added some error handling defines
 *	S011	Thu Sep 03 1992			buckm@sco.com
 *      - added opcode for int10.
 *	- define a port structure.
 *	- add a name to the mem structure.
 *	- change memory and port fixed arrays to be linked lists instead;
 *	  no more MAXMEM and MAXPORTS.
 *	- don't declare grafMemManage(); declare grafGetMemInfo();
 *	  get rid of grafGetMem() macro.
 *	- declare grafRunFunction().
 *	S012	Sun Mar 28 1993			buckm@sco.com
 *      - added opcode for callrom.
 *	S013    Tue June 22			edb@sco.com
 *	- changes to execute compiled grafinfo files
 */

#ifndef GRAFINFO_H
#define GRAFINFO_H 1

/* some limits */
#define CODESIZE	12000
#define NUMBER_REG	64

#define G_MAXMODES	10
#define G_MODELENGTH	255

#define	G_NUMBER	0
#define	G_REGISTER	1

/* codeTypes */

#define INTERPRET	0      /* S013 */
#define COMPILED	1      /* S013 */

/* opcodes */
#define	OP_END		0
#define	OP_ASSIGN	1
#define	OP_OUT		2
#define	OP_IN		3
#define	OP_AND		4
#define	OP_OR		5
#define	OP_XOR		6
#define	OP_NOT		7
#define	OP_SHR		8
#define	OP_SHL		9
#define	OP_WAIT		10
#define	OP_BOUT		11
#define	OP_OUTW		12
#define	OP_READB	13    /* S009 */
#define	OP_WRITEB       14    /* S009 */
#define	OP_READW	15    /* S009 */
#define	OP_WRITEW       16    /* S009 */
#define	OP_READDW	17    /* S009 */
#define	OP_WRITEDW      18    /* S009 */
#define	OP_INT10        19    /* S011 */
#define	OP_CALLROM      20    /* S012 */
 

/* error codes - S001 */
#define	GEOK		0	/* All OK - no error */
#define GEALLOC		1	/* Memory Allocation error */
#define GEMODESTRING	2	/* Illegal mode string */
#define GEFORMATDEV	3	/* Illegal format for field in grafdev */
#define GEFORMATDEF	4	/* Illegal format for field in grafinfo.def */
#define GENODEVDEF	5	/* Can't open grafdev or grafinfo.def files */
#define GEOPENFILE	6	/* Unable to open GrafInfo file */
#define GENOMODE	7	/* Unable to get mode */
#define GEPARSE		8	/* Unable to parse file */
#define GEBADMODE	9	/* bad mode */
#define GEBADPARSE	10 	/* Bad parse of section header */
#define GEBADREG	11 	/* Bad register number */

/* S004 */
#define GEMONFILE	12	/* Can't open moninfo file */
#define GEMONPARSE	13	/* Bad syntax in monitor file */
#define GEMONFORMAT	14	/* Illegal format of moninfo file */
#define GEMONOPEN	15	/* Can't open monitor file */

/* S010 */
#define GENOCLASS       16      /* No class in string list */
#define GEMAPCLASS      17      /* Map class call failed */
#define NUMERROR	18

/* some return codes */
#define	FAILURE		0
#define	SUCCESS		1
#define	G_STRING	2
#define	G_INTEGER	3
#define	G_FUNCTION	4



/* type definitions */

typedef int codeType;

typedef union _codePnt {                  /* S013 */
                codeType  * data;
                int      (* function)();
            } codePnt;

/* linked list structures for the data types in grafData */

							/* vvv S011 vvv */
typedef struct _memList {
	struct _memList		*next;
	char			*name;
	unsigned int		base;
	unsigned int		size;
	unsigned int		mapped;
} memList;

typedef struct _portList {
	struct _portList	*next;
	unsigned short		base;
	unsigned short		count;
} portList;
							/* ^^^ S011 ^^^ */

typedef struct _intList {
	struct _intList		*next;
	char			*id;
	int			val;
} intList;

typedef struct _stringList {
	struct _stringList	*next;
	char			*id;
	char			*val;
} stringList;

typedef struct _functionList {
	struct _functionList	*next;
	char			*id;
	codePnt			code;                           /* S013 */
} functionList;

typedef struct _grafData {
	memList			*memory;			/* S011 */
	portList		*ports;				/* S011 */
	functionList		*functions;
	intList			*integers;
	stringList		*strings;
	codeType		cType;                          /* S013 */
} grafData;

/*
 *   The following structure is the interface between the
 *   compiled grafinfo file and the server
 *   It will be created by mkdev graphics
 *   in  vidconf/vidparse/parse.c
 */
typedef struct _cFunctionStruct {                                /* S013 */
	char			*mode;
	char			*procedureName;
	int                     (* functionPnt)();
} cFunctionStruct;

void grafRunCode ();
char *grafGetFullMode ();
char *grafGetTtyMode ();
char *grafGetName ();
char *grafGetCName ();
int grafParseFile ();
int grafFreeMode ();
int grafGetInt ();
int grafGetString ();
int grafExec ();
int grafQuery ();
int grafParseMon ();	/* S004 */
int grafGetFunction ();	/* S005 */
void grafRunCode ();	/* S005 */
void grafRunFunction ();					/* S011 */
int grafGetMemInfo ();						/* S011 */

/* S002 */
extern int graferrno;

/* S004 */
#define MON_PREFIX 	"MON_"
#define MON_PREFIX_LEN	strlen(MON_PREFIX)

#endif /* GRAFINFO_H */
