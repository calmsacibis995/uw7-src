/*		copyright	"%c%" 	*/

#ident	"@(#)tr:mtr.c	1.2.3.1"

#include "tr.h"


#define CH(ss,i)	ss->ch[i]
#define RNG(ss,i)	ss->rng[i]
#define REP(ss,i)	ss->rep[i]
#define NCH(ss)		ss->nch

static struct string {
	wint_t *ch;
	wint_t *rng;
	unsigned long *rep;
	unsigned long nch;
	size_t	size;
} s1, s2;

/*
 * Number of wide characters to increment size of
 * string structure by.
 */
#define STRINC	256	

static void sbuild(struct string *, chs*);
static void csbuild(struct string *, struct string *);

int
mtr(string1, string2)
chs *string1, *string2;
{
register unsigned long j, k;
register int i;
wint_t save=WEOF;
wint_t wc;
unsigned char buf[LINE_MAX], *bp, *ep;
size_t p;	/* number of valid bytes in buffer */
	

  if(mode & TR_COMPLEMENT) {
	sbuild(&s2, string1);
	csbuild(&s1, &s2);
  } else
	sbuild(&s1, string1);
  sbuild(&s2, string2);

  /*
   * Buffered read:
   * If there are no bytes in the buffer, fill it.
   * If there are less than MB_CUR_MAX in the buffer, copy the remaining
   * 	bytes to the beginning of the buffer then fill the buffer.
   * 
   * This means there is always a sequence of MB_CUR_MAX (or less at EOF)
   * contiguous bytes at bp.
   */

  bp = buf;
  ep = &buf[LINE_MAX];
  p = 0;
  while (1){
	if (p < MB_CUR_MAX) {
		/* if at end of buffer - copy p bytes to beginning of buffer */
		bp = memmove(buf, bp, p);
		i = fread(bp + p, 1, ep - (bp +p), stdin);
		p += i;

		if (p == 0) {	/* Error/EOF only caught when buffer emptied */
			if (ferror(stdin)) {
				(void)pfmt(stderr, MM_ERROR, 
					":1395:Error reading from stdin: %s\n",
					strerror(errno));
				exit(1);
			} else /* eof */
				break;
		}
	}

	/*
	 * Convert up to p bytes to a side character.
	 * If an error occurs, use the first byte.
	 */

	if ((i = mbtowc(&wc, (char*) bp, p)) <= 0) {
		i = 1;
		wc = *bp;
	}

	
	p -= i;		/* Decrement bytes in buffer */
	bp += i;	/* Increment buffer pointer  */

	for(i=s1.nch; --i>=0 && (wc<s1.ch[i] || s1.ch[i]+s1.rng[i]<wc);)
		;
	if(i>=0) { /* wc is specified in string1 */
		if(mode & TR_DELETE) 
			continue;
		j = wc-s1.ch[i]+s1.rep[i];
		while(i-->0) 
			j += s1.rep[i]+s1.rng[i];
		/* j is the character position of wc in string1 */
		for(i=k=0; i<s2.nch; i++) {
			if((k += s2.rep[i]+s2.rng[i]) >= j) {
				wc = s2.ch[i]+s2.rng[i];
				if(s2.rng[i]) 
					wc -= k-j;
				if(!(mode&TR_SQUEEZE)||wc!=save)
					goto put;
				else  	 
					goto next;
			}
		}
		goto next;
	}
	for(i=s2.nch; --i>=0 && (wc<s2.ch[i] || s2.ch[i]+s2.rng[i]<wc);)
		;
	if(i<0 || !(mode&TR_SQUEEZE) || wc!=save) {
	put:
		save = wc;
		if (wc <= UCHAR_MAX)
			(void)putchar(wc);
		else
			(void)putwchar(wc);
	}
  next: ;
  }
  return 0;
}

static void
place(struct string *s, int *i, wint_t wc) {
  if(++(*i)>=s->size) {					
	s->size += STRINC;				
	s->ch = realloc(s->ch, s->size * sizeof(wint_t)); 
	s->rng = realloc(s->rng, s->size * sizeof(wint_t)); 
	s->rep = realloc(s->rep, s->size * sizeof(unsigned long)); 
	if (s->ch == NULL || s->rng == NULL||s->rep == NULL){
		(void)pfmt(stderr, MM_ERROR, ":1394:Cannot malloc: %s\n",
						strerror(errno));
		exit(1);					
	}
  }							
  CH(s,*i)=wc; REP(s,*i)=1; RNG(s,*i)=0;
}


static void
sbuild(s, t)
struct string *s;
chs *t;
{
int i = -1;
int j;

  while (t != NULL) {
	switch(t->flags) {
		case CHS_RANGE:	  place(s, &i, t->chr);
				  RNG(s, i) = t->nch -1;
				  break;
		case CHS_REPEAT:  place(s, &i, t->chr);
				  REP(s, i) = t->nch;
				  break;
		case CHS_CHARS:	  for (j = 0; j < t->nch; j++)
					place(s, &i, t->chrs[j]);
				  break;
		default:
			(void)pfmt(stderr, MM_ERROR, ":1397:Internal Error\n");
			exit(1);
		}
	t = t->next;
  }
	NCH(s) = i+1;
}

static void
csbuild(s, t)
struct string *s, *t;
{
	int i, j;
	int k, nj;
	int *link;
	long int i_y, j_y, st;

	if ((link = malloc(NCH(t) * sizeof(int))) == NULL){
		(void)pfmt(stderr, MM_ERROR, ":1394:Cannot malloc: %s\n",
						strerror(errno));
		exit(1);
	}
#define J	link[j]
	NCH(s) = 0;
	for(nj=0, i=NCH(t); i-->0; ) {
		for(j=0; j<nj && (j_y = CH(t,J)+RNG(t,J)) < CH(t,i); j++)
			;
		if(j>=nj)
			link[nj++] = i;
		else if ((i_y = CH(t,i)+RNG(t,i)) < j_y) {
			if(CH(t,i) < CH(t,J)) {
				if(i_y < CH(t,J)-1) {
					for(k=nj++; k>j; k--)
						link[k] = link[k-1];
					link[j] = i;
				} else {
					RNG(t,J) = j_y-(CH(t,J) = CH(t,i));
				}
			}
		} else if(CH(t,i) <= CH(t,J))
			link[j] = i;
		else if(i_y > j_y)
			RNG(t,J) = i_y-CH(t,J);
	}
	/* "link" has the sorted order of CH */
	for(st=0, i = -1, j=0; j<nj; j++) {
		if(st<CH(t,J)) {
			place(s, &i, st);
			RNG(s,i) = CH(t,J)-1-st;
			REP(s,i) = 1;
		}
		st = CH(t,J)+RNG(t,J)+1;
	}
	if (MAXWC > st) {
		place(s, &i, st);
		RNG(s, i) = MAXWC-st;
		REP(s, i) = 1;
	}
	NCH(s) = i+1;
}
