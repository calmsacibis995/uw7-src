#ident	"@(#)optim:common/optim.c	1.41.1.44"

/*	machine independent improvement routines */

#include "optim.h"

/* unit of allocatable space (in char *'s) */
#define NSPACE	1024


			/* Size of hash table for labels */
#define LBLTBLSZ	239	/* should be prime */

/* what to do if no input file specified */
#define NOFILE()	/* by default, use stdin */

/* what to report if file-opening fails */
#define FFILER(S)	"can't open %s\n"


/* symbol table entry */

typedef struct lblstrct {
	char *cp;		/* the symbol */
	BLOCK *bl;		/* the block it is defined in */
	struct lblstrct *next;	/* hash table chain */
} LBL;

/* data structures */

NODE n0;			/* header for text list */
NODE ntail = { NULL, NULL, TAIL };	/* trailer for text list */
REF r0;			/* header for non-text reference list */
REF *lastref;			/* pointer to last label reference */
BLOCK b0;		/* header for block list */
SWITCH_TBL sw0; /* header for switch table list */
static BLOCK * Lastb = NULL;	/* pointer to array of blocks previously
				** allocated for bldgr()
				*/
int number_of_blocks = 0;
#ifndef DEBUG
static
#endif
LBL *Lbltbl[LBLTBLSZ];	/* hash table of pointers to labels */
static int Numlbls;		/* count of labels in hash table */
static BLOCK *Prevb;		/* pointer to previous block during
				   traversal of list */
static int fnum = 0;		/* function counter */
static int npass;		/* pass-through-this-function counter */
static int idx;			/* block index (for debugging) */

/* space allocation control */

static struct space_t {	/* to manage space allocation */
	struct space_t *next;
	unsigned size_space_t;
	char *space[NSPACE - 1]; /* dummy -- we will malloc what we need */
} *s0 = NULL, **Space;
static char *Lasta, *Lastx;	/* pointers into allocatable space */
static long Maxu = 0, Maxm = 0, Maxa = 0;

/* statistic counters */

#define STATINT static int
STATINT ndisc = 0;		/* instructions discarded */
STATINT nmerge = 0;		/* redundant instructions */
#undef STATINT
int ninst = 0;			/* total instructions */
static int nunr = 0;		/* unreachable instructions */
static int nsave = 0;		/* branches saved */
static int nrot = 0;		/* rotated loops */
#define PCASES 13
static int Pcase[PCASES + 1];	/* block types during reconstruction of text */
static struct added {		/* to keep statistics on branches added */
	struct added *next;
	short fnum,
	n_added;
} a0, *lastadd = &a0;

/* debugging flags */

int bflag = 0;		/* enable new optimization, tune time. */
int cflag = 0;			/* debug rm_all_tvrs */
int dflag = 0;      /* free */
int nflag = 0;      /* free */
int lflag = 0;		/* was Labels deleted now: show loop labels */
static int mflag = 0;		/* Merged suffixes found */
static int hflag = 0;		/* Path reconstruction trace */
/* static */ int sflag = 0;		/* changed: debug stack locations. Statistics (on stderr) */
static int uflag = 0;		/* free */
static int wflag = 0;		/* free */
static int eflag = 0;		/* Execution trace */
#ifdef DEBUG
extern int vflag;			/*enable scheduling comments*/
extern int nflag;           /* free */
#endif
extern int fflag;			/*ebboptim debugging flag*/
int jflag = 0;              /*supress RISC peepholes*/
extern int no_safe_asms;		/* debugging safe asms */
extern int zflag;			/* debugging loop optimizations */

/* for readability */

#ifdef MEMFCN	/* replace strncpy when memset is available (5.0 and after) */
#define CLEAR(s, n)	(void) memset((char *)(s), 0, (int)(n))
#else
#define CLEAR(s, n)	(void) strncpy((char *)(s), "", (int)(n))
#endif
#define FLAG(c, LFLAG)	case c: LFLAG += 1; continue
#define ALLB(b, s)	b = (s); b != NULL; b = b->next
#define PRCASE(N)	if (hflag) { prcase(N, b); Pcase[N]++; }
#define TOPOFBLOCK(p)	((p) == NULL || islabel(p))
#ifdef DEBUG
#define TRACE(F)	if (eflag) PRINTF("%cStarting %s[%d, %d]\n", \
			    CC, F, fnum, npass)
#else
#define TRACE(F)
#endif
#define MPRINTF		if (mflag) PRINTF
#define PRINDEX(P)	(b->P == NULL ? 0 : b->P->index)
#define PSTAT(S, N)	if ((N) > 0) FPRINTF(stderr, (S), (N))
#define ISUNCBL(b)	((b)->length == 1 && isuncbr((b)->lastn))
#define ISREMBL(b)	(ISUNCBL(b) && !ishb((b)->lastn))
#define ISREMBR(p)	(isbr(p) && !ishb(p))
#define RMBR(p)		(ndisc++, nsave++, DELNODE(p))
/* TARGET follows branches until a non-branch is reached.  However,
** there is the danger that we will loop on ourselves if we encounter
** an infinite loop.  Solve the problem partially by preventing
** self-loops.
*/
#define TARGET(b)	while (b->nextl != NULL && ISUNCBL(b->nextl) &&\
				b->nextl != b) b = b->nextl
#define NEWBLK(n, type)	((type *) xalloc((n) * sizeof(type)))

/* function declarations */

static void mustopen();
static void rmlbls();		/* remove unreferenced labels */
/* static */ void bldgr();		/* build flow graph of procedure */
static void mrgbrs();		/* merge branches to unconditional branches */
static void comtail();		/* merge common tails from code blocks */
static boolean chktail();	/* merge tails of bi-> and bj-> */
static void rmtail();		/* remove tail of b */
static void modrefs();		/* change all refs from bi to bj */
static void reord();		/* reorder code */
void indegree();			/* compute indegree */
static void findlt();		/* find rotatable loops */
static BLOCK *reord1();
static int outdeg();
static void prcase();		/* print information during reord */
static BLOCK *nextbr();		/* select next block to process */
static void putbr();		/* append a branch to b->nextl onto b */
static char *label_left();	/* get label of b->nextl */
static void rmunrch();		/* remove unreachable code */
static void reach();
static void rmbrs();		/* remove redundant branches */
static void rmbrs0();		/* remove redundant branches */
static NODE *initw();		/* find first available window */
static void prwindow();		/* print "size" instructions starting at p */
static void mkltbl();		/* make label table with only definitions */
static void clrltbl();		/* Clear label table.	*/
static void addlbl();		/* Add label in label table.	*/
static BLOCK *findlbl();	/* Find label in label table and */
				/* return the block that it's in. */

extern char ** yysflgs();
extern void yyinit();
extern void yylex();
extern void wrapup();
extern void dstats();
#ifndef tolower
extern int tolower();
#endif
extern void prinst();
extern void revbr();
extern boolean ishlp();
extern unsigned int uses();
extern unsigned int sets();
extern SWITCH_TBL * get_base_label();
#ifdef DEBUG
void dprblocks();
void dprblock();
#endif

/************************************************************************/

	int
main(argc, argv) /* initialize, process parameters,  control processing, etc. */
	int argc; register char *argv[]; {

	char usrflag[10];
	int ufl = 0, i, fileseen = 0;

	/* process parameters on command line */

	while (--argc > 0) {
		if (**++argv != '-') { /* alternate file(s) */
			switch (fileseen) {
			case 0: /* none yet, open input file */
				mustopen(*argv, "r", stdin);
				fileseen++;
				continue;
			case 1: /* input seen, open output file */
				mustopen(*argv, "w", stdout);
				fileseen++;
				continue;
			}
			fatal(__FILE__,__LINE__,"too many filenames\n");
		}
		while ((i = *++*argv) != '\0') { /* debugging flags */
			switch (tolower(i)) {
			case 'i': /* alternate input file */
				if (--argc <= 0)
					fatal(__FILE__,__LINE__,"no argument for -i option\n");
				mustopen(*++argv, "r", stdin);
				break;
			case 'o': /* alternate output file */
				if (--argc <= 0)
					fatal(__FILE__,__LINE__,"no argument for -o option\n");
				mustopen(*++argv, "w", stdout);
				break;
#ifdef DEBUG
			FLAG('b', bflag);
			FLAG('c', cflag); /* kill comtail */
#endif
			FLAG('d', dflag);	/* suppress movw 2 2 movb's */
			FLAG('e', eflag);
			FLAG('f', fflag); /*ebboptim trace*/
			FLAG('g', no_safe_asms); /*debug safe asms */
			FLAG('h', hflag);
			FLAG('j', jflag); /*join instructions */
			FLAG('l', lflag);
			FLAG('m', mflag);
			/*FLAG('n', nflag);*/
			FLAG('r', nrot); /* kill loop rotations */
			FLAG('s', sflag);
			FLAG('u', uflag);
#ifdef DEBUG
			FLAG('v', vflag); /*suppress scheduler comments*/
#endif
			FLAG('w', wflag);
			FLAG('z', zflag);
			default:
				if (spflg(i)) {
					if (argv != yysflgs(argv)) {
						--argc;
						++argv;
					}
					*(*argv+1) = '\0';
					continue;
				}
				else
				usrflag[ufl++] = (char)i;
				continue;
			}
			break;
		}
	}
	if (bflag | eflag | lflag | mflag | hflag | uflag | wflag)
		setbuf(stdout, (char *)NULL); /* for easier debugging */
	; /* if no input file specified */

	/* initialize everything */

	init();
	usrflag[ufl] = '\0';
	yyinit(usrflag);

	/* transfer to the machine dependent part */

	yylex();
	wrapup();

	/* print statistics if asked for */

#if 0
	if (sflag) {
		register struct added *a;

		for (a = a0.next; a != NULL; a = a->next)
			FPRINTF(stderr, "%d branch(es) added to function %d\n",
			    a->n_added, a->fnum);
		dstats();	/* machine dependent statistics */
		PSTAT("%d unreachable instruction(s) deleted\n", nunr);
		PSTAT("%d branch(es) saved\n", nsave);
		PSTAT("%d instruction(s) merged\n", nmerge);
		FPRINTF(stderr, "%d of %d total instructions discarded\n",
		    ndisc, ninst);
		PSTAT("%d loop(s) rotated\n", nrot);
		FPRINTF(stderr,
		"%ld bytes used, %ld allocated\n%d function(s), %d optim(s)\n",
		    Maxm, Maxa, fnum - 1, noptim > 0 ? noptim : 0);
		if (hflag && noptim > 0)
			for (FPRINTF(stderr, "case\tnumber\n"),
			    i = 0; i <= PCASES; i++)
				FPRINTF(stderr, "%2d\t%3d\n", i, Pcase[i]);
	}
#endif
	return (0);
}

	static void
