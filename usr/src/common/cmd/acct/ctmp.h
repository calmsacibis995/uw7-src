/*		copyright	"%c%" 	*/

#ident	"@(#)acct:common/cmd/acct/ctmp.h	1.5.3.3"
#ident "$Header$"
/*
 *	connect time record (various intermediate files)
 */
struct ctmp {
	dev_t	ct_tty;			/* major minor */
	uid_t	ct_uid;			/* userid */
	char	ct_name[8];		/* login name */
	long	ct_con[2];		/* connect time (p/np) secs */
	time_t	ct_start;		/* session start time */
};
