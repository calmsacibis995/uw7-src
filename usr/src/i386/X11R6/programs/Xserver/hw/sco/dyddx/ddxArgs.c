/*
 *	SCO Modification history
 *
 *	S001	Wed Sep 02 16:36:32 PDT 1992	hiramc@sco.COM
 *		properly cast NULL initializers to eliminate compile warnings
 */
#include <ctype.h>
#include "misc.h"
#include "ddxArgs.h"
#include "commonDDX.h"

/***====================================================================***/

typedef struct _ddxArgList {
	char			*name;
	ddxArgDescPtr		 pArgs;
	struct _ddxArgList	*pNext;
} ddxArgList,*ddxArgListPtr;

static	ddxArgList	*argLists=	(ddxArgListPtr)NULL;

/***====================================================================***/

void
ddxRegisterArgs(char *name,ddxArgDescPtr pArgs)
{
ddxArgListPtr	pList;

    if (pArgs!=NULL) {
	pList=	(ddxArgListPtr)Xalloc(sizeof(ddxArgList));
	if (pList!=NULL) {
	    pList->name=	name;
	    pList->pArgs=	pArgs;
	    pList->pNext=	argLists;
	    argLists=	pList;
	}
	else {
	    ErrorF("Warning: allocation failure in ddxRegisterArgs\n");
	    ErrorF("         some options may not be recognized\n");
	}
    }
    return;
}

/***====================================================================***/

void
ddxFlaggedUseMsg(char *name,unsigned flags)
{
int		i;
ddxArgListPtr	pList=	argLists;
ddxArgDescPtr	pArg;

     while (pList) {
	if ((name!=NULL)&&(name[0]!='\0')&&(pList->name!=NULL)&&
					   (strcmp(name,pList->name))) {
	    pList=	pList->pNext;
	    continue;
	}
	pArg=	pList->pArgs;
	while (pArg->argString!=NULL) {
	     if ((argFlags(pArg)==flags)||(argFlags(pArg)&flags)) {
		ErrorF("%s ",pArg->argString);
	        i= strlen(pArg->argString)+1;
		switch (argType(pArg)) {
		    case ARG_INT:	ErrorF("<int> "); i+= 6;
					break;
		    case ARG_STRING:	ErrorF("<string> "); i+= 9;
					break;
		    case ARG_SPECIAL:	ErrorF("... "); i+= 4;
					break;
		    case ARG_OPT_INT:	ErrorF("[<int>] "); i+= 8;
					break;
		    case ARG_OPT_STRING:ErrorF("[<string>] "); i+= 11;
					break;
		    default:		break;
		}
		if (pArg->argDesc!=NULL) {
		    while (i<35) {
			ErrorF(" ");
			i++;
		    }
		    ErrorF("%s",pArg->argDesc);
		}
		ErrorF("\n");
	    }
	    pArg++;
	}
	pList=	pList->pNext;
     }
     return;
}

/***====================================================================***/

extern void scoUseMsg(void);

void
ddxUseMsg()
{
    ddxFlaggedUseMsg(NULL,0);
	scoUseMsg();
    return;
}

/***====================================================================***/


#define	strIsNumber(s)	(isdigit(*(s))||(((*s)=='-')&&(isdigit(*((s)+1)))))

extern int scoProcessArgument (int, char **, int );

