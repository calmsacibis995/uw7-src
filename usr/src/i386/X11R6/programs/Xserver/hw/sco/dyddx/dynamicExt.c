/*
 *	@(#)dynamicExt.c	6.2	1/17/96	15:05:51
 *
 *	Copyright (C) The Santa Cruz Operation, 1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Fri Sep 17 14:42:22 PDT 1993	mikep@sco.com
 *	- Created file
 *	S001	Thu Nov 11 18:41:15 PST 1993	buckm@sco.COM
 *	- Close opened files.
 *	S002	Mon Nov 15 13:36:52 PST 1993	mikep@sco.com
 *	- Avoid memory leaks on resets
 *	S003	Wed Jan 17 14:19:18 PST 1996	hiramc@sco.COM
 *	- Add dynamic library loading in addition to the dyddxload
 *      S004    Thu Jul 11 02:17:25 GMT 1996    kylec@sco.com
 *      - Use defines for DYEXTCONFIG and XLIBDIR
 *
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>	/*	dynamic libraries	S003	*/

#include <stdio.h>

#include "X.h"
#include "misc.h"
#include "commonDDX.h"

#include "dyddx.h"
#include "ddxScreen.h"

#if ! defined(DYEXTCONFIG)
#define DYEXTCONFIG "/usr/lib/X11/dyddx/extensions.cf"
#endif

#if ! defined(XLIBDIR)
#define	XLIBDIR "/usr/lib/X11"	
#endif

#define LOAD_EXT_PREINIT 0
#define LOAD_EXT_INIT 1
#define LOAD_EXT_CALL 2
#define MAXEXTS 128

typedef struct _extInfo {
	void (* initRoutine)();
	char *extensionName;
	char *fileName;
	char *initRoutineName;
	char loadType;
	char *screenName;
	Bool loaded;
	pointer osPriv;
} extInfo;


static int	dynamicExts = 0;
static extInfo *pDynDDX[MAXEXTS];

static void loadExtFile(extInfo *pext)
{
    int fd;
    struct dynldinfo extinfo;
    extInfo *loadPoint;
    void * handle;
    char modulepath[1024];

    /*
     * If the file name starts with a "/", use the complete path name
     * otherwise look in /usr/lib/X11
     */
    if(pext->fileName[0] == '/')
	strcpy(modulepath, pext->fileName);
    else
	(void)sprintf(modulepath,"%s/%s", XLIBDIR, pext->fileName);

	if ( (handle = dlopen(modulepath, RTLD_NOW)) == NULL ) { /* S003 vvv */
	    ErrorF("dlopen <%s> failed\nReason: %s\n", modulepath, dlerror());
	} else {

	pext->initRoutine = (void (*)()) dlsym(handle, pext->initRoutineName );

		if (pext->initRoutine) {
		    ErrorF( "dynamic .so extension loaded: %s\n", modulepath );
			(*pext->initRoutine)();
			pext->loaded = TRUE;
			return;
		} else {
		   ErrorF("dlsym failed to find %s\n", pext->initRoutineName );
			pext->loaded = FALSE;
			dlclose(handle);
		}
	}
	/*	When above was successful, it has returned already,	*/
	/*	When failed for whatever reason, fall thru, and		*/
	/*		try the older dyddxload method	S003	^^^	*/

    if ( (fd = open( modulepath, O_RDONLY ))>=0 ) {
	pext->initRoutine = (void (*)())dyddxload( fd, &extinfo );
	(void)close( fd );
	if (pext->initRoutine) {
	    (*pext->initRoutine)();
	    pext->loaded = TRUE;
	    ErrorF( "Dynamic .o extension loaded: %s\n", modulepath );
	} else {
	    ErrorF("Couldn't load %s\n");
	    pext->loaded = FALSE;
	}
    } else {
	ErrorF("Couldn't open %s\n", modulepath);
    }

    return;
}


