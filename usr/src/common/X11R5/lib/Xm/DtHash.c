#pragma ident	"@(#)m1.2libs:Xm/DtHash.c	1.1"
/*************************************************************************
 **  (c) Copyright 1993, 1994 Hewlett-Packard Company
 **  (c) Copyright 1993, 1994 International Business Machines Corp.
 **  (c) Copyright 1993, 1994 Sun Microsystems, Inc.
 **  (c) Copyright 1993, 1994 Novell, Inc.
 *************************************************************************/
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$"
#endif
#endif
/***********************************************************
Copyright 1987, 1988, 1990 by Digital Equipment Corporation, Maynard,
Massachusetts, and the Massachusetts Institute of Technology, Cambridge,
Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#include "_DtHashP.h"

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static unsigned int GetTableIndex() ;
static void ExpandHashTable() ;

#else

static unsigned int GetTableIndex( 
                        register DtHashTable tab,
                        register DtHashKey key,
#if NeedWidePrototypes
                        register int new) ;
#else
                        register Boolean new) ;
#endif /* NeedWidePrototypes */
static void ExpandHashTable( 
                        register DtHashTable tab) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


typedef unsigned long 	Signature;

typedef struct _DtHashTableRec {
    unsigned int mask;		/* size of hash table - 1 */
    unsigned int rehash;	/* mask - 2 */
    unsigned int occupied;	/* number of occupied entries */
    unsigned int fakes;		/* number occupied by DTHASHfake */
    DtHashEntryType *types;	/* lookup methods for key	*/
    unsigned short numTypes;    /* number of lookup methods	*/
    Boolean	 keyIsString;	/* whether the hash key is a string */
    DtHashEntry *entries;	/* the entries */
}DtHashTableRec;

static DtHashEntryRec DtHashfake;	/* placeholder for deletions */

#define HASH(tab,sig) ((sig) & tab->mask)
#define REHASHVAL(tab, idx) ((((idx) % tab->rehash) + 2) | 1)
#define REHASH(tab,idx,rehash) ((idx + rehash) & tab->mask)
#define KEY(tab, entry) \
  ((*  (tab->types[entry->hash.type]->hash.getKeyFunc) ) \
   (entry, tab->types[entry->hash.type]->hash.getKeyClientData))

#define RELEASE_KEY(tab, entry, key) \
{\
   if (tab->types[entry->hash.type]->hash.releaseKeyProc) \
     (*  (tab->types[entry->hash.type]->hash.releaseKeyProc)) \
       (entry, key); \
     }

static unsigned int 
#ifdef _NO_PROTO
GetTableIndex( tab, key, new )
        register DtHashTable tab ;
        register DtHashKey key ;
        register Boolean new ;
#else
GetTableIndex(
	      register DtHashTable tab,
	      register DtHashKey key,
#if NeedWidePrototypes
	      register int new)
#else
              register Boolean new)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    register DtHashEntry	*entries = tab->entries;
    register int		len, idx, i, rehash = 0;
    register char 		c;
    register Signature 		sig = 0;
    register DtHashEntry	entry;
    String			s1, s2;
    DtHashKey			compKey;

    if (tab->keyIsString) {
	s1 = (String)key;
	for (s2 = (char *)s1; c = *s2++; )
	  sig = (sig << 1) + c;
	len = s2 - s1 - 1;
    }
    else
      sig = (Signature)key;
    
    idx = HASH(tab, sig);
    while (entry = entries[idx]) {
	if (entries[idx] == &DtHashfake) {
	    if (new)
	      return idx;
	    else
	      goto nomatch;
	}
	if (tab->keyIsString) {
	    compKey = KEY(tab, entry);
	    for (i = len, s1 = (String)key, s2 = (String) compKey;
		 --i >= 0; ) {
		if (*s1++ != *s2++)
		  goto nomatch;
	    }
	}
	else {
	    if ((compKey = KEY(tab, entry)) != key)
	      s2 = " ";
	    else
	      s2 = "";
	}
	
	if (*s2) {
nomatch:    
	    RELEASE_KEY(tab, entry, compKey);
	    if (!rehash)
	      rehash = REHASHVAL(tab, idx);
	    idx = REHASH(tab, idx, rehash);
	    continue;
	}
	else
	  RELEASE_KEY(tab, entry, compKey);
	break;
    }
    return idx;
}



void 
#ifdef _NO_PROTO
_XmRegisterHashEntry( tab, key, entry )
        register DtHashTable tab ;
        register DtHashKey key ;
        register DtHashEntry entry ;
#else
_XmRegisterHashEntry(
        register DtHashTable tab,
        register DtHashKey key,
        register DtHashEntry entry )
