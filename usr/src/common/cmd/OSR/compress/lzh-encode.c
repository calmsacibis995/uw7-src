#ident	"@(#)OSRcmds:compress/lzh-encode.c	1.1"
#pragma comment(exestr, "@(#) lzh-encode.c 25.2 94/11/08 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1991-1994.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */
/*
 * lzh-encode.c
 *-----------------------------------------------------------------------------
 * From public domain code in zoo by Rahul Dhesi, who adapted it from "ar"
 * archiver written by Haruhiko Okumura.
 *=============================================================================
 */
/*
 * MODIFICATION HISTORY
 *	24 Apr 1992	sco!trulsm	S001
 *		- Fixed bug where origsize and compsize variables
 *		  were not being reset between files when compressing
 *		  multiple files.
 *		- Now initializing compsize to 2, which accounts for
 *		  the magic header of 2 bytes.
 *	31 Oct 1994	scol!anthonys	L002
 *		- Workaround to avoid some compiler warnings.
 */

#include "lzh-defs.h"

FILE *lzh_infile;
FILE *lzh_outfile;

#define PERC_FLAG ((unsigned) 0x8000)

/*
 * sliding dictionary with percolating update
 */

#define NIL        0
#define MAX_HASH_VAL (3 * DICSIZ + (DICSIZ / 512 + 1) * UCHAR_MAX)

typedef short node;

static uchar *text, *childcount;
static node pos, matchpos, avail,
	*position, *parent, *prev, *next = NULL;
static int remainder, matchlen;

#if MAXMATCH <= (UCHAR_MAX + 1)
     static uchar *level;
#    define T_LEVEL  uchar *
#else
     static ushort *level;
#    define T_LEVEL  ushort *
#endif

static void
allocate_memory()
{
	if (next != NULL) return;
	text = (uchar *) MemAlloc (DICSIZ * 2 + MAXMATCH);
	level = (T_LEVEL) MemAlloc ((DICSIZ + UCHAR_MAX + 1) * sizeof(*level));
	childcount = (uchar *) MemAlloc ((DICSIZ + UCHAR_MAX + 1) *
                                         sizeof(*childcount));
	position = (node *) MemAlloc ((DICSIZ + UCHAR_MAX + 1) * 
                                      sizeof(*position));
	parent = (node *) MemAlloc (DICSIZ * 2 * sizeof(*parent));
	prev = (node *) MemAlloc (DICSIZ * 2 * sizeof(*prev));
	next = (node *) MemAlloc ((MAX_HASH_VAL + 1) * sizeof(*next));
}

static void
init_slide ()
{
	node i;

	for (i = DICSIZ; i <= DICSIZ + UCHAR_MAX; i++) {
		level[i] = 1;
			position[i] = NIL;  /* sentinel */
	}
	for (i = DICSIZ; i < DICSIZ * 2; i++) parent[i] = NIL;
	avail = 1;
	for (i = 1; i < DICSIZ - 1; i++) next[i] = i + 1;
	next[DICSIZ - 1] = NIL;
	for (i = DICSIZ * 2; i <= MAX_HASH_VAL; i++) next[i] = NIL;
}

#define HASH(p, c) ((p) + ((c) << (DICBIT - 9)) + DICSIZ * 2)

/*
 * q's child for character c (NIL if not found)
 */
static node
child (node q,
       uchar c)
{
	node r;

	r = next[HASH(q, c)];
	parent[NIL] = q;  /* sentinel */
	while (parent[r] != q) r = next[r];
	return r;
}

/*
 * Let r be q's child for character c.
 */
static void
makechild (node q,
           uchar c,
           node r)
{
	node h, t;

	h = HASH(q, c);
	t = next[h];  next[h] = r;  next[r] = t;
	prev[t] = r;  prev[r] = h;
	parent[r] = q;  childcount[q]++;
}

/*
 *
 */
void
split (node old)
{
	node new, t;

	new = avail;  avail = next[new];  childcount[new] = 0;
	t = prev[old];  prev[new] = t;  next[t] = new;
	t = next[old];  next[new] = t;  prev[t] = new;
	parent[new] = parent[old];
	level[new] = matchlen;
	position[new] = pos;
	makechild(new, text[matchpos + matchlen], old);
	makechild(new, text[pos + matchlen], pos);
}

/*
 *
 */
