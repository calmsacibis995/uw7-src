/*              copyright       "%c%"   */

#ident  "@(#)pk1.c	1.2"
#ident "$Header$"

#include "uucp.h"

#define USER	1

#include "pk.h"
#include <sys/buf.h>

extern void pkfail(), pkzero(), pkoutput(), pkreset(), pkcntl(), pkgetpack(), pkfindsyn();
extern int pksack();
static void pkdata();
static int pkcget();

/*
 * Code added to allow translation of states from numbers to
 * letters, to be done in such a way as to be meaningful to 
 * John Q. Public
 */
struct {
	int state;
	char *msg;
} st_trans[] = {
	DEAD,	"Dead!",
	INITa,	"INIT code a",
	INITb,	"INIT code b",
	LIVE,	"O.K.",
	RXMIT,	"Rcv/Xmit",
	RREJ,	"RREJ?",
	PDEBUG,	"PDEBUG?",
	DRAINO,	"Draino...",
	WAITO,	"Waiting",
	DOWN,	"Link down",
	RCLOSE,	"RCLOSE?",
	BADFRAME,	"Bad frame",
	-1,	"End of the line",
};

extern char *_Protocol;	/* Protocol string with (options) */

#define PKMAXSTMSG 40
int Connodata = 0;		/* Continuous Non Valid Data Count */
int Ntimeout = 0;
#define CONNODATA	20	/* Max Continuous Non Valid Data Count */
#define NTIMEOUT	50	/* This is not currently used, but maybe in the future */

extern jmp_buf Getjbuf;

/*
 * packet driver support routines
 *
 */
extern struct pack *pklines[];

/*
 * start initial synchronization.
 */
struct pack *
pkopen(ifn, ofn)
int ifn, ofn;
{
	register struct pack *pk;
	register char **bp;
	register int i;
	int windows = WINDOWS;
	extern int xpacksize, packsize;

	if (++pkactive >= NPLINES)
		return(NULL);
	if ((pk = (struct pack *) malloc(sizeof (struct pack))) == NULL)
		return(NULL);
	pkzero((caddr_t) pk, sizeof (struct pack));
	pk->p_ifn = ifn;
	pk->p_ofn = ofn;
	DEBUG(7, "Setting up protocol parameters '%s'\n", _Protocol);
	if ( _Protocol[1] == '(' ) {
	    if (sscanf(_Protocol, "%*c(%d,%d)", &windows, &packsize) == 0)
	    sscanf(_Protocol, "%*c(,%d)", &packsize);
	    windows = ( windows < MINWINDOWS ? WINDOWS :
			( windows > MAXWINDOWS ? WINDOWS : windows ) );
	    packsize = ( packsize < MINPACKSIZE ? PACKSIZE :
			( packsize > MAXPACKSIZE ? PACKSIZE : packsize ) );
	}
	if ( (_Protocol[0] == 'g') && (packsize > OLDPACKSIZE) ) {
	    /*
	     * We reset to OLDPACKSIZE to maintain compatibility
	     * with old limited implementations. Maybe we should
	     * just warn the administrator and continue?
	     */
	    packsize = OLDPACKSIZE;
	}
	pk->p_xsize = pk->p_rsize = xpacksize = packsize;
	pk->p_rwindow = pk->p_swindow = windows;

	/*
	 * allocate input window
	 */
	for (i = 0; i < pk->p_rwindow; i++) {
		if ((bp = (char **) malloc((unsigned) pk->p_xsize)) == NULL)
			break;
		*bp = (char *) pk->p_ipool;
		pk->p_ipool = bp;
	}
	if (i == 0)
		return(NULL);
	pk->p_rwindow = i;

	/*
	 * start synchronization
	 */
	pk->p_msg = pk->p_rmsg = M_INITA;
	for (i = 0; i < NPLINES; i++) {
		if (pklines[i] == NULL) {
			pklines[i] = pk;
			break;
		}
	}
	if (i >= NPLINES)
		return(NULL);
	pkoutput(pk);

	for (i = 0; i < PKMAXSTMSG; i++) {
		pkgetpack(pk);
		if ((pk->p_state & LIVE) != 0)
			break;
	}
	if (i >= PKMAXSTMSG)
		return(NULL);

	pkreset(pk);
	return(pk);
}

/*
 * input framing and block checking.
 * frame layout for most devices is:
 *	
 *	S|K|X|Y|C|Z|  ... data ... |
 *
 *	where 	S	== initial synch byte
 *		K	== encoded frame size (indexes pksizes[])
 *		X, Y	== block check bytes
 *		C	== control byte
 *		Z	== XOR of header (K^X^Y^C)
 *		data	== 0 or more data bytes
 *
 */
