#pragma ident "@(#)common.h	29.1"
#pragma ident "$Header$"

/*
 *
 *      Copyright (C) The Santa Cruz Operation, 1993-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#include <hpsl.h>             /* REQUIRED */
#include <sys/types.h>        /* REQUIRED */
#include <sys/resmgr.h>       /* REQUIRED */
#include <sys/confmgr.h>      /* REQUIRED */
#include <sys/cm_i386at.h>    /* REQUIRED */

#ifdef DMALLOC
#undef u_int
#undef ulong
#undef u_long
#undef boolean_t
typedef unsigned int u_int;
typedef unsigned long ulong;
typedef unsigned long u_long;
typedef unsigned short u_short;
typedef unsigned char u_char;
typedef enum boolean { B_FALSE, B_TRUE } boolean_t;
#include "dmalloc.h"
#endif

#ifndef NETISL    /* Makefile invoked incorrectly */
#error NETISL must be defined
#endif

#if (NETISL == 0)
#define MAXDRIVERS	500   /* note that each bcfg file is 1 driver */
#define PROMPT		"ndcfg>"
#define TCLERRORPREFIX "NDCFG"
#else
#define MAXDRIVERS    200   /* note that each bcfg file is 1 driver */
#define PROMPT		"ndisl>"
#define TCLERRORPREFIX "NDISL"
#endif

/* arguments to tempnam(3S); change as needed */
#define TEMPNAMDIR "/var/tmp"
#define TEMPNAMPFX "ndcfg"

#define SIZE 1024 /* array sizes for strings */
#define RMTIMEOUT 60   /* seconds to wait for RMopen to succeed */
/* how many CUSTOM[x] entries we allow.  
 * if you raise this higher than 9 then you must add addition
 * CUSTOM[x] entires to symbol[]
 */
#define MAX_CUSTOM_NUM 9    

/* min and max for $version command.  increase as necessary */
#define MINCFGVERSION 0
#define MAXCFGVERSION 1

/* defines for variable "section" */
#define NOSECTION 0
#define MANIFEST 1
#define DRIVER 2
#define ADAPTER 3

/* defines for primitive types */
#define UNDEFINED 	0
#define SINGLENUM		1
#define NUMBERRANGE	2
#define STRING			3
#define NUMBERLIST	4
#define STRINGLIST	5

/* defines for variable "quotestate" */
#define NOQUOTES 0
#define YSND     1   /* yes in single; not in double */
#define NSYD     2   /* not in single; yes in double */
#define YSYD     3   /* yes in single; yes in double */

/* global hpsl structures */
extern HpslContInfo_t *hpslcontinfo;
extern u_int hpslcount;
extern SuspendedDrvInfoPtr_t hpslsuspendeddrvinfoptr;
extern u_int hpslsuspendedcnt;
extern int hpsl_ERR;

/* set when we get signal SIGINT */
extern u_int nomoreoutput;

extern int currentline;
extern int numvariables;
extern int section;
extern int dangerousdetect;
extern u_int cfgdebug, cfghasdebug, cfgversion, bcfglinenum, incomment;
extern u_int cmdlinenum;
extern u_int tclmode;
extern u_int moremore;
extern u_int candomore;
extern u_int OpGotError;
extern u_int moremode;
extern u_int candomore;
extern u_int hflag;

extern int errno;
extern struct bcfgfile bcfgfile[MAXDRIVERS];
extern int Dflag;  /* permanent debugging that lasts across bcfgs */
extern int nflag;  /* no PROMPT prompt please */
extern FILE *logfilefp;
extern char *rootdir;
extern int bcfgfileindex;
extern char ErrorMessage[];
extern char ListBuffer[];

/* section definitions */
#define SEC_GENERAL     0
#define SEC_SHOW        1
#define SEC_FILE        2
#define SEC_RESMGR      3
#define SEC_CA          4
#define SEC_ELF         5
#define SEC_BCFG        6
#define SEC_HPSL        7
#define SEC_MAXSECTIONS 8

