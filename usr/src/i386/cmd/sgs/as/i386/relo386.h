#ident	"@(#)nas:i386/relo386.h	1.2"
/* relo386.h */

/* Routines to handle i386-style relocations. */

#ifdef __STDC__
void relocaddr(Eval *, Uchar *, Section *);
void relocpcrel(Eval *, Uchar *, Section *);
#else
void relocaddr();
void relocpcrel();
#endif