#define GETRIES 10

/*
 * Pseudo-dma byte collection.
 */
void
pkgetpack(pk)
register struct pack *pk;
{
	register struct header *h;
	char *read_ptr, *start_ptr, *end_ptr;
	unsigned short sum;
	int i, k, ret, tries;
	char **bp, hdchk, msgline[80], delimc;

	if (pk->p_state & DOWN) 
		pkfail();

	/* Find a header	*/
	
	h = (struct header * ) &pk->p_ihbuf;
	start_ptr = (char *) &pk->p_ihbuf;
	read_ptr = start_ptr;
	end_ptr = start_ptr + HDRSIZ;

	for (tries = 0; ((tries < GETRIES) && (Connodata < CONNODATA)); tries++) {
	
		/* Read header. If some bytes were read in on	*/
		/* the previous loop iteration, read only the	*/
		/* missing bytes.				*/

		if ( pkcget(pk->p_ifn, read_ptr, end_ptr - read_ptr) < 0) {
			/* set up retransmit or REJ */

			pk->p_msg |= pk->p_rmsg;

			if (pk->p_msg == 0)
				pk->p_msg |= M_RR;

			if ((pk->p_state & LIVE) == LIVE)
				pk->p_state |= RXMIT;

			pkoutput(pk);
			continue;
		}

		/* Find the synchronization (SYN) byte.	*/
		/* If SYN is not a first byte, look for	*/
		/* another SYN in the header.		*/

		if (*start_ptr != SYN)
		{
			pkfindsyn(pk, start_ptr, &read_ptr, end_ptr);
			continue;
		}

		/* If SYN is a first byte, check the	*/
		/* the checksum of the header.		*/
		/* If checksum is incorrect, look for	*/
		/* another SYN and continue. Otherwise	*/
		/* a good header is found.		*/


		tries=0;
		Connodata++;
		DEBUG(9, "pkgetpack: Connodata=%d\n", Connodata);
		hdchk = start_ptr[1] ^ start_ptr[2] ^ start_ptr[3] ^ start_ptr[4];
		DEBUG(9, "rec h->cntl %o\n", (unsigned) h->cntl);

		if (hdchk != h->ccntl) {
			/* bad header */
			DEBUG(7, "bad header %o,", hdchk);
			DEBUG(7, "h->ccntl %o\n", h->ccntl);
			*start_ptr = '\0';
			pkfindsyn(pk, start_ptr, &read_ptr, end_ptr);
			continue;
		}		

		sum = (unsigned) start_ptr[2] & 0377;
		sum |= (unsigned) start_ptr[3] << 8;
		h->sum = sum;
		k = h->ksize;
	
		if (k != 9) {
			break;
		}

		if (h->sum + h->cntl != CHECK) 
		{
			/* bad header */
			DEBUG(7, "bad header %o\n", h->cntl);
			pk->p_state |= BADFRAME;
			*start_ptr = '\0';
			pkfindsyn(pk, start_ptr, &read_ptr, end_ptr);
			continue;
		}
												 
		pkcntl(h->cntl, pk);
		/* 
		 * New code added to make the state diagnostics
		 * meaningful to human beings that can't figure
		 * out bizarre, silly, cryptic numbers.
		 * 
		 * Exhibit A: DEBUG(7, "state - %o\n", pk->p_state);
		 */
		if(Debug >= 7) {
		    sprintf(msgline,"state -");
		    delimc = ' ';
		    for(i=0;st_trans[i].state!= -1;i++) {
			if(pk->p_state&st_trans[i].state){
			    sprintf(msgline,"%s%c[%s]",msgline,delimc,
				st_trans[i].msg);
			    delimc = '&';
			}
		    }
		    sprintf(msgline,"%s (%o)\n",msgline,pk->p_state);
		    DEBUG(7,"%s",msgline);
		}
				
		return;

	}

	if ((tries >= GETRIES) || (Connodata >= CONNODATA)) {
		DEBUG(4, "tries = %d\n", tries);
		pkfail();
	}


	if (k && pksizes[k] == pk->p_rsize) {
		pk->p_rpr = h->cntl & MOD8;
		pksack(pk);
		bp = pk->p_ipool;
		if ((bp = pk->p_ipool) == NULL) {
			DEBUG(7, "bp NULL, all buffers lost!%s\n", "");
			return;
		}
		pk->p_ipool = (char **) *bp;
		ret = pkcget(pk->p_ifn, (char *) bp, pk->p_rsize);
		if (ret == 0)
			pkdata(h->cntl, h->sum, pk, bp);
		else {
			*bp = (char *)pk->p_ipool;
			pk->p_ipool = bp;
		}
	}
	return;
}
static void
pkdata(c, sum, pk, bp)
register struct pack *pk;
unsigned short sum;
char c;
char **bp;
{
	register x;
	int t;
	char m;

	if (pk->p_state & DRAINO || !(pk->p_state & LIVE)) {
		pk->p_msg |= pk->p_rmsg;
		pkoutput(pk);
		goto drop;
	}
	t = next[pk->p_pr];
	for(x=pk->p_pr; x!=t; x = (x-1)&7) {
		if (pk->p_is[x] == 0)
			goto slot;
	}
drop:
	*bp = (char *)pk->p_ipool;
	pk->p_ipool = bp;
	return;

slot:
	m = mask[x];
	pk->p_imap |= m;
	pk->p_is[x] = c;
	pk->p_isum[x] = sum;
	pk->p_ib[x] = (char *)bp;
	return;
}

