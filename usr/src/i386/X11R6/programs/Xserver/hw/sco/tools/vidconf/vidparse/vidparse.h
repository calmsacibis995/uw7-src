#pragma comment(exestr, "@(#) vidparse.h 12.1 95/05/09 SCOINC")
/*	@(#)vidparse.h	3.1	8/29/96	21:31:42	*/

/************************************************************************* 
************************************************************************** 
**	Copyright (C) The Santa Cruz Operation, 1989-1993.
**	This Module contains Proprietary Information of
**	The Santa Cruz Operation and should be treated as Confidential.
**
**                              Constants
*************************************************************************/ 

/*
 *	S004	09 Aug 96	hiramc
 *	- working on Unixware
 *	S001	Sept 3 91	pavelr
 *	- added define for MAXPORT
 *	S002	Sept 24 91	pavelr
 *	- added null definition of portlist
 *	S003	June 16 93      edb
 *	- add typedef nameStruct 
 */

#ifdef DEBUG
#undef GRAFINFO_DIR
#define GRAFINFO_DIR		"../grafinfo"
#endif
#define CLASS_FILE		"./class.h"
#define NULL_FILE		"/dev/null"
#define XGI_SUFFIX		".xgi"



#define GOOD			1
#define BAD			0
#define YES			1
#define NO			0

#define ERROR			-1
#define EMPTY			0

#define BUF_LENGTH		125
#define MAX_BUF_LENGTH		256
#define MAXPORT			0x3FF	/* S001 */

#define DOT_C			'.'
#define NULL_C			'\0'
#define SLASH_C			'/'

#define HEADER		\
"\n/*\n\
 *\n\
 *\tTHIS FILE IS MACHINE GENERATED.\n\
 *\n\
 */\n\n\
struct portrange vidcNULL[] = {{ 0, 0 }};\n\n"

#define COPYRIGHT	\
"/*\n\
 *\tCopyright (C) The Santa Cruz Operation, 1989-1993.\n\
 *\tThis module contains Proprietary Information of the Santa\n\
 *\tCruz Operation and should be treated as confidential.\n\
 */\n\n"


#ifdef DEBUG_ALL
#define dbg(x)			fprintf(stderr, x); fflush(stderr);
#else
#define dbg(x)
#endif



typedef struct _nameStruct         /* S003 */
        {
                char * vendor;
                char * class;
                char * model;
                char * mode;
                char * xgiFile;
                char * cFile;
        } nameStruct;
        
