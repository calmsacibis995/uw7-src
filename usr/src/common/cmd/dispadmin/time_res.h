#ident	"@(#)mp.cmds:common/cmd/dispadmin/time_res.h	1.1"
/*
 *	Definitions for commonly used resolutions.
 */

#define	SEC		1
#define	MILLISEC	1000
#define MICROSEC	1000000
#define NANOSEC		1000000000

/* Scaling factor used to save
** fractional remainder.
*/

#define SCALE		1000000	/* Scaling factor used to save		*/
				/* fractional remainder.		*/

/*
 *	Definitions for specifying rounding mode.
 */

#define HRT_TRUNC	0	/* Round results down.	*/
#define HRT_RND		1	/* Round results (rnd up if fractional	*/
				/*   part >= .5 otherwise round down).	*/
#define	HRT_RNDUP	2	/* Always round results up.	*/

/*
 *	Definition for the type of internal buffer used with the
 *	HRT_STARTIT and HRT_GETIT commands.
 */


/*
 *	Structure used to represent a high-resolution time-of-day
 *	or interval.
 */

typedef struct hrtime {
	ulong	hrt_secs;	/* Seconds.				*/
	long	hrt_rem;	/* A value less than a second.		*/
	ulong	hrt_res;	/* The resolution of hrt_rem.		*/
} hrtime_t;