void
loadDynamicExtensions()
{
    FILE *fp;
    void (* loadPoint)() = NULL;
    char *extpath;
    Bool alreadyLoaded= FALSE;
    int	i, j;
    char fbuffer[1024];
    char fileName[80];
    char extensionName[80];
    char initRoutineName[80];
    char loadType[80];
    char screenName[80];

    /*
     * If we've already loaded, just call the init routine
     */
    if (dynamicExts)
	{
	for (j=0; j < dynamicExts; j++)
	    {
	    if (pDynDDX[j]->loaded)
		(pDynDDX[j]->initRoutine)();
	    }
	/* 
	 * This means if you want to add a new extension to the file
	 * you have to kill and restart the X server
	 */
	return;							/* S002 */
	}

    if ((fp = fopen(DYEXTCONFIG, "r")) == NULL)
	{
	ErrorF("Can't open %s\n", DYEXTCONFIG);
	return;
	}
    
    while(fgets(fbuffer, 1024, fp))
	{
	if (fbuffer[0] != '#')
	    {
	    if (sscanf(fbuffer,"%s %s %s %s %s", 
			extensionName, fileName, loadType, 
			initRoutineName, screenName) == 5)
		{
		pDynDDX[dynamicExts] = (extInfo *)xalloc(sizeof(extInfo));
		pDynDDX[dynamicExts]->extensionName = strdup(extensionName);
		pDynDDX[dynamicExts]->fileName = strdup(fileName);
		pDynDDX[dynamicExts]->initRoutineName = strdup(initRoutineName);
		pDynDDX[dynamicExts]->screenName = strdup(screenName);
		pDynDDX[dynamicExts]->loadType = (loadType[0] == 'C') ?
			LOAD_EXT_CALL : LOAD_EXT_INIT;
		pDynDDX[dynamicExts]->loaded = FALSE;
		++dynamicExts;
		}
	    else
		ErrorF("Bad line in %s:\n%s\n", DYEXTCONFIG, fbuffer);

	    if (dynamicExts == MAXEXTS)
		ErrorF("Maximum lines in extension config file reached: %d\n",
				MAXEXTS);
	    }
	}

    fclose(fp);							/* S001 */

    if (dynamicExts == 0) return; /* Nothing interesting in the file */

    /*
     * Load driver specific extensions first
     */
    for ( i = 0 ; i < ddxNumActiveScreens ; i++ )
	{
	for (j=0; j < dynamicExts; j++)
	    {
	    /*
	     * If there is a specific extension for this screen/Driver load it.
	     */
	    if (!strcmp(ddxActiveScreens[i]->screenName,pDynDDX[j]->screenName)
		    && (pDynDDX[j]->loadType == LOAD_EXT_INIT))
		{
		loadExtFile(pDynDDX[j]);
		}
	    }
	}

    /*
     * Now check for generic extensions to be loaded
     */
    for (j=0; j < dynamicExts; j++)
	{
	if ((pDynDDX[j]->screenName[0] == '*')
		    && (pDynDDX[j]->loadType == LOAD_EXT_INIT))
	    {
	    alreadyLoaded = FALSE;
	    /*
	     * If a screen specific version is already loaded, then we
	     * can't use this one.  Eventually, we want this to work
	     * for multiheaded servers.  Lots of issues there.
	     */
	    for (i=0; i < dynamicExts; i++)
		{
		if (!strcmp(pDynDDX[j]->extensionName,
					pDynDDX[i]->extensionName)
			&& (pDynDDX[i]->loaded))
		    {
		    alreadyLoaded = TRUE;
		    break;
		    }
		}
	    if (!alreadyLoaded)
		{
		loadExtFile(pDynDDX[j]);
		}
	    }
	}

}

/*
 * This routine is used to load an extension at first use
 */
Bool
loadExtension(char *name)
{
    int i, j;

    /*
     * Load driver specific extensions first.  Note we're just loading
     * the first extension we find.  This probably won't work for a
     * multiheaded server.
     */
    for ( i = 0 ; i < ddxNumActiveScreens ; i++ )
	{
	for (j=0; j < dynamicExts; j++)
	    {
	    /*
	     * If there is a specific extension for this screen/Driver load it.
	     */
	    if (!strcmp(name,pDynDDX[j]->extensionName)
		    && !strcmp(ddxActiveScreens[i]->screenName,
						pDynDDX[j]->screenName)
		    && (pDynDDX[j]->loadType == LOAD_EXT_CALL))
		{
		if(!pDynDDX[j]->loaded)
		    loadExtFile(pDynDDX[j]);
		return pDynDDX[j]->loaded;
		}
	    }
	}

    /*
     * Now check for generic extensions to be loaded
     */
    for (j=0; j < dynamicExts; j++)
	{
	/* Is the given extension in the list? */
	if (!strcmp(name,pDynDDX[j]->extensionName)
		&& (pDynDDX[j]->screenName[0] == '*') 
		&& (pDynDDX[j]->loadType == LOAD_EXT_CALL))
	    {
	    /*
	     * If a screen specific version is already loaded, then we
	     * can't use this one.  Eventually, we want this to work
	     * for multiheaded servers.  Lots of issues there.
	     */
	    for (i=0; i < dynamicExts; i++)
		{
		if (!strcmp(name, pDynDDX[i]->extensionName) 
			&& (pDynDDX[i]->loaded))
		    return TRUE;
		}
	    if(!pDynDDX[j]->loaded)
		loadExtFile(pDynDDX[j]);
	    return pDynDDX[j]->loaded;
	    }
	}

    return FALSE;

}