mustopen(name, dir, file) char *name, *dir; FILE *file; {

	if (freopen(name, dir, file) == NULL)
		fatal(__FILE__,__LINE__,FFILER(name), name);
}

	void
init() { /* reset pointers, counters, etc for next function */

	register struct space_t *p, *pp;
	long maxa = LBLTBLSZ * sizeof(LBL);

	if (Lastb != NULL)		/* free bldgr's storage */
	    xfree((char *) Lastb);
	Lastb = NULL;			/* no memory now allocated */

	for (p = s0, pp = NULL; p != NULL; p = pp) {
		maxa += p->size_space_t;
		pp = p->next;
		xfree((char *) p);
	}
	if ((Maxu += Numlbls * sizeof(LBL)) > Maxm)
		Maxm = Maxu;
	if (maxa > Maxa)
		Maxa = maxa;
	Maxu = 0;
	Space = &s0;
	s0 = NULL;
	Lasta = Lastx = NULL;
	n0.forw = &ntail;
	ntail.back = &n0;
	sw0.next_switch = NULL;
	b0.next = NULL;
	b0.firstn = b0.lastn = &n0;
	r0.nextref = NULL;
	lastref = &r0;
	fnum++;
	npass = 0;
}
static void
clean_label_sets_uses()
{
NODE *p;
	for(ALLN(p))
		if(is_any_label(p)) {
			p->sets = p->uses =0;
		}
}/*clean_label_sets_uses*/

/* control improvement sequencing */
	void
optim(debug) boolean debug;
{

#ifdef DEBUG
	if (debug)
		COND_RETURN("optim");
#endif
	if (n0.forw != &ntail) {

		int onsave = nsave;

		rmlbls();	
		bldgr(false);
        rmbrs0();
		mrgbrs();	/* merge branches to branches */
		rmunrch(false);	/* remove unreachable code, don't preserve
				** block/node connectivity
				*/
		comtail();	/* remove common tails */
		reord();	/* reorder code */
		bldgr(false);
		rmbrs();	/* remove redundant branches */
		rmlbls();
		ldanal();	/* perform live/dead analysis */
#if 0
		if (sflag && onsave > nsave) {
			register struct added *a = lastadd = lastadd->next =
			    NEWBLK(1, struct added);

			a->next = NULL;
			a->fnum = (short)fnum;
			a->n_added = onsave - nsave;
		}
#endif
	}
	clean_label_sets_uses();
}

	static void
rmlbls() { 

	register REF *r;
	register NODE *p;
	register char *s;

#ifdef FLIRT
	static int numexec=0;
#endif

	TRACE("rmlbls");

	COND_RETURN("rmlbls");

#ifdef FLIRT
	++numexec;
#endif

	clrltbl();

	/* add references from data section */

	for (r = r0.nextref; r != NULL; r = r->nextref)
		addlbl(r->lab, &b0);

	/* add references from branches in text section */

	for (ALLN(p)) {
		if (is_any_br(p) && (s = getp(p)) != NULL) {
			addlbl(s, &b0);
			continue;
		}
	}

	/* delete non-hard labels that are not now in the label table */

	for (ALLN(p)) if (islabel(p) && !ishl(p) && findlbl(p->ops[0]) == NULL) {
			FLiRT(O_RMLBLS,numexec);
			DELNODE(p);
		}

}

/* This routine attempts to economize on space be allocating a hunk
** of storage big enough for all program blocks.  It deallocates that
** hunk on the next call in hopes it can be reused.
*/
#define BLOCK_SIZE_LIMIT	100


	/*static*/ void
bldgr(call_terminates) boolean call_terminates; {
	register BLOCK *b = &b0;
	register NODE *p;
	int count; 

	TRACE("bldgr");

	if (Lastb != NULL)		/* deallocate old array, if any */
	    xfree((char *)Lastb);

	/* Count number of blocks so we can allocate an array */

	idx = 0;			/* use this to count blocks */
	p = n0.forw;			/* point at first node */
	while (p != &ntail)
	{
	    idx++;			/* count one more block */
	    while (is_any_label(p))		/* skip leading labels */
		p = p->forw;
	    
	    for (count =0 ; p != &ntail && !is_any_label(p) ; p = p->forw,count++) {
		    if (is_any_br(p) ||
				( call_terminates &&
				(count >BLOCK_SIZE_LIMIT || p->op == CALL || p->op == ASMS ||
				 is_safe_asm(p)))) {
			    p = p->forw;
			    break;
		    }
	    }
	}

	/* idx is now the number of blocks.  Allocate array. */
	number_of_blocks = idx;

	if(!idx)
		return;
	Lastb = (BLOCK *) xalloc(idx * sizeof(BLOCK));

	/* now build the flow graph */

	idx = 0;
	b = b0.next = Lastb;		/* point at prospective first block */
	clrltbl();
	for (p = n0.forw; p != &ntail; ) {

		register NODE *prevn = p->back;

		b->next = b + 1;	/* "next" will be physically next */
		b->prev = b - 1;	/* "prev" will be physically prev */
		b->index = ++idx;
		b->length = b->marked = b->indeg = b->indeg2 = 0;

		/* a block starts with 0 or more labels */

		while (is_any_label(p)) {
			addlbl(p->ops[0],b);
			p = p->forw;
		}

		/* followed by 0 or more non-branch instructions
		   terminated with a branch or before another label */

		for (count = 0 ; p != &ntail && !is_any_label(p); p = p->forw, count++) {
			b->length++;
			if (is_any_br(p) ||
				(call_terminates &&
				(count > BLOCK_SIZE_LIMIT || p->op == CALL || p->op == ASMS ||
				 is_safe_asm(p)))) {
				p = p->forw;
				break;
			}
		}
		b->lastn = p->back;
		if ((b->firstn = prevn->forw) != p) /* if non-empty block */
			b++;		/* we will next do next block */
	}
	b[-1].next = NULL;		/* (assumes at least one block) next
					** pointer of last block we filled in
					** is NULL
					*/


	/* set branch pointers */

	for (ALLB(b, b0.next)) {
		char *s;

		p = b->lastn;
		b->nextl = b->next;
		b->nextr = is_any_br(p) && (s = getp(p)) != NULL ?
		    findlbl(s) : NULL;
		if (is_any_uncbr(p)) {
			b->nextl = b->nextr;
			b->nextr = NULL;
		}
#ifdef DEBUG
		if (0) {
		fprintf(stderr,
		"%c\n%cblock %d (left: %d, right: %d, length: %d)\n%cfirst:\t",
			    CC, CC, b->index, PRINDEX(nextl), PRINDEX(nextr),
			    b->length, CC);
			fprinst(b->firstn);
			fprintf(stderr,"%clast:\t", CC);
			fprinst(p);
		}
#endif
	}
}

void
set_refs_to_blocks()
{
REF *r;
BLOCK *b;
NODE *p;
SWITCH_TBL *sw;
int found;
	for (sw = sw0.next_switch ; sw ; sw = sw->next_switch) {
		for (r = sw->first_ref; ; r = r->nextref) {
			found = false;
			for (b = b0.next; b; b = b->next) {
				p = b->firstn;
				while (islabel(p)) {
					if (!strcmp(p->opcode,r->lab)) {
						r->switch_entry = b;
						found = true;
						break;
					}
					p = p->forw;
				}
				if (found) break;
			}
			if ( r == sw->last_ref )
				break;
		}
	}
} /*end set_refs_to_blocks*/

	static void
mrgbrs() { /* merge branches to unconditional branches */

	register BLOCK *b, *bb;

#ifdef FLIRT
	static int numexec = 0;
#endif

	TRACE("mrgbrs");

	COND_RETURN("mrgbrs");

#ifdef FLIRT
	++numexec;
#endif

	/* merge unconditional branches to their destinations */

	for (ALLB(b, b0.next))
		if ((bb = b->nextl) == b->next) { /* fall-through */
			if ((b = bb) == NULL)
				break;
		}
		else if (bb != NULL && bb != b &&
			(!func_data.two_entries || b != b0.next) &&
		    islabel(b->firstn) && ISREMBL(b)) {
			ndisc++;
			nsave++;
			modrefs(b->lastn->back, b, bb);
		}

	/*
	 * It is assumed that "ret" is an unconditional branch;
	 * that getp on a "ret" returns NULL; that this can be
	 * placed on an unconditional branch ("jbr");
	 * that prinst() will convert "jbr NULL" back to "ret";
	 * but that the NULL
	 * cannot be placed on a conditional branch ("jne").
	 * (NULL is also returned by a multi-way branch (switch).)
	 */

	for (ALLB(b, b0.next)) {

		char *t;
		register NODE *p = b->lastn;

		if (!isbr(p))
			continue;
		if (isuncbr(p))
			while (   (bb = b->nextl) != NULL
			       && bb != bb->nextl
			       && ISREMBL(bb)) {
				register NODE *pp = bb->lastn;

				if ((t = getp(pp)) != NULL) {
					FLiRT(O_MRGBRS,numexec);
					p->op1 =  t;
					p->uses = uses(p);
				}
				else { /* pp is a dead-end */
					(void) memcpy((char *)p->ops,
					    (char *)pp->ops,
					    sizeof(pp->ops));
					(void) memcpy((char *) &p->userdata,
						(char *) &pp->userdata,
						sizeof(USERTYPE));
					p->op = pp->op;
					FLiRT(O_MRGBRS,numexec);
				}
				b->nextl = bb->nextl;
			}
		else
			/* p points to a conditional branch; no indirection. */
			while (   (bb = b->nextr) != NULL
			       && bb != bb->nextl
			       && ISREMBL(bb)
			       && (t = getp(bb->lastn)) != NULL
			       && (*t != '*')) {
				p->op1 = t;
				p->uses = uses(p);
				FLiRT(O_MRGBRS,numexec);
				b->nextr = bb->nextl;
			}
	}
}

	static void
