/*
 *	@(#)dynamicScr.c	6.2	1/17/96	15:05:52
 *
 *	Copyright (C) The Santa Cruz Operation, 1992.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Mon Aug 31 19:28:10 PDT 1992	mikep@sco.com
 *	- Create file from statStub.c and move loadDynamicDDX() to here.
 *	This is so staticScr.c can have it's own version that does 
 *	something intelligent.  This will have to be rethought if we
 *	want to mix a static and dynamic server.
 *
 */

/*
 * Stub for a fully dynamic server
 */
#include <sys/types.h>
#include <sys/param.h>
#ifdef sco
#include <dirent.h>
#define MAXPATHLEN MAXNAMLEN
#else
#include <sys/dir.h>
#endif
#include <fcntl.h>
#include <dlfcn.h>	/*	dynamic libraries	*/
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "X.h"
#include "misc.h"
#include "commonDDX.h"

#include "dyddx.h"

#if !defined DYNADDXDIR
#define	DYNADDXDIR "/usr/lib/X11/dyddx"	
#endif

#if !defined DYNADRIVER
#define	DYNADRIVER "XDriver.o"
#endif

#if !defined DYNADRIVERLIB
#define	DYNADRIVERLIB "libXDriver.so"
#endif

#include "ddxScreen.h"

Bool
loadDynamicDDX(ddxScreenRequest *pRequest)
{
int			 fd;
ddxScreenInfo		*loadPoint= (ddxScreenInfo *)NULL,*pDDXInfo;
char			*ddxpath;
char			 modulepath[MAXPATHLEN];
char			 initRoutine[MAXPATHLEN];
struct dynldinfo	 ddxinfo;
Bool			 loaded= FALSE;
Bool			 newDDX= FALSE;
int			 i;
void	* handle;

static	int		nLoadedDynDDX;
static	ddxScreenInfo	*pDynDDX[MAXSCREENS];

    if (((pRequest->ddxLoad!=DDX_LOAD_DYNAMIC)&&
	 (pRequest->ddxLoad!=DDX_LOAD_UNSPEC))||
	((ddxDfltLoadType!=DDX_LOAD_DYNAMIC)&&
	 (ddxDfltLoadType!=DDX_LOAD_UNSPEC))) {
	NOTICE1(DEBUG_INIT,"Skipping dynamic DDX for \"%s\"\n",pRequest->type);
	return(FALSE);
    }

    /* see if we've loaded the DDX layer already */
    for (i=0;(i<nLoadedDynDDX)&&(loadPoint==NULL);i++) {
	if (!strcmp(pDynDDX[i]->pRequest->type,pRequest->type)) {
	    NOTICE2(DEBUG_INIT,"re-using dynamic DDX for \"%s\" (0x%x)\n",
						pRequest->type,loadPoint);
	    loadPoint=	pDynDDX[i];
	}
    }

    if (loadPoint==NULL) {

	/* S005 */
        ddxpath = DYNADDXDIR;

	/* S004 */
    (void)sprintf(modulepath,"%s/%s/%s",ddxpath,pRequest->type,DYNADRIVERLIB);

	if ( (handle = dlopen(modulepath, RTLD_NOW)) == NULL ) {
	    ErrorF("dlopen <%s> failed\nReason: %s\n", modulepath, dlerror());
	} else {
	    (void)sprintf(initRoutine, "%s%s", pRequest->type,  "ScreenInfo" );

		loadPoint = (ddxScreenInfo *) dlsym(handle, initRoutine );

		if ( !loadPoint ) {
		    ErrorF("Couldn't find loadpoint %s\n", initRoutine );
		    dlclose(handle);
		}
	}
	/*	If above was successful, we have a load point,	*/
	/*	otherwise, try again for the older driver types	*/

	if (loadPoint==NULL) {

	(void)sprintf(modulepath,"%s/%s/%s",ddxpath,pRequest->type,DYNADRIVER);
	if ( (fd = open( modulepath, O_RDONLY ))>=0 ) {
	    loadPoint= (ddxScreenInfo *)dyddxload( fd, &ddxinfo );
	    newDDX=	TRUE;
	    NOTICE2(DEBUG_INIT,"using dynamic DDX for \"%s\" from \"%s\"\n",
						  pRequest->type,modulepath);
	    (void) close( fd );
	}
	}
    }
    if (loadPoint&&(*loadPoint->screenProbe)( DDX_DO_VERSION, pRequest )) {
	pDDXInfo= (ddxScreenInfo *)xalloc(sizeof(ddxScreenInfo));
	if (pDDXInfo!=NULL) {
	    *pDDXInfo= *loadPoint;
	    ddxActiveScreens[ddxNumActiveScreens++]=	pDDXInfo;
	    pDDXInfo->pRequest=				pRequest;
	    loadPoint->pRequest=			pRequest;
	    loaded= TRUE;
	    if (newDDX) {
		pDynDDX[nLoadedDynDDX++]=	loadPoint;
	    }
	}
    }

    if ((!loaded)&&(newDDX)) {/* Get rid of it */
	dyddxunload( &ddxinfo ) ;
    }
    return(loaded);
}

Bool
loadStaticDDX(ddxScreenRequest *pRequest)
{
return(FALSE);
}
