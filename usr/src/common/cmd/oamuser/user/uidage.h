#ident	"@(#)uidage.h	1.2"
#ident  "$Header$"


#define	ADD	 	  1
#define	CHECK	 	  2
#define	REM	 	  3
#define	UID_MIN		100
#define	BUFSIZE 	256

#define UIDAGEF		"/etc/security/ia/ageduid"
#define UATEMP		"/etc/security/ia/autemp"
#define OUIDAGEF	"/etc/security/ia/oageduid"

struct uidage {
	uid_t	uid;	/* uid being aged */
	long	age;	/* date when uid becomes available */
	};


/*
 * The uid_blk structure is used in the search for the default
 * uid.  Each uid_blk represent a range of uid(s) that are currently
 * used on the system.
*/
struct	uid_blk { 			
	struct	uid_blk	*link;
	uid_t		low;		/* low bound for this uid block */
	uid_t		high; 		/* high bound for this uid block */
};