comtail() { /* merge common tails from code blocks */

	boolean changed;
	BLOCK *b;

#ifdef FLIRT
	static int numexec=0;
#endif

	TRACE("comtail");
    COND_RETURN("comtail");

#ifdef FLIRT
	++numexec;
#endif

	do {
		register BLOCK *bi, *bj, *bi0, *bj0;

		changed = false;
		indegree(false); /* compute indegree (0 from bldgr()) */
		for (ALLB(bi, b0.next)) { /* compute a key for each block */
			bi->marked = 0;
			if (bi->length == 1 && isbr(bi->lastn))
				continue;
			bi0 = bi;
			if (isbr(bi->lastn) && !isuncbr(bi->lastn)) {
				TARGET(bi0);
				bi->marked += bi0->lastn->op;
			}
			bi->marked += bi0->nextl - &b0;
		}
		for (ALLB(bi, b0.next)) {

			if (!bi->marked)
				continue;
			for (ALLB(bj, bi->next)) {
				if (bi->marked != bj->marked)
					continue; /* quick sieve on key */
				if (bi->nextr != bj->nextr)
					continue;
				bi0 = bi; bj0 = bj;
				/* if both blocks end in conditional branches,
				 * look ahead for left targets */
				if (isbr(bj->lastn) && !isuncbr(bj->lastn)) {
					if (bi->lastn->op != bj->lastn->op)
						continue;
					if(bi->nextr == NULL &&
						!same_inst(bi->lastn,bj->lastn))
						continue;
					TARGET(bi0);
					TARGET(bj0);
				}
				/* blocks must fall through to same place */
				if (bi0->nextl != bj0->nextl)
					continue;
				/* dead-end branches must have same text */
				if (bi0->nextl == NULL &&
				    !same_inst(bi0->lastn, bj0->lastn))
					continue;
				if (chktail(bi, bj, bi0->nextl) == true)
					 changed = true;
					 FLiRT(O_COMTAIL,numexec);
			}
		}
	} while (changed == true);
	for (b = b0.next; b && b->next; b = b->next) {
		b->lastn->forw = b->next->firstn;
		b->next->firstn->back = b->lastn;
	}
}

	static boolean
chktail(bi, bj, bl) /* merge tails of bi-> and bj-> */
	register BLOCK *bi, *bj; BLOCK *bl; {

	extern void rmtail();
	register BLOCK *bn;
	NODE *pi = bi->lastn, *pj = bj->lastn, *firstn, *lastn, *pb = NULL;
	int length = 0, isbri = 0, isbrj = 0;

	/* pi and pj scan backwards through blocks bi and bj 
	   until difference or no more code */

	if (isbr(pi)) { /* trailing branches have already been matched */
		pb = pi;
		pi = pi->back;
		isbri++;
	}
	if (isbr(pj)) {
		pb = pj;
		pj = pj->back;
		isbrj++;
	}
	for (firstn = lastn = pj; !TOPOFBLOCK(pi) && !TOPOFBLOCK(pj) &&
	   same_inst(pi, pj) == true; length++) {
		firstn = pj;
		pi = (pi == bi->firstn) ? NULL : pi->back;
		pj = (pj == bj->firstn) ? NULL : pj->back;
	}
	if (length == 0)
		return (false);

	/* if blocks identical, change references to one to the other */

	if (TOPOFBLOCK(pi) && TOPOFBLOCK(pj)) {
		isbri = 0;
		modrefs(pj, bj, bn = bi);
		MPRINTF("%cblock %d merged into block %d and deleted\n",
		    CC, bj->index, bi->index);
	}

	/*
	 * Conservative common-tail merging avoids adding a branch to
	 * achieve a merge.  It merges only blocks which join with no other
	 * blocks joining at that point, so that the joining branch is merely
	 * raised above the common tail, and no new branch is added.
	 */

	else if (bl == NULL || bl->indeg > 2)
		return (false); /* conservative common tails */

	/* if one block is a tail of the other, remove the tail from the
	   larger block and make it reference the smaller */

	else if (TOPOFBLOCK(pi)) {
		isbri = 0;
		bj->lastn = pj;
		bj->length -= length + isbrj;
		bj->nextl = bn = bi;
		rmtail(bj);
	}
	else if (TOPOFBLOCK(pj)) {
		isbrj = 0;
		bi->lastn = pi;
		bi->length -= length + isbri;
		bi->nextl = bn = bj;
		rmtail(bi);
	}

	/* otherwise make a new block, remove tails from common blocks and
	   make them reference the new block */

	else {
		bi->lastn = pi;
		bj->lastn = pj;
		bi->length -= length + isbri;
		bj->length -= length + isbrj;
		bn = GETSTR(BLOCK);
		*bn = *bj;
		bn->firstn = firstn;
		bn->lastn = lastn;
		bn->length = (short)length;
		bn->index = ++idx;
		bn->indeg = 2;
		bi->nextl = bj->nextl = bj->next = bn;
		bi->nextr = bj->nextr = NULL;
		MPRINTF("%ctails of %d and %d merged into new block %d\n",
		    CC, bi->index, bj->index, idx);
	}
	if (pb != NULL && !isbr(bn->lastn)) { /* save final branch */
		ndisc--;
		nsave--;
		bn->length++;
		pb->back = bn->lastn;
		bn->lastn = bn->lastn->forw = pb;
	}

	for (pb = bn->firstn; pb != NULL; pb = pb->forw) {
		pb->uniqid = IDVAL;
		if (pb == bn->lastn)
			break;
	}

	ndisc += length + isbri + isbrj;
	nmerge += length;
	nsave += isbri + isbrj; /* don't blame resequence for added branch */
	MPRINTF("%c%d instruction(s) common to blocks %d and %d\n",
	    CC, length, bi->index, bj->index);
	return (true);
}

	static void
rmtail(b) register BLOCK *b; { /* remove tail of b */

	b->nextr = NULL;
	MPRINTF("%ctail of block %d deleted\n", CC, b->index);
}

	static void
modrefs(pi, bi, bj) /* change all refs from bi to bj */
	register NODE *pi; register BLOCK *bi, *bj; {

	register BLOCK *b;

	if (pi != NULL) { /* transfer labels, if any, from bi to bj */
		/* bi->firstn points to the first label to be transferred,
		 * pi points to the last. */
		bj->firstn->back = pi;
		pi->forw = bj->firstn;
		bj->firstn = bi->firstn;
		for ( ; ; pi = pi->back) { /* update the label table */
			addlbl(pi->ops[0], bj);
			if (pi == bi->firstn)
				break;
		}
	}
	for (ALLB(b, &b0)) { /* update the block structure */
		if (b->next == bi)
			b->next = bi->next;
		if (b->nextl == bi)
			b->nextl = bj;
		if (b->nextr == bi)
			b->nextr = bj;
	}
}

	static void
reord() { /* reorder code */

	extern BLOCK *reord1();
	extern void findlt();
	register BLOCK *b;

#ifdef FLIRT
	static int numexec=0;
#endif

	TRACE("reord");
    COND_RETURN("reord");

#ifdef FLIRT
	++numexec;
#endif

	if (hflag) {
		for (ALLB(b, b0.next)) {
			PRINTF(
			"%cB4\n%cblock %d (left: %d, right: %d, length: %d)\n%cfirst:\t",
			    	CC, CC, b->index, PRINDEX(nextl), PRINDEX(nextr),
			    	b->length, CC);
				prinst(b->firstn);
				PRINTF("%clast:\t", CC);
				prinst(b->lastn);
		}
	}
	for (ALLB(b, b0.next)) {
		b->ltest = NULL;
		b->marked = 0; /* mark all blocks as unprocessed */
	}
	indegree(false); /* compute indegree */

#if 0
	if (nrot >= 0)
		findlt(); /* find rotatable loops */
#endif

	/* tie blocks back together */

	if (hflag)
		PRINTF("%cblock\tleft\tright\tcase\tlabels\n", CC);
	Prevb = &b0;
	for (b = b0.next; b != NULL; )
		b = reord1(b);
	if (Prevb->nextl != NULL)
	    putbr(Prevb);
	Prevb->lastn->forw = &ntail;
	ntail.back = Prevb->lastn; /* tack on tail node to text list */
	FLiRT(O_REORD,numexec);
	mkltbl(); /* make label table with only definitions */
	rmunrch(true); /* remove unreachable code */
		if (hflag) {
		for (ALLB(b, b0.next)) {
		PRINTF(
		"%cC5\n%cblock %d (left: %d, right: %d, length: %d)\n%cfirst:\t",
			    CC, CC, b->index, PRINDEX(nextl), PRINDEX(nextr),
			    b->length, CC);
			prinst(b->firstn);
			PRINTF("%clast:\t", CC);
			prinst(b->lastn);
		}
		}
}

/* compute indegree, for a basic block, how many basic blocks point to it. */
void
indegree(with_sw) int with_sw;
{

	register BLOCK *b, *bb;
	REF *r;
	SWITCH_TBL *sw;

	for (ALLB(b, b0.next))
		b->indeg = 0;
	for (ALLB(b, b0.next)) { /* compute indegree */
		if ((bb = b->nextl) != NULL)
			bb->indeg++;
		if ((bb = b->nextr) != NULL)
			bb->indeg++;
		if ( with_sw && is_jmp_ind(b->lastn) && ((sw = get_base_label(b->lastn->op1)) != 0)) {
			for (r = sw->first_ref; r; r = r->nextref) {
				if (sw->switch_table_name == r->switch_table_name)
					r->switch_entry->indeg++;
		    	if ( r == sw->last_ref )
					break;
		    }
  		}
	}
}

/* compute indegree from a given basic block to where it reachs
** use to test domination: a block b dominates a block b1 iff
** b1->indeg == b1->indeg2, where indeg2 was called with b.
*/


void
indegree2(b) BLOCK *b;
{
	REF *r;
	SWITCH_TBL *sw;
	if (b->marked)
		return;
	b->marked = true;
	if (b->nextl) {
		b->nextl->indeg2++;
		indegree2(b->nextl);
	}
	if (b->nextr) {
		b->nextr->indeg2++;
		indegree2(b->nextr);
	}
	if (is_jmp_ind(b->lastn) && ((sw = get_base_label(b->lastn->op1)) != 0)) {
		for (r = sw->first_ref; r; r = r->nextref) {
			if (sw->switch_table_name == r->switch_table_name) {
				r->switch_entry->indeg2++;
				indegree2(r->switch_entry);
			}
		    if ( r == sw->last_ref )
				break;
  		}
  	}
}/*end indegree2*/

void
drive_indegree2(b) BLOCK *b;
{
BLOCK *a;
	for (a = b0.next; a; a = a->next) {
		a->marked = a->indeg2 = 0;
	}
	indegree2(b);
}

