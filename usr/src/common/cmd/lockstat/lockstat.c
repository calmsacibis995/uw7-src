#ident	"@(#)mp.cmds:common/cmd/lockstat/lockstat.c	1.5"
#ident	"$Header$"


#include <fcntl.h>
#include <libelf.h>
#include <nlist.h>
#include <stdio.h>
#include <storclass.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/var.h>
#include <errno.h>
#include <sys/dl.h>
#include <sys/list.h>
#include <sys/ksynch.h>



/*
 * Todo:
 *	
 *	- if user indicates that the "raw" data be saved in a file,
 *	  do so. This would mean extracting all the lksblks, inverting
 *	  correspondence, and printing out the lkinfobuffers, statbuffers,
 *	  and string names in a file.
 *	- use user supplied "maximum" values for numbers of lksblks, lkinfos,
 *	  and statbuffers.
 *	- print processed data: with selected columns.
 *	- handle the double long stats: wait and hold times.
 */

/* 
 * Lock stats extractor.
 *
 *	The stats extractor reads /dev/kmem to extract the lksblk_t
 *	images. It then creates the following "inverted" data structures
 *	in order to get at the lkinfo structures and the lock name strings
 *	whose addresses are stored in the lockinfo structures:
 *
 *	lkinfohash[]:	
 *		A hash array to translate quickly from a lkinfo pointer
 *		to a single entry in an array called lkinfobuffers,
 *	
 *	lkinfobuffers[]:
 *		Each element of this array stores a pointer to a chain
 *		of statistics buffers, so that all stat blocks that
 *		shared the same lock info structure are on a single chain.
 *		The lkinfobuffers are hashed onto the lkinfohash[] chains
 *		described above.
 *
 *	statbuffers[]:
 *		Each element of this array is used to record the statistics
 *		that were extracted from the kernel. Exactly one statbuffer
 *		corresponds to each lock stat structure read from the kernel.
 *		The statbuffers that correspond to the same lkinfo structure 
 *		are singly linked into a chain anchored at the corresponding
 *		lkinfobuffer[] entry.
 *
 *	The top level routine through which the correspondence is inverted
 *	is invert_correspondence(). 
 *		For each lock statistics buffer, invert_correspondence()
 *	calls insert_stat(), which inserts a statbuffer replica of the 
 *	kernel locking statistics buffer, onto a linked list anchored at 
 *	the appropriate lkinfobuffer. 
 *		To do its work, insert_stat in turn calls getlkinfobuf with 
 * 	the address of the kernel lkinfo structure, so that getlkinfobuf can 
 *	search for the lkinfobuffer.
 *		getlkinfobuf() uses the supplied address as a hash-tag, and
 *	searches the appropriate hash chain (lkinfohash[]) for the 
 *	lkinfobuffer. If none can be found, it allocates a new buffer, and
 *	initializes it with the supplied tag.
 */

#define	DODEBUG 1


#ifdef	DODEBUG
#define	ASSERT(x, func, anum)	{if (!(x)) {(void)errmsg((func), (anum));}}
#else
#define ASSERT(x, func, anum) 
#endif


#define DLEV	0

#define	ENTER(x,y)	{ \
		if (DLEV > 0) { \
			printf("enter %s\t%d\t%d\n", x, y, callcnt); \
			++callcnt; \
		} \
	}

#define	LEAVE(x,y)	{ \
		if (DLEV > 0) { \
			printf("leave %s\t%d\t%d\n", x, y, callcnt); \
			++callcnt; \
		} \
	}

#define	ENTER2(x,y)	{ \
		if (DLEV > 1) { \
			printf("enter %s\t%d\t%d\n", x, y, callcnt); \
			++callcnt; \
		} \
	}

#define	LEAVE2(x,y)	{ \
		if (DLEV > 1) { \
			printf("leave %s\t%d\t%d\n", x, y, callcnt); \
			++callcnt; \
		} \
	}



/*
 * Accumulate a double long integer [high32, low32], into a running
 * double long sum [accum_hi32, accum_lo32]. 
 * For simplicity we are not checking for a 64 bit
 * overflow, assuming it to be extremely improbable.
 */
#define laccum(accum_hi32, accum_lo32, high32, low32) { \
	if (((accum_lo32) += (low32)) < (low32))  \
		(accum_hi32) += ((high32) + 1); \
	else \
		(accum_hi32) += high32; \
}

/*
 * Return the larger of two words.
 */

