#ident	"@(#)ksh93:src/lib/libast/include/stack.h	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * homogenous stack routine definitions
 */

#ifndef _STACK_H
#define _STACK_H

typedef struct stacktable* STACK;	/* stack pointer		*/
typedef struct stackposition STACKPOS;	/* stack position		*/

struct stackblock			/* stack block cell		*/
{
	void**		  stack;	/* actual stack			*/
	struct stackblock* prev;	/* previous block in list	*/
	struct stackblock* next;	/* next block in list		*/
};

struct stackposition			/* stack position		*/
{
	struct stackblock* block;	/* current block pointer	*/
	int		index;		/* index within current block	*/
};

struct stacktable			/* stack information		*/
{
	struct stackblock* blocks;	/* stack table blocks		*/
	void*		error;		/* error return value		*/
	int		size;		/* size of each block		*/
	STACKPOS	position;	/* current stack position	*/
};

/*
 * map old names to new
 */

#define mkstack		stackalloc
#define rmstack		stackfree
#define clrstack	stackclear
#define getstack	stackget
#define pushstack	stackpush
#define popstack	stackpop
#define posstack	stacktell

extern STACK		stackalloc(int, void*);
extern void		stackfree(STACK);
extern void		stackclear(STACK);
extern void*		stackget(STACK);
extern int		stackpush(STACK, void*);
extern int		stackpop(STACK);
extern void		stacktell(STACK, int, STACKPOS*);

#endif
