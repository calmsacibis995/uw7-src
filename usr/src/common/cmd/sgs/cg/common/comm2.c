#ident	"@(#)cg:common/comm2.c	1.12.2.8"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "mfile2.h"
#include "arena.h"

# ifndef EXIT
# define EXIT exit
# endif

int nerrors = 0;  /* number of errors */
#ifndef NODBG
int overflow_count; 	/*
			** number of times a freed tree could not be pushed
			** on the tfree_stack
			*/
#endif
int watchdog;

#ifdef STATSOUT
# undef INI_TREESZ
# define INI_TREESZ 1
#endif

/*Number of the lowest node*/
/*Only made nonzero by cg*/
#ifndef	FIRST_NODENO
#define	FIRST_NODENO 0
#endif

static void freestr();

static nNODE *tfree_stack_top;
Arena global_arena; /* function scope arena */

#ifndef NODBG
static nodes=0;	    /* counts nodes in the arena */
#endif

void
tinit()
{
#ifndef NODBG
	nodes = 0;
#endif
	global_arena = arena_init();
	tfree_stack_top = 0;
}

void
tfreeall()
{
	arena_term(global_arena);
	tinit();
}

static nNODE allocated; /*
			** Dummy node that can never be allocated by talloc.  talloc
			** sets the free_list_next field of a node to point here
			** whenever it hands out a node.  This is how we tell
			** a node is properly allocated, not created in some
			** flakey way by acomp or cg.
			*/

#define ALLOCATED &allocated

NODE *
talloc()
{
	nNODE *rtn = 0;

		/*
		** Pop the stack until we find a tfreed node that has
		** not been scribbled on behind our backs.  
		*/

	for(; tfree_stack_top; tfree_stack_top=tfree_stack_top->free_list_next) {
		if (tfree_stack_top->node.in.op == FREE) {
			rtn = tfree_stack_top;
			tfree_stack_top = tfree_stack_top->free_list_next;
			break;
		}
	}

	if(! rtn ) {
		rtn = Arena_alloc(global_arena,1,nNODE);
	}

#ifndef NODBG
	rtn->_node_no = nodes++;
#endif

	rtn->node.in.strat = 0;
	rtn->node.in.name = 0;
	rtn->node.in.goal = 0;
	rtn->node.in.op = NOT_FREE;
	rtn->node.fn.opt = 0;
	rtn->free_list_next = ALLOCATED; 
	rtn->node.in.branch_stuff = 0;
	return (NODE *)rtn;
}

int
tcheck()
{
	 /* ensure that all nodes have been freed */

	int count = 0;
	if( !nerrors ) {

		/*
		** In days of old, when men were bold, we
		** checked here that all nodes had "FREE" written
		** to them.
		*/
	}
	else
	    tinit();
	freestr();
	return count;
}

void
tshow()
{
		/*print all nodes that have not been freed*/
}

/* nfree(p) puts p on the free list.  Since code used to free a
** node by doing p->op = FREE, it used to make the assumption that 
** other fields of p could be dereferenced until talloc() was called.
** nfree() will keep this assumption valid.
*/
void
nfree(p)
NODE *p;
{
	/* Free a single node */
	p->in.op = FREE;

	/* Make sure it was really allocated by us */

	if( ((nNODE *)p)->free_list_next == ALLOCATED ) {
		((nNODE *)p)->free_list_next = tfree_stack_top;
		tfree_stack_top = (nNODE *)p;
	}
}
	
void  
tfree(p)
register NODE *p; 
{
	/* allow tree fragments to be freed, also */
	if( !p ) return;
	switch( optype( p->tn.op ) )
	{
	case BITYPE:
		tfree( p->in.right );
		/*FALLTHRU*/
	case UTYPE:
		tfree( p->in.left );
	}
	nfree(p);
}

char	ftitle[100] = "\"\"";	/* title of the file */
extern int	lineno;		/* line number of the input file */


#undef	NTSTRBUF

#ifndef	TSTRSZ
#define TSTRSZ		2048
#endif

static char istrbuf[TSTRSZ];
static char * ini_tstrbuf[INI_NTSTRBUF] = { istrbuf };
static TD_INIT( td_tstr, INI_NTSTRBUF, sizeof(char *),
		TD_ZERO, ini_tstrbuf, "temp strings table");
#define	tstrbuf ((char **)(td_tstr.td_start))
#define	NTSTRBUF (td_tstr.td_allo)
#define	curtstr (td_tstr.td_used)	/* number of FILLED string buffers */
static int tstrused;


/* Reset temporary string space. */
static void
freestr()
{
    curtstr = 0;
    tstrused = 0;
}


char *
tstr( cp )			/* place copy of string into temp storage */
	register char *cp;	/* strings longer than TSTRSZ will break tstr */
{
	register int i = strlen( cp );
	register char *dp;

	if ( tstrused + i >= TSTRSZ )
	{
		/* not enough room in current string buffer */
		if (++curtstr >= NTSTRBUF)	/* need to enlarge tree ptrs */
		    td_enlarge(&td_tstr, 0);
		tstrused = 0;			/* nothing used in this buffer */
		if ( tstrbuf[curtstr] == 0 )	/* allocate one if not there */
		{
			if ( ( dp = (char *) malloc( TSTRSZ ) ) == 0 )
				cerror(gettxt(":608", "out of memory [tstr()]" ));
			tstrbuf[curtstr] = dp;
		}
	}
	strcpy( dp = tstrbuf[curtstr] + tstrused, cp );
	tstrused += i + 1;
	return ( dp );
}

#include "dope.h"