struct help {
   int section;
   char *cmd;
   char *description;
};

extern struct help help[];

typedef struct {
	ulong type;  /* set to SINGLENUM */
	ulong number;
   union primitives *uma;  /* UnionMallocAddress: for PrimitiveFreeMem */
} singlenum_t;


typedef struct {
	ulong type; /* NUMBERRANGE */
	ulong low;
	ulong high;
   union primitives *uma; /* UnionMallocAddress: for PrimitiveFreeMem */
} numrange_t;

typedef struct string {
	ulong type; /* STRING */
	char *words;  /* space malloced elsewhere */
   union primitives *uma; /* UnionMallocAddress: for PrimitiveFreeMem */
} string_t;

typedef struct numberlist {
	ulong type; /* NUMBERLIST */
   ulong ishighvalid;  /* could just be a single value and not a range */
	ulong low;
	ulong high;
	struct numberlist *next;
   union primitives *uma; /* UnionMallocAddress: for PrimitiveFreeMem */
} numlist_t;

typedef struct stringlist {
	ulong type; /* STRINGLIST */
	char *string;
	struct stringlist *next;
   union primitives *uma; /* UnionMallocAddress: for PrimitiveFreeMem */
} stringlist_t;

union primitives {
   ulong type; /* SINGLENUM, NUMBERRANGE, STRING, NUMBERLIST, STRINGLIST */
   singlenum_t num;
   numrange_t numrange;
   string_t string;
   numlist_t numlist;
   stringlist_t stringlist;
   /* char *strval; */
} primitive;

#define BCFGLEXFLUFF 		0x00000001
#define BCFGLEXSTATE			0x00000002
#define BCFGLEXSECTION		0x00000004
#define BCFGLEXRETURN		0x00000008
#define BCFGYACCFLUFF		0x00000010
#define BCFGYACCSTUFF		0x00000020
#define CMDLEXFLUFF			0x00000100
#define CMDLEXRETURN			0x00000200
#define CMDYACCFLUFF       0x00001000
#define CMDRESPUT				0x00010000

#if NCFGDEBUG == 1
 /* the DP1 means 1 argument if you don't want to use Pstderr which uses
  * varags
  */
#define DP1(x,y1) if (cfgdebug & x) Pstderr(y1);
#define DP2(x,y1,y2) if (cfgdebug & x) Pstderr(y1,y2);
#define DP3(x,y1,y2,y3) if (cfgdebug & x) Pstderr(y1,y2,y3);
#define DP4(x,y1,y2,y3,y4) if (cfgdebug & x) Pstderr(y1,y2,y3,y4);
#define DP5(x,y1,y2,y3,y4,y5) if (cfgdebug & x) Pstderr(y1,y2,y3,y4,y5);
#define DP6(x,y1,y2,y3,y4,y5,y6) if (cfgdebug & x) Pstderr(y1,y2,y3,y4,y5,y6);
#else
#define DP1(x,y1)
#define DP2(x,y1,y2)
#define DP3(x,y1,y2,y3)
#define DP4(x,y1,y2,y3,y4)
#define DP5(x,y1,y2,y3,y4,y5)
#define DP6(x,y1,y2,y3,y4,y5,y6)
#endif

/* symbol stuff */
struct symbol {
   /* no bitfields here for space purposes: time is more important */
   char *name;       /* ADDRM, BUS, INT, WRITEFW, etc. */
   u_int version;    /* $version where defined */
   u_int section;    /* default stanza section: for UW2.1 bcfg ($version 0) */
   u_int ismulti;    /* can have multiple values or a single text/numeric? */
   u_int isnumeric;  /* if contents are only numbers or number ranges */
   u_int decorhex;   /* if isnumeric true are numbers in dec. or hex */
   u_int istf;       /* can this value only be true/false */
   u_int iscustom;   /* is this a funky CUSTOM variable */
   u_int reqbcfg;    /* must sym be def. in all bcfgs for for its $version? */
   u_int reqisa;     /* must symbol be defined if BUS=ISA? */
};
/* symbol related defines.  Odd values make debugging easier
 * (that's also why symbol[] doesn't use bitfields too)
 */
