/*	Copyright (c) 1993 UNIVEL					*/

#ident	"@(#)sr_hash.c	23.1"

#ifdef _KERNEL
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <mem/kmem.h>
#include <io/dma.h>
#include <fs/file.h>
#include <mem/immu.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <proc/signal.h>
#include <io/conf.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <net/dlpi.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <io/ddi.h>
#include <io/ddi_i386at.h>
#include <util/mod/moddefs.h>
#endif /*_KERNEL */

#include <io/sr/sr.h>


#ifdef	DBG
extern int	sr_debug;
#endif


/*
 * Space.c variables
 */
extern int	(*sr_insert)();			/* by default set in Space.c to
						 * sr_insert_latest
						 */

extern int	sr_age;				/* used only if sr_insert is
						 * set to sr_insert_age
						 */

extern int	sr_broadcast;			/* by default set in Space.c to
						 * SINGLE_ROUTE_BCAST
						 */

/*
 * End of Space.c candidates
 */


sr_elem_t *sr_tab[MAX_TAB_SIZE];
sr_mem_t *sr_mem;



sr_elem_t *
sr_tab_lookup(macaddr)
unsigned char *macaddr;
{
ushort slot = SR_HASH(macaddr);
sr_elem_t *sr_elemp = sr_tab[slot];


	while (sr_elemp) {
		if (SAME_MACADDR(sr_elemp->sr_macaddr,macaddr))	{

#ifdef	DBG
			if (sr_debug > 1)	{
				printf("SR: found entry ");
				print_macaddr(macaddr);
				print_route(sr_elemp->sr_route, sr_elemp->sr_route_size);
			}
#endif

			return sr_elemp;
		}
		else
			sr_elemp = sr_elemp->sr_next;
	}

#ifdef	DBG
	if (sr_debug > 1)	{
		printf("SR: couldn't find entry ");
		print_macaddr(macaddr);
	}
#endif

	return NULL;
}


void
init_sr_mem(memp)
sr_mem_t *memp;
{
register int i;
sr_elem_t *sr_elemp;


	memp->sr_hdr_next = NULL;
	memp->sr_num_avail = MAX_LIST_SIZE;

	for (i = MAX_LIST_SIZE,sr_elemp = memp->sr_elem_list;i;i--,sr_elemp++){
		sr_elemp->sr_avail = 0;
		sr_elemp->sr_next = NULL;
		sr_elemp->sr_memp = (caddr_t)memp;
	}
}

void
sr_tab_uninit()
{
sr_mem_t *mem1,*mem2;


	mem1 = sr_mem;
	while (mem1) {
		mem2 = mem1->sr_hdr_next;
		kmem_free(mem1,SR_PAGE_SIZE);
		mem1 = mem2;
	}
}


int
sr_tab_init()
{
register int i;


	for (i = 0; i < MAX_TAB_SIZE; i++)
		sr_tab[i] = NULL;
	if ((sr_mem = (sr_mem_t *)kmem_alloc(SR_PAGE_SIZE,KM_NOSLEEP)) == NULL)
		return -1;
	init_sr_mem(sr_mem);
	return 0;
}

sr_elem_t *
sr_find_slot()
{
register int i;
sr_mem_t *sr_memp,*tsr_memp;
sr_elem_t *sr_elemp;


	for (sr_memp = sr_mem;sr_memp; sr_memp = sr_memp->sr_hdr_next) {
		if (sr_memp->sr_num_avail)
			break;
	}

	if (!sr_memp) {
		tsr_memp = sr_mem;
		while (tsr_memp->sr_hdr_next)
			tsr_memp = tsr_memp->sr_hdr_next;
		if ((sr_memp = (sr_mem_t *)kmem_alloc(SR_PAGE_SIZE,KM_NOSLEEP)) == NULL)
			return NULL;
		init_sr_mem(sr_memp);
		tsr_memp->sr_hdr_next = sr_memp;
	}

	sr_elemp = sr_memp->sr_elem_list;
	for (i = MAX_LIST_SIZE; i--; i,sr_elemp++) {
		if (!(sr_elemp->sr_avail)) {
			sr_elemp->sr_avail = 1;
			sr_memp->sr_num_avail--;
			return sr_elemp;
		}
	}
	return NULL;
}


