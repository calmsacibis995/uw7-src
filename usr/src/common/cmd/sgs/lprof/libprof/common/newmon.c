#ident	"@(#)lprof:libprof/common/newmon.c	1.10"

#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include "dprof.h"

static const char *progname;	/* incoming program name */

long	_out_tcnt;	/* bucket for out-of-bounds PCs */

/*
* The "mon.out" file format is:
*  1. Artificial mostly-zero SOentry block holding:
*	- number of following SOentry blocks (size)
*	- number of bytes used for pathnames (ccnt)
*	- number of out-of-bounds PC values (baseaddr)
*  2. The N actual SOentry blocks, modified by:
*	- next_SO is a "self" pointer
*	- SOpath is an index into the pathnames
*  3. The pathnames for the N SOentry blocks.
*  4. The M call count structures; M is the sum of the N SOentry "ccnt"s.
*  5. The N histograms, each one's length given by its SOentry "size".
*
* The filename is determined by the environment variable "PROFDIR".
* If it does not exist, use "mon.out"; if it exists and is empty,
* do not start profiling; otherwise, use "PROFDIR/pid.progname".
*/

static void
writeBlocks(SOentry *actso, const char *str) /* write profiling information */
{
	static const char mon_out[] = MON_OUT; /* "mon.out" */
	char path[PATH_MAX];
	SOentry *so, *list;
	char *strbase, *p;
	unsigned long nc;
	int nso, nb, fd;
	Cntb *cbp;

	if (str == 0)
		str = mon_out;
	else {
		if (progname == 0)
			progname = mon_out;
		snprintf(path, sizeof(path), "%s/%ld.%s",
			str, (long)getpid(), progname);
		str = path;
	}
	if ((fd = creat(str, 0666)) < 0) {
		perror(str);
		return;
	}
	/*
	* Append inactive SO list to active list for convenience.
	* Count number of SOentries and lengths of their paths.
	* Round up the number of pathname bytes so that the call
	* counts will be aligned.  Also count the expected number
	* of call count structures.
	*/
	_last_SO->next_SO = _inact_SO;
	nso = 1;
	nb = 0;
	nc = 0;
	for (so = actso; so != 0; so = so->next_SO) {
		if (so->SOpath != 0)
			nb += strlen(so->SOpath) + 1;
		nc += so->ccnt;
		nso++;
	}
	nb = (nb + sizeof(Cnt) - 1) / sizeof(Cnt) * sizeof(Cnt);
	/*
	* Allocate space for outgoing SOentries and their paths.
	* Fill it in, changing SOpath to be an index and next_SO
	* into self pointer.  Write it all out.
	*/
	if ((list = malloc(nso * sizeof(SOentry) + nb)) == 0) {
	err:;
		perror("mcount(writeBlocks)");
		goto out;
	}
	strbase = (char *)&list[nso];
	memset(&list[0], 0, sizeof(SOentry));
	list[0].size = nso - 1;
	list[0].ccnt = nb;
	list[0].baseaddr = _out_tcnt;
	nso = 1;
	p = strbase;
	for (so = actso; so != 0; so = so->next_SO) {
		list[nso] = *so;
		list[nso].next_SO = so;
		if (so->SOpath != 0) {
			list[nso].SOpath = (char *)(p - strbase);
			strcpy(p, so->SOpath);
			p += strlen(so->SOpath) + 1;
		}
		nso++;
	}
	memset(p, 0, nb - (p - strbase));
	nb += nso * sizeof(SOentry);
	if (write(fd, list, nb) != nb)
		goto err;
	/*
	* Write the call count buffers.  There should be nc of them.
	* All but the first must be full; the first still has _countbase
	* available entries.
	*/
	nc += _countbase;
	nb = (FCNTOT - _countbase) * sizeof(Cnt);
	for (cbp = _cntbuffer; cbp != 0; cbp = cbp->next) {
		nc -= FCNTOT;
		if (write(fd, cbp->cnts, nb) != nb)
			goto err;
		nb = sizeof(cbp->cnts);
	}
	if (nc != 0)
		perror("mcount(writeBlocks--call count mismatch)");
	/*
	* Finally, write the histogram data for each SO.
	*/
	for (so = actso; so != 0; so = so->next_SO) {
		nb = so->size * sizeof(WORD);
		if (write(fd, so->tcnt, nb) != nb)
			goto err;
	}
out:;
	close(fd);
	free(list);
}

void
_newmon(unsigned long low) /* turn on/off regular profiling */
{
	extern int etext; /* end of text; type is bogus */
	struct pack {
		SOentry	ent;
		Cntb	cnt;
		WORD	hst[1];	/* length calculated */
	} *pck;
	SOentry *so;
	size_t n;
	char *p;

	if ((p = getenv("PROFDIR")) != 0 && *p == '\0')
		return; /* profiling has been disabled */
	if (low == 0) {
		/*
		* Turn off profiling; write results.
		*/
		_dprofil(0);
		if ((so = _act_SO) != 0) {
			_act_SO = 0; /* record no more activity */
			writeBlocks(so, p);
		}
		return;
	}
	/*
	* Set progname if appropriate for writeBlocks().
	* Note that it can still be modified by the program,
	* but this prevents confusion due to argv[0] being
	* modified during the program execution.
	*/
	if (p != 0 && ___Argv != 0 && (p = ___Argv[0]) != 0) {
		progname = p;
		if ((p = strrchr(p, '/')) != 0)
			progname = p + 1;
	}
	/*
	* Start up profiling.  First allocate a struct pack
	* with enough histogram WORDs.  The scaling is
	* hardcoded to be 8; see SOinout.c and dprofil.c.
	*/
	n = ((unsigned long)&etext - low + 7) >> 3;
	pck = malloc(offsetof(struct pack, hst) + n * sizeof(WORD));
	if (pck == 0) {
		perror("profiling--initial allocation");
		return;
	}
	so = &pck->ent;
	_cntbuffer = &pck->cnt;
	so->SOpath = ""; /* denotes the a.out */
	so->tmppath = 0;
	so->tcnt = &pck->hst[0];
	so->prev_SO = 0;
	so->next_SO = 0;
	so->baseaddr = 0; /* unused by this profiling */
	so->textstart = low;
	so->endaddr = (unsigned long)&etext;
	so->size = n;
	so->ccnt = 0;
	/*
	* Make this the only active SO.
	* Turn on profiling timer.
	*/
	_inact_SO = 0;
	_act_SO = so;
	_last_SO = so;
	_curr_SO = so;
	_dprofil(1);
}

/*
* _search() and _mnewblock() are called by libc's _mcountNewent().
*/

void *
_search(unsigned long pc) /* find SO containing PC value */
{
	SOentry *so;

	for (so = _act_SO; so != 0; so = so->next_SO) {
		if (so->textstart <= pc && pc < so->endaddr) {
			_curr_SO = so; /* note most recent SO */
			return so;
		}
	}
	return 0; /* no match */
}

void
_mnewblock(void) /* attempt to allocate a new block of call counters */
{
	Cntb *cbp;

	sighold(SIGPROF);
	if ((cbp = malloc(sizeof(Cntb))) == 0)
		perror("mcount(mnewblock)");
	else {
		cbp->next = _cntbuffer;
		_cntbuffer = cbp;
		_countbase = FCNTOT;
	}
	sigrelse(SIGPROF);
}