#define V0 			0x10
#define V1 			0x20
#define V2 			0x40
#define V3 			0x80

#define DRVSEC 	0x100
#define ADPSEC 	0x200
#define NETSEC 	0x400
#define MANSEC 	0x800

#define NOMULTI  	0x1000
#define YESMULTI 	0x2000

#define NONUM     0x4000
#define YESNUM		0x8000

#define NA			0x10000
#define DEC			0x20000
#define HEX			0x40000
#define OCT			0x80000

#define NOTF		0x100000
#define YESTF		0x200000

#define NOCUSTOM  0x400000
#define YESCUSTOM 0x800000

#define MAND		0x1000000
#define OPT			0x2000000

extern void sysdat(void);
extern void donlist(int, char *, unsigned long, int);
extern void doxid(char *);
extern void dotest(char *);
extern int DoAssignment(char *, union primitives);
extern void ensurevalid(void);
extern void initsymbols(void);
extern int ensurev1(void);
extern void mdi_printcfg(int);
extern void EnsureAllAssigned(void);
extern void EnsureNumerics(void);
extern struct symbol symbol[];
extern void notice(char *fmt, ...);
extern void error(uint_t reason, char *fmt, ...);
extern void fatal(char *fmt, ...);
extern void ProcessDirectory(int, char *, char *);
extern void LoadDirHierarchy(char *);
extern void ClearError(void);
extern void EndList(void);
extern void ExitIfNOTBCFGError(void);
extern void Cleanup(void);
extern void ShowAllTopologies(char *);
extern void ShowTopo(char *);
extern void StartList(u_int, ...);
extern void AddToList(u_int, ...);
extern void EndList(void);
extern int ResShowUnclaimed(int, char *, char *);
extern int ResmgrDumpAll(void);
extern int ResmgrDumpOne(rm_key_t, u_int);
extern int getisaparams(char *, int);
extern int showcustomnum(int, char *);
extern void showcustom(char *,char *);
extern int idinstall(union primitives);
extern int idmodify(union primitives);
extern int isaautodetect(int fromidmodify, 
                         int installindex, 
                         rm_key_t,
                         union primitives);
extern void idremove(char *, char *);
extern int ResmgrDelInfo(rm_key_t, int, int, int, int);
extern char *FindStringListText(stringlist_t *, char *);
extern int irqsharable(rm_key_t, int, int);
extern int irqatkey(rm_key_t, int *, int);
extern int determinepromfromkey(rm_key_t, rm_key_t, int);
extern int DelAllVals(rm_key_t, char *);
extern HpslSocketInfoPtr_t GetHpslSocket(rm_key_t);
extern void ResetBcfgLexer(void);
extern int StartStatus(char *);
extern int ResmgrGetLowestBoardNumber(char *);

/* defines for ops.c and for putting things in the resmgr */
/* similar to <sys/cm_i386at.h> */
struct nameoffsettable {
   char *name;
   unsigned int offset;
};

