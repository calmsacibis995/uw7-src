#ident	"@(#)OSRcmds:compress/huf-tree.c	1.1"
#pragma comment(exestr, "@(#) huf-tree.c 23.1 91/11/27 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1991.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */
/*
 * huf-tree.c -- make Huffman tree
 *-----------------------------------------------------------------------------
 * From public domain code in zoo by Rahul Dhesi, who adapted it from "ar"
 * archiver written by Haruhiko Okumura.
 *=============================================================================
 */

#include "lzh-defs.h"

static int    n, heapsize;
static short  heap[NC + 1];
static ushort *freq, *sortptr, len_cnt[17];
static uchar  *len;

/* 
 * call with i = root
 */
static void
count_len (int i)
{
	static int depth = 0;

	if (i < n) len_cnt[(depth < 16) ? depth : 16]++;
	else {
		depth++;
		count_len((int) left [i]);
		count_len((int) right[i]);
		depth--;
	}
}

/*
 *
 */
static void
make_len (int root)
{
	int i, k;
	uint cum;

	for (i = 0; i <= 16; i++) len_cnt[i] = 0;
	count_len(root);
	cum = 0;
	for (i = 16; i > 0; i--)
		cum += len_cnt[i] << (16 - i);
	while (cum != ((unsigned) 1 << 16)) {
		(void) fprintf(stderr, "17");
		len_cnt[16]--;
		for (i = 15; i > 0; i--) {
			if (len_cnt[i] != 0) {
				len_cnt[i]--;  len_cnt[i+1] += 2;  break;
			}
		}
		cum--;
	}
	for (i = 16; i > 0; i--) {
		k = len_cnt[i];
		while (--k >= 0) len[*sortptr++] = i;
	}
}

/*
 * Priority queue; send i-th entry down heap.
 */
static void
downheap (int i)
{
	int j, k;

	k = heap[i];
	while ((j = 2 * i) <= heapsize) {
		if (j < heapsize && freq[heap[j]] > freq[heap[j + 1]])
		 	j++;
		if (freq[k] <= freq[heap[j]]) break;
		heap[i] = heap[j];  i = j;
	}
	heap[i] = k;
}

/*
 *
 */
static void
make_code (int    j,
           uchar  length [],
           ushort code [])
{
	int    i;
	ushort start[18];

	start[1] = 0;
	for (i = 1; i <= 16; i++)
		start[i + 1] = (start[i] + len_cnt[i]) << 1;
	for (i = 0; i < j; i++) code[i] = start[length[i]]++;
}

/*
 * make tree, calculate len[], return root
 */
int 
make_tree (int    nparm,
           ushort freqparm [],
           uchar  lenparm [],
           ushort codeparm [])
{
	int i, j, k, avail;

	n = nparm;  freq = freqparm;  len = lenparm;
	avail = n;  heapsize = 0;  heap[1] = 0;
	for (i = 0; i < n; i++) {
		len[i] = 0;
		if (freq[i]) heap[++heapsize] = i;
	}
	if (heapsize < 2) {
		codeparm[heap[1]] = 0;  return heap[1];
	}
	for (i = heapsize / 2; i >= 1; i--)
		downheap(i);  /* make priority queue */
	sortptr = codeparm;
	do {  /* while queue has at least two entries */
		i = heap[1];  /* take out least-freq entry */
		if (i < n) *sortptr++ = i;
		heap[1] = heap[heapsize--];
		downheap(1);
		j = heap[1];  /* next least-freq entry */
		if (j < n) *sortptr++ = j;
		k = avail++;  /* generate new node */
		freq[k] = freq[i] + freq[j];
		heap[1] = k;  downheap(1);  /* put into queue */
		left[k] = i;  right[k] = j;
	} while (heapsize > 1);
	sortptr = codeparm;
	make_len(k);
	make_code(nparm, lenparm, codeparm);
	return k;  /* return root */
}