void
indegree3(b,removed) BLOCK *b,*removed;
{
REF *r;
SWITCH_TBL *sw;
	b->marked = true;
	if (b->nextl && b->nextl != removed) {
		b->nextl->indeg2++;
		if (b->nextl->marked == 0)
			indegree3(b->nextl,removed);
	}
	if (b->nextr && b->nextr != removed) {
		b->nextr->indeg2++;
		if (b->nextr->marked == 0)
			indegree3(b->nextr,removed);
	}
	if (is_jmp_ind(b->lastn) && ((sw = get_base_label(b->lastn->op1)) != 0)) {
		for (r = sw->first_ref; r; r = r->nextref) {
			if (sw->switch_table_name == r->switch_table_name) {
				if (r->switch_entry != removed) {
					r->switch_entry->indeg2++;
					if (r->switch_entry->marked == 0)
						indegree3(r->switch_entry,removed);
				}
			}
		    if ( r == sw->last_ref )
				break;
  		}
  	}
}/*end indegree3*/

void
drive_indegree3(b,removed) BLOCK *b,*removed;
{
BLOCK *c;
	for (c = b0.next; c; c = c->next) {
		c->indeg2 = c->marked = 0;
	}
	indegree3(b,removed);
}/*end drive_indegree3*/

	static void
findlt() { /* find rotatable loops */

	/*
	 * To identify the top and termination-test of a rotatable loop:
	 * Look at the target of an unconditional backward branch.
	 * If it has only one reference, then it isn't the start of a loop.
	 * Then look at all intermediate blocks in lexical order
	 * to find a conditional jump past the backward branch.
	 * This is a very simplistic heuristic approach, because the loop
	 * test is actually never made.
	 * But it seems to give reasonable results rather rapidly.
	 * If there is more than one exit from the loop,
	 * rotate at the exit nearest to the bottom,
	 * in order to keep the elements of a compound test near each
	 * other (in case of window optimization)
	 * and near the bottom (in case of span-dependent branches).
	 */

	register BLOCK *b, *bl, *bb, *br;

#ifdef DEBUG
	if (0)
		PRINTF("%cltests are:", CC);
#endif
	for (ALLB(b, b0.next)) {
		if (b->nextr != NULL || (bl = b->nextl) == NULL ||
		    bl->indeg < 2 || bl->index > b->index || bl->ltest != NULL)
			continue;
		for (bb = bl; bb != NULL && bb->index < b->index; bb = bb->next)
			if ((br = bb->nextr) != NULL && br->index > b->index)
				bl->ltest = bb;
#ifdef DEBUG
		if (0 && bl->ltest != NULL)
			PRINTF(" %d/%d", bl->index, bl->ltest->index);
#endif
	}
#ifdef DEBUG
	if (0)
		PUTCHAR('\n');
#endif
}

#define B_EXIT	2

	static BLOCK *
reord1(b) register BLOCK *b; {

	extern BLOCK *nextbr();
	extern void prcase();
	register BLOCK *bl, *br, *blt;

	/*fprintf(stderr,"reord1 of "); fprinst(b->firstn); */
	/* top of rotatable loop */
	/* don't rotate unless there already must be a branch to the entry */

	if (b->ltest != NULL && b != Prevb->nextl &&
		b->ltest->nextl != NULL &&
	    (bl = b->ltest->nextl)->ltest == NULL && !bl->marked) {
		b->ltest = NULL;
		nrot++;
		/*fprintf(stderr,"return block of "); fprinst(bl->firstn); */
		return (bl);
	}

	/* mark block as processed and tie it in */

	b->marked++;
	if (b != Prevb->nextl)
		putbr(Prevb);
	Prevb->lastn->forw = b->firstn;
	b->firstn->back = Prevb->lastn;
	Prevb = b;

	/* dead-end block */

	if ((bl = b->nextl) == NULL) {
		PRCASE(0);
		/*fprintf(stderr,"0, return block of "); fprinst((nextbr(b))->firstn); */
		return (nextbr(b));
	}

	bl->indeg--;
	/*fprintf(stderr,"reduced to %d indeg of ",bl->indeg); fprinst(bl->firstn); */
	if ((br = b->nextr) != NULL)
		br->indeg--;

	/* top of rotatable loop */

	if ((blt = bl->ltest) != NULL
	    && blt->nextl != NULL
	    && blt->nextl->ltest == NULL &&
	    !blt->nextl->marked
	    && blt->nextr != NULL 
	    && !blt->nextr->marked && outdeg(bl) <= 1) {
		PRCASE(1);
		b = blt->nextl;
		bl->ltest = NULL;
		nrot++;
		/*fprintf(stderr,"1, return block of "); fprinst(b->firstn); */
		return (b);
	}

	if (br == NULL) { /* unconditional branch or conditional to dead-end */

		if (!bl->marked) { /* to unprocessed block */

			if (bl->indeg <= 0) { /* with indeg 1 */
				PRCASE(2);
		/*fprintf(stderr,"2, return block of "); fprinst(bl->firstn); */
				return (bl);
			}

			/* branch to block with indeg > 1
			   that originally followed this one */

			if (bl == b->next) {
				PRCASE(3);
		/*fprintf(stderr,"3, return block of "); fprinst(bl->firstn); */
				return (bl);
			}

			/* branch to dead-end block */

			if (bl->nextl == NULL) {
				PRCASE(4);
		/*fprintf(stderr,"4, return block of "); fprinst(bl->firstn); */
				return (bl);
			}

		}

		/* all other unconditional branches */

		PRCASE(5);
		/*fprintf(stderr,"5, return block of "); fprinst(nextbr(b)->firstn); */
		return (nextbr(b));
	}

	/* conditional branch to processed block */

	if (br->marked && !bl->marked) {

		/* fall through to unprocessed block with indeg = 1 */

		if (bl->indeg <= 0) {
			PRCASE(6);
		/*fprintf(stderr,"6, return block of "); fprinst(bl->firstn); */
			return (bl);
		}

		/* fall through to unprocessed block with indeg > 1
		   that originally followed this one */

		if (bl == b->next) {
			PRCASE(7);
		/*fprintf(stderr,"7, return block of "); fprinst(bl->firstn); */
			return (bl);
		}
	}

	/* reversible conditional branch to unprocessed block,
	   fall through to processed block */

	if (bl->marked && !br->marked && isrev(b->lastn)) {
		revbr(b->lastn);
		b->lastn->op1 =  label_left(b);
		b->nextr = b->nextl;
		b->nextl = br;
		PRCASE(8);
		return (br->indeg <= 0 ? br : nextbr(b));
	}

	/* all other conditional branches that have one leg or the
	   other going to processed blocks */

	if (bl->marked || br->marked) {
		PRCASE(9);
		/*fprintf(stderr,"9, return block of "); fprinst(nextbr(b)->firstn); */
		return (nextbr(b));
	}

	/* fall through to block with indeg = 1
	   but not if it is an unlabeled unconditional transfer */

	if (bl->indeg <= 0 && !(isuncbr(bl->firstn) && isrev(b->lastn))) {
		PRCASE(10);
		/*fprintf(stderr,"10, return block of "); fprinst(bl->firstn); */
		return (bl);
	}

	/* reversible branch to block with indeg = 1 */

	if (br->indeg <= 0 && isrev(b->lastn) && (b->lastn->dependents == 0)) {
		revbr(b->lastn);
		b->lastn->op1 =  label_left(b);
		b->nextr = b->nextl;
		b->nextl = br;
		PRCASE(11);
		/*fprintf(stderr,"11 (%d), return block of ",bl->indeg); fprinst(br->firstn); */
		return (br);
	}

	/* fall through to block with indeg > 1 that
	   originally followed this block */

	if (bl == b->next) {
		PRCASE(12);
		/*fprintf(stderr,"12, return block of "); fprinst(bl->firstn); */
		return (bl);
	}

	/* everything else */

	PRCASE(13);
		/*fprintf(stderr,"13, return block of "); fprinst(nextbr(b)->firstn); */
	return (nextbr(b));
}

/* Routine outdeg works in conjunction with loop rotation.  It uses a
** heuristic to determine how many of the loop exit target's remaining
** incoming arcs are due to exits from the loop that is to be rotated.
** Outdeg is called with a pointer to the top-of-loop block.  It scans
** lexically through the blocks that follow the top (much like findlt())
** until
**	1) there is no next block
**	2) the "left" path points at the loop top, indicating the block
**		is the loop end
**	3) the new block's index is at or past the loop target (since
**		findlt calls something a loop exit when the block index
**		of the "right" path is beyond the loop end)
**
** As we scan through the blocks lexically, we decrement the effective
** indegree of the loop target whenever we find a "right" path that
** goes to the target from an unmarked block.  (If the block was marked,
** its contribution to indegree has already been accounted for.)
*/

	static int
outdeg(top)
BLOCK * top;				/* pointer to top of loop */
{
    BLOCK * target = top->ltest->nextr;	/* loop exit target */
    BLOCK * bp;				/* scanning block pointer */
    int in_degree = target->indeg;	/* in-degree of target block */

    /* As a short-circuit, discontinue searching when the new in_degree
    ** is <= 1
    */

    if (in_degree <= 1)
	return(in_degree);
    
    for (bp = top;
	      bp != NULL		/* have a block */
	   && bp->nextl != top		/* it doesn't close the loop */
	   && bp->index < target->index ; /* it isn't past the target */
	 bp = bp->next)
    {
	if (
	      ! bp->marked		/* the block is unmarked */
	    && bp->nextr == target	/* it branches cond. to target */
	    && --in_degree <= 1		/* time to quit */
	    )
	    break;
    }

    return(in_degree);			/* return effective indegree */
}


	static void
prcase(n, b) int n; register BLOCK *b; { /* print information during reord */

	register NODE *p;

	PRINTF("%c%d\t%d\t%d\t%d", CC, b->index,
	    PRINDEX(nextl), PRINDEX(nextr), n);
	for (p = b->firstn; islabel(p); p = p->forw)
		PRINTF("\t%s", p->ops[0]);
	PRINTF("\n");
}

	static BLOCK *
