/*	@(#)mkcfiles.c	3.1	8/29/96	21:31:17	*/
/*
 *	@(#) mkcfiles.c 9.2 93/06/22 SCOINC
 *
 *	Copyright (C) The Santa Cruz Operation, 1989-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *	MODIFICATION HISTORY
 *
 *	S003	09 Aug 96	hiramc
 *	- working on Unixware
 *      S002    Thu Dec 23 11:39:05 PST 1993    davidw@sco.com
 *      - vidconf & oa routines going away - hardcode vid.h defines
 *	and don't use oa/error() routine.
 *	Also, hard code viderrs.h and vidstrs.h defines here.
 *	S001	31-Oct-1993 toma@sco.com
 *              - Changed #include "viderrs" to #include "../viderrs" 
 *	          and same for vidstrs.h.
 *	S000	10-June-1993 edb@sco.com
 *              - Created
 */


/************************************************************************* 
************************************************************************** 
**
**  File Name:          mkcfiles.c
**  ---------
**
**  Author:		Ed Bauboeck
**  ------
**
**  Overview:	
**  --------
**  Reads GRAFINFO_DIR/grafdev
**  Creates a list of <vendor>.<model> strings.
**  Scans the corresponding .xgi files 
**  GRAFINFO_DIR/<vendor>/<model>.xgi
**  extracts the PROCEDURE clauses
**  and creates a file with C code
**  GRAFINFO_DIR/<vendor>/<model>.c
**
**  External Functions:
**  ------------------
**  
*************************************************************************** 
***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <malloc.h>

#define SCRNLEN  80					/* S002 vvv */
#define BUFLEN   60             /*max size of char buffer passed to disp_msg */
#define PATHLEN  128            /* path name length */  /* S002 ^^^ */

extern void errexit();

/*************************************************************************
*
*                                Types.
*
**************************************************************************/

typedef struct _adaptorStr
{
    char                  *name;
    struct _adaptorStr     *next;
} adaptorStr;


/*************************************************************************
*
*                          Static global variables.
*
**************************************************************************/


static adaptorStr *adaptorList = (adaptorStr *) NULL;
int		errno;


/*************************************************************************
 *
 *  main ( nr, *grafdir )
 *
 *  Description:
 *  -----------
 *  Opens grafdir[0]/grafdev and creates a list of xgi file names
 *
 *  Arguments:
 *  ---------
 *  if( nr >= 2 ) grafinfo_root = grafdir[1];
 *  else          grafinfo_root = "/usr/lib/grafinfo"
 *
 *  Output:
 *  ------
 *  Returns 1 upon susccessful completion else 0.
 *
 *************************************************************************/


main( nr, grafdir )
int nr;
char *grafdir[];
{
    FILE		*fp;
    char                buf[SCRNLEN];
    char                msg[BUFLEN];
    struct stat		statbuf;
    static char         *grafdir_def = { GRAFINFO_DIR };
    char                *grafinfo_root;
    char                grafdevsFile[PATHLEN];
    char                tty[PATHLEN];
    char                model[PATHLEN];
    char                vendor[PATHLEN];
    char                xgi_file[PATHLEN];
    char                c_file[PATHLEN];
    adaptorStr          *cur_adaptor;
    adaptorStr          *new_adaptor;

    InitClassData();
    if( nr >= 2 ) grafinfo_root = grafdir[1];
    else          grafinfo_root = grafdir_def;
   /*
    *    Check that grafinfoDir exists.
    */
    if (stat(grafinfo_root, &statbuf) == -1)
    {
	sprintf(msg, "%s %s", "Can't stat directory", grafinfo_root);
        errexit(msg);
    }

    sprintf(grafdevsFile ,"%s/grafdev", grafinfo_root);
    if (stat(grafdevsFile, &statbuf) == -1)
    {
	sprintf(msg, "%s %s", "Can't stat file", grafdevsFile);
        errexit(msg);
    }

    if ((fp = fopen(grafdevsFile, "r")) == NULL)
    {
       /*
	* File exists but can't be opened.
	*/
	sprintf(msg, "%s %s", "Unable to read", grafdevsFile); /* S002 vvv*/
	errexit(msg);					       /* S002 ^^^*/
    }

   /*
    * Set up the adaptor list.
    */
    while (fgets(buf, SCRNLEN, fp) != NULL)
    {
        if(sscanf (buf, "%[^:]:%[^.].%[^.]", tty, vendor,model) < 3 )
            continue;

        sprintf( xgi_file, "%s/%s/%s.xgi", grafinfo_root, vendor, model );
        sprintf(   c_file, "%s/%s/%s.c"  , grafinfo_root, vendor, model );

        cur_adaptor = adaptorList;
        while( cur_adaptor != NULL )
        {
            if( strcmp( cur_adaptor->name, xgi_file ) == 0 )
                break;
            cur_adaptor = cur_adaptor->next;
        }
        /*
         *  if( adaptor not yet in list parse and create c_file
         */
        if( cur_adaptor == NULL )
        {
            SetVendor( vendor );
            SetModel(  model  );
            ParseXGIFile( xgi_file, c_file );

            /* add to list */
            new_adaptor = (adaptorStr *)malloc( sizeof( adaptorStr ));
            new_adaptor->name = strdup( xgi_file );
            new_adaptor->next = adaptorList;
            adaptorList    = new_adaptor;
        }
    }

    fclose (fp);
    return 0;
}
