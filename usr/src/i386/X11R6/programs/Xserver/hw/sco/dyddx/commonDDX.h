#ifndef COMMONDDX_H
#define	COMMONDDX_H 1

extern	int	 ddxWrapX;
extern	int	 ddxWrapY;

extern	int	 ddxDfltDepth;
extern	int	 ddxDfltClass;
extern	int	 ddxDfltLoadType;

extern	Bool	 ddxLoad(char *name);
extern	int	 ddxAddScreenRequest(int argc, char *argv[], int index);
extern	int	 ddxSetVisualRequest(int argc, char *argv[], int index );
extern	int	 ddxSetLoadType(int argc, char *argv[], int index );

extern	char	*corePtrName;
extern	int	 corePtrXIndex;
extern	int	 corePtrYIndex;

	/*
	 * DEBUGGING PRINT FUNCTION
	 *   Use the NOTICE macros to get #ifdef DEBUG clutter out of
	 *	code without affecting the product servers.
	 *
	 *   if ((flags&ddxVerboseLevel)!=0) the printf-like format
	 *	string is printed with the defined arguments.
	 *   The value of ddxVerboseLevel is controlled by the -verbose 
	 *   flag.
	 *
	 *   All of this stuff is disabled if the server isn't compiled
	 *   -DDEBUG.
	 */
#ifdef DEBUG

	/*
	 * SYMBOLIC NAMES FOR DEBUGGING FLAGS
	 *   Define symbolic names for debugging values here (e.g. DEBUG_DDX)
	 *      so we don't have collision problems.  Note that we don't
	 *	need values for specific DDX's (yet) because only one is 
	 *	loaded at any time.  We should probably try to conserve 
	 *	flags because we only have 32 of them.
	 *   I've defined debugging flags for the first 4 boards,  so we
	 *	can turn on output from a single board in multi-head
	 *	systems.   I'm allocating board specific flags from the
	 *	low end of the word and the rest from the high end so
	 *	we can avoid collisions for as long as possible.
	 */
#define	DEBUG_BOARD(n)	(1<<(n))
#define	DEBUG_BOARD0	(1<<0)
#define	DEBUG_BOARD1	(1<<1)
#define	DEBUG_DDX	(1<<31)
#define	DEBUG_IRIX	(1<<30)
#define	DEBUG_INIT	(1<<29)
/* Next flag is (1<<28) */

extern	unsigned	ddxVerboseLevel;
extern	void		Notice(unsigned flags,char *format,void *a0,void *a1,
						void *a2,void *a3,void *a4);
#define	NOTICE(fl,fm)		Notice(fl,fm,0,0,0,0,0)
#define	NOTICE1(fl,fm,a0)	Notice(fl,fm,(void *)a0,0,0,0,0)
#define	NOTICE2(fl,fm,a0,a1)	Notice(fl,fm,(void *)a0,(void *)a1,0,0,0)
#define	NOTICE3(fl,fm,a0,a1,a2)	\
	Notice(fl,fm,(void *)a0,(void *)a1,(void *)a2,0,0)
#define	NOTICE4(fl,fm,a0,a1,a2,a3) \
	Notice(fl,fm,(void *)a0,(void *)a1,(void *)a2,(void *)a3,0)
#define	NOTICE5(fl,fm,a0,a1,a2,a3,a4) \
	Notice(fl,fm,(void *)a0,(void *)a1,(void *)a2,(void *)a3,(void *)a4)
#else
#define	NOTICE(fl,fm)
#define	NOTICE1(fl,fm,a0)
#define	NOTICE2(fl,fm,a0,a1)
#define	NOTICE3(fl,fm,a0,a1,a2)
#define	NOTICE4(fl,fm,a0,a1,a2,a3)
#define	NOTICE5(fl,fm,a0,a1,a2,a3,a4)
#endif

#endif /* COMMONDDX_H */