#define CM_ADDRM					"ADDRM"
#define N_ADDRM					0
#define CM_AUTOCONF				"AUTOCONF"
#define N_AUTOCONF				1
#define CM_BOARD_IDS				"BOARD_IDS"
#define N_BOARD_IDS				2
#define CM_BUS						"BUS"
#define N_BUS						3
#define CM_CONFIG_CMDS			"CONFIG_CMDS"
#define N_CONFIG_CMDS			4
#define CM_CONFORMANCE			"CONFORMANCE"
#define N_CONFORMANCE			5
#define CM_CUSTOM1				"CUSTOM[1]"
#define N_CUSTOM1					6
#define CM_CUSTOM2				"CUSTOM[2]"
#define N_CUSTOM2					7
#define CM_CUSTOM3				"CUSTOM[3]"
#define N_CUSTOM3					8
#define CM_CUSTOM4				"CUSTOM[4]"
#define N_CUSTOM4					9
#define CM_CUSTOM5				"CUSTOM[5]"
#define N_CUSTOM5					10
#define CM_CUSTOM6				"CUSTOM[6]"
#define N_CUSTOM6					11
#define CM_CUSTOM7				"CUSTOM[7]"
#define N_CUSTOM7					12
#define CM_CUSTOM8				"CUSTOM[8]"
#define N_CUSTOM8					13
#define CM_CUSTOM9				"CUSTOM[9]"
#define N_CUSTOM9					14
#define CM_CUSTOM_NUM			"CUSTOM_NUM"
#define N_CUSTOM_NUM				15
#define CM_DEPEND					"DEPEND"
#define N_DEPEND					16
#define CM_DLPI					"DLPI"
#define N_DLPI						17
#define CM_DMA						"DMA"
#define N_DMA						18
#define CM_DRIVER_NAME			"DRIVER_NAME"
#define N_DRIVER_NAME			19
#define CM_EXTRA_FILES			"EXTRA_FILES"
#define N_EXTRA_FILES			20
#define CM_FILES					"FILES"
#define N_FILES					21
#define CM_IDTUNE_ARRAY1		"IDTUNE_ARRAY[1]" /* just fits RM_MAXPARAMLEN */
#define N_IDTUNE_ARRAY1			22
#define CM_IDTUNE_ARRAY2		"IDTUNE_ARRAY[2]"
#define N_IDTUNE_ARRAY2			23
#define CM_IDTUNE_ARRAY3		"IDTUNE_ARRAY[3]"
#define N_IDTUNE_ARRAY3			24
#define CM_IDTUNE_ARRAY4		"IDTUNE_ARRAY[4]"
#define N_IDTUNE_ARRAY4			25
#define CM_IDTUNE_ARRAY5		"IDTUNE_ARRAY[5]"
#define N_IDTUNE_ARRAY5			26
#define CM_IDTUNE_ARRAY6		"IDTUNE_ARRAY[6]"
#define N_IDTUNE_ARRAY6			27
#define CM_IDTUNE_ARRAY7		"IDTUNE_ARRAY[7]"
#define N_IDTUNE_ARRAY7			28
#define CM_IDTUNE_ARRAY8		"IDTUNE_ARRAY[8]"
#define N_IDTUNE_ARRAY8			29
#define CM_IDTUNE_ARRAY9		"IDTUNE_ARRAY[9]"
#define N_IDTUNE_ARRAY9			30
#define CM_IDTUNE_NUM			"IDTUNE_NUM"
#define N_IDTUNE_NUM				31
#define CM_INT						"INT"
#define N_INT						32
#define CM_MEM						"MEM"
#define N_MEM						33
#define CM_NAME					"NAME"
#define N_NAME						34
#define CM_NUM_PORTS				"NUM_PORTS"
#define N_NUM_PORTS				35
#define CM_ODIMEM					"ODIMEM"
#define N_ODIMEM					36
#define CM_OLD_DRIVER_NAME		"OLD_DRIVER_NAME"  /* just fits RM_MAXPARAMLEN */
#define N_OLD_DRIVER_NAME		37
#define CM_PORT					"PORT"
#define N_PORT						38
#define CM_POST_SCRIPT			"POST_SCRIPT"
#define N_POST_SCRIPT			39
#define CM_PRE_SCRIPT			"PRE_SCRIPT"
#define N_PRE_SCRIPT				40
#define CM_REBOOT					"REBOOT"
#define N_REBOOT					41
#define CM_RM_ON_FAILURE		"RM_ON_FAILURE"
#define N_RM_ON_FAILURE			42
#define CM_TOPOLOGY				"TOPOLOGY"
#define N_TOPOLOGY				43
#define CM_TYPE					"TYPE"
#define N_TYPE						44
#define CM_UNIT					"UNIT"
#define N_UNIT						45
#define CM_UPGRADE_CHECK		"UPGRADE_CHECK"
#define N_UPGRADE_CHECK			46
#define CM_VERIFY					"VERIFY"
#define N_VERIFY					47
#define CM_WRITEFW				"WRITEFW"
#define N_WRITEFW					48
#define CM_FAILOVER				"FAILOVER"
#define N_FAILOVER				49
#define CM_MAX_BD					"MAX_BD"
#define N_MAX_BD					50
#define CM_ACTUAL_SEND_SPEED			"ACTUAL_SEND_SPEED" /*won't fit in resmgr*/
#define N_ACTUAL_SEND_SPEED				51
#define CM_ACTUAL_RECEIVE_SPEED		"ACTUAL_RECEIVE_SPEED"/*won't go in resmgr*/
#define N_ACTUAL_RECEIVE_SPEED			52
#define CM_NET_BOOT				"NET_BOOT"
#define N_NET_BOOT				53
#define CM_HELPFILE				"HELPFILE"
#define N_HELPFILE				54
#define CM_ISAVERIFY          "ISAVERIFY"
#define N_ISAVERIFY           55
#define CM_PROMISCUOUS        "PROMISCUOUS"
#define N_PROMISCUOUS         56