#define	MAX(a,b)	((a) > (b) ? (a) : (b))




void 		errmsg();		/* err print routine */

void 		insert_stat();		/* copy kernel stat structure into */
					/* a stat buffer and link it on the*/
					/* chain at corresponding lkinfobuf*/
void 		invert_correspondence();/* create the inverse mapping from */
					/* lkinfo structs to stat structs  */
void 		extract_lkinfops();	/* copy lkinfo vaddrs into an array*/
vaddr_t 	getnextlsbp();		/* read next lksblk ptr.	   */
void 		getlksblk();		/* read indicated lksblk.	   */
void 		printlksblk();		/* print indicated lksblk buffer   */
void 		printlkinfobufs();	/* print extracted lkinfobufs      */

void 	getlksblk();		/* read an entire lksblk from /dev/kmem */
void	printlksblk();		/* utility print routine */
vaddr_t	getnextlsbp();		/* obtain kernel v. addr of next stats block */

void 	db_check();		/* debugging routine */


#define	UINT_MAX	4294967295U
#define DEVICE_NAME	"/dev/kmem"
#define PATH_LEN	100		/* path name length for unix object */

#define	MAXLKSBLKS	400	/* max number of lksblks expected */
#define	MAXSTATBUFS	(MAXLKSBLKS*LSB_NLKDS)
#define	MAXLKINFOBUFS	512	/* max number of "distinct" types of locks */
#define	LKINFO_HASHSZ	128	/* width of the hash chain used to speed */
				/* lkinfop tag matching */
#define	LOCKNAMELEN	80

#define	LKINFOHASH(x)	(((x) >> 4)&(LKINFO_HASHSZ -1))

typedef struct	_statbuf {
	struct	_statbuf *sb_next;	
	lkstat_t sb_lkstat;
	vaddr_t	debug_statp;
	vaddr_t	debug_lkinfop;
} statbuf_t;

typedef struct	_lkinfobuf {
	struct _lkinfobuf *lb_next;
	vaddr_t lb_tag;
	int	lb_statcnt;
	statbuf_t *lb_statbufs;	
} lkinfobuf_t;

typedef struct	_hashbin {
	lkinfobuf_t *hb_lkinfos;	
} hashbin_t;


struct nlist nl[] = {
	{"lsb_first", 0, 0, 0, C_EXT, 0},
	{NULL}
};


/*
 * Following variables maintain running totals for lock stat buffers
 * sharing the same lkinfo pointer:
 */
ulong_t		wrcnt_sum;	
ulong_t		rdcnt_sum;
ulong_t		solordcnt_sum;
ulong_t		fail_sum;

/*
 * NOTES: 
 *
 * Computing hold times (in microseconds):
 *	(total cum_hold_time for buffers w. same lkinfop)/(wrcnt_sum+rdcnt_sum)
 * Computing wait times (in microseconds):
 *	(total cum_wait_time for buffers w. same lkinfop)/(fail_sum)
 *
 * Mean hold time for a particular lock buffer: cum_hold_time/(rdcnt + wrcnt)
 * Mean wait time for a particular lock buffer: cum_wait_time/(failed attempts)
 *
 * We shall report :
 *	global mean hold and wait times for buffers sharing the
 * 	same lkinfop, 
 * and 
 *	Maximum of the mean lock hold times, and the maximum of the mean
 *	lock wait times. (Maximum among all buffers sharing the same lkinfop.)
 */

dl_t		global_total_hold_time;
dl_t		global_total_wait_time;

ulong_t		peak_mean_wait_time;
ulong_t		peak_mean_hold_time;
ulong_t		peak_wrcnt;
ulong_t		peak_rdcnt;
ulong_t		peak_solordcnt;
ulong_t		peak_fail;


/*
 * NOTES:
 *
 * Computing the quotient, for a dl_t dividend and a ulong_t divisor:
 *	ldiv(H, L, D), where:
 *		H is the high order word of dividend,
 *		L is the low  order word of dividend, and
 *		D is the 32 bit divisor; 
 *	is computed as:
 *		if (H >= D): 	== UINT_MAX (i.e., 0xffffffff)
 *		else : 		== (UINT_MAX/D)*H + (L/D).
 *	which may be slightly inaccurate, but will do for our purposes.
 *
 *	The alternative would be to write an assembler function, which
 *	for x86, is as follows:
 *
 *	.text; .type ldivs,@function; .globl ldivs; .align 8; ldivs:
 *		movl	4(%esp), %edx	/ Move high order 32 bits to EDX
 *		movl	8(%esp), %eax	/ Move low  order 32 bits to EAX
 *		movl	12(%esp), %ecx	/ Move divisor into ECX
 *		div	%ecx		/ (EDX:EAX) divided by ECX --> EAX.
 *		ret
 *
 */ 