nextbr(b) register BLOCK *b; { /* select next block to process */

	register BLOCK *bb;

	/* first look for orphan blocks (no more references) from the top */

	for (ALLB(bb, b0.next))
		if (!bb->marked && bb->indeg <= 0)
			return (bb);

	/* now look for unmarked block with live consequent (circularly) */

	for (bb = b->next; bb != b; bb = bb->next)
		if (bb == NULL) /* circular scan for next block */
			bb = &b0;
		else if (!bb->marked &&
		    bb->nextl != NULL && !bb->nextl->marked)
			return (bb);

	/* now look for any unmarked block (circularly) */

	for (bb = b->next; bb != b; bb = bb->next)
		if (bb == NULL) /* circular scan for next block */
			bb = &b0;
		else if (!bb->marked)
			return (bb);

	return (NULL); /* no more blocks to process */
}

	static void
putbr(b) register BLOCK *b; { /* append a branch to b->nextl onto b */

	register NODE *p, *pl = b->lastn;
	char *s;

	if (b == &b0 || is_any_uncbr(pl))
		return;
	ndisc--;
	nsave--;
	b->length++;
	p = Saveop(0, "" , 0, GHOST); /* make node but don't link */
	b->lastn = pl->forw = p; /* link at end of this block */
	p->back = pl;
#if EH_SUP
	p->in_try_block = pl->in_try_block;
#endif
	s = label_left(b); /* get destination label, in 2 steps */
	setbr(p, s); /* in case setbr is a macro which double-evaluates */
	new_sets_uses(p);
}

	static char *
label_left(b) register BLOCK *b; { /* get label of b->nextl */

	register NODE *pf, *p;
	register BLOCK *bl;

	if ((bl = b->nextl) == NULL)
	{
		NODE *p;
		fprintf(stderr,"left of block of "); fprinst(b->firstn); 
		fprinst(b->lastn); 
		fatal(__FILE__,__LINE__,"label of nonexistent block requested\n");
	}
	for ( ; ISUNCBL(bl) && bl->nextl != bl; bl = bl->nextl)
		if (bl->nextl == NULL) {

			char *s = getp(bl->lastn);

			if (s == NULL) /* no target */
				break;
			b->nextl = NULL; /* dead-end */
			return (s);
		}
	b->nextl = bl; /* re-aim b at final target */
	pf = bl->firstn;
	if (islabel(pf) && !ishl(pf))
		return (pf->ops[0]);
	p = Saveop(0, newlab(), 0, GHOST); /* make node but don't link */
	p->forw = pf; /* link at beginning of this block */
	p->back = pf->back;
#if EH_SUP
	p->in_try_block = pf->in_try_block;
#endif
	if (bl->marked) /* this block already processed by reord */
		pf->back->forw = p;
	bl->firstn = pf->back = p;
	setlab(p);
	return (p->ops[0]);
}

	static void
rmunrch(preserve) boolean preserve; { /* remove unreachable code */

	extern void reach();
    register BLOCK *b, *prevb = NULL;
	REF *r;

#ifdef FLIRT
	static int numexec=0;
#endif

	TRACE("rmunrch");
    COND_RETURN("rmunrch");

#ifdef FLIRT
	++numexec;
#endif

	if (b0.next == NULL)
		return;
	for (ALLB(b, b0.next))
		b->marked = 0;
	reach(b0.next); /* mark all blocks reachable from initial block */

    /* mark all blocks reachable from non-text references */

    for (r = r0.nextref; r != NULL; r = r->nextref)
        if ((b = findlbl(r->lab)) != NULL && !b->marked)
            reach(b);

    /* mark all blocks reachable from hard-label blocks */

    for (ALLB(b, b0.next)) if (!b->marked )
    {
        NODE *p;
        for(p=b->firstn;islabel(p); p=p->forw)
        {
            if( p->opcode[0] != '.' ||  p->op == HLABEL || p->op == DHLABEL )
            {
                reach(b);
                break;
            }
        }
    }

	for (ALLB(b, b0.next)) /* remove unmarked blocks */
		if (b->marked)
			prevb = b;
		else {
			ndisc += b->length;
#if 0
			if (ISUNCBL(b) && islabel(b->firstn))
				nsave++;
			else {
				if (0)
					PRINTF("%cunreachable block %d removed\n",
					    CC, b->index);
				nunr += b->length;
			}
#endif
			if (preserve) { /* node sequence must be preserved */
			    b->firstn->back->forw = b->lastn->forw;
			    b->lastn->forw->back = b->firstn->back;
				FLiRT(O_RMUNRCH,numexec);
			}
			prevb->next = b->next;
			FLiRT(O_RMUNRCH,numexec);
		}
}


#define DONE	1
#define RIGHT	2
#define LEFT	3

	static void
reach(bcur)
register BLOCK * bcur;
{
	register BLOCK *bback, *tmp = NULL;

	bback = bcur;

	while(bcur->marked != DONE) {

		bcur->marked = LEFT;
		/*
	 	* Link around the second of successive removable branches 
		* with same op-codes; multi-way branches (switches) must 
		* be identical in text.
	 	*/
	
    		while (tmp != bcur->nextl &&
			(tmp = bcur->nextl) != NULL && 
			!tmp->marked && tmp->length == 1 &&
	    		tmp->lastn->op == bcur->lastn->op && 
				ISREMBR(tmp->lastn)) {

	    		   if(!(isuncbr(tmp->lastn) || 
		     	   tmp->nextr != NULL || same_inst(tmp->lastn, bcur->lastn)))
				   break;
			   bcur->nextl = tmp->nextl;
		}
	
		if(tmp != NULL && !tmp->marked) {
			bcur->nextl = bback;
			bback = bcur;
			bcur = tmp;
			continue;
		}

		if((tmp = bcur->nextr) != NULL && !tmp->marked) {
			bcur->nextr = bback;
			bback = bcur;
			bcur->marked = RIGHT;
			bcur = tmp;
			continue;
		}

		bcur->marked = DONE;
		tmp = bcur;
		bcur = bback;

		switch(bcur->marked) {
			
			case LEFT:	bback = bcur->nextl;
					bcur->nextl = tmp;
					break;

			case RIGHT:	bback = bcur->nextr;
					bcur->nextr = tmp;

		}
	}
}

/* it eliminates jumps such as shown below correctly .
    .L
        jmp .L1    ====>    .L
    .L1                     .L1
        jmp .L2             .L2
    .L2                         jmp .L1
        jmp .L1

 */
static void
rmbrs0() { /* remove redundant branches */

    register BLOCK *b, *bi;

	COND_RETURN("rmbrs0");
    for (b=b0.next ; b != NULL; )
    {
        if( ISREMBL(b) && islabel(b->firstn) && b->nextl == b->next )
        {
            bi = b->next;
            modrefs( b->lastn->back, b, bi);
            b = bi;
        }
        else b = b->next;
    }
}


static void
rmbrs() { /* remove redundant branches */

	register BLOCK *b, *bl;
	register NODE *p;

#ifdef FLIRT
	static int numexec=0;
#endif

	TRACE("rmbrs");
    COND_RETURN("rmbrs");

#ifdef FLIRT
	++numexec;
#endif

	for (ALLB(b, b0.next))
		if ((bl = b->nextl) != NULL &&
		    (p = b->lastn)->forw == bl->firstn && ISREMBR(p)) {

			/* delete unconditional branch ahead of target */

			if (isuncbr(p)) {
				RMBR(p);
				FLiRT(O_RMBRS,numexec);
				continue;
			}

			/* delete conditional branch ahead of target
			   or ahead of unconditional branch to same target */

			do {
				if (b->nextr == bl || b->nextr == NULL &&
				    bl->nextl == NULL && ISUNCBL(bl) &&
				    sameaddr(p, bl->lastn)) {
					FLiRT(O_RMBRS,numexec);
					RMBR(p);
					break;
				}
			} while (    ISUNCBL(bl)
				 &&  bl != bl->nextl	/* avoid self-loop */
				 && (bl = bl->nextl) != NULL);
		}
}

void optim_rmbrs() {
	bldgr(false);
	rmbrs();
}


char *
c2s(int c) 
{
	switch (c) {
		case CMP_PTR_ERR: return "CMP_PTR_ERR";
		case GUARD_RET: return "GUARD_RET";
		case CMP_FUNC_CONST: return "CMP_FUNC_CONST";
		case GUARD_CALL: return "GUARD_CALL";
		case GUARD_LOOP: return "GUARD_LOOP";
		default: return "GARBAGE";
	}
	/* NOTREACHED */
}

boolean
block_has_call(BLOCK *b)
{
NODE *p;
	for (p = b->firstn; p != b->lastn->forw; p = p->forw)
		if (iscall(p)) return true;
	return false;
}

boolean
fall2call(BLOCK *b)
{
	b->marked = 1;
	if (block_has_call(b)) return true;
	if (b->nextr == NULL) {
		if (b->nextl == NULL) return false; /*for switch tables*/
		if (b->nextl->marked == 1) return false; /*protect against infinite loop*/
		return fall2call(b->nextl);
	}
	return false;
}

boolean
fall2return(BLOCK *b)
{
	b->marked = 1;
	if (isret(b->lastn)) return true;
	if (b->nextr == NULL) {
		if (b->nextl == NULL) return false; /*for switch tables*/
		if (b->nextl->marked == 1) return false; /*protect against infinite loop*/
		return fall2return(b->nextl);
	}
	return false;
}

static void
clear_block_marks()
{
BLOCK *b;
	for (b = b0.next; b; b = b->next) {
		b->marked = 0;
	}
}

static void
improve_marks()
{
BLOCK *b;
extern struct regals *first_label ;
boolean b1,b2,c1,c2;
	for (b = b0.next; b ; b = b->next) {
		if (iscbr(b->lastn)) {
			COND_SKIP(continue,"improve %d %s",second_idx,b->lastn->op1,0);
			clear_block_marks();
			b1 = fall2return(b->nextl);
			clear_block_marks();
			b2 = fall2return(b->nextr);
			clear_block_marks();
			c1 = fall2call(b->nextl);
			clear_block_marks();
			c2 = fall2call(b->nextr);
			if (b1 && !b2) {
				b->lastn->dependents = GUARD_RET;
			} else if (b2 && !b1) {
				if ((b->lastn->dependents == CMP_FUNC_CONST
					|| b->lastn->dependents == CMP_PTR_ERR) && (c1 && !c2))
					/* EMPTY */ ;
				else b->lastn->dependents = -GUARD_RET;
			}
			if (b->lastn->dependents == 0 && c1 && !c2)
				b->lastn->dependents = GUARD_CALL;
			if (lookup_regals(b->lastn->op1,first_label)) {
				b->lastn->dependents = 0;
			}
			if (b->lastn->dependents && (b->index < b->nextr->index)
                && !lookup_regals(b->nextr->firstn->opcode,first_label)) {
                BLOCK *c;
                for (c = b->next; c&& c->index < b->nextr->index; c = c->next) {
                    if (lookup_regals(c->firstn->opcode,first_label)) {
                        b->lastn->dependents = 0;
                        break;
                    }
                }
            }
			if (b->lastn->dependents == CMP_FUNC_CONST
				&& is_short_fwd_branch(b))
				b->lastn->dependents = 0;
#if EH_SUP
			/*disable reversing branches that are go outside of a try block*/
			if (b->lastn->in_try_block != b->nextl->lastn->in_try_block
				|| b->lastn->in_try_block != b->nextr->lastn->in_try_block) {
				b->lastn->dependents = 0;
			}
#endif
		}/*endif iscbr*/
	}
}/*end improve marks*/

