/*		copyright	"%c%" 	*/

#ident	"@(#)adt_evtparse.c	1.2"
#ident  "$Header$"

/* LINTLIBRARY */
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/vnode.h>
#include <sys/mac.h>
#include <sys/systm.h>
#include <sys/privilege.h>
#include <sys/proc.h>
#include <ctype.h>
#include <pfmt.h>
#include <sys/fcntl.h>
#include <sys/resource.h>
#include <audit.h>
#include <sys/auditrec.h>
#include "auditrptv4.h"

/** 
 ** externl routines
 **/
extern void zsetallsys(), zclremask();

/** 
 ** local routines
 **/
static 	void	clremask(), 
		setall();

static	int	isalias();



/**
 ** external variables - defined in auditrptv4.c
 **/
extern	int 	s_type, 		/* no. of event types, classes in map */
		s_class;	
extern 	cls_t 	*clsbegin;		/* beginning of class map */
extern	ids_t	*typebegin;		/* beginning of event type map */

/**
 ** Routine for parsing auditable event option (-e).
 **/
int
evtparse(sp,emask)
char	*sp;
adtemask_t	emask;
{
	char	*evttab[EVTMAX];	/* pointers to events to be set */
	char	*evtlist[EVTMAX]; 
	register char *p;
	int	i, j;
	ent_t	*cur;
	int 	numevt	= 0;
	int 	minus 	= 0;
	int 	found	= 0;
	int	evtnum 	= 0;
	int	class_found;
	cls_t 	*clstbl;
	ids_t	*typetbl;

	if (*sp == '!'){
		setall(emask);
		minus++;
		sp++;
		if (*sp == NULL) {
			(void)pfmt(stderr,MM_ERROR,RE_NOARG,"e");
			usage();
			return(-1);
		}
	} else clremask(emask);
	/* parse arguments to -e and enter specified names to evttab */
	while (sp) {
		if (p = strchr(sp,',')) 
			*p++ ='\0';
		if (isalnum(sp[0])==0){ /*if first char not alphnumeric*/
			(void)pfmt(stderr,MM_ERROR,RE_BAD_EVENT,sp);
			return(-1);
		}
		else{
			if (evtnum < EVTMAX){
				evttab[evtnum++] = sp;
			}
			else{
				(void)pfmt(stderr,MM_ERROR,E_TOOLONG, "-e");
				return(-1);
			}
		}
		sp = p;
	}
	/* process valid class names, and enter event names in evtlist */
	for (i=0; i<evtnum; i++) {
		clstbl=clsbegin;
		if ((isalias(evttab[i],clstbl)) == 1) { /* a class name */
			clstbl=clsbegin;
			class_found = 0;
			while (!class_found) {
				if (strcmp(evttab[i],clstbl->alias)==0){
					cur=clstbl->tp;
					while (cur->tp != NULL) {
						if (numevt < EVTMAX){
							evtlist[numevt++]=cur->type;
							cur=cur->tp;
						}
						else{
							(void)pfmt(stderr,MM_ERROR,E_TOOLONG, "-e");
							return(-1);
						}
					}
					class_found = 1;
				} else {
					clstbl=clstbl->next;
				}
			}
				
		} 
		else{ /* not a class name */
			if (numevt < EVTMAX)
				evtlist[numevt++]= evttab[i];
			else{
				(void)pfmt(stderr,MM_ERROR,E_TOOLONG, "-e");
				return(-1);
			}
		}
	}
	/* validate event names  (in evtlist) */
	for (i=0; i < numevt; i++) {
		found=0;
		typetbl=typebegin;
		for (typetbl=typebegin,j=0; j<s_type; j++) {
			if (strcmp(evtlist[i], typetbl->name)==0) {
				if (minus) 
					EVENTDEL(typetbl->id,emask); 
				else
					EVENTADD(typetbl->id,emask);
				
				found++;
				break;
			} else
				typetbl++;
		}
		if (!found) {
			(void)pfmt(stderr,MM_ERROR,RE_BAD_EVENT,evtlist[i]);
			return(-1);
		}
	}
	return(0);
}

/** 
 ** Set all in event mask emask.
 **/
void
setall(emask)
adtemask_t emask;
{
	int i;
	for (i=1; i<=ADT_NUMOFEVTS; i++) 
		EVENTADD(i, emask);
}

