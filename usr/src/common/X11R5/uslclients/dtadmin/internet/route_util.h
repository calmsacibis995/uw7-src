#ifndef	NOIDENT
#pragma	ident	"@(#)dtadmin:internet/route_util.h	1.3"
#endif

#ifndef _ROUTE_UTIL_H
#define _ROUTE_UTIL_H

typedef enum {
	C_LEVEL,
	C_FULLPATH,
	C_OVERRIDE,
	C_ISRUNNING,
	C_CONFIGFILE,
	C_OPTIONS
} ConfigIndex_t;

typedef struct _config_token {
	char *	ct_level;
	char *	ct_fullCmdPath;
	char *	ct_overridingCmdPath;
	char *	ct_isRunning;
	char *	ct_configFile;
	char *	ct_cmdOptions;
} ConfigToken_t;

typedef enum {
	I_PREFIX,
	I_UNIT,
	I_ADDR,
	I_DEVICE,
	I_IFCONFIGOPT,
	I_SLINKOPT
} InterfaceIndex_t;

typedef struct _interface_token {
	char *	it_prefix;
	char *	it_unit;
	char *	it_addr;
	char *	it_device;
	char *	it_ifconfigOptions;
	char *	it_slinkOptions;
} InterfaceToken_t;

typedef enum {
	DataLine,
	CommentLine,
	VersionLine,
	BlankLine,
	BadLine,
	DeleteLine
} LineKind_t;

typedef struct _config_line {
	LineKind_t		cl_kind;
	ushort_t		cl_cmdStarted;
	int			cl_cmdReturned;
	char *			cl_origLine;
	char *			cl_newLine;
	void *			cl_tokens;
	struct _config_line *	cl_next;
} ConfigLine_t;

typedef enum { ConfigFile, InterfaceFile } FileKind_t;

typedef struct _config_file {
	FileKind_t	cf_kind;
	ConfigLine_t *	cf_lines;
	ConfigLine_t *	cf_sortedLines;
	char *		cf_fileName;
	char *		cf_version;
	ushort_t	cf_minFields;
#define	CF_FIELDS	6
#define	IF_FIELDS	6
	ushort_t	cf_numUpdates;
	uint_t		cf_numLines;
	char		cf_fieldSeparator;
	char		cf_comment;
	int		(*cf_compareFcn)();
} ConfigFile_t;

typedef struct _query_config {
	char *	qc_string;
	uint_t	qc_flags;
#define QC_MATCH	0x01
#define QC_DEFAULT	0x02
#define QC_REPLACE	0x04
#define QC_END		0x08
#define QC_ISREGULAR	0x10
#define QC_UPDATEMASK	0x20
#define QC_UPDATEROUTE	0x40
} QueryConfig_t;

/* /etc/inet/config tokens */

#define	INITSOCKTOK	"1"
#define	NAMEDTOK	"5"
#define	PPPDTOK		"3"
#define	GATEDTOK	"4a"
#define	ROUTEDTOK	"4b"
#define	ROUTETOK	"4c"
#define	XNTPDTOK	"6"
#define	SHTOK		"7"

/* some interesting netmasks */

#define	CLASS_A_NETMASK	"0xFF000000"
#define	CLASS_B_NETMASK	"0xFFFF0000"
#define	CLASS_C_NETMASK	"0xFFFFFF00"

/* various and sundry paths */
#define	ROUTEPATH	"/usr/sbin/route"
#define CONFIGPATH	"/etc/inet/config"
#define INTERFACEPATH	"/etc/confnet.d/inet/interface"
#define NETDRIVERSPATH	"/etc/confnet.d/netdrivers"

/* miscellany */
typedef enum { ClassA, ClassB, ClassC, ClassOther } Class_t;
typedef enum { IsName = 1, IsIPAddr } AddrKind_t;
enum { Disable, Enable };

#define	INROUTEDPROC	"/usr/sbin/in.routed"
#define	INGATEDPROC	"/usr/sbin/in.gated"
#define	ROUTECMDPATH	"/usr/sbin/route"

/* fcn prototypes */
extern int	readConfigFile(ConfigFile_t *);
extern int	writeConfigFile(ConfigFile_t *);
extern void	cleanupConfigFile(ConfigFile_t *);
extern int	modifyNetmask(ConfigFile_t *, char *, char *);
extern int	modifyDefaultRouter(ConfigFile_t *, char *);
extern void	mungeAddr(char *, char **, char **, Class_t);
extern int	decideAddrClass(char *, Class_t *, char **);
extern int	getNetmask(ConfigFile_t *, char * addr, char **, Class_t *);
extern int	getDefaultRouter(ConfigFile_t *, char **, AddrKind_t *);
extern int	findProcess(char *, pid_t *);
extern void	breakupAddr(char *, char *, char *, char *, char *);
extern int	killProcess(pid_t);
extern int	modifyRoutedEntry(ConfigFile_t *, int);
extern int	getAddrFromName(char *, char **);
extern int	getNameFromAddr(char *, char **);
extern int	validateNetmask(ulong_t);
extern int	getBroadcastFromAddr(char *, ulong_t, char **);
extern char *	strndup(char *, int);

#endif /* _ROUTE_UTIL_H */