statbuf_t 	statbuffers[MAXSTATBUFS];
lkinfobuf_t 	lkinfobuffers[MAXLKINFOBUFS];
hashbin_t 	lkinfohash[LKINFO_HASHSZ];
vaddr_t		lkinfo_addr[MAXLKINFOBUFS];

char	lockname[LOCKNAMELEN];

int 	callcnt = 0;
int	nstatbufs = 0;
int	nlkinfobufs = 0;

int kmemfd;			/* file descriptor for /dev/kmem */
lksblk_t lksblkbuf[MAXLKSBLKS];	/* buffers into which lksblks will be read */
int nlksblkbufs = 0;		/* number of lksblk buffers */
vaddr_t nextlsbp;		/* kernel virtual addr of the next stats block*/
int nbytes;			/* number of bytes read from /dev/kmem */
char unixname[PATH_LEN +1];	/* name of unix; default: /stand/unix */
lksblk_t *lksblkbufp;		/* utility pointer to buffers */

lkinfo_t	tmplkinfo;


/*
 * ulong_t
 * ldiv(long high32, ulong_t low32, ulong_t div32)
 *	Return the result of a double_long division. The double_long
 *	dividend is [high32, low32], while the 32 bit divisor is div32.
 * Remarks:
 *	Eventually this can become a macro.
 * 
 */
ulong_t
ldiv(ulong_t high32, ulong_t low32, ulong_t div32)
{
	if (div32 <= high32)
		return(UINT_MAX);
	else
		return(((UINT_MAX/div32)*high32 + (low32/div32)));
}


/*
 * int
 * get_stats(lkinfobufp)
 * lkinfobuf_t *lkinfobufp;
 *	Collect cumulative statistics for the indicated lkinfobuffer.
 *	Return the actual number of stat buffers, which should
 *	match the statcnt in the lkinfobuffer.
 */
int
get_stats(lkinfobufp)
lkinfobuf_t	*lkinfobufp;
{
	statbuf_t *sbp; 
	lkstat_t  *lkstatp;
	int nsbs = 0;

	if ((lkinfobufp == NULL) || ((sbp = lkinfobufp->lb_statbufs) == NULL)) {
		fprintf(stderr,"collect_stats: no stat buffer chain \n");
		return(0);
	}

	wrcnt_sum = 0;
	rdcnt_sum = 0;
	solordcnt_sum = 0;
	fail_sum = 0;

	global_total_hold_time.dl_hop = 0;
	global_total_hold_time.dl_lop = 0;	
	global_total_wait_time.dl_hop = 0;
	global_total_wait_time.dl_lop = 0;	

	peak_mean_hold_time = 0;
	peak_mean_wait_time = 0;
	peak_wrcnt = 0;
	peak_rdcnt = 0;
	peak_solordcnt = 0;
	peak_fail = 0;

	while (sbp != NULL) {

		ulong_t	rd_wr_sum;	/* rdcnt + wrcnt  for a single buffer */
		ulong_t mean_htime; /* mean hold time for a single buffer */
		ulong_t mean_wtime; /* mean wait time for a single buffer */

		++nsbs;
		lkstatp = &sbp->sb_lkstat;
		wrcnt_sum += lkstatp->lks_wrcnt;
		rdcnt_sum += lkstatp->lks_rdcnt;
		solordcnt_sum += lkstatp->lks_solordcnt;
		fail_sum += lkstatp->lks_fail;

		peak_wrcnt = MAX(lkstatp->lks_wrcnt, peak_wrcnt);
		peak_rdcnt = MAX(lkstatp->lks_rdcnt, peak_rdcnt);	
		peak_solordcnt = MAX(lkstatp->lks_solordcnt, peak_solordcnt);
		peak_fail = MAX(lkstatp->lks_fail, peak_fail);

		if ((rd_wr_sum = lkstatp->lks_wrcnt + lkstatp->lks_rdcnt) > 0) {
			mean_htime = ldiv((ulong_t)lkstatp->lks_htime.dl_hop,
				lkstatp->lks_htime.dl_lop, rd_wr_sum);
			peak_mean_hold_time = 
				MAX(mean_htime, peak_mean_hold_time);
			laccum(	global_total_hold_time.dl_hop,
			    	global_total_hold_time.dl_lop,
				lkstatp->lks_htime.dl_hop,
				lkstatp->lks_htime.dl_lop );
		} 

		if (lkstatp->lks_fail > 0) {
			mean_wtime = ldiv((ulong_t)lkstatp->lks_wtime.dl_hop,
				lkstatp->lks_wtime.dl_lop, lkstatp->lks_fail);
			peak_mean_wait_time = 
				MAX(mean_wtime, peak_mean_wait_time);
			laccum(	global_total_wait_time.dl_hop,
			    	global_total_wait_time.dl_lop,
				lkstatp->lks_wtime.dl_hop,
				lkstatp->lks_wtime.dl_lop );
		}

		sbp = sbp->sb_next;
	}
	if (nsbs != lkinfobufp->lb_statcnt) {
		fprintf(stderr,"incorrect number of stat buffers\n");
		return(nsbs);
	}
	return(nsbs);
}