#define MAX_BCFG_VARIABLES		57   /* 0-56 */


/* remember that drivers can have multiple bcfg files 
 * we copy all DSP files to holding tank, editing the Drvmap file
 * and System file, and do the actual idinstall from a
 * different directory
 */
struct variables {
   union primitives strprimitive; /* received from parser: always STRINGLIST */
   union primitives primitive; /* after translation to "proper" format */
};

struct bcfgfile {
	char *location;   /* location of the bcfg, relative to $ROOT */
   char *driverversion;  /* from ELF .ndnote section or ndversion[] symbol */
   int version;      /* the #$version of this file */
   int fullDSPavail; /* set with loadihvdisk.  indicates if full DSP available
                      * for later use with idinstall command.  If this bcfg
                      * was loaded with "loadihvdisk 1", indicating netisl,
                      * then full DSP package wasn't available and only the
                      * bcfg file and Driver.mod files were available.
                      * later on when user does the "idinstall 0" command we
                      * make sure that fullDSPavail is 1
                      */
   struct variables variable[MAX_BCFG_VARIABLES];
};

/* define here if compiling as POSIX or ANSI; ndcfg uses these */

#if defined(_POSIX_SOURCE) || (__STDC__ - 0 == 1)
extern FILE *popen(const char *, const char *);
extern char *cuserid(char *);
extern char *tempnam(const char *, const char *);
extern char *optarg;
extern int  optind, opterr, optopt;
extern int  getopt(int, char *const *, const char *);
extern int  getw(FILE *);
extern int  putw(int, FILE *);
extern int  pclose(FILE *);
#endif

extern char *UndefinedPrint(int, int);
extern char *SingleNumPrint(int, int);
extern char *NumberRangePrint(int, int);
extern char *StringPrint(int, int);
extern char *NumberListPrint(int, int);
extern char *StringListPrint(int, int);
extern int StringListNumWords(int, int);
extern char *StringListWordX(int, int, int);
extern int StringListNumLines(int, int);
extern char *StringListLineX(int, int, int);
extern int resdump(char *);
extern int getbustypes(void);
extern void rejectbcfg(uint_t);
extern uint_t bcfghaserrors;
extern int bequiet,fflag;
extern int resget(char *, char *);
extern int resput(char *, char *,int);
extern rm_key_t ResmgrHighestKey(void);
extern int ResmgrNextKey(rm_key_t *, int);
extern int ResmgrGetVals(uint_t, const char *, int, char *, int);
extern void Pstdout(char *fmt, ...);
extern void Pstderr(char *fmt, ...);
extern int ResBcfgUnclaimed(u_int);
extern int ResODIMEMCount(void);
extern void showserialttys(void);
extern int bcfgpathtoindex(int, char *);
extern rm_key_t resshowkey(int, char *, char *,int);
extern int showISAcurrent(char *, int, char *, char *, char *, char *);
extern int showCUSTOMcurrent(char *);
extern int AtMAX_BDLimit(int);
extern int ResGetNameTypeDeviceDepend(rm_key_t,int,char *,
                                      char *,char *, char *);