void
sr_delete(macaddr)
unsigned char *macaddr;
{
ushort slot = SR_HASH(macaddr);
sr_elem_t *sr_elemp = sr_tab[slot];
sr_elem_t *tmp;
sr_mem_t *sr_memp;


	for (tmp = NULL,sr_elemp = sr_tab[slot];sr_elemp; tmp = sr_elemp,
						sr_elemp = sr_elemp->sr_next) {
		if (SAME_MACADDR(sr_elemp->sr_macaddr,macaddr)) {
			if (!tmp)
				sr_tab[slot] = sr_elemp->sr_next;
			else
				tmp->sr_next = sr_elemp->sr_next;
			sr_elemp->sr_next = NULL;
			sr_elemp->sr_avail = 0;
			sr_memp = (sr_mem_t *)sr_elemp->sr_memp;
			sr_memp->sr_num_avail++;

#ifdef	DBG
			if (sr_debug > 1)	{
				printf("SR: deleted entry ");
				print_macaddr(macaddr);
				print_route(sr_elemp->sr_route, sr_elemp->sr_route_size);
			}
#endif

			return;
		}
	}

#ifdef	DBG
	if (sr_debug > 1)	{
		printf("SR: entry not found, delete failed for ");
		print_macaddr(macaddr);
	}
#endif

}


int
sr_insert_latest(macaddr,sr_route,sr_size)
unsigned char *macaddr,*sr_route;
int sr_size;
{
ushort slot = SR_HASH(macaddr);
sr_elem_t *sr_elemp;
sr_elem_t *tsr_elemp;
int	new;


	/* 
	   Check whether we are updating an existing route or adding the
	   route for the first time. 
	*/
	if ((sr_elemp = sr_tab_lookup(macaddr)) == NULL) {

		/* Obtain a new slot */
		if ((sr_elemp = sr_find_slot()) == NULL)
			return -1;

		/* Insert the element in its proper position in the hashtable */
		sr_elemp->sr_next = NULL;
		if ( (tsr_elemp = sr_tab[slot]) == NULL)
			sr_tab[slot] = sr_elemp;
		else {
			for(;tsr_elemp->sr_next;tsr_elemp = tsr_elemp->sr_next)
				;
			tsr_elemp->sr_next = sr_elemp;
		}

		new = 1;
	}
	else
		new = 0;

	sr_elemp->sr_route_size = sr_size;
	if (new)
		COPY_MACADDR(macaddr,sr_elemp->sr_macaddr);
	if (sr_size)
		bcopy(sr_route,sr_elemp->sr_route,sr_size);

	return 0;
}


