/*
 *	@(#) symtab.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1991.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *
 */
#if defined(SCCS_ID)
static char Sccs_Id[] = 
	 "@(#) symtab.c 11.1 97/10/22
#endif

/*
   (c) Copyright 1989 by Locus Computing Corporation.  ALL RIGHTS RESERVED.

   This material contains valuable proprietary products and trade secrets
   of Locus Computing Corporation, embodying substantial creative efforts
   and confidential information, ideas and expressions.  No part of this
   material may be used, reproduced or transmitted in any form or by any
   means, electronic, mechanical, or otherwise, including photocopying or
   recording, or in connection with any information storage or retrieval
   system without permission in writing from Locus Computing Corporation.
*/

/*
   symtab.c - Symbol table handler
*/


#include <stdio.h>
#include <string.h>
#include <search.h>

#include "symtab.h"
#include "alloc.h"

SymtabHead *
InitSymtab(sizeIncr)
int sizeIncr;
{
    SymtabHead *tab;
    
    tab = (SymtabHead *)AllocMem(sizeof(SymtabHead));
    
    tab->symbols = NULL;
    tab->sorted = 0;
    tab->curSize = 0;
    tab->maxSize = 0;
    tab->incSize = sizeIncr;
    
    return tab;
}

void
DestroySymtab(tab, freePvals)
SymtabHead *tab;
int freePvals;
{
    int size;
    SymtabEntry *ent;
    
    size = tab->curSize;
    for (ent=tab->symbols; size--; ent++) {
	FreeMem(ent->symbol);
	if (freePvals) FreeMem(ent->pval);
    }
    
    FreeMem(tab->symbols);
    FreeMem(tab);
}

void
AddSymbol(tab, sym, lval, pval)
SymtabHead *tab;
char *sym;
long lval;
void *pval;
{
    SymtabEntry *ent;
    
    if (tab->curSize >= tab->maxSize) {
	tab->maxSize += tab->incSize;
	tab->symbols = (SymtabEntry *)ReallocMem(tab->symbols, sizeof(SymtabEntry) * tab->maxSize);
    }
    
    ent = &tab->symbols[tab->curSize++];
    ent->symbol = StrDup(sym);
    ent->lval = lval;
    ent->pval = pval;
    
    tab->sorted = 0;
}

static int
tabCompare(ent1, ent2)
SymtabEntry *ent1;
SymtabEntry *ent2;
{
    return strcmp(ent1->symbol, ent2->symbol);
}

SymtabEntry *
LookupSymbol(tab, sym)
SymtabHead *tab;
char *sym;
{
    SymtabEntry dent;
    
    if (tab->curSize == 0) return NULL;
    
    if (!tab->sorted) {
	qsort(tab->symbols, tab->curSize, sizeof(SymtabEntry), tabCompare);
	tab->sorted = 1;
    }
    
    dent.symbol = sym;
    
    return (SymtabEntry *)bsearch(&dent, tab->symbols, tab->curSize, 
		sizeof(SymtabEntry), (int (*)())tabCompare);
}

int
CheckDupSyms(tab)
SymtabHead *tab;
{
    int size;
    SymtabEntry *ent;
    int dups;
    
    if (!tab->sorted) {
	qsort(tab->symbols, tab->curSize, sizeof(SymtabEntry), tabCompare);
	tab->sorted = 1;
    }
    
    dups = 0;
    size = tab->curSize - 1;
    for (ent=tab->symbols; size-- > 0; ent++) {
	if (strcmp(ent[0].symbol, ent[1].symbol) == 0) {
	    fprintf(stderr, "Doubly defined: %s\n", ent[0].symbol);
	    dups++;
	}
    }
    
    return dups;
}