/**
 ** Clear event mask emask.
 **/
void
clremask(emask)
adtemask_t emask;
{
	int i;
	for (i=0; i<ADT_NUMOFEVTS; i++)
		EVENTDEL(i, emask);
}

/**
 ** Determine if the specified event is a class.
 **/
int
isalias(evt,clst)
char	*evt;
cls_t	*clst;
{
	register int i;
	for(i=0; i < s_class; i++) {
		if (strcmp(evt,clst->alias) == 0)
			return(1);
		else
			clst=clst->next;
	}
	return(0);
}

/**
 ** Routine for parsing auditable event option (-z).
 **/
int
zevtparse(sp,emask)
char	*sp;
adtemask_t	emask;
{
	char	*evttab[EVTMAX];	/* pointers to events to be set */
	char	*evtlist[EVTMAX]; 
	register char *p;
	int	i, j;
	ent_t	*cur;
	int 	numevt	= 0;
	int 	minus 	= 0;
	int 	found	= 0;
	int	evtnum 	= 0;
	int	class_found;
	cls_t 	*clstbl;
	ids_t	*typetbl;

	if (*sp == '!'){
		(void)zsetallsys(emask);
		minus++;
		sp++;
		if ((*sp == NULL) || (strcmp(sp, "all") == 0))  {
			(void)pfmt(stderr,MM_ERROR,RE_NOARG,"z");
			usage();
			return(-1);
		}
	} 
	else {
		if (strcmp(sp, "all") == 0)
		{
			(void)zsetallsys(emask);
			return(0);
		}
		else
			(void)zclremask(emask);
	}

	/* parse optarg to -z and enter specified names to evttab */
	/* the optarg may contain events and/or event classes     */          
	while (sp) {
		if (p = strchr(sp,',')) 
			*p++ ='\0';
		if (isalnum(sp[0])==0){ /*if first char not alphnumeric*/
			(void)pfmt(stderr,MM_ERROR,RE_BAD_EVENT,sp);
			return(-1);
		}
		else{
			if (evtnum < EVTMAX){
				evttab[evtnum++] = sp;
			}
			else{
				(void)pfmt(stderr,MM_ERROR,E_TOOLONG, "-z");
				return(-1);
			}
		}
		sp = p;
	}

	/*Loop through evttab[]: If entry is a class, expand the class and */
        /*populate the evtlist[] with the event types. If entry is an event*/
	/*type populate the evtlist[] with the event type.                */
	for (i=0; i<evtnum; i++) {
		clstbl=clsbegin;
		if ((isalias(evttab[i],clstbl)) == 1) { /* a class name */
			clstbl=clsbegin;
			class_found = 0;
			while (!class_found) {
				if (strcmp(evttab[i],clstbl->alias)==0){
					cur=clstbl->tp;
					while (cur->tp != NULL) {
						if (numevt < EVTMAX){
							evtlist[numevt++]=cur->type;
							cur=cur->tp;
						}
						else{
							(void)pfmt(stderr,MM_ERROR,E_TOOLONG, "-z");
							return(-1);
						}
					}
					class_found = 1;
				} else {
					clstbl=clstbl->next;
				}
			}
				
		} 
		else{ /* not a class name */
			if (numevt < EVTMAX)
				evtlist[numevt++]= evttab[i];
			else{
				(void)pfmt(stderr,MM_ERROR,E_TOOLONG, "-z");
				return(-1);
			}
		}
	}

	/*validate evtlist[] entries*/
	for (i=0; i < numevt; i++) {
		found=0;
		typetbl=typebegin;
		for (typetbl=typebegin,j=0; j<s_type; j++) {
			if (strcmp(evtlist[i], typetbl->name)==0) {
				if ((typetbl->id >=128) && (typetbl->id <= 255)) {
					if (minus) 
						EVENTDEL(typetbl->id,emask); 
					else
						EVENTADD(typetbl->id,emask);
				
					found++;
					break;
				}
			}
			typetbl++;
		}
		if (!found) {
			(void)pfmt(stderr,MM_ERROR,RE_BAD_EVENT,evtlist[i]);
			return(-1);
		}
	}
	return(0);
}
