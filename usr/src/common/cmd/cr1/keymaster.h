/*		copyright	"%c%" 	*/

#ident	"@(#)keymaster.h	1.2"
#ident  "$Header$"

/*  Declarations used in cr1 daemon implementations  */

#define NPOLLFDS 64	/* # file descriptors polled by daemon */

#define DEF_KEYFIL	"keys"
#define DEF_KMPID	"kmpid"
#define DEF_LOGDIR	"/var/iaf"
#define DEF_KMLOG	"log"

#define OLD_MASTER	"Enter master key for '%s' scheme:"
#define OLD_MASTERID	":47"
#define NEW_MASTER	"Enter new master key for '%s' scheme:"
#define NEW_MASTERID	":48"
#define VER_MASTER	"Re-enter new master key for '%s' scheme:"
#define VER_MASTERID	":49"

#define OLD_KEY		"Enter old %s key:"
#define OLD_KEYID	":50"
#define NEW_KEY		"Enter new %s key:"
#define NEW_KEYID	":51"
#define VER_KEY		"Re-enter new %s key:"
#define VER_KEYID	":52"

#define CLEARTEXT	"CLEARTEXT"
#define CIPHERTEXT	"CIPHERTEXT"
#define CLEARTEXT_FILE	"CLEARTEXT\tKEYS\n"

#ifdef DEBUG1
#define DEBUG
#define DUMP(a,b,c)	{ \
				fprintf(logfp, "Buffer '%s' contents: ", c); \
				fwrite((a), 1, (b), logfp); \
				fprintf(logfp, "\n"); \
				fflush(logfp); \
			}
#else
#define DUMP(a,b,c)
#endif

#ifdef DEBUG
#define DLOG(a,b)	LOG(a,b)
#else
#define DLOG(a,b)
#endif

#define LOG(a,b)	{ \
			if (logfp) { \
				fprintf(logfp, "(%ld%c) ", mypid, role); \
				fprintf(logfp, a, b); \
				fflush(logfp); \
			} \
			}

typedef struct Ids {
	uid_t	uid;
	uid_t	gid;
} Ids;

typedef struct core_key {
	Principal local;
	Principal remote;
	Key key;
	struct core_key *next;
} CORE_KEY;