/*
 * getlkname(vaddr_t kern_lkinfo_addr)
 *	Get the string associated with the lkinfo whose kernel
 *	virtual address is kern_lkinfo_addr.
 *	Read the lkinfo struct at the address, obtain the string
 *	addr, and then read the string into "lockname".	
 *
 */ 


void
getlkname(kern_lkinfo_addr)
vaddr_t	kern_lkinfo_addr;
{
	int	lkinfosz = sizeof(lkinfo_t);
	vaddr_t	kern_string_addr;
	int j;


	lockname[0] = '\0';
	
	if (lseek(kmemfd,kern_lkinfo_addr, SEEK_SET) == -1) {
		fprintf(stderr, "getlkname: seek to %x failed, err %d\n",
			kern_lkinfo_addr, errno);
		return;
	}
	if ((nbytes = read(kmemfd, &tmplkinfo, lkinfosz)) == -1) {
		fprintf(stderr, "getlkname: read /dev/kmem failed, err %d\n",
			errno);
		return;
	}
	if (nbytes != lkinfosz) {
		fprintf(stderr,"getlkname: read only %d lkinfo bytes, not %d\n",
			nbytes, lkinfosz);
		return;
	}

	kern_string_addr = (vaddr_t)(tmplkinfo.lk_name);

#ifdef	DEBUGMODE	
	fprintf(stderr,"getlkname: string is at addr %x\n", kern_string_addr);
#endif

	if (lseek(kmemfd,kern_string_addr, SEEK_SET) == -1) {
		fprintf(stderr, "getlkname: seek to %x failed, err %d\n",
			kern_string_addr, errno);
		return;
	}

	if ((nbytes = read(kmemfd, &lockname[0], LOCKNAMELEN)) == -1) {
		fprintf(stderr, "getlkname: read /dev/kmem failed, err %d\n",
			errno);
		return;
	}
	if (nbytes == LOCKNAMELEN)
		lockname[LOCKNAMELEN - 1] = '\0';

	/* make the name pretty by padding blanks, etc. */

	for (j = 0; j < 32; j++) {
		if (lockname[j] == '\0') {
			while (j < 32) {
				lockname[j] = ' ';
				++j;
			}
			break;
		}
	}
	lockname[32] = '\0';

	return;
	
}




/*
 * lkinfobuf_t *
 * getlkinfobuf(tag)
 * vaddr_t tag;
 *
 *	Search the hash chain for "tag" to get at the lockinfo buffer.
 *	If none is found, then allocate a new one, and initialize
 *	it with the tag. 
 */
lkinfobuf_t *
getlkinfobuf(tag)
vaddr_t	tag;
{
	int hashnum;
	lkinfobuf_t *lkinfobufp;

	{ ENTER("getlkinfobuf", tag); }

	hashnum = LKINFOHASH(tag);

	ASSERT((hashnum >= 0 && hashnum < LKINFO_HASHSZ), "lkinfohash", 1); 

	lkinfobufp = lkinfohash[hashnum].hb_lkinfos;

	while (lkinfobufp != NULL) {
		if (lkinfobufp->lb_tag == tag) {
			{ LEAVE("getlkinfobuf", lkinfobufp); }
			return(lkinfobufp);
		}
		lkinfobufp = lkinfobufp->lb_next;
	}

	/* allocate a new lkinfobuf, initialize it, and attach it */
	/* to the hash chain */

	lkinfobufp = &(lkinfobuffers[nlkinfobufs]);

	if (++nlkinfobufs >= MAXLKINFOBUFS) {
		fprintf(stderr,"lksblksnap: exceeded %d lock info count\n",
			MAXLKINFOBUFS);
		exit(1);
	}

	lkinfobufp->lb_tag = tag;
	lkinfobufp->lb_statcnt = 0;
	lkinfobufp->lb_statbufs = NULL;
	lkinfobufp->lb_next = lkinfohash[hashnum].hb_lkinfos;
	lkinfohash[hashnum].hb_lkinfos = lkinfobufp;
	{ LEAVE("getlkinfobuf", lkinfobufp); }	
	return(lkinfobufp);	
}