int
ddxProcessArgument(int argc,char *argv[],int i)
{
ddxArgListPtr	pList=	argLists;
ddxArgDescPtr	pArg;
int skip ;

#if defined(sco)
	extern Bool MustBeConsole;

	MustBeConsole = TRUE;
#endif

     while (pList) {
	pArg=	pList->pArgs;
	while (pArg->argString!=NULL) {
	    if (!strcmp(argv[i],pArg->argString)) {
		switch (argType(pArg)) {
		    case ARG_NONE:
			*((Bool *)pArg->pSet)= TRUE;
			return(1);
		    case ARG_OPT_INT:
		    case ARG_INT:
			if (i<(argc-1)) {
			    int tmp;
			    if (sscanf(argv[i+1],"%i",&tmp)==1) {
				*((int *)pArg->pSet)= tmp;
				return(2);
			    }
			}
			if (argType(pArg)&ARG_OPTIONAL) {
			    *((int *)pArg->pSet)= pArg->dfltInt;
			    return(1);
			}
			ErrorF("\"%s\" flag needs an integer argument\n",
								argv[i]);
			break;
		    case ARG_OPT_STRING:
		    case ARG_STRING:
			if (i<(argc-1)) {
			    *((char **)pArg->pSet)= argv[i+1];
			    return(2);
			}
			if (argType(pArg)&ARG_OPTIONAL) {
			    *((char **)pArg->pSet)= pArg->dfltStr;
			    return(1);
			}
			ErrorF("\"%s\" flag needs a string argument\n",argv[i]);
			break;
		    case ARG_SPECIAL:
			if (pArg->parser) {
			    skip= (*pArg->parser)(argc,argv,i);
			    if (skip!=0) {
				return(skip);
			    }
			}
			break;
		    default:
			ErrorF("Internal error: weird type 0x%x for arg %d\n",
						argType(pArg),pArg->argString);
			break;
		}
	    }
	    pArg++;
	}
	if ((pArg->argString==NULL)&&(pArg->parser!=NULL)) {
	    skip=	(*pArg->parser)(argc,argv,i);
	    if (skip!=0) 
		return(skip);
	}
	pList=	pList->pNext;
     }
	return ( scoProcessArgument (argc, argv, i ) );
}

/***====================================================================***/

#ifdef SGI
	/* THESE DON'T BELONG HERE.  FIND A HOME FOR THEM SOMEDAY */
char		*corePtrName=   "mouse";
int		 corePtrXIndex= 0;
int		 corePtrYIndex= 1;
unsigned	 RRMBOARDBASE=	0x20000000;
unsigned	 RRMBOARDSIZE=	0x00200000;
#endif

ddxArgDesc commonArgs[]= {

#ifdef SGI
    { "-hw",    ARG_SPECIAL, NULL, 
		"specify screen", ddxAddScreenRequest, 0, NULL },
    { "-depth", ARG_INT,     (pointer)&ddxDfltDepth,
		"depth of default visual(s)", NULL, 0, NULL},
    { "-class", ARG_SPECIAL, NULL, 
		"class of default visual(s)", ddxSetVisualRequest, 0, NULL},
    { "-static", ARG_SPECIAL,	NULL,
		"force use of static DDX layer", ddxSetLoadType, 0, NULL},
    { "-dynamic", ARG_SPECIAL,	NULL,
		"force use of dynamic DDX layer", ddxSetLoadType, 0, NULL},
    { "-boardsize", ARG_INT,     (pointer)&RRMBOARDSIZE, 
		"Amount of space to reserve per board in bytes (hex)", NULL, 0, NULL},
    { "-boardbase", ARG_INT,     (pointer)&RRMBOARDBASE, 
		"Amount of space to reserve per board in bytes (hex)", NULL, 0, NULL},
    { "-wrapx", ARG_NONE,	(pointer)&ddxWrapX,
		"wrap cursor in X direction", NULL, 0, NULL},
    { "-wrapy", ARG_NONE,	(pointer)&ddxWrapY,
		"wrap cursor in Y direction", NULL, 0, NULL},
    { "-ptrdev", ARG_STRING,	(pointer)&corePtrName,
		"Name of core pointer device", NULL, 0, NULL },
    { "-xindex", ARG_INT,	(pointer)&corePtrXIndex,
		"Index of X valuator in pointer device", NULL, 0, NULL},
    { "-yindex", ARG_INT,	(pointer)&corePtrYIndex,
		"Index of Y valuator in pointer device", NULL, 0, NULL},
#endif

#ifdef  DEBUG
    { "-verbose", ARG_INT,	(pointer)&ddxVerboseLevel,
		"Controls how much debugging info is printed", NULL, 0, NULL},
#endif

    { (char *) NULL, (unsigned) NULL, (void *) NULL, (char *) NULL,
		(ddxArgParser) NULL, 0, (char *) NULL }	/*	S001	*/
};

#ifdef SGI
extern ddxArgDesc irixArgs[];
#endif

void
ddxInitArguments()
{
    ddxRegisterArgs("common",commonArgs);
#ifdef SGI
    ddxRegisterArgs("irix",irixArgs);
#endif
    return;
}

/***====================================================================***/

void
ddxHandleDelayedArgs()
{
    return ;
}