extern int ResmgrGetNetX(rm_key_t, char *);
extern int showhelpfile(char *);
extern int ResDumpCA(char *, u_int, u_int);
extern int ca_print_pci(rm_key_t, u_int, u_int);
#ifdef CM_BUS_I2O
extern int ca_print_i2o(rm_key_t, u_int, u_int);
#endif
extern int ca_print_eisa(rm_key_t, u_int, u_int);
extern int ca_print_mca(rm_key_t, u_int, u_int);
extern void InitPCIVendor(void);
extern int stamp(char *, char *, u_int);
extern int getstamp(int, char *, char *, u_int, u_int, int *);
extern int GetDriverVersionInfo(int, char *);
extern int IsParam(int, int, char *, int, rm_key_t, char *);
extern int printauths(void);
extern int netisl;
extern void PrimeErrorBuffer(int, char *, ...);
extern void Status(int, char *, ...);
extern int elementtoindex(int, char *);
extern int bcfghasverify(char *);
extern int CheckResmgrAndDevs(char *);
extern void StartBackupKey(rm_key_t);
extern void EndBackupKey(char *);
extern rm_key_t CreateNewBackupKey(rm_key_t, char *, int, int);
extern int DependStillNeeded(char *);
extern int unloadall(int);
extern void ClearTheScreen(void);
extern void CmdFreeWords(void);
extern int ItypeSet(rm_key_t);
extern int IplSet(rm_key_t);
extern int copyfile(int, char *, char *, char *, char *);
#define COPYFILE(A) \
   if (copyfile(0,sourcedir,A,destdir,A) != 0) { \
      error(NOTBCFG,"Problem copying %s/%s to %s/%s", \
         sourcedir,A,destdir,A); \
      goto fail; \
   };

extern int showpcivendor(unsigned long);
extern int orphans(void);
extern int iicard(void);
extern int ResmgrEnsureGoodBrdid(rm_key_t, u_int);
extern int promiscuous(void);
extern int determineprom(char *);
extern int hpsldump(void);
extern int hpslsuspend(char *);
extern int hpslresume(char *);
extern int hpslgetstate(char *);
extern int hpslcanhotplug(char *);
extern int gethwkey(char *);
extern int dlpimdi(char *);
extern int DelAllKeys(char *);
extern int toastmodname(rm_key_t);

/* simple one way linked list */
struct reject {
   uint_t reason;   /* bitmask */
   char *pathname;  /* pathname to the offending bcfg */
   struct reject *next;  /* next rejection */
};
extern struct reject *rejectlist;

/* reasons for rejecting a bcfg; stored in reason above */
#define SYNTAX 	  0x00000001 /* general syntax error */
#define SECTIONS	  0x00000002 /* section problem */
#define VERSIONS    0x00000004 /* version number problem */
#define BADVAR      0x00000008 /* bad variable */
#define MULTDEF     0x00000010 /* multiply defined symbol */
#define MULTVAL     0x00000020 /* can't be multivalued */
#define TRUEFALS    0x00000040 /* can only be true/false */
#define NOTBUS      0x00000080 /* bcfg isn't applicable for bus */
#define NOTDEF      0x00000100 /* mandatory symbol not defined for $version X */
#define BADNUM      0x00000200 /* bad number */
#define BADODI      0x00000400 /* $version 0,no DLPI=true,no $interface odiXX*/
#define ISANOTDEF   0x00000800 /* mandatory for ISA symbol not defined */
#define EISANOTDEF  0x00001000 /* mandatory for EISA symbol not defined */
#define PCINOTDEF   0x00002000 /* mandatory for PCI symbol not defined */ 
#define MCANOTDEF   0x00004000 /* mandatory for MCA symbol not defined */
#ifdef CM_BUS_I2O
#define I2ONOTDEF   0x00008000 /* mandatory for I2O symbol not defined */
#endif
#define BADELF      0x00010000 /* ELF problem in dealing with Driver.o */
#define BADDSP      0x00020000 /* not all required DSP components present */
#define NOERRNL     0x20000000 /* don't add a newline at end of text */
#define NOTBCFG     0x40000000 /* error not related to bcfg */
#define CONT        0x80000000 /* continuation line - no "error: " header */