/*
 * Start transmission on output device 
 * device associated with pk.
 * For asynch devices (t_line==1) framing is
 * imposed.  For devices with framing and crc
 * in the driver (t_line==2) the transfer is
 * passed on to the driver.
 */
void
pkxstart(pk, cntl, x)
register struct pack *pk;
int x;
char cntl;
{
	register char *p;
	register short checkword;
	register char hdchk;
	int ret;

	p = (caddr_t) &pk->p_ohbuf;
	*p++ = SYN;
	if (x < 0) {
		*p++ = hdchk = 9;
		checkword = cntl;
	} else {
		*p++ = hdchk = pk->p_lpsize;
		checkword = pk->p_osum[x] ^ (unsigned)(cntl & 0377);
	}
	checkword = CHECK - checkword;
	*p = checkword;
	hdchk ^= *p++;
	*p = checkword>>8;
	hdchk ^= *p++;
	*p = cntl;
	hdchk ^= *p++;
	*p = hdchk;

 /*
  * writes
  */
	p = (caddr_t) & pk->p_ohbuf;
	if (x < 0) {
		ret = Write(pk->p_ofn, p, HDRSIZ);
		PKASSERT(ret == HDRSIZ, "PKXSTART ret", "", ret);
	} else {
		register char *b, *q;
		register int i;
		char buf[MAXPACKSIZE + HDRSIZ]; 

		i = HDRSIZ;
		b = buf;
		q = p;
		do
			*b++ = *q++;
		while(--i);
		if ((i = pk->p_xsize) != 0) {
			q = pk->p_ob[x];
			do
				*b++ = *q++;
			while(--i);
		}
		ret = Write(pk->p_ofn, buf, pk->p_xsize + HDRSIZ);
		PKASSERT(ret == pk->p_xsize + HDRSIZ,
		  "PKXSTART ret", "", ret);
	}
	if (pk->p_msg)
		pkoutput(pk);
	return;
}




/*
 * get n characters from input
 *	b	-> buffer for characters
 *	fn	-> file descriptor
 *	n	-> requested number of characters
 * return: 
 *	n	-> number of characters returned
 *	0	-> end of file
 */

static int
pkcget(fn, b, n)
register int n;
register char *b;
register int fn;
{
	register int ret;

	if (n == 0)
		return(0);
	if (setjmp(Getjbuf)) {
		Ntimeout++;
		DEBUG(4, "alarm %d\n", Ntimeout);
		return(-1);
	}

	(void) alarm( (unsigned) ( 10 + (n >> 7)) );

	for (;;) {
		if ( (ret = Read(fn, b, n)) <= 0 ) {
			if (ret == -1 && errno == EINTR)
				continue;
			/* EOF or Read failure */
			(void) alarm(0);
			DEBUG(4, "pkcget, read failed, errno %d\n", errno);
			return(-1);
		}
		if ((n -= ret) <= 0)
			break;
		b += ret;
	}
	(void) alarm(0);
	return(0);
}

void
pkfindsyn(pk, start_ptr, read_ptr, end_ptr)
register struct pack *pk;
char *start_ptr, **read_ptr, *end_ptr;
{

char *sync_ptr; 


	if ((sync_ptr=memchr(start_ptr,SYN,HDRSIZ)) == NULL)
	{
		*read_ptr = start_ptr;
		return ;
	}

	/* if SYN is in the middle, move SYN	*/
	/* and the remaining bytes to the start	*/
	/* of the header. Set up read_ptr to	*/
	/* read the missing bytes only.		*/

	while ( sync_ptr < end_ptr )
		*start_ptr++ = *sync_ptr++;
	
	*read_ptr = start_ptr;
}

