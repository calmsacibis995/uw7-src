/*
 *	@(#) symtab.h 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1991.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 */

/*
   @(#) symtab.h 11.1 97/10/22
*/


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
   symtab.h - definitions for symbol table routines
*/

typedef struct SymtabHead {
    struct SymtabEntry *symbols;
    int sorted;
    int curSize;
    int maxSize;
    int incSize;
} SymtabHead;

typedef struct SymtabEntry {
    char *symbol;
    void *pval;
    long lval;
} SymtabEntry;

extern  struct SymtabHead *InitSymtab();
extern  void DestroySymtab();
extern  void AddSymbol();
static  int tabCompare();
extern  struct SymtabEntry *LookupSymbol();
extern  int CheckDupSyms();

/* <<< Function Prototypes >>> */

#if !defined(NOPROTO)

extern  struct SymtabHead *InitSymtab(int );
extern  void DestroySymtab(struct SymtabHead *,int );
extern  void AddSymbol(struct SymtabHead *,char *,long ,void *);
static  int tabCompare(struct SymtabEntry *,struct SymtabEntry *);
extern  struct SymtabEntry *LookupSymbol(struct SymtabHead *,char *);
extern  int CheckDupSyms(struct SymtabHead *);

#endif
