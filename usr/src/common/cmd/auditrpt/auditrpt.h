/*		copyright	"%c%" 	*/

#ident	"@(#)auditrpt.h	1.2"
#ident  "$Header$"

#ifndef _LIMITS_H
#include <limits.h>
#endif

#define MAXOBJ		100	/* maximum # of objects for post-selection */
#define MXUID		100	/* max # of lognames/uids for post-selection */
#define EVTMAX		128	/* maximum # of events for post-selection */
#define MAXNMS		200	/* maximum number of names in a path */
#define NAMELEN		32	/* maximum number of characters in logname, 
				 * group name, system call name, privilege 
				 * name in the auditmap file
				 */
#define	MAXNUM		20	/* maximum number of characters in a number */
#define	SUBTYPEMAX	20	/* maximum number of characters in miscellaneous subtype */

#define STD_IN	"stdin"
#define VNULL ""
#define V1 "1.0"
#define V2 "2.0"
#define V3 "3.0"
#define V4 "4.0"
#define V1RPT "/etc/security/audit/auditrpt/auditrptv1"
#define V1FLTR "/etc/security/audit/auditrpt/auditfltrv1"
#define V4RPT "/etc/security/audit/auditrpt/auditrptv4"
#define V4FLTR "/etc/security/audit/auditrpt/auditfltrv4"

#define REG		 0x1 
#define	CHAR		 0x2
#define	BLOCK	         0x4
#define	LINK	         0x8
#define	DIR       	0x10
#define	PIPE     	0x20
#define	SEMA		's'
#define	SHMEM		'h'      
#define	MSG 		'm'       
#define	MODULE 		"module"

/* define auditrpt options */
#define O_MASK		0x1	/* union */ 
#define B_MASK		0x2	/* backward */ 
#define W_MASK  	0x4	/* follow mode */
#define E_MASK  	0x8	/* events */
#define U_MASK  	0x10	/* user */
#define F_MASK  	0x20	/* filename */
#define T_MASK  	0x40	/* type */
#define L_MASK  	0x80	/* level */
#define S_MASK  	0x100	/* start time */
#define H_MASK  	0x200	/* end time */
#define AS_MASK 	0x400	/* status: success */
#define AF_MASK 	0x800	/* status: failure */
#define M_MASK  	0x1000	/* map */
#define P_MASK  	0x2000	/* privilege */
#define R_MASK  	0x4000	/* level range */
#define I_MASK		0x8000	/* standard input */
#define V_MASK  	0x10000	/* miscellaneous subtype */
#define Z_MASK  	0x20000	/* application records */
#define X_MASK          0x40000 /* LWP ID */

#define adt_privbit(p)		(priv_t)(1 << (p))

#define SUCCESS	's'
#define FAIL   	'f'

/* 
 * messages for catalog
 */

/* error messages */
#define RE_BAD_OUT ":86:invalid outcome specified \n"
#define RE_BAD_PRIV ":87:invalid privilege \"%s\" supplied\n"
#define RE_INCOMPLETE ":88:additional options required\n"
#define RE_BAD_COMB ":89:invalid option combination %s, %s\n"
#define RE_NOT_ON ":90:auditing currently disabled, logfile must be specified\n"
#define RE_BAD_TIME ":91:start time must be earlier than the end time \n"
#define RE_BAD_EVENT ":92:event type or class %s does not exist\n"
#define RE_BAD_NAME ":93:full pathname must be specified for %s\n"
#define RE_BAD_TIME2 ":94:invalid time format\n"
#define RE_BAD_RANGE ":95:maximum security level does not dominate minimum security level\n"
#define RE_BAD_OTYPE ":96:invalid object type specified: %s\n"
#define RE_W_DISABLE ":97:auditing disabled\n"
#define RE_BADDIR ":98:cannot open auditmap directory %s\n"
#define RE_NOARG ":99:Option requires an argument -- %s\n"
#define RE_NOPKG ":34:system service not installed\n"

#define E_NO_LOGS ":100:all event log files specified are inaccessible\n"
#define E_BAD_MIN ":101:invalid minimum security level specified\n"
#define E_BAD_MAX ":102:invalid maximum security level specified\n"
#define E_LOG_RTYPE ":103:bad log record type %d\n"
#define E_MAP_RTYPE ":104:bad map record type %d\n"
#define E_PERM  ":17:Permission denied\n"
#define E_BDMALLOC  ":19:unable to allocate space\n"
#define E_BAD_ARCH  ":105:log file's format or byte ordering(%s) is not readable on current architecture\n"
#define E_INV_A	":106:invalid argument given to %s option\n"
#define E_TOOMANY ":107:too many levels specified\n"
#define E_TOOLONG ":108:argument list for option %s too long\n"
#define E_CHMOD ":109:chmod(2) failed for temporary file, errno = %d\n"
#define E_FMERR ":110:error manipulating file\n"
#define E_BADBGET ":111:could not get buffer attributes\n"
#define E_BADLGET ":112:could not get current log attributes\n"
#define E_BADSTAT ":113:could not determine status of auditing\n"
#define E_LVLOPER ":35:%s() failed, errno = %d\n"
#define E_BAD_LVL ":114:security level specified does not exist in map\n"

