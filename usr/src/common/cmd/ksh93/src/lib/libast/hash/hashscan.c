#ident	"@(#)ksh93:src/lib/libast/hash/hashscan.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * hash table library
 */

#include "hashlib.h"

/*
 * hash table sequential scan
 *
 *	Hash_position_t*	pos;
 *	Hash_bucket_t*		b;
 *	pos = hashscan(tab, flags);
 *	while (b = hashnext(&pos)) ...;
 *	hashdone(pos);
 */

/*
 * return pos for scan on table
 */

Hash_position_t*
hashscan(register Hash_table_t* tab, register int flags)
{
	register Hash_position_t*	pos;

	static Hash_bucket_t		empty;

	if (!(pos = newof(0, Hash_position_t, 1, 0))) return(0);
	pos->tab = pos->top = tab;
	pos->bucket = &empty;
	pos->slot = tab->table - 1;
	pos->limit = tab->table + tab->size;
	if (tab->scope && !(flags & HASH_NOSCOPE))
	{
		pos->flags = HASH_SCOPE;
		do
		{
			register Hash_bucket_t*	b;

			if (tab->flags & HASH_SCANNING)
			{
				register Hash_bucket_t**	sp = tab->table;
				register Hash_bucket_t**	sx = tab->table + tab->size;

				while (sp < sx)
					for (b = *sp++; b; b = b->next)
						b->hash &= ~HASH_HIDDEN;
			}
		} while (tab = tab->scope);
		tab = pos->tab;
	}
	else pos->flags = 0;
	tab->flags |= HASH_SCANNING;
	tab->frozen++;
	return(pos);
}

/*
 * return next scan element
 */

Hash_bucket_t*
hashnext(register Hash_position_t* pos)
{
	register Hash_bucket_t*	b;

	if (!pos) return(hash_info.last = pos->tab->root->last.bucket = 0);
	b = pos->bucket;
	for (;;)
	{
		if (!(b = b->next))
		{
			do
			{
				if (++pos->slot >= pos->limit)
				{
					pos->tab->flags &= ~HASH_SCANNING;
					pos->tab->frozen--;
					if (!pos->flags || !pos->tab->scope) return(0);
					pos->tab = pos->tab->scope;
					pos->limit = (pos->slot = pos->tab->table) + pos->tab->size;
					pos->tab->flags |= HASH_SCANNING;
					pos->tab->frozen++;
				}
			} while (!(b = *pos->slot));
		}
		if (!(b->hash & HASH_DELETED) && (!(pos->tab->flags & HASH_VALUE) || b->value) && (!pos->flags || !(b->hash & (HASH_HIDDEN|HASH_HIDES)))) break;
		if (b->hash & HASH_HIDES)
		{
			register Hash_bucket_t*	h = (Hash_bucket_t*)b->name;

			if (!(h->hash & HASH_HIDDEN))
			{
				h->hash |= HASH_HIDDEN;
				if (!(b->hash & HASH_DELETED)) break;
			}
		}
		else b->hash &= ~HASH_HIDDEN;
	}
	return(hash_info.last = pos->tab->root->last.bucket = pos->bucket = b);
}

/*
 * terminate scan
 */

void
hashdone(register Hash_position_t* pos)
{
	if (pos)
	{
		if (pos->tab->flags & HASH_SCANNING)
		{
			pos->tab->flags &= ~HASH_SCANNING;
			pos->tab->frozen--;
		}
		free(pos);
	}
}