boolean
is_short_fwd_branch(BLOCK *b)
{
	if (!iscbr(b->lastn)) return false;
	return (b->next == b->nextl && b->next->next == b->nextr
		&& b->next->nextr == NULL);
}

void
reverse_branches()
{
NODE *p,*pnew;
NODE *target = &ntail;
BLOCK *b,*fallthru,*taken;
extern NODE *prepend();
extern NODE *insert();
extern NODE *find_try_block_end();
NODE *q1,*q2;
boolean changed = true;
int added_jumps;
	COND_RETURN("reverse_branches");
	bldgr(false);
	improve_marks();
	/*cant just loop in the blocks. Whenever a change is made,
	**flow graph needs to be rebuilt.
	*/
	while (changed) {
		changed = false;
		for (b = b0.next; b; b = b->next) {
			if (!(b->nextr) || b->nextr->index <= b->index) continue; /*fwd branch*/ /* ??? this is branch backward in the code sequence ??? jlw */
			p = b->lastn;
			if (!iscbr(p) || p->dependents <= 0) continue;
			/*fprintf(stderr,"look at "); fprinst(p); */
			if (p->dependents == CMP_PTR_ERR && !reverse_ptr_err(p)) {
				p->dependents = -(p->dependents);
				/*fprintf(stderr,"dont reverse ptr err "); */
				/*fprinst(p); */
				continue;
			}
			if (p->dependents == CMP_FUNC_CONST && !reverse_func_const(p)) {
				p->dependents = -(p->dependents);
				continue;
			}
			fallthru = b->next;
			taken = b->nextr;
#if 0
			if (p->dependents == GUARD_RET)
				if ((!isret(fallthru->lastn) && !isret(fallthru->next->lastn))
					|| isret(taken->lastn)
					|| (!isbr(taken->lastn) && isret(taken->next->lastn)))
				{
				continue;
				}
#endif
			COND_SKIP(continue,"%d %s",second_idx,p->op1,0);
			/*fprintf(stderr,"reverse br %s ",c2s(p->dependents)); fprinst(p); */
			if (fallthru == taken) continue;
			added_jumps = 0;
			if (!isuncbr(fallthru->lastn) && !isuncbr(taken->lastn)
				&& !isuncbr(taken->prev->lastn)) {
				p->dependents = 0;
				continue;
			}
#if EH_SUP
			/*If the branch is in a C++ try block, then fallthru does not
			**move to the end of the funciton but to the end of this try block.
			**Find this place. If it does not end with a jump then add a jump
			**to the next instruction. Do it before any change to the code
			**so that this can be suppressed if it turns out to add too many
			**jumps.
			*/
			if (p->in_try_block) {
				target = find_try_block_end(p);
				if (!is_any_uncbr(target->back)) {
					if (!is_any_label(target)) {
						pnew = prepend(target,NULL);
						pnew->op = LABEL;
						pnew->opcode = newlab();
						pnew->op1 = pnew->op2 = NULL;
						pnew->uses = pnew->sets = 0;
						target = pnew;
					}
					/* target is outside this particular
					   try block; JMP should be part of 
					   the try block. */
					pnew = insert(target->back);
					chgop(pnew,JMP,"jmp");
					pnew->op1 = target->opcode;
					added_jumps ++;
					pnew->op2 = NULL;
					pnew->uses = pnew->sets = 0;
				}
			}
#endif
			if (!islabel(fallthru->firstn)) {
				/* add a label to the beginning of the fallthru block
				** to enable the added branch to it.
				*/
				pnew = prepend(fallthru->firstn,NULL);
				fallthru->firstn = pnew;
				pnew->opcode = newlab();
				pnew->op = LABEL;
				new_sets_uses(pnew);
			}
			if (!isuncbr(fallthru->lastn)) {
				/*make it jump to it's previous fallthru*/
				/*fprintf(stderr,"fallthru->lastn was "); fprinst(fallthru->lastn); */
				pnew = insert(fallthru->lastn);
				pnew->op1 = newlab();
				chgop(pnew,JMP,"jmp");
				added_jumps ++;
				fallthru->lastn = pnew;
				/*fprintf(stderr,"move fallthru->lastn to ");*/
				/*fprinst(fallthru->lastn);*/
				/*fprintf(stderr,"it's back is "); fprinst(fallthru->lastn->back); */
				new_sets_uses(pnew);
				pnew = prepend(pnew->forw,NULL);
				pnew->op = LABEL;
				pnew->opcode = pnew->back->op1;
				new_sets_uses(pnew);
				fallthru->next->firstn = pnew;
			}
			if (!isuncbr(taken->lastn)) {
				/*make it jump to it's previous fallthru*/
				pnew = insert(taken->lastn);
				taken->lastn = pnew;
				chgop(pnew,JMP,"jmp");
				pnew->op1 = newlab();
				new_sets_uses(pnew);
				added_jumps++;
				pnew = prepend(pnew->forw,NULL);
				pnew->op = LABEL;
				pnew->opcode = pnew->back->op1;
				taken->next->firstn = pnew;
				new_sets_uses(pnew);
			}
			if (!isuncbr(taken->prev->lastn)) {
				pnew = insert(taken->prev->lastn);
				chgop(pnew,JMP,"jmp");
				pnew->op1 = taken->firstn->opcode;
				new_sets_uses(pnew);
				taken->prev->lastn = pnew;
				added_jumps++;
			}
			q1 = taken->firstn->back;
			q2 = taken->lastn->forw;
			q1->forw = q2;
			q2->back = q1;
	
			taken->lastn->forw = fallthru->lastn->forw;
			fallthru->lastn->forw->back = taken->lastn;
	
			target->back->forw = fallthru->firstn;
			fallthru->firstn->back = target->back;
	
			fallthru->lastn->forw = target;
			target->back = fallthru->lastn;

			p->forw = taken->firstn;
			taken->firstn->back = p;

			revbr(p);
			p->op1 = fallthru->firstn->opcode;
			if (added_jumps >= 2) p->dependents = 0;
			else p->dependents = -(p->dependents);

			bldgr(false);
			changed = true;
			break;
		}
	}
	ldanal();
}

#if 0
/* ldanal -- perform live/dead analysis over flow graph
**
** This routine calculates live/dead register information for the
** entire flow graph in these steps:
**
**  1.	Allocate temporary array to hold block-level live/dead info.
**	Initialize it.
**  2.	On a block-wise basis, determine registers set and used by
**	each instruction.  Determine registers used and set by the
**	block.
**  3.	Propagate register use/set information throughout the flow
**	graph blocks.
**  4.	Propagate final information back through each block to
**	reflect correct live/dead information for each instruction.
**
**  We use the live/dead algorithm described in the Aho and Ullman
**  "dragon book".
*/

/*  Some physical regsiters can hold two variables simultaneously: for example,
    one variable may be assigned to %ah and another variable may be assigned to
    %al. A given physical register can have 3 separate live-dead bits. For
    %eax, the Eax bit is used for %eax or %ax; the AH bit is used for %ah; the
    AL bit is used for %al

    Assigning to %ah kills Eax and AH but does not kill AL. Assigning to %eax
    or %ax kills AH, AL and Eax.
*/

	void
try_ldanal() { /* perform live-dead register analysis */ /* under if 0 */

    typedef unsigned int LDREG;
    register BLOCK * b;			/* pointer to current block */
    register NODE * p;			/* pointer to current inst. node */
	REF *r;
	SWITCH_TBL *sw;
    struct ldinfo			/* temporary block-level structure */
    {
		LDREG buses;			/* registers used by block */
		LDREG bdef;			/* registers defined (set) by block */
		LDREG bin;			/* registers live coming into block */
		LDREG bout;			/* registers live exiting block */
    };
    struct ldinfo * lddata;		/* array of data for each block */
    register struct ldinfo * ldptr;	/* pointer to one of the above */
    boolean changed;
    unsigned liveregs = LIVEREGS;	/* Changed to a variable to
					   accomodate PIC code. */

    TRACE("ldanal");

    bldgr(false);
	set_refs_to_blocks(); /* connect switch tables  nanes to blocks */
    lddata = NEWBLK(idx + 1, struct ldinfo);

/* Initialize:  set the recently allocated array to zero.  The idea, here,
** is that each entry in the array corresponds to one block in the flow
** graph.  We assume that blocks have sequential index numbers and that
** idx is the last index number.
*/

    CLEAR(lddata, (idx + 1) * sizeof(struct ldinfo));
  	ldptr = lddata + b0.next->index;
  	ldptr->buses = SAVEDREGS;

/* Step 2.  Calculate uses/def for each node and for the containing block. */

	for (ALLB(b,b0.next)) {
		ldptr = lddata + b->index;
		for (p = b->lastn; p != b->firstn->back; p = p->back) {
			p->nlive = p->uses | liveregs; /* what's used here, + always live */
			if (isret(p))
				p->nlive |= SAVEDREGS;
			p->ndead = p->sets & ~p->nlive; /* what's set, but not used, here */
			ldptr->buses = (p->nlive | (ldptr->buses & ~p->ndead)) & REGS;
			/* current live registers */
			ldptr->bdef = (p->ndead | (ldptr->bdef & ~p->nlive)) & REGS;
			/* current registers killed by block */
		}
	}

/* Propagate live/dead data throughout the flow graph, using Aho and
** Ullman algorithm.
*/

    do
    {
	changed = false;		/* will continue until no changes */

	for (ALLB(b,b0.next))
	{
	    LDREG in, out;

	    if (b->nextr == NULL && (b->nextl == NULL ||
			(is_any_br(b->lastn) && !is_any_uncbr(b->lastn))))
	    {
	    /* This case represents a return, or an unconditional indexed
	    ** jump, or a switch.  If we had better connectivity in the
	    ** flow graph, we could trace all successors correctly.  As
	    ** things are, we have to assume the worst about what registers
	    ** are live going into the next block.  For a return, this means
	    ** those registers that can be used to return a value.  For
	    ** others, we mark all registers live.
	    */
			if (is_jmp_ind(b->lastn) &&
				((sw = get_base_label(b->lastn->op1)) != 0)) {
				out = 0;		/* registers out of current block. */
				for (r = sw->first_ref; r; r = r->nextref) {
					if (sw->switch_table_name == r->switch_table_name)
		    			out |= lddata[r->switch_entry->index].bin;
		    		if ( r == sw->last_ref )
						break;
		    	}
  			} else
		  		out = isret(b->lastn) ? (RETREG | SAVEDREGS) : REGS;
	    }
	    else
	    {
		/* OUT = union (of successors) IN */
		out = 0;		/* registers out of current block. */
		if (b->nextr != NULL)
		    out |= lddata[b->nextr->index].bin;
		if (b->nextl != NULL)
		    out |= lddata[b->nextl->index].bin;
	    }

	    ldptr = lddata + b->index;	/* point at data for current block */
	    /* IN = OUT - DEF u USE */
	    in = (out & ~ldptr->bdef) | ldptr->buses;

	    /* see what changed */

	    if (in != ldptr->bin || out != ldptr->bout)
	    {
		changed = true;
		ldptr->bin = in;	/* set changed values */
		ldptr->bout = out;
	    }
	} /* end for */
    } while (changed);

/* Now set the final live/dead (really, just live) information in
** each node of each block.
*/

	for (ALLB(b,b0.next)) {
		/* go backward again through each block */
		/* initial live is outgoing regs of block */

		LDREG live = lddata[b->index].bout;
		for (p = b->lastn; p != b->firstn->back; p = p->back) {
			LDREG newlive = (p->nlive | (live & ~p->ndead)) & REGS;
			p->nlive = live;		/* live for this node is what was
									** live going into successor
									*/
			live = newlive;		/* live for next node is whatever
								** else we used, but didn't kill
								*/
		}
	}

    xfree((char *) lddata);		/* free up temp. storage */
}
#endif

	void