/*
 * void
 * insert_stat(lkstatp)
 * lkstat_t *lkstatp;
 *	allocate and fill out a statbuffer with the contents of the
 *	referenced lock statistics structure. Link the statbuffer
 *	onto the chain anchored at its lockinfo buffer.
 */	
void
insert_stat(lkstatp)
lkstat_t *lkstatp;
{
	vaddr_t	xtag;
	lkinfobuf_t *xlkinfobufp;
	statbuf_t *xstatbufp;

	ENTER2("insert_stat", lkstatp);

	/* if the stat structure has no lock info pointer, skip */
	xtag = (vaddr_t)(lkstatp->lks_infop);
	if (xtag == NULL) {
		{ LEAVE2("insert_stat", 0); }
		return;
	}

	xlkinfobufp = getlkinfobuf(xtag);
	++(xlkinfobufp->lb_statcnt);
	
	/* allocate a stat buffer, copy contents of *lkstatp into it */
	/* and then insert it on the lock stat chain of *xlkinfobufp */

	xstatbufp = &statbuffers[nstatbufs];
	if (++nstatbufs >= MAXSTATBUFS) {
		fprintf(stderr,"lksblksnap: exceeded %d stat buffers\n",
			MAXSTATBUFS);
		exit(1);
	}
	
	xstatbufp->sb_lkstat = *lkstatp;	/* Structure assignment */

	xstatbufp->sb_next = xlkinfobufp->lb_statbufs;
	xlkinfobufp->lb_statbufs = xstatbufp;

	/* for debugging/sanity checks */
	xstatbufp->debug_lkinfop =  (vaddr_t)xtag;
	xstatbufp->debug_statp =  (vaddr_t)lkstatp;

	{ LEAVE2("insert_stat", 0); }
}

void
checkall2()
{
	int i;
	vaddr_t xtag;
	lkstat_t *xlkstatp;
	statbuf_t *xstatbufp;

	for (i=0; i < nstatbufs; i++) {
		xstatbufp = &(statbuffers[i]);
		xtag = (vaddr_t)(xstatbufp->debug_lkinfop);
		xlkstatp = (lkstat_t *)(xstatbufp->debug_statp);
		if (xlkstatp->lks_infop != (lkinfo_t *)xtag) {
			fprintf(stderr,
			"checkall2 : statbuf # %d \tlkinfop %x \tlkstatp %x\n",
			i, xtag, xlkstatp);
			exit(1);
		}
	}
}


void
checkall()
{
	int	i;

	for (i=0; i < nlkinfobufs; i++)
		(void)db_check(i);
}

void
db_check(tagindex)
int tagindex;
{
	int errnum = 0;
	vaddr_t	xtag;
	int hashbin;
	lkinfobuf_t *xlkinfobufp;	
	statbuf_t *xstatbufp;
	int xstatcnt;

	if (tagindex < 0 || tagindex >= nlkinfobufs) {
		errnum = 1;
		goto bad;
	}

	xtag = lkinfo_addr[tagindex];
	if (xtag != lkinfobuffers[tagindex].lb_tag) {
		errnum = 2;
		goto bad;
	}

	hashbin = LKINFOHASH(xtag);
	if (hashbin < 0 || hashbin >= LKINFO_HASHSZ) {
		errnum = 3;
		goto bad;
	}

	xlkinfobufp = getlkinfobuf(xtag);

	if (xlkinfobufp != &(lkinfobuffers[tagindex])) {
		errnum = 4;
		goto bad;
	}
	xstatcnt = 0;
	xstatbufp = xlkinfobufp->lb_statbufs;
	while (xstatbufp != NULL) {
		if ((++xstatcnt) > xlkinfobufp->lb_statcnt) {
			errnum = 5;
			goto bad;
		}
		if (xstatbufp->debug_lkinfop != xtag) {
			errnum = 6;
			goto bad;
		}
		if( ((lkstat_t *)(xstatbufp->debug_statp))->lks_infop 
			!= (lkinfo_t *)xtag){
			errnum = 7;
			goto bad;
		}
		xstatbufp = xstatbufp->sb_next;
	}
	if (xstatcnt != xlkinfobufp->lb_statcnt) {
		errnum = 8;
		goto bad;
	}
	return;
bad:
	fprintf(stderr,"Sanity Test failure: tagindx %d lkinfop %x error %d\n",
		tagindex, xtag, errnum);
	exit(1);
}