#define VB_SIZE   512

/* for showalltopologies.  used in bitmask, so can't start with 0 */
#define LAN 0x1
#define WAN 0x2

struct topo {
   u_int type;     /* WAN or LAN */
   char *name;     /* that we should pass to the showtopo command.
                    * must be upper case here in this table
                    */
   char *fullname;
   u_int value;    /* bitmask, each must be unique */
};
extern struct topo topo[];
extern u_int numtopos;

/* the following belong in some netdriver header file
 * dlpi.h, mdi.h, scomdi.h, etc.
 * ISA drivers will need to do a cm_getval on them to retrieve
 * their value at init time.
 */
#define CM_OLDIRQ				"OLDIRQ"
#define CM_OLDDMAC			"OLDDMAC"
#define CM_OLDIOADDR			"OLDIOADDR"
#define CM_OLDMEMADDR		"OLDMEMADDR"

/* used by DoRemainingStuff and showreskey commands */
#define CM_NETCFG_ELEMENT	"NETCFG_ELEMENT"

/* used by DoRemainingStuff and showCUSTOMcurrent */
#define CM_NIC_CUST_PARM	"NIC_CUST_PARM"

/* used by DoReminingStuff and idremove command */
#define CM_DRIVER_TYPE		"DRIVER_TYPE"
#define CM_ODI_TOPOLOGY		"ODI_TOPOLOGY"
#define CM_MDI_NETX			"MDI_NETX"

/* NOTE:  do not change the name DEV_NAME unless you change dlpi's 
 * mdilib.c too! 
 */
#define CM_DEV_NAME			"DEV_NAME"
#define CM_NETINFO_DEVICE	"NETINFO_DEVICE"
/* CM_CUSTOM_CHOICE is appended onto the parameter that goes in the resmgr 
 * the total parameter length cannot exceed RM_MAXPARAMLEN(15) else RMputvals
 * will fail with EINVAL.  CUSTOM_CHOICE used to be "_CHOICE" but that
 * exceeded this limit so we reduced it to just an underscore and updated
 * the doc/bcfgfiles CUSTOM documentation appropriately.
 */
#define CM_CUSTOM_CHOICE   "_"  /* for custom param FOO, this is FOO_ */

/* used by assorted routines */
#define CM_BCFGPATH        "BCFGPATH"

/* size of buffer you *should* send to getstamp().  Also controls max size
 * of string in .ndnote ELF section 
 */
#define NDNOTEMAXSTRINGSIZE 255

/* list buffer sizes for our internal housekeeping */
#define LISTBUFFERSIZE (96 * 1024)

/* macros for IsParm */
#define PARAM_AVAIL 0
#define PARAM_TAKEN 1

#define ISPARAMAVAIL(B, C) IsParam(PARAM_AVAIL, B, C, 0, RM_KEY, NULL)
#define ISPARAMTAKEN(B, C) IsParam(PARAM_TAKEN, B, C, 0, RM_KEY, NULL)

/* the RO_ versions of these macros indicate that the resmgr is already
 * open and that IsParam shouldn't open it up again
 */
#define RO_ISPARAMAVAIL(B, C) IsParam(PARAM_AVAIL, B, C, 1, RM_KEY, NULL)
#define RO_ISPARAMTAKEN(B, C) IsParam(PARAM_TAKEN, B, C, 1, RM_KEY, NULL)

