#ident	"@(#)lprof:libprof/common/SOinout.c	1.7"

#include <link.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dprof.h"

#pragma weak _CAstopSO	/* don't bring in line profiling code unnecessarily */

char	**___Argv = 0;	/* set to argv by crt's */
SOentry	*_curr_SO = 0;	/* most recent SO, null for line profiling */
SOentry	*_act_SO = 0;	/* head of the active SO list (the a.out) */
SOentry	*_last_SO = 0;	/* tail of the active SO list */
SOentry	*_inact_SO = 0;	/* head of the inactive SO list */

/*
* _SOin() - called primarily by rtld, indirectly from
* _r_debug_state() whenever its link map is modified.
* See ".rtld.event" in [mp]crt1.s.  Check for any changes,
* updating our active list to match rtld's.
*/

void 
_SOin(unsigned long ptr)
{
	static struct link_map *linkmap;
	SOentry *so, *nso, *pso;
	struct link_map *lm;
	const char *p;

	if ((lm = linkmap) == 0) {
		struct r_debug *dep = (struct r_debug *)ptr;

		if (dep == 0 || (linkmap = dep->r_map) == 0)
			return;
		lm = linkmap;
	}
	if ((so = _act_SO) == 0)
		return;
	/*
	* Walk our list looking for entries not present
	* on rtld's lastest one.
	*/
	do {
		nso = so->next_SO;
		if (lm != 0) {
			if ((p = lm->l_name) == 0) /* the a.out */
				p = "";
			if (strcmp(p, so->SOpath) == 0) {
				lm = lm->l_next;
				continue;
			}
		}
		/*
		* For line profiling, we need to process things now,
		* before it's moved to the inactive list.  Moreover,
		* we are counting on rtld NOT yet having unmapped
		* the data for the SO.
		*/
		if (_curr_SO == 0)
			_CAstopSO(so);
		/*
		* This SO has been dlclose()d.
		* Move it from the active to the inactive list.
		*/
		pso = so->prev_SO;
		if (nso == 0) /* so == _last_SO */
			_last_SO = pso;
		else
			nso->prev_SO = pso;
		pso->next_SO = nso;
		so->next_SO = _inact_SO;
		_inact_SO = so;
	} while ((so = nso) != 0);
	/*
	* Add anything left on rtld's list as new.
	*/
	while (lm != 0) {
		if ((so = malloc(sizeof(SOentry))) == 0) {
		err:;
			perror("profiling--shared object allocation");
			/*
			* Disable any further profiling.
			*/
			_act_SO = 0;
			_inact_SO = 0;
			_curr_SO = 0;
			return;
		}
		so->SOpath = lm->l_name;
		so->tmppath = 0;
		so->tcnt = 0;
		so->next_SO = 0;
		so->baseaddr = lm->l_addr;
		so->textstart = lm->l_tstart;
		so->endaddr = 0;
		so->size = 0;
		so->ccnt = 0;
		if (lm->l_tsize != 0) {
			so->endaddr = lm->l_tstart + lm->l_tsize;
			if (_curr_SO != 0) {
				/*
				* Scaling is hardcoded to be 8;
				* see dprofil.c and newmon.c.
				*/
				so->size = (lm->l_tsize + 7) >> 3;
				so->tcnt = calloc(so->size, sizeof(WORD));
				if (so->tcnt == 0)
					goto err;
			}
		}
		lm = lm->l_next;
		/*
		* Append it to the end of our active list.
		* There is always at least one entry in the active list
		* (the application), so no check for a null _last_SO.
		*/
		so->prev_SO = _last_SO;
		_last_SO->next_SO = so;
		_last_SO = so;
	}
}