ldanal() { /* perform live-dead register analysis */

    typedef unsigned int LDREG;
    register BLOCK * b;			/* pointer to current block */
    register NODE * p;			/* pointer to current inst. node */
	REF *r;
	SWITCH_TBL *sw;
    struct ldinfo			/* temporary block-level structure */
    {
		LDREG buses;			/* registers used by block */
		LDREG bdef;			/* registers defined (set) by block */
		LDREG bin;			/* registers live coming into block */
		LDREG bout;			/* registers live exiting block */
    };
    struct ldinfo * lddata;		/* array of data for each block */
    register struct ldinfo * ldptr;	/* pointer to one of the above */
    boolean changed;
    unsigned liveregs = LIVEREGS;	/* Changed to a variable to
					   accomodate PIC code. */

    TRACE("ldanal");

    bldgr(false);
	set_refs_to_blocks(); /* connect switch tables  nanes to blocks */
    lddata = NEWBLK(idx + 1, struct ldinfo);

/* Initialize:  set the recently allocated array to zero.  The idea, here,
** is that each entry in the array corresponds to one block in the flow
** graph.  We assume that blocks have sequential index numbers and that
** idx is the last index number.
*/

    CLEAR(lddata, (idx + 1) * sizeof(struct ldinfo));
  	ldptr = lddata + b0.next->index;
  	ldptr->buses = SAVEDREGS;

/* Step 2.  Calculate uses/def for each node and for the containing block. */

	add_live_to_second_entry();
    for (ALLB(b,b0.next))
    {
	ldptr = lddata + b->index;
	for (p = b->lastn; !is_any_label(p); p = p->back)
	{
	    p->nlive = p->uses | liveregs; /* what's used here, + always live */
		if (isret(p))
			p->nlive |= SAVEDREGS;
	    p->ndead = p->sets & ~p->nlive; /* what's set, but not used, here */
	    ldptr->buses = (p->nlive | (ldptr->buses & ~p->ndead)) & REGS;
					/* current live registers */
	    ldptr->bdef = (p->ndead | (ldptr->bdef & ~p->nlive)) & REGS;
					/* current registers killed by block */

	    if (p == b->firstn)		/* stop if reached first node */
		break;
	}

    }

/* Propagate live/dead data throughout the flow graph, using Aho and
** Ullman algorithm.
*/

    do
    {
	changed = false;		/* will continue until no changes */

	for (ALLB(b,b0.next))
	{
	    LDREG in, out;

	    if (b->nextr == NULL && (b->nextl == NULL ||
			(is_any_br(b->lastn) && !is_any_uncbr(b->lastn))))
	    {
	    /* This case represents a return, or an unconditional indexed
	    ** jump, or a switch.  If we had better connectivity in the
	    ** flow graph, we could trace all successors correctly.  As
	    ** things are, we have to assume the worst about what registers
	    ** are live going into the next block.  For a return, this means
	    ** those registers that can be used to return a value.  For
	    ** others, we mark all registers live.
	    */
			if (is_jmp_ind(b->lastn) &&
				((sw = get_base_label(b->lastn->op1)) != 0)) {
				out = 0;		/* registers out of current block. */
				for (r = sw->first_ref; r; r = r->nextref) {
					if (sw->switch_table_name == r->switch_table_name)
		    			out |= lddata[r->switch_entry->index].bin;
		    		if ( r == sw->last_ref )
						break;
		    	}
  			} else
		  		out = isret(b->lastn) ? (RETREG | SAVEDREGS) : REGS;
	    }
	    else
	    {
		/* OUT = union (of successors) IN */
		out = 0;		/* registers out of current block. */
		if (b->nextr != NULL)
		    out |= lddata[b->nextr->index].bin;
		if (b->nextl != NULL)
		    out |= lddata[b->nextl->index].bin;
	    }

	    ldptr = lddata + b->index;	/* point at data for current block */
	    /* IN = OUT - DEF u USE */
	    in = (out & ~ldptr->bdef) | ldptr->buses;

	    /* see what changed */

	    if (in != ldptr->bin || out != ldptr->bout)
	    {
		changed = true;
		ldptr->bin = in;	/* set changed values */
		ldptr->bout = out;
	    }
	} /* end for */
    } while (changed);

/* Now set the final live/dead (really, just live) information in
** each node of each block.
*/

    for (ALLB(b,b0.next))
    {
	/* go backward again through each block */
	/* initial live is outgoing regs of block */

	LDREG live = lddata[b->index].bout;
	for (p = b->lastn; !is_any_label(p); p = p->back)
	{
	    LDREG newlive = (p->nlive | (live & ~p->ndead)) & REGS;
	    p->nlive = live;		/* live for this node is what was
					** live going into successor
					*/
	    live = newlive;		/* live for next node is whatever
					** else we used, but didn't kill
					*/
	    if (p == b->firstn)
		break;			/* quit if first node in block */
	}
    }
	dehack_second_entry();

    xfree((char *) lddata);		/* free up temp. storage */
}
void
regal_ldanal() { /* perform live-dead /REGAL analysis */

    typedef unsigned int LDREG;
    register BLOCK * b;			/* pointer to current block */
    register NODE * p;			/* pointer to current inst. node */
	register REF *r;
	SWITCH_TBL *sw;
	void set_regal_bits();
    struct ldinfo			/* temporary block-level structure */
    {
	LDREG buses;			/* /REGALs used by block */
	LDREG bdef;			/* /REGALs defined (set) by block */
	LDREG bin;			/* /REGALs live coming into block */
	LDREG bout;			/* /REGALs live exiting block */
    };
	LDREG used_first, use_first();			/* /REGALs used befors set in some ebb */
    struct ldinfo * lddata;		/* array of data for each block */
    register struct ldinfo * ldptr;	/* pointer to one of the above */
    boolean changed;

	set_refs_to_blocks(); /* connect switch tables  nanes to blocks */
    lddata = NEWBLK(idx + 1, struct ldinfo);

/* Initialize:  set the recently allocated array to zero.  The idea, here,
** is that each entry in the array corresponds to one block in the flow
** graph.  We assume that blocks have sequential index numbers and that
** idx is the last index number.
*/

    CLEAR(lddata, (idx + 1) * sizeof(struct ldinfo));

/* Step 2.  Calculate uses/def for each node and for the containing block. */

    for (ALLB(b,b0.next))
    {
	ldptr = lddata + b->index;
	for (p = b->lastn; !islabel(p); p = p->back)
	{
		set_regal_bits(p); /* p->nrlive is what's used here  */
		                   /* p->nrdead is what's set, but not used, here */
		ldptr->buses = (p->nrlive | (ldptr->buses & ~p->nrdead));
					/* current live /REGALs */
	    ldptr->bdef = (p->nrdead | (ldptr->bdef & ~p->nrlive));
					/* current /REGALs killed by block */

	    if (p == b->firstn)		/* stop if reached first node */
		break;
	}

    }
	used_first = use_first();
/* Propagate live/dead data throughout the flow graph, using Aho and
** Ullman algorithm.
*/

    do
    {
	changed = false;		/* will continue until no changes */

	for (ALLB(b,b0.next))
	{
	    LDREG in, out;

	    if (b->nextr == NULL && (b->nextl == NULL ||
			(isbr(b->lastn) && !isuncbr(b->lastn))))
	    {
	    /* This case represents a return, or an unconditional indexed
	    ** jump, or a switch.  If we had better connectivity in the
	    ** flow graph, we could trace all successors correctly.  As
	    ** things are, we have to assume the worst about what /REGALs
	    ** are live going into the next block.  For a return, this means
	    ** those /REGALs that can be used to return a value.  For
	    ** others, we mark all /REGALs live.
	    */

			if (is_jmp_ind(b->lastn) &&
				((sw = get_base_label(b->lastn->op1)) != 0)) {
				out = 0;		/* registers out of current block. */
				for (r = sw->first_ref; r; r = r->nextref) {
					if (sw->switch_table_name == r->switch_table_name)
		    			out |= lddata[r->switch_entry->index].bin;
		    		if ( r == sw->last_ref )
						break;
		    	}
  			} else
				out = isret(b->lastn) ? 0 : used_first;
	    }
	    else
	    {
		/* OUT = union (of successors) IN */
		out = 0;		/* /REGALs out of current block. */
		if (b->nextr != NULL)
		    out |= lddata[b->nextr->index].bin;
		if (b->nextl != NULL)
		    out |= lddata[b->nextl->index].bin;
	    }

	    ldptr = lddata + b->index;	/* point at data for current block */
	    /* IN = OUT - DEF u USE */
	    in = (out & ~ldptr->bdef) | ldptr->buses;

	    /* see what changed */

	    if (in != ldptr->bin || out != ldptr->bout)
	    {
		changed = true;
		ldptr->bin = in;	/* set changed values */
		ldptr->bout = out;
	    }
	} /* end for */
    } while (changed);

/* Now set the final live/dead (really, just live) information in
** each node of each block.
*/

    for (ALLB(b,b0.next))
    {
	/* go backward again through each block */
	/* initial live is outgoing regs of block */

	LDREG live = lddata[b->index].bout;
	for (p = b->lastn; !islabel(p); p = p->back)
	{
	    LDREG newlive = (p->nrlive | (live & ~p->nrdead));
	    p->nrlive = live;		/* live for this node is what was
					** live going into successor
					*/
	    live = newlive;		/* live for next node is whatever
					** else we used, but didn't kill
					*/
	    if (p == b->firstn)
		break;			/* quit if first node in block */
	}
    }

    xfree((char *) lddata);		/* free up temp. storage */
}


