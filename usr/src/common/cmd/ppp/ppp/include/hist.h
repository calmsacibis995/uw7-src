#ifndef _HIST_H
#define _HIST_H
#ident	"@(#)hist.h	1.2"

#include <sys/types.h>
#include <time.h>

/*
 * This structure is used to hold a call event ... an a series
 * of these are kept in a list to provide a call history
 */
struct histent_s {
	int	he_op;	       		/* Operation */
	time_t	he_time;		/* Time of event */
	int	he_error;		/* Errors, errno */

	int	he_alflags;		/* Links flags (al_flags) */
	int	he_alreason;		/* Link reason codes (al_reason) */
	int	he_abreason;		/* Bundle reason codes (ab_reason) */

	char 	he_linkid[MAXID + 1]; 	/* link name */
	char 	he_bundleid[MAXID + 1]; /* bundle name */
	char 	he_dev[MAXID + 1]; 	/* device name */

	char	he_uid[MAXUIDLEN + 1];	/* Users login if any */
	char	he_aid[MAXID + 1];	/* Auth Id */
	char 	he_cid[MAXID + 1];	/* Caller Id */
	char 	he_sys[MAXID + 1];	/* System Id */
};

/*
 * Values for he_op
 */
#define HOP_ADD 0x01
#define HOP_DROP 0x02


#endif /* _ACT_H */
