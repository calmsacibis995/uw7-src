/*		copyright	"%c%" 	*/


/*	Portions Copyright (c) 1988, Sun Microsystems, Inc.	*/
/*	All Rights Reserved.					*/

#ident	"@(#)tcpio:ttoc.h	1.2.1.2"
#ident "$Header$"

#include    "tsec.h"

    /* this should be a system wide definition */
#define	LVL_SZ 	    32	/* lenght of level/class/... name + NULL + 1 */

#define IDs	3	/* number of different types of relevant IDs */

/*
**  Save stat/data flags.
**  Used in the Sava_stat and Save_data vars to indicate
**  which DB/LOGs to save the state and/or information of.
*/

#define S_UID	    0x00000001
#define S_GID	    0x00000002
#define S_LID	    0x00000004

/*
**    Type Specifiers
*/

/* NOTE: a related number (TS_SZ - 1) is hard coded in other places: */
/* (sprintf, sscanf) */
#define	TS_SZ  	    3	/* length of TS (test + NULL). */

#define UID	    "UI"
#define GID	    "GI"
#define LID	    "LI"

/*
** Instead of using the character representation of the ID type, in most cases 
** the TS enumerator is used to locate/identify the type.
*/

enum TS {
    UI,
    GI,
    LI,
    TS_ERR = 999
};


/*
**    TTOC/validate state flags
*/

/*
** V_... flags are used in the validate structure field v_flags which refers
** to an entire group of value-string mappings (either UI, GI, or LI).
**
** VS_... flags are used in the val_str structure field vs_state which refers
** to a particular ID (UID, GID, or LID).
*/

#define V_OK	    0x00000001     /* DB did not change or an   */
				   /* override option was used  */
#define V_CHANGE    0x00000002	   /* state of a DB has changed */
#define V_TESTED   (V_OK | V_CHANGE)   /* DB is tested */

#define V_DB   	    0x00000010     /* database state was saved  */
#define V_DATA      0x00000020     /* actual DB data was saved  */
#define V_LOG	    0x00000040     /* logfile state was saved   */

#define V_REMAP     0x00000100     /* use the alternative value */
#define	V_LAST	    0x00000200	   /* last-segment-of-TTOC flag */


#define VS_OK	    0x00000001	   /* ID is valid or fixed in some way */
				   /* or an override option was used */
/* 
 * ID could not be fixed (no remap options).
 * For LIDs it means the LID was put in the valid-inactive state since the
 * archive was created.
 * For UID and GIDs it means they are reused (i.e. have different "name").
 */
#define VS_CHANGE   0x00000002	   

#define VS_BAD      0x00000004     /* bad ID (nonexistent or can't verity) */
#define VS_TESTED   (VS_OK | VS_CHANGE | VS_BAD)   /* ID is tested */
#define VS_MAPPED   0x00000010	   /* ID has been mapped via the TTOCTT */


/*
**  location of info DB/logs
*/

#define	UI_DB	"/etc/passwd"
#define	GI_DB	"/etc/group"
#define	LI_DB	"/etc/security/mac/lid.internal"

#define	UI_LOG	""
#define	GI_LOG	""
#define	LI_LOG	"/etc/security/mac/hist.lid.del"


/*
**  information about ID databases
**  and log files.
*/

struct id_info {
    char  it_ts[TS_SZ];	/* character rep. of the ID (Type Specifier) */
    char *it_db;    	/* database containing the ID <-> text maps */
    char *it_log;   	/* log files, containing records of changes */
    FILE *it_stp;   	/* stream pointer (to log file)	    	    */
};


#define	ID_REC(ts)  	&Db_log[ts]
#define	TS_NAME(ts)	Db_log[ts].it_ts /* the TS string */

/*
** val_str is the basic structure used to describe
** the mapping between an ID and its character string.
** The "extra fields" are used in case the mapping
** has changed since "save" time.
*/

typedef struct val_str {	/* generic format: value && string */
    ulong	vs_state;	/* record state */
    id_t	vs_value;   	/* original numeric ID value */
    id_t	vs_current;	/* the "correct" value (original/new/remap) */
    char	*vs_name;   	/* text representation of this ID */
} vs, *VS;

/*
** the size of a vs srtucture (including the string it points to)
*/

#define VS_SZ	(3*8)
#define	VS_MAX_SZ    (11 + 32 + 2)
#define	VS_CNT	2

#define	VAL_CNT	10   	/* validate structure */
#define	VALSZ	(10*8)	/* validate */


/*
** validate is the structure that describe the state
** of each of the relevant IDs.
** Note that not all the fileds are save on the medium.
*/

typedef struct validate {
    ulong   v_flags;        /* are ALL the IDs valid? etc.*/
    enum TS v_ts;           /* ID type specifier    	  */
    time_t  v_db_mdate;     /* last-modify date [stat(2)] */
    time_t  v_db_cdate;     /* last-change date [stat(2)] */
    off_t   v_db_size;      /* size of database file	  */
    time_t  v_log_mdate;    /* last-modify date [stat(2)] */
    time_t  v_log_cdate;    /* last-change date [stat(2)] */
    off_t   v_log_size;     /* size of history file 	  */
    off_t   v_med_size;     /* size of medium-version file */
    id_t    v_nelm;         /* number of elements (IDs) */
    id_t    v_remap;        /* value of ID to map to (for -R/-N) */
    struct val_str *v_map;  /* pointer to the data table */
} valid_info, *VP;


/*
** action on changed/bad ID
*/

#define	MOD 	0		/* modify ID to new value */
#define	USE 	1		/* use bad (old) ID value anyway */


/*
** sub-options of the -n option
**
*/

/* NOTE: NO_OPS	5 (number of check overrides) is defined in tsec.h */

#define	NO_SYS	0x001		/* no host (system) name check */
#define	NO_UID	0x002		/* no UID checks */
#define	NO_GID	0x004		/* no GID checks */
#define	NO_LID	0x008		/* no LID checks */
#define	NO_ACT	0x010		/* no inactive-LID checks */
