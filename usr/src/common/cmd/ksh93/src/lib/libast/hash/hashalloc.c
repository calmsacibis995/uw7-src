#ident	"@(#)ksh93:src/lib/libast/hash/hashalloc.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * hash table library
 */

static const char id_hash[] = "\n@(#)hash (AT&T Bell Laboratories) 05/09/95\0\n";

#include "hashlib.h"

#if _DLL_INDIRECT_DATA && !_DLL
static Hash_info_t	hash_info_data;
Hash_info_t		hash_info = &hash_info_data;
#else
Hash_info_t		hash_info = { 0 };
#endif

/*
 * create a new hash table
 */

Hash_table_t*
hashalloc(Hash_table_t* ref, ...)
{
	register Hash_table_t*	tab;
	register Hash_table_t*	ret = 0;
	register int		internal;
	int			n;
	va_list			ap;
	va_list			va[4];
	va_list*		vp = va;
	HASHregion		region = 0;
	void*			handle;

	va_start(ap, ref);

	/*
	 * check for HASH_region which must be first
	 */

	n = va_arg(ap, int);
	if (!ref && n == HASH_region)
	{
		region = va_arg(ap, HASHregion);
		handle = va_arg(ap, void*);
		n = va_arg(ap, int);
		if (!(tab = (Hash_table_t*)(*region)(handle, NiL, sizeof(Hash_table_t), 0)))
			goto out;
		memset(tab, 0, sizeof(Hash_table_t));
	}
	else if (!(tab = newof(0, Hash_table_t, 1, 0)))
		goto out;
	tab->bucketsize = (sizeof(Hash_header_t) + sizeof(char*) - 1) / sizeof(char*);
	if (ref)
	{
		tab->flags = ref->flags & ~HASH_RESET;
		tab->root = ref->root;
		internal = HASH_INTERNAL;
	}
	else
	{
		if (region)
		{
			if (!(tab->root = (Hash_root_t*)(*region)(handle, NiL, sizeof(Hash_root_t), 0)))
				goto out;
			memset(tab->root, 0, sizeof(Hash_root_t));
		}
		else if (!(tab->root = newof(0, Hash_root_t, 1, 0)))
			goto out;
		if (!(tab->root->local = newof(0, Hash_local_t, 1, 0)))
			goto out;
		if (tab->root->local->region = region)
			tab->root->local->handle = handle;
		tab->root->meanchain = HASHMEANCHAIN;
		internal = 0;
	}
	tab->size = HASHMINSIZE;
	for (;;)
	{
		switch (n) 
		{
		case HASH_alloc:
			if (ref) goto out;
			tab->root->local->alloc = va_arg(ap, HASHalloc);
			break;
		case HASH_bucketsize:
			n = (va_arg(ap, int) + sizeof(char*) - 1) / sizeof(char*);
			if (n > UCHAR_MAX) goto out;
			if (n > tab->bucketsize) tab->bucketsize = n;
			break;
		case HASH_clear:
			tab->flags &= ~(va_arg(ap, int) & ~internal);
			break;
		case HASH_compare:
			if (ref) goto out;
			tab->root->local->compare = va_arg(ap, HASHcompare);
			break;
		case HASH_free:
			if (ref) goto out;
			tab->root->local->free = va_arg(ap, HASHfree);
			break;
		case HASH_hash:
			if (ref) goto out;
			tab->root->local->hash = va_arg(ap, HASHhash);
			break;
		case HASH_meanchain:
			if (ref) goto out;
			tab->root->meanchain = va_arg(ap, int);
			break;
		case HASH_name:
			tab->name = va_arg(ap, char*);
			break;
		case HASH_namesize:
			if (ref) goto out;
			tab->root->namesize = va_arg(ap, int);
			break;
		case HASH_region:
			goto out;
		case HASH_set:
			tab->flags |= (va_arg(ap, int) & ~internal);
			break;
		case HASH_size:
			tab->size = va_arg(ap, int);
			if (tab->size & (tab->size - 1)) tab->flags |= HASH_FIXED;
			break;
		case HASH_table:
			tab->table = va_arg(ap, Hash_bucket_t**);
			tab->flags |= HASH_STATIC;
			break;
		case HASH_va_list:
			if (vp < &va[elementsof(va)]) *vp++ = ap;
			ap = va_arg(ap, va_list);
			break;
		case 0:
			if (vp > va)
			{
				ap = *--vp;
				break;
			}
			if (tab->flags & HASH_SCOPE)
			{
				if (!(tab->scope = ref)) goto out;
				ref->frozen++;
			}
			if (!tab->table)
			{
				if (region)
				{
					if (!(tab->table = (Hash_bucket_t**)(*region)(handle, NiL, sizeof(Hash_bucket_t*) * tab->size, 0)))
						goto out;
					memset(tab->table, 0, sizeof(Hash_bucket_t*) * tab->size);
				}
				else if (!(tab->table = newof(0, Hash_bucket_t*, tab->size, 0))) goto out;
			}
			if (!ref)
			{
				tab->root->flags = tab->flags & HASH_INTERNAL;
				tab->root->next = hash_info.list;
				hash_info.list = tab->root;
			}
			if (!region)
			{
				tab->next = tab->root->references;
				tab->root->references = tab;
			}
			ret = tab;
			goto out;
		default:
			goto out;
		}
		n = va_arg(ap, int);
	}
 out:
	va_end(ap);
	if (!ret) hashfree(tab);
	return(ret);
}