/*
 * The top level routine to build the inverse lists of lock stat buffers
 * anchored at the lock info structures.
 */
void
invert_correspondence()
{
	int	i, j;
	lksblk_t	*lksblkp;
	lkstat_t	*lkstatp;	

	for (i=0; i < nlksblkbufs; i++) {
		lksblkp = &(lksblkbuf[i]);
		for (j=0; j < LSB_NLKDS; j++) {
			lkstatp = &(lksblkp->lsb_bufs[j]);
			(void)insert_stat(lkstatp);
		}
	}
	(void)extract_lkinfops();
}

#ifdef	REMOVE_LATER
invert_correspondence()
{
	int j;
	lksblk_t *lksblkp;
	lkstat_t *lkstatp;

	ENTER("invert_correspondence", 0);

	lksblkp = (&(lksblkbuf[0]));
	for (j=0; j < LSB_NLKDS; j++) {
		lkstatp = &(lksblkp->lsb_bufs[j]);
		(void)insert_stat(lkstatp);
	}
	(void)extract_lkinfops();

	LEAVE("invert_correspondence", 0);
}
#endif


/*
 *	Utility routine to compact all the virtual address of kernel lkinfo
 *	structures into a single array, in order to drive the string 
 *	extraction from kernel.
 */
void
extract_lkinfops()
{
	int i;

	ENTER("extract_lkinfops", 0);
	for (i=0; i < nlkinfobufs; i++) {
		lkinfo_addr[i] = lkinfobuffers[i].lb_tag;
	}
	LEAVE("extract_lkinfops", 0);
}

int zflg = 0;				/* zero the stats */
int errflg = 0;	
int aflg = 0;				/* all stats: peak and mean values */
int gflg = 0;				/* mean values */
int nflg = 0;				/* # stat buffers for each type */
int tflg = 0;				/* aggregate for each type */

