/*
 *	@(#) dyddx.h 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *	S001	Mon Aug 19          PST 1991	pavelr@sco.com
 *	- changed ptr definition to be tighter and made the file
 *        compileable with rcc
 *	S002	Tue Oct 13 18:25:06 PDT 1992	mikep@sco.com
 *	- add DYNA_EXTERN macro.
 *	S003	Tue Nov 08          PST 1994	davidw@sco.com
 *	- rcc is gone, use a different define name.
 */
/* dynamic linking definitions */


typedef struct _symbolDef {
	char *name;
	void *(* ptr) ();	/* S001 */
} symbolDef;

struct dynldinfo {
	void *base ;
} ;


#ifndef GLOBALSFILE	/* S001/S003 */
extern void *(* dyddxload( int , struct dynldinfo * ) )() ;
extern void dyddxunload( struct dynldinfo *infop );
#endif

/*
 * These aren't all necessarily function calls, but this
 * is the easiest way to define them.
 */
#define DYNA_EXTERN(name)   extern void *name()
