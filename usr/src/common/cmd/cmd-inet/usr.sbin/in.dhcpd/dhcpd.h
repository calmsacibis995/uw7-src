#ident "@(#)dhcpd.h	1.3"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1996 Computer Associates International, Inc.
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

/*
 * Definitions for the DHCP server (dhcpd)
 */

/*
 * Hardware type codes
 */

#define HTYPE_ETHERNET	1
#define HTYPE_IEEE802	6

/*
 * Lengths for the above
 */

#define HLEN_ETHERNET	6
#define HLEN_IEEE802	6

/*
 * "Opaque" is a special client ID type which means that the id is
 * an arbitrary string of bytes.
 */

#define IDTYPE_OPAQUE	0

/*
 * Option types
 */

#define OTYPE_UINT8	0
#define OTYPE_INT8	1
#define OTYPE_UINT16	2
#define OTYPE_INT16	3
#define OTYPE_UINT32	4
#define OTYPE_INT32	5
#define OTYPE_BOOLEAN	6
#define OTYPE_STRING	7
#define OTYPE_BINARY	8
#define OTYPE_ADDR	9
#define OTYPE_NB_NODE	10

/*
 * OTYPE_MASK masks the portion of the type value that should be checked
 * against the above.  If the OTYPE_AUTO flag is set, the option allows the
 * keyword "auto", which means the server uses some automatic method to
 * determine the value of the option.  A setting of "auto" is represented
 * by a NULL val pointer in the OptionSetting structure.
 */

#define OTYPE_MASK	0x7f
#define OTYPE_AUTO	0x80
#define OTYPE(t)	((t) & OTYPE_MASK)

typedef struct {
	char *name;		/* config file name */
	u_short code;		/* option code */
	u_char type;		/* option type (see above) */
	u_char array;		/* if nonzero, this is an array */
	/*
	 * min_len and max_len apply to arrays, strings, and binary items.
	 */
	int min_len;
	int max_len;
	/*
	 * To avoid problems with initialization, the min and max values are
	 * pointers to values of the appropriate type.  These are only
	 * used for integers.  If either is NULL, that constraint
	 * does not apply.
	 */
	void *min_val;
	void *max_val;
} OptionDesc;

/*
 * MAX_OPT_CODE is the largest possible DHCP option code.
 */

#define MAX_OPT_CODE		255

/*
 * Codes for "options" that are not actually DHCP options (i.e.,
 * things that go in the fixed part).  These all have values above
 * 255, whicn is the maximum value for a real option code.
 */

#define OPT_BOOT_SERVER		256
#define OPT_BOOT_FILE_NAME	257
#define OPT_BOOT_DIR		258
#define OPT_TFTP_DIR		259

#define NUM_OPTS		260

/*
 * Limits on user-defined options
 */

#define MIN_USER_OPTION		1
#define MAX_USER_OPTION		254

/*
 * The OptionSetting structure represents a setting of a particular option.
 */

typedef struct OptionSetting {
	struct OptionSetting *next;	/* ptr to next in list */
	struct OptionSetting *merge;	/* used to construct merged list */
	OptionDesc *opt;	/* ptr to option description */
	void *val;		/* ptr to the value or NULL for "auto" */
	int len;		/* length for string, binary, & array */
	int size;		/* size in bytes of the value */
} OptionSetting;

/*
 * The following structures represent entries in the config file.
 */

typedef struct Client {
	u_char id_type;
	u_char *id;
	int id_len;
	struct in_addr addr;
	OptionSetting *options;
} Client;

typedef struct UserClass {
	char *class;
	int class_len;
	OptionSetting *options;
} UserClass;

typedef struct VendorClass {
	u_char *class;
	int class_len;
	OptionSetting *options;
} VendorClass;

typedef struct Subnet {
	struct Subnet *next;
	/*
	 * subnet and mask are stored in host order.
	 */
	struct in_addr subnet;
	struct in_addr mask;
	char *pool;
	u_long lease_dflt;
	u_long lease_max;
	int t1;
	int t2;
	OptionSetting *options;
} Subnet;

/*
 * The ServerParams structure contains server operating parameters that
 * are set in the configuration file.
 */

typedef struct {
	int set;			/* nonzero if params have been set */
	int aas_remote;			/* use remote AAS if nonzero */
	struct in_addr aas_server;	/* address server host if remote */
	char *aas_password;		/* password if remote */
	u_char option_overload;		/* if nonzero option overload is ok */
	u_long lease_res;		/* initial lease reservation time */
	/*
	 * The lease padding is stored as a number by which the client lease
	 * time is multiplied to get the amount of time for which the lease
	 * is reserved (i.e., for 5% padding, lease_pad == 1.05).
	 */
	double lease_pad;
	u_char address_probe;		/* if nonzero ping addresses */
} ServerParams;

/*
 * Default values for server parameters
 */

#define DFLT_OPTION_OVERLOAD	0
#define DFLT_LEASE_RES		180
#define DFLT_LEASE_PAD		10	/* units: 1/1000 */
#define DFLT_ADDRESS_PROBE	1

/*
 * Default values for subnet parameters
 */

#define DFLT_LEASE_DFLT		86400
#define DFLT_LEASE_MAX		LEASE_INFINITY
#define DFLT_T1			500
#define DFLT_T2			875

/*
 * Configuration file limits
 */

#define MAX_LINE_LEN	1023
#define MAX_PASSWORD_LEN 256

/*
 * Service name used with address server
 */

#define DHCP_SERVICE_NAME	"DHCP"

/*
 * Memory allocation macros
 */

#define str_alloc(t)		((t *) malloc(sizeof(t)))
#define tbl_alloc(t, n)		((t *) malloc((n) * sizeof(t)))
#define tbl_grow(p, t, n)	((t *) realloc((p), (n) * sizeof(t)))

/*
 * Miscellaneous macros
 */

/*
 * MSGSIZETODHCPSIZE converts a maximum message size value to the
 * amount of space available for the DHCP packet.
 */

#define IP_HEADER_SIZE		20
#define UDP_HEADER_SIZE		8
#define MSGSIZETODHCPSIZE(s)	((s) - IP_HEADER_SIZE - UDP_HEADER_SIZE)

/*
 * Global variable declarations
 */

extern ServerParams server_params;
extern OptionSetting *global_options;
extern OptionDesc *user_options[];
extern OptionSetting *option_defaults[];
extern char *config_file;
extern int reconfig_flag;
extern int config_ok;
extern int standalone;
extern int debug;
extern int bootp_fwd;
extern struct sockaddr_in bootpd_addr;

/*
 * TLI externs that aren't declared in tiuser.h
 */

extern int t_errno;
extern int t_nerr;
extern char *t_errlist[];
