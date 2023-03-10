#ident	"@(#)ksh93:src/lib/libast/hash/hashfree.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * hash table library
 */

#include "hashlib.h"

/*
 * free (remove) a hash table
 * can be called for partially constructed tables
 * scope covered table pointer is returned
 * root info freed when last reference freed
 */

Hash_table_t*
hashfree(register Hash_table_t* tab)
{
	register Hash_bucket_t**	sp;
	register Hash_bucket_t*		b;
	register Hash_bucket_t*		p;
	Hash_bucket_t**			sx;
	Hash_root_t*			rp;
	Hash_table_t*			tp;
	HASHfree			freevalue;
	HASHfree			freebucket;
	HASHregion			region;
	void*				handle;

	if (!tab) return(0);
	if (tab->table)
	{
		freebucket = 0;
		freevalue = 0;
		if (tab->root->local->free)
		{
			if (tab->root->flags & HASH_BUCKET) freebucket = tab->root->local->free;
			else freevalue = tab->root->local->free;
		}
		if (region = tab->root->local->region)
			handle = tab->root->local->handle;
		sx = &tab->table[tab->size];
		sp = &tab->table[0];
		while (sp < sx)
		{
			b = *sp++;
			while (b)
			{
				p = b;
				b = b->next;
				if (freebucket) (*freebucket)((char*)p);
				else if (freevalue && p->value) (*freevalue)(p->value);
				if (!(p->hash & HASH_KEEP))
				{
					if (region) (*region)(handle, p, 0, 0);
					else free(p);
				}
				else if (p->hash & HASH_HIDES)
				{
					p->hash &= ~HASH_HIDES;
					p->name = ((Hash_bucket_t*)p->name)->name;
				}
			}
		}
		if ((tab->flags & (HASH_RESIZE|HASH_STATIC)) != HASH_STATIC)
		{
			if (region) (*region)(handle, tab->table, 0, 0);
			else free(tab->table);
		}
	}
	else region = 0;
	if (tab->root)
	{
		if (!region)
		{
			/*
			 * remove from the table lists
			 */

			if ((tp = tab->root->references) != tab)
			{
				for (; tp; tp = tp->next)
					if (tp->next == tab)
					{
						tp->next = tab->next;
						break;
					}
			}
			else if (!(tab->root->references = tp->next))
			{
				if ((rp = hash_info.list) != tab->root)
				{
					for (; rp; rp = rp->next)
						if (rp->next == tab->root)
						{
							rp->next = tab->root->next;
							break;
						}
				}
				else hash_info.list = rp->next;
			}
		}
		if (!(tab->root->references))
		{
			if (tab->root->local)
				free(tab->root->local);
			if (region) (*region)(handle, tab->root, 0, 0);
			else free(tab->root);
		}
	}
	if (tp = tab->scope) tp->frozen--;
	if (region) (*region)(handle, tab, 0, 0);
	else free(tab);
	return(tp);
}