/* the skipkey versions of these macros */
#define ISPARAMAVAILSKIPKEY(B, C, D) IsParam(PARAM_AVAIL, B, C, 0, D, NULL)
#define ISPARAMTAKENSKIPKEY(B, C, D) IsParam(PARAM_TAKEN, B, C, 0, D, NULL)
#define RO_ISPARAMAVAILSKIPKEY(B, C, D) IsParam(PARAM_AVAIL, B, C, 1, D, NULL)
#define RO_ISPARAMTAKENSKIPKEY(B, C, D) IsParam(PARAM_TAKEN, B, C, 1, D, NULL)

/* and the special case macro */
#define RO_ISPARAMTAKENBYDRIVER(B, C, E) IsParam(PARAM_TAKEN,B,C,1,RM_KEY,E)

/* HIERARCHY is where DEPEND= and ODI(lsl msm odimem etc) drivers live
 * the "real" DLPI driver (used by all MDI drivers) and "dummy",
 * the multiple instance fake DLPI driver, also live here
 */
#define HIERARCHY "/etc/inst/nics/drivers"
#define NDHIERARCHY "/etc/inst/nd"

/* where we write netcfg files to give to stacks */
#define NETXINFOPATH "/usr/lib/netcfg/info"
#define NETXINITPATH "/usr/lib/netcfg/init"
#define NETXREMOVEPATH "/usr/lib/netcfg/remove"
#define NETXLISTPATH "/usr/lib/netcfg/list"
#define NETXRECONFPATH "/usr/lib/netcfg/reconf"
#define NETXCONTROLPATH "/usr/lib/netcfg/control"

/* directory where we write our dlpimdi file */
#define DLPIMDIDIR    NDHIERARCHY

/* this is the parameter where we store the real MODNAME in the backup key */
#define CM_BACKUPMODNAME "BACKUPMODNAME"

/* REALKEY is stored at the backup key to identify the key that this entry
 * is backing up.   We see if NCFG_ELEMENT at the the backup key and
 * the key pointed to by REALKEY are the same.  If not then something
 * bad happened or autoconf just shifted things around.
 */
#define CM_REALKEY        "REALKEY"

/* BACKUPKEY is stored at the primary key to identify the backup key that
 * is currently associated with it.
 * autoconf can shift these around as boards come and go 
 */
#define CM_BACKUPKEY      "BACKUPKEY"

/* NIC_CARD_NAME has the textual description of the card */
#define CM_NIC_CARD_NAME  "NIC_CARD_NAME"

/* this is the prefix that we add to IRQ, MEMADDR, and DMA when performing
 * ISA verify functions.  We must use a separate prefix instead of the
 * normal CM_IRQ, CM_MEMADDR, CM_DMA because a later call to ISPARAMAVAIL
 * or ISPARAMTAKEN would find them in the resmgr, and that's not what we
 * want.  By appending our prefix we can search the resmgr for other
 * occurances of the numeric value to determine if a conflict exists.
 */
#define CM_ISAVERIFYPREFIX "_"

/* parameter added to resmgr to denote what mode we're in for
 * isaautodetect work (either get or set)
 */
#define CM_ISAVERIFYMODE "ISAVERIFYMODE"

/* parameter which has all of the DEPEND= from bcfg file */
#define CM_NIC_DEPEND "NIC_DEPEND"

/* parameter set by usr/src/i386/sysinst/cmd/desktop/ii_do_netinst
 * at the resmgr key we used to do a netinstall
 */
#define CM_IICARD "IICARD"

/* parameter set in the resmgr to denote if this nic supports promiscuous
 * mode
 */

/* where was stamp found (returned by getstamp()) */
#define STAMP_NOT_FOUND 0
#define STAMP_FOUND_IN_NDNOTE 1
#define STAMP_FOUND_IN_SYMBOL 2

/* unit or instance number */
/* NOTE:  do not change the name NDCFG_UNIT unless you change dlpi's 
 * mdilib.c too! 
*/
#define CM_NDCFG_UNIT "NDCFG_UNIT" 