main(argc, argv)
int	argc;	/* arg count */
char	**argv;	/* arg vector */
{


	int i, nstats;
	ulong_t	mean_lock_hold_time;
	ulong_t	mean_lock_wait_time;

	int c;
	int lockstatfd;
	extern char *optarg;
	extern int optind;
	
	if (argc > 1) {
		while ((c = getopt(argc, argv, "zagnt")) != EOF) {
			switch(c) {
				case 'z':
					++zflg;
					break;
				case 'a':
					++aflg;
					break;
				case 'g':
					++gflg;
					break;
				case 'n':
					++nflg;
					break;
				case 't':
					++tflg;
					break;
				case '?':
					++errflg;
					break;
			}
		}
	} 


	if (errflg) {
		fprintf(stderr, "Usage:\t lockstat [- z|a|g ] \n");
		exit(2);
	}

	if (zflg) {
		/* Zero out the data in all kernel lkstat buffers. */
		if ((lockstatfd = open("/dev/lockstat",O_RDWR)) == -1) {
			fprintf(stderr,
			"lockstat: cannot open /dev/lockstat : errno %d\n",
			errno);
			exit(2);
		}
		if (ioctl(lockstatfd, 0, 0) == -1) {
			fprintf(stderr,
			"lockstat: ioctl failed : errno %d\n",
			errno);
			exit(2);
		}
		(void)close(lockstatfd);
		exit(1);
	}


	unixname[PATH_LEN] = '\0';
	strcpy(unixname, "/stand/unix");
	nlist(unixname, nl);

	if (nl[0].n_value == 0) {
		fprintf(stderr, "devkmem: no namelist.\n");
		exit(1);
	} 

#ifdef	DEBUGMODE
	else {
		fprintf(stderr, "devkmem: kvaddr(v) = %x.\n", nl[0].n_value);
	}
#endif

	nextlsbp = nl[0].n_value;

	if ((kmemfd = open(DEVICE_NAME,O_RDWR)) == -1) {
		fprintf(stderr, "devkmem: no /dev/kmem.\n");
		exit(1);
	}

	nlksblkbufs = 0;
	
	do {
		/*
		 * add code to check that the lsb pointer chain, as
		 * we follow it, is indeed sane. to do this, remember
		 * nextlsbp values from one iter to next and compare
		 * with the appropriate fields in the lksblkbufs after
		 * reads. TODO.
		 */
		lksblkbufp = &(lksblkbuf[nlksblkbufs]);
		(void)getlksblk(lksblkbufp, nextlsbp);

#ifdef	DEBUGMODE		
		fprintf(stderr,"%d : Calling getnext [ %x ] %x ...\n", 
			nlksblkbufs, lksblkbufp, nextlsbp);
#endif		

		nextlsbp = getnextlsbp(lksblkbufp);

#ifdef	DEBUGMODE	
		fprintf(stderr,"...... Next Addr %x \n", nextlsbp);
#endif

		if (++nlksblkbufs > MAXLKSBLKS) {
			fprintf(stderr, 
			 "lksblksnap: exceeded max lksblk count %d\n", 
				MAXLKSBLKS);
			break;
		}

	} while (nextlsbp != NULL);


	sleep(4);

#ifdef	DEBUGMODE
	fprintf(stderr, "lksblksnap: read %d lock stat blocks\n", nlksblkbufs);
#endif

	/*
	 * Initialize lkinfohash
	 */

	for (i=0; i< LKINFO_HASHSZ; i++) {
		lkinfohash[i].hb_lkinfos = (lkinfobuf_t *)NULL;
	}

	sleep(4);

	(void)invert_correspondence();

	/*
	(void)printlkinfobufs();	
	*/

	checkall();

	checkall2();
	
	/* Print out the stats */

	if (nflg) {
		printf("Number of stat buffers for each type\n");
		printf("Lockname\t\tCount\n");
	} else if (gflg) {
		printf("\t\tStatistics Averaged for Each Lock Type\n");
		printf("Lockname\t\tWrite\tRead\tSoloRd\tBlock\tPeak(A.Hold)\tPeak(A.Wait)\tA.Hold\tA.Wait\n\n");
	} else if (aflg) {
		printf("\t\tPeak and Averaged Statistics For Each Lock Type\n");
		printf("Lockname\t\tWrite\tRead\tSoloRd\tBlock\tPeak(A.Hold)\tPeak(A.Wait)");
		printf("\tA.Write\tA.Read\tA.SoloRd\tA.Block\tA.Hold\tA.Wait\n\n");
	} else if (tflg) {
		printf("\t\tAggregate Counts for Each Lock Type\n");
		printf("Lockname\t\tWrite\tRead\tSoloRd\tBlock\n\n");
	} else {
		printf("\t\tPeak Statistics Among Locks of Each Type\n");
		printf("Lockname\t\tWrite\tRead\tSoloRd\tBlock\tPeak(A.Hold)\tPeak(A.Wait)\n\n");	
	}

	for (i=0; i < nlkinfobufs; i++) {

		getlkname(lkinfo_addr[i]);
		nstats = get_stats(&lkinfobuffers[i]);

		if (nstats != 0) {

			if ((wrcnt_sum + rdcnt_sum) > 0) {
				mean_lock_hold_time = ldiv(
					(ulong_t)global_total_hold_time.dl_hop,
					global_total_hold_time.dl_lop,
					(wrcnt_sum + rdcnt_sum));
			} else {
				mean_lock_hold_time = 0;
			}

			if (fail_sum > 0) {
				mean_lock_wait_time = ldiv(
					(ulong_t)global_total_wait_time.dl_hop,
					global_total_wait_time.dl_lop,
					fail_sum);
			} else {
				mean_lock_wait_time = 0;
			}
			printf("%s\t", lockname);
			if (nflg != 0) {
				printf("%d\n", nstats);
				continue;
			}	

			if (tflg) {
				printf("%d\t%d\t%d\t%d\n",
					wrcnt_sum,
					rdcnt_sum,
					solordcnt_sum,
					fail_sum);
				continue;
			}

			if (gflg) {
				printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
					(wrcnt_sum/nstats),
					(rdcnt_sum/nstats),
					(solordcnt_sum/nstats),
					(fail_sum/nstats),
					peak_mean_hold_time,
					peak_mean_wait_time,
					mean_lock_hold_time,
					mean_lock_wait_time);
				continue;
			} 
			if (aflg) {
				printf("%d\t%d\t%d\t%d\t%d\t%d\t",
					peak_wrcnt,
					peak_rdcnt,
					peak_solordcnt,
					peak_fail,
					peak_mean_hold_time,
					peak_mean_wait_time);
				printf("%d\t%d\t%d\t%d\t%d\t%d\n",
					(wrcnt_sum/nstats),
					(rdcnt_sum/nstats),
					(solordcnt_sum/nstats),
					(fail_sum/nstats),
					mean_lock_hold_time,
					mean_lock_wait_time);
				continue;
			} 
			printf("%d\t%d\t%d\t%d\t%d\t%d\n",
					peak_wrcnt,
					peak_rdcnt,
					peak_solordcnt,
					peak_fail,
					peak_mean_hold_time,
					peak_mean_wait_time);
			
		}
	}

	close(kmemfd);
}