int
sr_insert_age(macaddr,sr_route,sr_size)
unsigned char *macaddr,*sr_route;
int sr_size;
{
ushort slot = SR_HASH(macaddr);
sr_elem_t *sr_elemp;
sr_elem_t *tsr_elemp;
ulong	ticks;
int	new;


	drv_getparm(LBOLT, &ticks);

	/* 
	   Check whether we are updating an existing route or adding the
	   route for the first time. 
	*/
	if ((sr_elemp = sr_tab_lookup(macaddr)) == NULL) {

		/* Obtain a new slot */
		if ((sr_elemp = sr_find_slot()) == NULL)
			return -1;

		/* Insert the element in its proper position in the hashtable */
		sr_elemp->sr_next = NULL;
		if ( (tsr_elemp = sr_tab[slot]) == NULL)
			sr_tab[slot] = sr_elemp;
		else {
			for(;tsr_elemp->sr_next;tsr_elemp = tsr_elemp->sr_next)
				;
			tsr_elemp->sr_next = sr_elemp;
		}

		new = 1;
	}
	else	{

		/*
		 * age entries the same amount of time that ARP does, i.e. if
		 * the route hasn't changed in 20 minutes, replace it by the
		 * new one, else drop the new one.  Later we can get more
		 * sophisticated, eg. look at the route size, frame size, etc.
		 */
		if (TICKS_BETWEEN(sr_elemp->sr_time, ticks) < sr_age * HZ)	{

#ifdef	DBG
			if (sr_debug > 1)	{
				printf("SR: ignored route update ");
				print_macaddr(macaddr);
				print_route(sr_route, sr_size);
			}
#endif

			return 0;
		}

		new = 0;
	}

#ifdef	DBG
	if (sr_debug)	{
		printf("SR: updated route ");
		print_macaddr(macaddr);
		print_route(sr_route, sr_size);
	}
#endif

	sr_elemp->sr_time = ticks;
	sr_elemp->sr_route_size = sr_size;
	if (new)
		COPY_MACADDR(macaddr,sr_elemp->sr_macaddr);
	if (sr_size)
		bcopy(sr_route,sr_elemp->sr_route,sr_size);

	return 0;
}


sr_dump_route_table(mp)
mblk_t *mp;
{
unsigned char *routep = mp->b_rptr;
sr_elem_t *sr_elemp;
register int i;
register int num_routes = 0;

	for (i = 0; i < MAX_TAB_SIZE; i++) {
		if (!sr_tab[i])
			continue;
		for (sr_elemp = sr_tab[i]; sr_elemp; 
						sr_elemp = sr_elemp->sr_next) {
			bcopy((caddr_t)sr_elemp,(caddr_t)routep,BASIC_ROUTE_INFO_SIZE);
			num_routes++;
			routep += BASIC_ROUTE_INFO_SIZE;
			if (routep >= mp->b_wptr)
				goto finish;
		}
	}
	finish:
		return num_routes;
}


static char *hex[] = {
	"0","1","2","3","4","5","6","7","8","9","a","b","c","d","e","f"
};

print_macaddr(ptr)
unsigned char *ptr;
{
register int i;

	for(i = 0; i < 6; i++,ptr++) {
		if (i != 0)
			printf(":");
		printf("%s%s", hex[(*ptr >> 4) & 0xf],hex[*ptr &0xf]);
	}
	printf("\n");
}


print_route(ptr,size)
unsigned char *ptr;
int	size;
{
register int i;

        for(i = 0; i < size; i++,ptr++) {
		if (i != 0)
			printf(":");
		printf("%s%s", hex[(*ptr >> 4) & 0xf],hex[*ptr &0xf]);
	}
	if (size == 0)
		printf("<NULL-ROUTE>\n");
	else
		printf("\n");
}


/* 	Incoming message block contains a DL_UNITDATA_REQ primitive with the 
	destination address being a broadcast address.
*/ 

sr_proc_broadcast(mp)
mblk_t *mp;
{
dl_unitdata_req_t	*unitdata;
unsigned char		*destaddr;
unsigned char		*routep;


#ifdef	DBG
	if (sr_debug)
		printf("SR: processed broadcast request\n");
#endif

	unitdata = (dl_unitdata_req_t *)mp->b_rptr;
	destaddr = (unsigned char *)unitdata + unitdata->dl_dest_addr_offset;
	routep = (unsigned char *)(destaddr + unitdata->dl_dest_addr_length);
	*routep++ = sr_broadcast|INIT_ROUTE_HDR_SIZE;
	*routep = LARGEST_FRAME_SIZE; 
	unitdata->dl_dest_addr_length += INIT_ROUTE_HDR_SIZE;
	mp->b_wptr += INIT_ROUTE_HDR_SIZE;
}
