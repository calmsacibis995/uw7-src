/* #ident "@(#)usltools/buildtool.c	1.1" */

/*
 * Copyright 1993 UNIX System Laboratories (USL)
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of USL not be used in 
 * advertising or publicity pertaining to distribution of the software 
 * without specific, written prior permission.  USL make 
 * no representations about the suitability of this software for any purpose.  
 * It is provided "as is" without express or implied warranty.
 *
 * USL DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO 
 * EVENT SHALL USL BE LIABLE FOR ANY SPECIAL, INDIRECT OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* 
	buildtool.c  - simple C frontend to smake/sclean
                       writes tet journal file results
	A build/clean is a test execution whose results depends
        on the return from the smake/sclean tool
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <tet_api.h>

void tbuild();
void (*tet_startup)() = NULL, (*tet_cleanup)() = NULL;

struct tet_testlist tet_testlist[] = { 
	tbuild, 1, 
	NULL, 0 
};

#define BUILDTOOL "buildtool"
#define BUFLEN  512    /* buffer length for command line */
#define SMAKECMD  "smake"
#define SCLEANCMD  "sclean"
#define BUILDARGS  " > bld_res"
#define BUILDRESFILE "bld_res"      /* the same file as in buildargs */
#define BUILDSUCCESS 0   /* successful return from buildtool */


static void tbuild()
{
char command_line[BUFLEN];
char bres_line[BUFSIZ];
int  retstat;
FILE *bres;

        if (strcmp(tet_pname, BUILDTOOL) == 0) {
        	(void) strcpy(command_line, SMAKECMD);
             	(void) strcat(command_line," INSTALL");
             	(void) strcat(command_line,BUILDARGS);
	}
	else { /* if not build tool assume clean tool */
        	(void) strcpy(command_line, SCLEANCMD);
             	(void) strcat(command_line," CLEAN");
             	(void) strcat(command_line, BUILDARGS);
	}
	
	retstat=system(command_line);

	/*
         * check command terminated normally, and
         * then check return from command = SUCCESS 
         */

	if ( WIFEXITED(retstat) && (WEXITSTATUS(retstat) == BUILDSUCCESS) )
                tet_result(TET_PASS);
 	else { 

	/* if build fails then append build output to journal file
         * using tet_infoline
         */

		bres = fopen (BUILDRESFILE, "r");
		if (bres == NULL)
			tet_infoline("Warning unable to read results file.");
		else {
			while (fgets(bres_line,BUFSIZ,bres) != NULL) 
			   tet_infoline(bres_line);
			(void) fclose(bres);
		}
		tet_result(TET_FAIL);	
	}
}


