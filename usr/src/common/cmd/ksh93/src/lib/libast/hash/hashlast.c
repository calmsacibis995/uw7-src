#ident	"@(#)ksh93:src/lib/libast/hash/hashlast.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * hash table library
 */

#include "hashlib.h"

/*
 * return last lookup bucket for table
 */

Hash_bucket_t*
hashlast(Hash_table_t* tab)
{
	return(tab->root->last.bucket);
}
