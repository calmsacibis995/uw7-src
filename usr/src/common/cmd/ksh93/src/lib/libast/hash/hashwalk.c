#ident	"@(#)ksh93:src/lib/libast/hash/hashwalk.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * hash table library
 */

#include "hashlib.h"

/*
 * apply walker to each active bucket in the table
 */

int
hashwalk(Hash_table_t* tab, int flags, register(*walker)(const char*, char*, void*), void* handle)
{
	register Hash_bucket_t*	b;
	register int		v;
	Hash_position_t*	pos;

	if (!(pos = hashscan(tab, flags))) return(-1);
	hash_info.table = tab->root->last.table = tab;
	v = 0;
	while (b = hashnext(pos))
		if ((v = (*walker)(hashname(b), (tab->flags & HASH_VALUE) ? b->value : (char*)b, handle)) < 0)
			break;
	hashdone(pos);
	return(v);
}