/* warning messages */
#define RW_MISMATCH ":115:machines for log file \"%s\" (%s) and map file (%s) do not match\n"
#define RW_NO_MATCH  ":116:no match found in event log file(s)\n"
#define RW_NO_LOG ":117:event log file %s does not exist\n"
#define RW_IGNORE ":118:log file \"%s\" ignored\n"
#define RW_OUTSEQ ":119:event log file(s) are not in sequence or missing\n"
#define RW_NO_USR1 ":120:user id %d does not exist in audit map \n"
#define RW_NO_USR2 ":121:user id %s does not exist in audit map \n"
#define RW_NO_VH0 ":122:data in audit buffer will not be immediately displayed\n"
#define RW_NO_MAP ":123:cannot open audit map file %s\n"
#define RW_BADLTDB ":124:the ltdb files are missing or incomplete in the auditmap directory\n"
#define RW_SPEC ":125:cannot read and write character special device simultaneously\n"
#define RW_MISC ":126:misformed miscellaneous record\n"
#define RW_KEYWORD ":127:keyword \"%s\" should not be used in conjunction with individual privileges\n"

#define W_MISS_PATH ":128:missing pathname for process P%d\n"
#define W_NOPROC ":129:process information for P%d is incomplete\n"

/* usage */
#define USAGE1 ":130:usage:\n"  
#define USAGE2 ":131:    auditrpt [-o] [-i] [-b | -w] [-e [!]event[,...]] [-u user[,...]]\n"
#define USAGE3 ":132:             [-f object_id[,...]] [-t object_type[,...]] [-s time]\n"
#define USAGE4 ":133:             [-h time] [-l level | -r levelmin-levelmax] [-a outcome]\n" 
#define USAGE5 ":134:             [-m map] [-p all|priv[,...]] [-v subtype] [log [...]]\n"

/* other cataloged messages */
#define M_1 ":135:Command Line Entered: "
#define M_2 ":136:\nDATE: %s%s, "
#define M_3 ":137:LOG NUMBER: %03d, "
#define M_4 ":138:AUDIT VERSION: %s\n"
#define M_5 ":139:\nMACHINE ID: %s\n"


#define V4USAGE2 ":175:    auditrpt [-o] [-i] [-b | -w] [-x] [-e [!]event[,...]] [-u user[,...]]\n"
#define W_NOCRED ":176:credential information for P%d is incomplete\n"
#define W_CRED_NO_FREE ":177:credential structure could not be freed\n"
#define E_NO_VER ":178:could not obtain version number\n"
#define E_BAD_VER ":179:unknown audit version number\n"
#define E_BAD_XOP ":180:-x may not be used with this version\n"
#define E_NOVERSPEC ":181:Version specific auditrpt not found: %s\n"
#define E_RPTNOTX ":182:Version specific auditrpt not executable: %s\n"
#define E_DUPCRED ":183:Duplicate credential sequence numbers encountered\n"
#define E_NOVERSPECF ":184:Version specific auditfltr not found: %s\n"
#define E_RPTNOTXF ":185:Version specific auditfltr not executable: %s\n"
#define E_NOCOMPATVER ":186:Incompatible log file version number\n"

/* define modes used when printing dac records */
#define AREAD		0x4	/* read permission */
#define AWRITE		0x2	/* write permission */
#define AEXEC		0x1	/* execute permission */
#define GREAD		0x20	/* grp read permission */
#define GWRITE		0x10	/* grp write permission */
#define GEXEC		0x8	/* grp execute permission */
#define OREAD		0x100	/* owner read permission */
#define OWRITE		0x80	/* owner write permission */
#define OEXEC		0x40	/* owner execute permission */

#define ACCESS_ACL	0x1	/* user specified "-a" */
#define DEFAULT_ACL	0x2	/* user specified "-d" */
#define	BUFSIZE	20479	

/* filenames for LTDB files */
#define LDF 		"/lid.internal"
#define ALASF 		"/ltf.alias"
#define CATF 		"/ltf.cat"
#define CLASSF 		"/ltf.class"
 
/* filenumbers for LTDB files */
#define LID 		1
#define ALAS 		2
#define CTG 		3
#define CLS 		4

/* structures for loading the auditmap */
typedef struct id_tbl {
	char name[NAMELEN+1];
	int  id;
}ids_t;

typedef struct cls_tbl {
	char alias[ADT_EVTNAMESZ+1];
	struct ent_tbl *tp;
	struct cls_tbl *next;
}cls_t;

typedef struct ent_tbl {
	char type[ADT_EVTNAMESZ+1];
	struct ent_tbl *tp;
}ent_t;

/* audit version and time time zone info obtained from the map */
typedef struct env_info{	
	char	version[ADT_VERLEN];
	time_t	gmtsecoff;		
}env_info_t;

/* global variables shared between auditrpt and auditfltr */
extern int	char_spec;	/* is file character special? */	
extern int 	free_size;	/* size of free-format data */

#define	d_spec		spec_data.spec
#define r_rtype 	cmn.c_rtype
#define	r_size		cmn.c_size
#define r_pid		cmn.c_pid
#define r_rtime 	cmn.c_time
#define	r_event		cmn.c_event
#define	r_status	cmn.c_status

/* global functions */
extern	int	adt_getrec();
extern	void	adt_exit();
extern 	int	adt_lvldom();
extern	int	getspec();
extern	int	has_path();
extern	int	magic_number();
extern	void	usage();
