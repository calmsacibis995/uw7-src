#ifndef DDXARGS_H
#define	DDXARGS_H

/***====================================================================***/

	/*\
	 *
	 * Generic command line parser.
	 *
	 * Each software layer may register a structure which describes
	 * all of the command line arguments that layer supports.  When the OS
	 * layer parses the command line, ddxProcessArgument looks through
	 * all registered argument lists (searching the most recently
	 * registered ddxArgList first) for each argument.  Note that
	 * ddxProcessArgument makes *no* attempt to detect conflicts; if
	 * two argument lists define the same string, the definition that
	 * was registered last wins.   
	 *
	 * A command line option consists of the string which introduces the 
	 * flag, and some number (including zero) of additional arguments.   
	 * An argument description (ddxArgDescRec) contains the string
	 * that introduces the argument, a one-line description of the
	 * argument (for a usage message),  a description of any
	 * additional arguments the flag may need, and a pointer to a
	 * variable which is assigned an appropriate value IFF the flag
	 * is specified.   This variable is *not* changed if the flag is
	 * not specified.   The description of any flag with an optional 
	 * argument also includes a default value which is assigned to the
	 * variable if the optional argument is not specified.
	 *
	 * The additional arguments may be any of (note: all of the following
	 * descriptions assume a flag "-flag" which is described by a 
	 * ddxArgDesc named "pArg"):
	 * ARG_NONE -- the flag takes no additional arguments.   An example of
	 *	a flag of type ARG_NONE is:
	 *	   -flag
	 *	which results in:
	 *	   *((Bool *)pArg->pSet)= TRUE;
	 *
	 * ARG_INT  -- the flag takes a single integer argument which must be
	 *	specified.   An example of a flag of type ARG_INT is:
	 *	   -flag 173
	 *	which results in:
	 *	   *((int *)pArg->pSet)= 173;
	 *
	 * ARG_STRING -- the flag takes a single string argument which must be
	 *	specified.   An example use of a flag of type ARG_STRING is:
	 *	    -flag SomeString
	 *      which results in:
	 *	    *((char **)pArg->pSet)= "SomeString";
	 *	IMPORTANT:  ddxProcessArgument does *not* allocate this string
	 *	    it assigns; it merely assigns argv[whatever].  This 
	 *	    string *must not* be freed or modified.
	 * ARG_SPECIAL -- the syntax of the flag is not specified.  The special
	 *	argument parser defined in pArg should parse the command line
	 *	and return the number of arguments it uses.  An example of a
	 *	flag of type ARG_SPECIAL whose parser uses all arguments until
	 *	the special terminator "--" is:
	 *	    command -flag this that the other -- -otherflag
	 *	which results in:
	 *	    skip= (*pArg->parser)(argc,argv,1);
	 *	The parser function should return a count of all arguments it
	 *	processed up to and including the terminator.  In this example,
	 *	the parser function returns 6.   If the parser function returns
	 *	0 (no arguments matched), ddxProcessArguments keeps looking
	 *	for another matching argument.
	 * ARG_OPT_INT -- The flag takes an optional integer argument.  Some
	 *	examples of flags of type ARG_OPT_INT are:
	 *	    -flag 42 or -flag
	 *	which result in:
	 *	    *((int *)pArg->pSet)= 42; 		 or
	 *	    *((int *)pArg->pSet)= pArg->dfltInt; respectively
	 * ARG_OPT_STRING -- The flag takes an optional string argument.  Some
	 *	examples of flags of type ARG_OPT_STRING are:
	 *	    -flag "some string" or -flag
	 *	which result in:
	 *	    *((char **)pArg->pSet)= "some string"; or
	 *	    *((char **)pArg->pSet)= pArg->dfltStr; respectively
	 *	IMPORTANT: ddxProcessArgument does not allocate the string it
	 *	    assigns.  Do not modify or free the string it assigns.
	 *
	 * An argument list is of type (ddxArgDesc *).   
	 * ddxProcessArguments(argc,argv,i) iterates through each argument 
	 * list for each command, comparing all argString fields to argv[i].
	 * Each argument list is terminated by a ddxArgDesc structure with
	 * argName==NULL.
	 * 
	 * 
	 * If the terminal ddxArgDesc has an non-NULL "parser" function, that 
	 * function is called to try to make use of the argument.   If this 
	 * last-ditch parser returns 0 (no arguments matched), 
	 * ddxProcessArguments continues searching the next list, or 
	 * returns 0 if no lists remain.
	 *
	\*/

typedef	int	(*ddxArgParser)(
#ifndef NO_PROTO
	int	 argc,
	char	*argv[],
	int	 i
#endif /* ndef NO_PROTO */
);

typedef	struct _ddxArgDesc {
	char		 	 *argString;
	unsigned	  	  flags;
	void			 *pSet;
	char		 	 *argDesc;
	ddxArgParser	  	  parser;
	int			  dfltInt;
	char			 *dfltStr;
} ddxArgDesc,*ddxArgDescPtr;

#define	ARG_NONE	(0x00)
#define	ARG_INT		(0x01)
#define	ARG_STRING	(0x02)
#define	ARG_SPECIAL	(0x07)
#define	ARG_OPTIONAL	(0x08)

#define	ARG_OPT_INT	(ARG_INT|ARG_OPTIONAL)
#define	ARG_OPT_STRING	(ARG_STRING|ARG_OPTIONAL)
#define	ARG_TYPE_MASK	(0xff)
#define	argType(pa)	((pa)->flags&ARG_TYPE_MASK)
#define	argFlags(pa)	((pa)->flags&(~ARG_TYPE_MASK))

#define	ARG_HIDDEN	(0x0800)

/***====================================================================***/


extern	void	ddxRegisterArgs(
#ifndef NO_PROTO
	char		*name,
	ddxArgDescPtr	pArgs
#endif
);

extern	void	ddxUseMsg();
extern	void	ddxFlaggedUseMsg(
#ifndef NO_PROTO
	char	 *name,
	unsigned  flags
#endif /* ndef NO_PROTO */
);

extern	int	ddxProcessArgument(
#ifndef NO_PROTO
	int	 argc,
	char	*argv[],
	int	 i
#endif /* ndef NO_PROTO */
);

/***====================================================================***/

#endif /* ndef DDXARGS_H */