static void
insert_node ()
{
	node q, r, j, t;
	uchar c, *t1, *t2;

	if (matchlen >= 4) {
		matchlen--;
		r = (matchpos + 1) | DICSIZ;
		while ((q = parent[r]) == NIL) r = next[r];
		while ((int)level[q] >= matchlen) {			/* L002 */
			r = q;  q = parent[q];
		}
			t = q;
			while (position[t] < 0) {
				position[t] = pos;  t = parent[t];
			}
			if (t < DICSIZ) position[t] = pos | PERC_FLAG;
	} else {
		q = text[pos] + DICSIZ;  c = text[pos + 1];
		if ((r = child(q, c)) == NIL) {
			makechild(q, c, pos);  matchlen = 1;
			return;
		}
		matchlen = 2;
	}
	for ( ; ; ) {
		if (r >= DICSIZ) {
			j = MAXMATCH;  matchpos = r;
		} else {
			j = level[r];
			matchpos = position[r] & ~PERC_FLAG;
		}
		if (matchpos >= pos) matchpos -= DICSIZ;
		t1 = &text[pos + matchlen];  t2 = &text[matchpos + matchlen];
		while (matchlen < j) {
			if (*t1 != *t2) {  split(r);  return;  }
			matchlen++;  t1++;  t2++;
		}
		if (matchlen >= MAXMATCH) break;
		position[r] = pos;
		q = r;
		if ((r = child(q, *t1)) == NIL) {
			makechild(q, *t1, pos);  return;
		}
		matchlen++;
	}
	t = prev[r];  prev[pos] = t;  next[t] = pos;
	t = next[r];  next[pos] = t;  prev[t] = pos;
	parent[pos] = q;  parent[r] = NIL;
	next[r] = pos;  /* special use of next[] */
}

/*
 *
 */
static void
delete_node ()
{
	node q, r, s, t, u;

	if (parent[pos] == NIL) return;
	r = prev[pos];  s = next[pos];
	next[r] = s;  prev[s] = r;
	r = parent[pos];  parent[pos] = NIL;
	if (r >= DICSIZ || --childcount[r] > 1) return;
		t = position[r] & ~PERC_FLAG;
	if (t >= pos) t -= DICSIZ;
		s = t;  q = parent[r];
		while ((u = position[q]) & PERC_FLAG) {
			u &= ~PERC_FLAG;  if (u >= pos) u -= DICSIZ;
			if (u > s) s = u;
			position[q] = (s | DICSIZ);  q = parent[q];
		}
		if (q < DICSIZ) {
			if (u >= pos) u -= DICSIZ;
			if (u > s) s = u;
			position[q] = s | DICSIZ | PERC_FLAG;
		}
	s = child(r, text[t + level[r]]);
	t = prev[s];  u = next[s];
	next[t] = u;  prev[u] = t;
	t = prev[r];  next[t] = s;  prev[s] = t;
	t = next[r];  prev[t] = s;  next[s] = t;
	parent[s] = parent[r];  parent[r] = NIL;
	next[r] = avail;  avail = r;
}

/*
 *
 */
static void
get_next_match ()
{
	int n;

	remainder--;
	if (++pos == DICSIZ * 2) {
		memmove ((char *) &text[0], (char *) &text[DICSIZ],
			 DICSIZ + MAXMATCH);
		n = fread_crc(&text[DICSIZ + MAXMATCH], DICSIZ, lzh_infile);
		remainder += n;  pos = DICSIZ;  
	}
	delete_node();  insert_node();
}

/*
 * Read from infile, compress, write to outfile.
 */
void
lzh_encode (FILE          *infile,
            FILE          *outfile,
            unsigned long *originalSize,
            unsigned long *compressSize)
{
	int lastmatchlen;
	node lastmatchpos;

	/* make input/output files visible to other functions */
	lzh_infile = infile;
	lzh_outfile = outfile;

	/* Initialize compsize and origsize variables.		S001 */
	compsize = 2;	/* Includes two byte magic header	S001 */
	origsize = 0;	/*					S001 */

	allocate_memory();  init_slide();  huf_encode_start();
	remainder = fread_crc(&text[DICSIZ], DICSIZ + MAXMATCH, lzh_infile);

	matchlen = 0;
	pos = DICSIZ;  insert_node();
	if (matchlen > remainder) matchlen = remainder;
	while (remainder > 0 && ! unpackable) {
		lastmatchlen = matchlen;  lastmatchpos = matchpos;
		get_next_match();
		if (matchlen > remainder) matchlen = remainder;
		if (matchlen > lastmatchlen || lastmatchlen < THRESHOLD) {
			HufOutput(text[pos - 1], 0);
		} else {

			HufOutput ((uint) (lastmatchlen + (UCHAR_MAX + 1 - THRESHOLD)),
				   (uint) ((pos - lastmatchpos - 2) & (DICSIZ - 1)));
			while (--lastmatchlen > 0) get_next_match();
			if (matchlen > remainder) matchlen = remainder;
		}
	}
	huf_encode_end();
        *originalSize = origsize;
        *compressSize = compsize;
}

