#pragma ident	"@(#)m1.2libs:Xm/DragBSI.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.2
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#ifndef _XmDragBSI_h
#define _XmDragBSI_h

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  atoms and targets table structures
 */

typedef struct {
  Atom		atom;
  Time		time;
} xmAtomsTableEntryRec, *xmAtomsTableEntry;

typedef struct {
  Cardinal	numEntries;
  xmAtomsTableEntry entries;
} xmAtomsTableRec, *xmAtomsTable;

typedef struct {
    Cardinal	numTargets;
    Atom	*targets;
}xmTargetsTableEntryRec, *xmTargetsTableEntry;

typedef struct {
    Cardinal	numEntries;
    xmTargetsTableEntry entries;
}xmTargetsTableRec, *xmTargetsTable;

/*
 *  The following are structures for property access.
 *  They must have 64-bit multiple lengths to support 64-bit architectures.
 */

typedef struct {
    CARD32	atom B32;
    CARD16	name_length B16;
    CARD16	pad B16;
}xmMotifAtomPairRec;

typedef struct {
    BYTE	byte_order;
    BYTE	protocol_version;
    CARD16	num_atom_pairs B16;
    CARD32	heap_offset B32;
    /* xmMotifAtomPairRec 	 atomPairs[];	*/
}xmMotifAtomPairPropertyRec;

typedef struct {
    CARD32	atom B32;
    CARD32	time B32;
}xmMotifAtomsTableRec;

typedef struct {
    BYTE	byte_order;
    BYTE	protocol_version;
    CARD16	num_atoms B16;
    CARD32	heap_offset B32;
    /* xmMotifAtomsTableRec atoms[]; 	*/
}xmMotifAtomsPropertyRec;

typedef struct {
    BYTE	byte_order;
    BYTE	protocol_version;
    CARD16	num_target_lists B16;
    CARD32	heap_offset B32;
}xmMotifTargetsPropertyRec;

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmDragBSI_h */