#endif /* _NO_PROTO */
{
    unsigned int idx;

    if ((tab->occupied + (tab->occupied >> 2)) > tab->mask)
	ExpandHashTable(tab);

    idx = GetTableIndex(tab, key, True);
    if (tab->entries[idx] == &DtHashfake)
      tab->fakes--;
    tab->occupied++;
    tab->entries[idx] = entry;
}

void 
#ifdef _NO_PROTO
_XmUnregisterHashEntry( tab, entry )
        register DtHashTable tab ;
        register DtHashEntry entry ;
#else
_XmUnregisterHashEntry(
        register DtHashTable tab,
        register DtHashEntry entry )
#endif /* _NO_PROTO */
{
    register int 		idx, rehash;
    register DtHashEntry	*entries = tab->entries;
    DtHashKey			key = KEY(tab, entry);

    idx = GetTableIndex(tab, key, False);
    RELEASE_KEY(tab, entry, key);
    entries[idx] = &DtHashfake;
    tab->fakes++;
    tab->occupied--;
}


static void 
#ifdef _NO_PROTO
ExpandHashTable( tab )
        register DtHashTable tab ;
#else
ExpandHashTable(
        register DtHashTable tab )
#endif /* _NO_PROTO */
{
    unsigned int oldmask;
    register DtHashEntry *oldentries, *entries;
    register int oldidx, newidx, rehash, len;
    register DtHashEntry entry;
    register DtHashKey key;

    oldmask = tab->mask;
    oldentries = tab->entries;
    tab->fakes = 0;
    if ((tab->occupied + (tab->occupied >> 2)) > tab->mask) {
	tab->mask = (tab->mask << 1) + 1;
	tab->rehash = tab->mask - 2;
    }
    entries = tab->entries = (DtHashEntry *) XtCalloc(tab->mask+1, sizeof(DtHashEntry));
    for (oldidx = 0; oldidx <= oldmask; oldidx++) {
	if ((entry = oldentries[oldidx]) && entry != &DtHashfake) {
	    newidx = GetTableIndex(tab, key = KEY(tab, entry), True);
	    RELEASE_KEY(tab, entry, key);
	    entries[newidx] = entry;
	}
    }
    XtFree((char *)oldentries);
}


DtHashEntry 
#ifdef _NO_PROTO
_XmEnumerateHashTable( tab, enumFunc, clientData )
        register DtHashTable tab ;
        register DtHashEnumerateFunc enumFunc;
        register XtPointer clientData;
#else
_XmEnumerateHashTable(
        register DtHashTable tab,
	register DtHashEnumerateFunc enumFunc,
        register XtPointer clientData )
#endif /* _NO_PROTO */
{
    register unsigned int i;

    for (i = 0; i <= tab->mask; i++)
      if (tab->entries[i] && 
	  tab->entries[i] != &DtHashfake &&
	  ((*enumFunc) (tab->entries[i], clientData)))
	return tab->entries[i];
    return NULL;
}


DtHashEntry 
#ifdef _NO_PROTO
_XmKeyToHashEntry( tab, key )
        register DtHashTable tab ;
        register DtHashKey key ;
#else
_XmKeyToHashEntry(
        register DtHashTable tab,
        register DtHashKey key )
#endif /* _NO_PROTO */
{
    register int idx, rehash, len;
    register DtHashEntry entry, *entries = tab->entries;

    if (!key) return NULL;
    idx = GetTableIndex(tab, key, False);
    return entries[idx];
}

DtHashTable 
#ifdef _NO_PROTO
_XmAllocHashTable( hashEntryTypes, numHashEntryTypes, keyIsString )
    DtHashEntryType	*hashEntryTypes;
    Cardinal		numHashEntryTypes;
    Boolean 		keyIsString ;
#else
_XmAllocHashTable(DtHashEntryType	*hashEntryTypes,
		   Cardinal		numHashEntryTypes,
#if NeedWidePrototypes
		   int 			keyIsString)
#else
                   Boolean 		keyIsString)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    register DtHashTable tab;

    tab = (DtHashTable) XtMalloc(sizeof(struct _DtHashTableRec));
    tab->types = hashEntryTypes;
    tab->numTypes = numHashEntryTypes;
    tab->keyIsString = keyIsString;
    tab->mask = 0x7f;
    tab->rehash = tab->mask - 2;
    tab->entries = (DtHashEntry *) XtCalloc(tab->mask+1, sizeof(DtHashEntry));
    tab->occupied = 0;
    tab->fakes = 0;
    return tab;
}

void 
#ifdef _NO_PROTO
_XmFreeHashTable( hashTable )
        DtHashTable hashTable ;
#else
_XmFreeHashTable(
        DtHashTable hashTable )
#endif /* _NO_PROTO */
{
    XtFree((char *)hashTable->entries);
    XtFree((char *)hashTable);
}