/*
 * vaddr_t
 * getnextlsbp(lksblkbufp)
 * lksblk_t *lksblkbufp;
 *	Get kernel virtual address of the lksblk next in link to the
 *	current one.
 */
vaddr_t
getnextlsbp(lksblkbufp)
lksblk_t *lksblkbufp;
{
	vaddr_t	retval = NULL;
	retval = (vaddr_t)(lksblkbufp->lsb_next);
	return(retval);
}


/*
 * void
 * getlksblk(lksblkbufp, kern_lksblk_addr)
 * lksblk_t *lksblkbufp;
 * vaddr_t kern_lksblk_addr;
 * 	Read the lksblk buffer that is at the kernel virtual address
 *	(kern_lksblk_addr) that is passed in.
 */
void
getlksblk(lksblkbufp, kern_lksblk_addr)
lksblk_t *lksblkbufp;		/* pointer to the receiving buffer */
vaddr_t	kern_lksblk_addr;	/* kmem vaddr to start the read from */
{

	ENTER("getlksblk", 0);

	if (lseek(kmemfd, kern_lksblk_addr, SEEK_SET) == -1) {
		fprintf(stderr, "lseek /dev/kmem fails.\t errno %d\n", errno);
		close(kmemfd);
		exit(1);
	}

	if ((nbytes = read(kmemfd, lksblkbufp, sizeof(lksblk_t))) == -1) {
		fprintf(stderr, "read /dev/kmem fails.\t errno %d\n", errno);
		close(kmemfd);
		exit(1);
	}
	

	/* (void)printlksblk((int *)lksblkbufp, nbytes); */

	LEAVE("getlksblk", 0);
}


/*
 * Utility print routine.
 */
void
printlksblk(p, n)
int *p;
int n;
{
	int	w1, w2, w3, w4, w5, w6, w7, w8, w9, wa;
	int nwords = n/4;

	while (nwords > 10) {

		w1 = *p; ++p; --nwords;
		w2 = *p; ++p; --nwords;
		w3 = *p; ++p; --nwords;
		w4 = *p; ++p; --nwords;
		w5 = *p; ++p; --nwords;
		w6 = *p; ++p; --nwords;
		w7 = *p; ++p; --nwords;
		w8 = *p; ++p; --nwords;
		w9 = *p; ++p; --nwords;
		wa = *p; ++p; --nwords;

		printf("%x %x %x %x %x %x %x %x %x %x\n",
			w1, w2, w3, w4, w5, w6, w7, w8, w9, wa);
	}

	while (nwords > 0) {
		w1 = *p; ++p; --nwords;
		printf("%x ", w1); printf("\n");
	}
}

void
printlkinfobufs()
{
	int i;
	lkinfobuf_t	*lbufp;

	printf("LKINFOS: %d\n", nlkinfobufs);

	for (i=0; i<nlkinfobufs; i++) {
		lbufp = &(lkinfobuffers[i]);
		printf("%d\t%x\n", i, lkinfo_addr[i]);
		printf("\t\tNxt %x Tag %x SNum %d\n",
			lbufp->lb_next,
			lbufp->lb_tag,
			lbufp->lb_statcnt);
	}	

	printf("Total Stat Cnt is %d\n", nstatbufs);

	
}


/* 
 * Utility errmsg: print error. If anum > 0, exit.
 */
void
errmsg(f,n)
char *f;
int n;
{
	fprintf(stderr,"%s: ass. failed %d\n", f, n);
	if (n > 0)
		exit(1);	
}



