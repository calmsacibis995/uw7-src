/*		copyright	"%c%" 	*/

#ident	"@(#)acct:common/cmd/acct/ptmp.h	1.5.1.3"
#ident "$Header$"
/*
 *	per process temporary data
 */
struct ptmp {
	uid_t	pt_uid;			/* userid */
	char	pt_name[8];		/* login name */
	long	pt_cpu[2];		/* CPU (sys+usr) P/NP time tics */
	unsigned pt_mem;		/* avg. memory size (64byte clicks) */
};	
