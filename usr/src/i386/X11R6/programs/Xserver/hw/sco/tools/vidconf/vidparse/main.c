#pragma comment(exestr, "@(#) main.c 12.2 95/11/03 ")

/************************************************************************* 
************************************************************************** 
**	Copyright (C) The Santa Cruz Operation, 1989
**	This Module contains Proprietary Information of
**	The Santa Cruz Operation and should be treated as Confidential.
**
**
**  File Name:  	main.c        
**  ---------
**
**  Author:		Kyle Clark
**  ------
**
**  Creation Date:	21 November 1989
**  -------------
**
**  Overview:	
**  --------
**  Parse /usr/lib/grafinfo files and generate
**  /etc/conf/pack.d/cn/class.h
**
**  External Functions:
**  ------------------
** 
**  Data Structures:
**  ---------------
**
**  Bugs:
**  ----
**
*************************************************************************** 
***************************************************************************/

/* MODIFICATION HISTORY */
/*
 * S000	Fri Nov  3 15:16:31 PST 1995	brianm@sco.com
 *	- added in '-g' option which specifies the location of the
 *	  grafinfo files.
 *
 */

#include <stdio.h>
#include "vidparse.h"



/************************************************************************ 
*
*                            External functions
*
*************************************************************************/

int InitClassData();
int ParseFiles(char *);
int WriteClassData(char *);



/*************************************************************************
 *
 *  main ()
 *
 *  Description:
 *  -----------
 *  Initialization, parsing, writing output.
 *
 *  Arguments:
 *  ---------
 *
 *  Output:
 *  ------
 *
 *************************************************************************/

int
main (argc, argv)

int argc;
char *argv[];

{
    extern char *optarg;
    extern int optind;
    int c;

    char *class_file = CLASS_FILE;
    char *grafinfo_dir = GRAFINFO_DIR;

    while ((c = getopt( argc, argv, "g:" )) != -1)
        switch(c)
        {
            case 'g':
                grafinfo_dir = optarg;
                break;
            default:
                fprintf( stderr, "Usage: %s [-g grafinfo_dir] [output_file]\n",
                    argv[0] );
                exit(1);
                /* NOT REACHED */
        }

    if (optind < argc)
        class_file = argv[optind];

    InitClassData();
    ParseFiles(grafinfo_dir);

    WriteClassData(class_file);

    exit(0);
}



