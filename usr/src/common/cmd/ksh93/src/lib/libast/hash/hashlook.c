#ident	"@(#)ksh93:src/lib/libast/hash/hashlook.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * hash table library
 */

#include "hashlib.h"

/*
 * hash table lookup
 */

char*
hashlook(register Hash_table_t* tab, const char* name, long flags, const char* value)
{
	register Hash_bucket_t*	b;
	register unsigned int	n;
	register Hash_last_t*	last;
	Hash_table_t*		top;
	Hash_bucket_t*		prev;
	unsigned int		i;

	if ((flags & (HASH_LOOKUP|HASH_INTERNAL)) == (HASH_LOOKUP|HASH_INTERNAL))
	{
		register char*		s1;
		register const char*	s2;
		register int		c;

		if (flags & HASH_HASHED) n = *((unsigned int*)value);
		else
		{
			s2 = name;
			n = 0;
			while (c = *s2++) HASHPART(n, c);
		}
		i = n;
		for (;;)
		{
			HASHMOD(tab, n);
			for (b = tab->table[n]; b; b = b->next)
			{
				s1 = hashname(b);
				s2 = name;
				while ((c = *s1++) == *s2++)
					if (!c) return((flags & HASH_VALUE) ? b->value : (char*)b);
			}
			if (!(tab = tab->scope) || (flags & HASH_NOSCOPE))
				return(0);
			n = i;
		}
	}
	tab->root->accesses++;
	top = tab;
	last = &tab->root->last;
	if (name)
	{
		hash_info.table = last->table = tab;
		if (flags & (HASH_BUCKET|HASH_INSTALL))
		{
			last->bucket = (Hash_bucket_t*)name;
			name = hashname(last->bucket);
		}
		else last->bucket = 0;
		hash_info.last = last->bucket;
		last->name = name;
		if (flags & HASH_BUCKET) n = last->bucket->hash;
		else if (tab->flags & HASH_HASHED)
		{
			n = (unsigned int)name;
			if (!(flags & HASH_HASHED)) n >>= 3;
		}
		else if (flags & HASH_HASHED) n = *((unsigned int*)value);
		else HASH(tab->root, name, n);
		last->hash = i = HASHVAL(n);
		for (;;)
		{
			HASHMOD(tab, n);
			for (prev = 0, b = tab->table[n]; b; prev = b, b = b->next)
			{
				if (i == HASHVAL(b->hash) && ((b->hash & (HASH_DELETED|HASH_OPAQUED)) != HASH_DELETED || (flags & (HASH_CREATE|HASH_DELETE|HASH_INSTALL))))
				{
					if (!tab->root->local->compare)
					{
						register char*		s1 = hashname(b);
						register const char*	s2 = name;

						if (tab->root->namesize)
						{
							register char*	s3 = s1 + tab->root->namesize;

							while (*s1++ == *s2++)
								if (s1 >= s3) goto found;
						}
						else while (*s1 == *s2++)
							if (!*s1++) goto found;
					}
					else if (tab->root->namesize)
					{
						if (!(*tab->root->local->compare)(hashname(b), name, tab->root->namesize)) goto found;
					}
					else if (!(*tab->root->local->compare)(hashname(b), name)) goto found;
				}
				tab->root->collisions++;
			}
			if (!tab->scope || (flags & (HASH_CREATE|HASH_INSTALL|HASH_NOSCOPE)) == HASH_NOSCOPE) break;
			tab = tab->scope;
			n = i;
		}
	}
	else
	{
		tab = last->table;
		name = last->name;
		n = i = last->hash;
		prev = 0;
		HASHMOD(tab, n);
		if (b = last->bucket)
		{
			/*
			 * found the bucket
			 */
		
		found:
			if (prev && !(tab->flags & HASH_SCANNING))
			{
				/*
				 * migrate popular buckets to the front
				 */
		
				prev->next = b->next;
				b->next = tab->table[n];
				tab->table[n] = b;
			}
			switch (flags & (HASH_CREATE|HASH_DELETE|HASH_INSTALL))
			{
			case HASH_CREATE:
			case HASH_CREATE|HASH_INSTALL:
			case HASH_INSTALL:
				if (tab != top && !(flags & HASH_SCOPE)) break;
				if (flags & HASH_OPAQUE) b->hash |= HASH_OPAQUED;
				goto exists;

			case HASH_DELETE:
				value = 0;
				if (tab == top || (flags & HASH_SCOPE))
				{
					if (flags & HASH_OPAQUE) b->hash &= ~HASH_OPAQUED;
					else if (!(tab->root->flags & HASH_BUCKET))
					{
						if (tab->root->local->free && b->value)
						{
							(*tab->root->local->free)(b->value);
							b->value = 0;
						}
						else if (tab->flags & HASH_VALUE)
						{
							value = b->value;
							b->value = 0;
						}
					}
					tab->buckets--;
					if (tab->frozen || (b->hash & HASH_OPAQUED)) b->hash |= HASH_DELETED;
					else
					{
						tab->table[n] = b->next;
						if (tab->root->local->free && (tab->root->flags & HASH_BUCKET)) (*tab->root->local->free)((char*)b);
						else if (!(b->hash & HASH_KEEP))
						{
							if (tab->root->local->region) (*tab->root->local->region)(tab->root->local->handle, b, 0, 0);
							else free(b);
						}
					}
				}
				return((char*)value);
			default:
				if (!(b->hash & HASH_DELETED)) goto exists;
				return(0);
			}
		}
	}
	if (!(flags & (HASH_CREATE|HASH_INSTALL))) return(0);

	/*
	 * create a new bucket
	 */

	if (tab == top) prev = 0;
	else
	{
		if (prev = b)
		{
			name = (b->hash & HASH_HIDES) ? b->name : (char*)b;
			i |= HASH_HIDES;
		}
		if (!(flags & HASH_SCOPE)) tab = top;
	}

	/*
	 * check for table expansion
	 */

	if (!tab->frozen && !(tab->flags & HASH_FIXED) && tab->buckets > tab->root->meanchain * tab->size) hashsize(tab, tab->size << 1);
	if (flags & HASH_INSTALL)
	{
		b = last->bucket;
		i |= HASH_KEEP;
	}
	else
	{
		int	m = tab->bucketsize * sizeof(char*);

		if (flags & HASH_VALUE)
		{
			tab->flags |= HASH_VALUE;
			if (m < sizeof(Hash_bucket_t))
			{
				tab->bucketsize = (sizeof(Hash_bucket_t) + sizeof(char*) - 1) / sizeof(char*);
				m = tab->bucketsize * sizeof(char*);
			}
			n = m;
		}
		else if (!(n = HASH_SIZEOF(flags)))
		{
			if (!(flags & HASH_FIXED)) n = m;
			else if ((n = (int)value) < m) n = m;
		}
		else if (n < m) n = m;
		if (!prev && (tab->flags & HASH_ALLOCATE))
		{
			m = tab->root->namesize ? tab->root->namesize : strlen(name) + 1;
			if (tab->root->local->region)
			{
				if (!(b = (Hash_bucket_t*)(*tab->root->local->region)(tab->root->local->handle, NiL, n + m, 0)))
					return(0);
				memset(b, 0, n + m);
			}
			else if (!(b = newof(0, Hash_bucket_t, 0, n + m)))
				return(0);
			b->name = (char*)b + n;
			memcpy(b->name, name, m);
		}
		else
		{
			if (tab->root->local->region)
			{
				if (!(b = (Hash_bucket_t*)(*tab->root->local->region)(tab->root->local->handle, NiL, n, 0)))
					return(0);
				memset(b, 0, n);
			}
			else if (!(b = newof(0, Hash_bucket_t, 0, n)))
				return(0);
			b->name = (char*)name;
		}
	}
	b->hash = n = i;
	HASHMOD(tab, n);
	b->next = tab->table[n];
	tab->table[n] = b;
	tab->buckets++;
	if (flags & HASH_OPAQUE)
	{
		tab->buckets--;
		b->hash |= HASH_DELETED|HASH_OPAQUED;
		return(0);
	}
 exists:

	/*
	 * finally got the bucket
	 */

	if (b->hash & HASH_DELETED)
	{
		b->hash &= ~HASH_DELETED;
		tab->buckets++;
	}
	hash_info.last = last->bucket = b;
	hash_info.table = last->table = tab;
	switch (flags & (HASH_CREATE|HASH_VALUE))
	{
	case HASH_CREATE|HASH_VALUE:
		if (tab->root->local->free && !(tab->root->flags & HASH_BUCKET) && b->value) (*tab->root->local->free)(b->value);
		if (value && tab->root->local->alloc) value = (*tab->root->local->alloc)((unsigned int)value);
		b->value = (char*)value;
		return((char*)hashname(b));
	case HASH_VALUE:
		return(b->value);
	default:
		return((char*)b);
	}
}