static NODE *pf;	/* pointer to first window node */
static NODE *opf;	/* pointer to predecessor of first window node */
static int wsize;	/* window size for peephole trace */

	void
window(size, func) register int size; boolean (*func)(); { /* peephole scan */

	extern NODE *initw();
	register NODE *pl;
	register int i;

	TRACE("window");

	/* find first window */

	wsize = size;
	if ((pl = initw(n0.forw)) == NULL)
		return;

	/* move window through code */

	for (opf = pf->back; ; opf = pf->back) {
		COND_SKIP(goto advance,"w%d %d",size,second_idx,0);
		if ((*func)(pf, pl) == true) {
#ifdef DEBUG
		if (start && last_func() && last_one()) {
			fprintf(stderr,"+ ");
		}
			if (0)
				if (opf->forw == pl->forw)
					fprintf(stderr,"%cdeleted\n", CC);
				else {
					fprintf(stderr,"%cchanged to:\n", CC);
					prwindow(opf->forw, size);
				}
#endif
			if (size > 1) {

				/* move window back in case
				   there is an overlapping improvement */

				for (i = 2; i <= size; i++)
					if ((opf = opf->back) == &n0) {
						opf = n0.forw;
						break;
					}
				if ((pl = initw(opf)) == NULL)
					return;
				continue;
			}
		}
#ifdef DEBUG
		else if (start && last_func() && last_one()) {
			fprintf(stderr,"- ");
		}
#endif

		/* move window ahead */
#ifdef DEBUG
		advance:
#endif

		if ((pl = pl->forw) == &ntail)
			return;
		pf = pf->forw;
		if (islabel(pl) && (pl = initw(pl->forw)) == NULL)
			return;
	}
}

	static NODE *
initw(p) register NODE *p; { /* find first available window */

	register int i;

	if ((pf = p) == NULL)
		return (NULL);

	/* move p down until window is large enough */

	for (i = 1; i <= wsize; i++) {
		if (p == &ntail) /* no more windows */
			return (NULL);
		if (islabel(p)) { /* restart scan */
			pf = p->forw;
			i = 0;
		}
		p = p->forw;
	}
	return p->back;
}

#ifdef DEBUG
	void
wchange() { /* print window before change */

	if (0) {
		PRINTF("%cwindow:\n", CC);
		prwindow(opf->forw, wsize);
	}
}
#endif

	static void
prwindow(p, size) /* print "size" instructions starting at p */
	register NODE *p; register int size; {

	for ( ; --size >= 0 && p != &ntail && !islabel(p); p = p->forw) {
		/*PUTCHAR(CC);*/
		/*PRINTF("(live: 0x%X)", p->nlive);*/
		fprintf(stderr,"w:");
		fprinst(p);
	}
}

	static void
mkltbl() { /* make label table with only definitions */

	register BLOCK *b;
	register NODE *p;

	clrltbl();

	/* add definitions from labels in text section */

	for (ALLB(b, b0.next))
		for (p = b->firstn; islabel(p); p = p->forw)
			addlbl(p->ops[0], b);
}

	static void
clrltbl() 		 /* Clear label table.	*/
{extern int Numlbls;	/* Number of labels encountered */
 register LBL **p, *q, *r;
 for (p = &Lbltbl[0]; p <= &Lbltbl[LBLTBLSZ-1]; p++)
	for (q = *p, r = NULL; q != NULL; q = r)
		{r = q->next;
		 xfree((char *)q);
		}

 CLEAR(&Lbltbl[0], LBLTBLSZ * sizeof(LBL *));
 Numlbls = 0;
 return;
}

#define	HASH(hp,l) {\
	register int lh = 0, c;\
	register char *ll = (l);\
	while ((c = *ll++) != '\0')\
		lh += c;\
	(hp) = &Lbltbl[(unsigned int)lh % LBLTBLSZ];\
	}

	static void
addlbl(l,b)			/* Add label in label table.	*/
register char *l;		/* Char pointer to label.	*/
BLOCK *b;			/* Block this label is in.	*/

{extern int Numlbls;		/* Number of labels encountered.	*/
 register LBL *p;
 LBL **hp;

 HASH(hp,l);
 for(p = *hp; p != NULL; p = p->next)
   {
	/*
	 * The precheck on the third character avoids many superfluous
	 * calls to strcmp when the hash table is fairly full.  The third
	 * character is chosen because most compilers generate labels
	 * with an invariant two-character prefix.  In a later version,
	 * perhaps labels should be converted at the machine-dependent
	 * level into unique integers instead of being stored as strings.
	 */
     if(p->cp[2] == l[2] && strcmp(p->cp, l) == 0 )	/* if we found it */
       {p->bl = b;	/* install the new block it's in */
	return;
      }
   }

 /* didn't find it */
 p = NEWBLK(1,LBL);
 p->next = *hp;			/* insert new LBL at the */
 p->cp = l;			/* Put in pointer to label. */
 *hp = p;			/* beginning of the hash chain */
 Numlbls++;			/* increment total # labels */
 p->bl = b;			/* install the new block it's in */
 return;
}
	static BLOCK *
findlbl(l)			/* Find label in label table and */
				/* return the block that it's in. */
register char *l;		/* Char pointer to label.	*/
{
 register LBL *p;
 LBL **hp;

 HASH(hp,l);
 for(p = *hp; p != NULL; p = p->next)
   {
	/*
	 * The precheck on the third character avoids many superfluous
	 * calls to strcmp when the hash table is fairly full.  The third
	 * character is chosen because most compilers generate labels
	 * with an invariant two-character prefix.  In a later version,
	 * perhaps labels should be converted at the machine-dependent
	 * level into unique integers instead of being stored as strings.
	 */
     if(p->cp[2] == l[2] && strcmp(p->cp, l) == 0 )	/* if we found it */
       return(p->bl);	/* return the block it's in. */
   }
 return(NULL);			/* didn't find it */
}
#undef HASH

	char *
getspace(n) register unsigned n; { /* return a pointer to "n" bytes */

    register char *p = Lasta;
    
    /* round up so pointers are always word-aligned */
    /* int conversions are to avoid call to software remaindering */
    
    n += sizeof(char *) - ((int) n % (int) sizeof(char *));
    Maxu += n;
    while ((Lasta += n) >= Lastx) {
	unsigned k, j;
	for(k = NSPACE; k*sizeof(char *)<n; k += k);
	j = sizeof(struct space_t) + (k - NSPACE)*sizeof(char *);
	*Space = (struct space_t *)xalloc(j);
						
	p = Lasta = (char *) &(*Space)->space[0];
	Lastx = (char *) &(*Space)->space[k - 1];
	(*Space)->next = NULL;

	/* Enter actual size of the struct */
	(*Space)->size_space_t = j; 

	Space = &(*Space)->next; /* Point at end of queue */
    }
    return (p);
}
/* Branch shortening
**
** This code shortens span-dependent branches with assistance from
** machine dependent routines.  The interface is as follows:
**
**	bspan(flag)	is the entry point available to machine-
**			dependent routines; the flag is true to print
**			debugging information
**
**	BSHORTEN	symbol, defined in "defs.h"; enables all this
**
**	int instsize(node)
**			routine or macro; returns upper bound on size of
**			instruction in node in arbitrary units
**	void bshorten(node,dist)
**			routine or macro; changes op at node to be
**			shortened version of branch, based on (long)
**			distance (dist) between branch and target
**
** The algorithm proceeds in two passes over the blocks of the program.
** The first pass calculates the relative PC (program counter) value for
** the beginning of each block.  (Remember, labels are always at the
** beginning of a block.)  Since branches are always at the end of blocks,
** the assumption is that the machine's program counter register always
** points to the beginning of the block following the branch when the
** branch is executed.  (This assumption for the purpose of calculating
** distances.)  Branches are assumed to be shortenable both forward and
** backward.
**
** The PC values for the blocks are kept in a dynamically allocated
** array.  The array is size idx+2, where idx is the highest block
** number.  The +2 accounts for not using array[0] (we index into
** the array by block index numbers which are non-zero) and for one
** additional entry at the end to contain the PC just after the
** last block.
**
** Pass two calculates distances between branches and their targets
** and shortens branches which can be shortened.
*/


#ifdef DEBUG

/*
**	Functions to print block data: call from sdb.
*/

void
dprblock(b)
BLOCK *b;
{
	NODE *p;
      	FILE save;
      	save = *stdout;
      	*stdout = *stderr;
	fprintf(stderr,
		"block: %d, n: %d l: %d r: %d indeg: %d ins_count: %d marked: %d\n",
b->index,((b->next)?(b->next->index):-1),
((b->nextl)?(b->nextl->index):-1),((b->nextr)?(b->nextr->index):-1),
b->indeg,b->length,b->marked);
	for(p=b->firstn;p;p=p->forw) {
		prinst(p);
		if(p == b->lastn) break;
	}
	*stdout = save;
	fflush(stderr);
}

void
dprblocks(s)
char *s;
{
	int count;
	BLOCK *b;
	fprintf(stderr, "%s\n",s);
	fprintf(stderr, "blocklist:\n");
	for(b = &b0, count = 0; b && count < 10; b = b->next, count++) {
		dprblock(b);
	}
}
#endif
