#pragma comment(exestr, "@(#) parse.c 12.2 95/09/06 SCOINC")

/************************************************************************* 
************************************************************************** 
**	Copyright (C) The Santa Cruz Operation, 1989-1993
**	This Module contains Proprietary Information of
**	The Santa Cruz Operation and should be treated as Confidential.
**
**
**  File Name:  	parse.c        
**  ---------
**
**  Author:		Kyle Clark
**  ------
**
**  Creation Date:	23 Nov. 1989
**  -------------
**
**  Overview:	
**  --------
**  Implements all functions for parsing *.xgi files.
**
**  External Functions:
**  ------------------
** 
**  Data Structures:
**  ---------------
**
**  Bugs:
**  ----
 * Modification History:
 *	S015    06-Sep-95 davidw@sco.com
 *	- Exact match only of XGI_SUFFIX and remove S014 - custom changed.
 *      S014    15-Dec-94 davidw@sco.com
 *	- ignore custom backfiles - files starting with # - #vga.xgi.
 *      S013    08-Nov-94 davidw@sco.com
 *	- Correct NULL pointer dereferincing.
 *      S012    03-Jan-94 davidw@sco.com
 *	- Commented out "Parsing grafinfo files" string for vidconfGUI.
 *      S011    21-July-93 edb@sco.com
 *      - Change numbering of duplicate function names
 *      S010    29-June-93 edb@sco.com
 *	- Put #defines into the include file mkcfiles.h
 *	  and change initCFiles() accordingly
 *	  Change the declaration for r1.. r63 to be an array
 *      S009    18-June-93 edb@sco.com
 *      - Add printGlobals() and related code
 *      S008    17-June-93 edb@sco.com
 *	- Fix broken vidparse
 *      S007    16-June-93 edb@sco.com
 *	- Changes to allow arguments in PROCEDURE clause
 *      S006    15-June-93 edb@sco.com
 *      - fprintf CR after last procedure
 *      S005    15-June-93 edb@sco.com
 *      - Add   ParseProcedure(), InitCfile() 
 *	S004	Sun Nov 08 11:21:12 PST 1992	buckm@sco.com
 *	- Do a closedir() for every opendir().
 *	S003	Tue Oct 06 12:22:32 PDT 1992	buckm@sco.com
 *	- Add support for named memory.
 *	  This changes the handling of multiple memory ranges.
 *	S002	Fri Oct 02 11:22:27 PDT 1992	buckm@sco.com
 *	- Generate extra class names for handling multiple memory ranges.
 *	- Add parsing of port ranges and groups.
 *	- Make class, vendor, model parsing more like graflib.
 *	- Use sscanf instead of HexToInt.
 *	- Eliminate stdin/stdout mucking; set yyin/yyout as needed.
 *	S001	Sat Jun 20 12:52:11 PDT 1992	mikep@sco.com
 *	- Parsing is a word, not an acronym.
 *     10 June 1990  kylec   M000 
 *     Modified ParseMemory() to parse memory data as base + length
 *
**
*************************************************************************** 
***************************************************************************/

#include "vptokens.h"
#include "vidparse.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <malloc.h>


/************************************************************************ 
*
*                         External variables
*
*************************************************************************/ 

extern FILE *yyin, *yyout;					/* S002 */
extern char yytext[];
extern int yyleng;

extern char vmcm_buf[];						/* S002 */



/************************************************************************ 
*
*                         External functions
*
*************************************************************************/ 

extern int errexit(char *);
extern int warning(char *);
extern int SetCurrentFile(char *);
extern int AddPort(unsigned long, char *);
extern int SetBaseAddress(unsigned long, char *); 	/* M000 */
extern int SetMemoryLength(unsigned long, char *); 	/* M000 */



/************************************************************************ 
*
*                       Forward declarations
*
*************************************************************************/ 

int ParseFiles(char *);
int ParseXGIFile(char *, char *);
int ParsePort(char *);
int ParseMemory(char *);
int ParseProcedure( FILE * );
char *ParseClass();
static void initCFile( char *, FILE *);
void printDeclaration( FILE * );



/************************************************************************ 
*
*                           Global Data
*
*************************************************************************/ 

typedef struct _procStr
        {
           char * mode_string;
           char * proc_name;
           char * fun_name;
           int ( * function)();
           struct _procStr * next;
        } procStr;

static nameStruct   currentData = {  "",  "",  "", "", "", "" }; /* S013 */
       nameStruct * currentStr  = &currentData;

int currentToken = NO_TOKEN;
int in_procedure = 0;

static char vendor[BUF_LENGTH];					/* S002 */
static char model[BUF_LENGTH]; 					/* S002 */

static procStr *procListHead;
static procStr *procListLast;



/*************************************************************************
 *
 *  ParseFiles()
 *
 *  Description:
 *  -----------
 *  For each *.XGI_SUFFIX file in the grafinfoDir tree, call ParseXGIFile
 *  to parse it.
 *
 *  Arguments:
 *  ---------
 *  grafinfoDir - directory containing subdirectories with 
 *  *.XGI_SUFFIX files.
 *
 *  Output:
 *  ------
 *  GOOD - if succerssful
 *  BAD - if unsuccesful
 *
 *************************************************************************/

int
ParseFiles(grafinfoDir)

char *grafinfoDir;

{
    char subDirName[BUF_LENGTH];
    char xgiFile[BUF_LENGTH];
    char *fileSuffix;
    char msg[MAX_BUF_LENGTH];
    DIR *topDir_p;
    struct dirent *dirEntry_p;
    struct dirent *subDirEntry_p;
    DIR *subDir_p;
    struct stat stat_buf;


    dbg("\n*ParseFiles*\n");
    /*fprintf(stdout, "Parsing grafinfo files\n");*/

    if ((topDir_p = opendir(grafinfoDir)) == NULL)
    {
	sprintf(msg, "%s %s", "cannot open", grafinfoDir);
	errexit(msg);
    }

    /* S002 */
    if ((yyout = fopen(NULL_FILE, "a+")) == NULL)
    {
	sprintf(msg, "system: cannot open %s", NULL_FILE);
	errexit(msg);
    }

    while ((dirEntry_p = readdir(topDir_p)) != NULL)
    {

        sprintf(subDirName, "%s/%s", grafinfoDir, dirEntry_p->d_name);

	if (dirEntry_p->d_name[0] == DOT_C)
	    continue;

	if (stat(subDirName, &stat_buf) == ERROR)
	{
	    sprintf(msg, "%s %s", "can't stat", subDirName);
	    warning(msg);
	    continue;
	}

        if ((stat_buf.st_mode & S_IFMT) != S_IFDIR)
	    continue;

	if ((subDir_p = opendir(subDirName)) == NULL)
	{
	    sprintf(msg, "%s %s", "cannot open", subDirName);
	    warning(msg);
	    continue;
	}

	SetVendor(dirEntry_p->d_name);				/* S002 */

	while ((subDirEntry_p = readdir(subDir_p)) != NULL)
	{
	    /* custom backup files begin with '#' ("#vga.xgi") - ignore them */
	    if ((subDirEntry_p->d_name[0] == '#') || \
	       ((fileSuffix = strrchr(subDirEntry_p->d_name, DOT_C)) == NULL)) 
		continue;

            if (strcmp(fileSuffix, XGI_SUFFIX) == 0)		/* S015 */
	    {

	       /*
	        * We've found a grafinfo.xgi file!
	        */

		sprintf(xgiFile, "%s/%s", subDirName, subDirEntry_p->d_name);

                if (stat(xgiFile, &stat_buf) == ERROR)
		{
	            sprintf(msg, "%s %s", "can't stat", xgiFile);
	            warning(msg);
		}
	        else if ((stat_buf.st_mode & S_IFMT) == S_IFREG)
		{
		    *fileSuffix = NULL_C;			/* S002 */
		    SetModel(subDirEntry_p->d_name);		/* S002 */
	            ParseXGIFile(xgiFile, NULL);
		}
	    }
		    
	}

	closedir(subDir_p);					/* S004 */

    }

    closedir(topDir_p);						/* S004 */

    return(GOOD);
}






/*************************************************************************
 *
 *  ParseXGIFile()
 *
 *  Description:
 *  -----------
 *  Uses lex to parse an xgi file.  Data is saved for writing to class.h
 *
 *  Arguments:
 *  ---------
 *  xgiFile - .xgi file to parse
 *
 *  Output:
 *  ------
 *  cFile   - .c   file to receive C-code in procedures
 *                 if( cFile == NULL ) no C-code output
 *
 *************************************************************************/


int
ParseXGIFile(xgiFile, cFile)

char *xgiFile;
char *cFile;

{
    FILE *fpc;
    char *class = NULL;


    dbg("\n*ParseXGIFile*\n");

    /*
     * Save name of current file for error messages.
     */
    SetCurrentFile(xgiFile);

    /*
     * Open xgi file for reading
     */
    if ((yyin = fopen(xgiFile, "r")) == NULL)			/* S002 */
    {
	warning("cannot open");
        SetCurrentFile((char *)NULL);
	return(BAD);
    }

    /*
     * Open c file , truncate or create for update
     */
    fpc = NULL;
    if( cFile != NULL )
    {
        if ((fpc = fopen(cFile, "w+")) == NULL)			/* S002 */
        {
	     warning("cannot open c-File");
        }
        else
        {
            initCFile( cFile , fpc );
            procListHead = NULL;
            procListLast = NULL;
        }
    }
    /*
     * Parse file.
     */
    Match(NO_TOKEN);
    while (!feof(yyin)) 
    {
	switch (currentToken)
	{
	    case VENDOR_TOKEN:
	    {
		ParseVendor();					/* S002 */
		break;
	    }

	    case MODEL_TOKEN:
	    {
		ParseModel();					/* S002 */
		break;
	    }

	    case CLASS_TOKEN:
	    {
	        if (class != (char*)NULL)
		    free((char*)class);

	        class = ParseClass();
	        break;
	    }

	    case MODE_TOKEN:
	    {
		ParseMode();					/* S002 */
		break;
	    }

	    case MEMORY_TOKEN:
	    {
	        Match(MEMORY_TOKEN);
	        ParseMemory(class);
	        break;
	    }
    
	    case PORT_TOKEN:
	    {
	        Match(PORT_TOKEN);
	        ParsePort(class);
	        break;
	    }

            case PROCEDURE_TOKEN:                               /* S005 */
	    {
                in_procedure = 1;                               /* S008 */
	        Match(PROCEDURE_TOKEN);
	        ParseProcedure( fpc );
                in_procedure = 0;                               /* S008 */
	        break;
	    }

	    default:
	    {
	        Match(currentToken);
	        break;
	    }
	}
    }

    /*
     * Close files
     */
    if( fpc != NULL )
    {
         printGlobals( fpc );
         fclose( fpc );
    }
    SetCurrentFile((char *)NULL);
    fclose(yyin);						/* S002 */

    return(GOOD);
}





/*************************************************************************
 *
 *  ParseClass()
 *
 *  Description:
 *  -----------
 *  Get class name and comment, add them to database.
 *
 *  Arguments:
 *  ---------
 *
 *  Output:
 *  ------
 *  Returns a pointer to the class name.
 *
 *************************************************************************/

							/* vvv S002 vvv */
char *
ParseClass()

{
    char *class;
    int i=0;

    dbg("\n*ParseClass*\n");
    if ((class = strdup(vmcm_buf)) == (char *)NULL)
	errexit("out of memory");
	
   /*
    * Add class name to database.
    */
    AddClass(class);

    while (vmcm_buf[i] != NULL_C)
    {
	vmcm_buf[i] = tolower(vmcm_buf[i]);
	i++;
    }
    if( strcmp( currentStr->class, vmcm_buf ) != 0 )
    {
         currentStr->class = strdup( vmcm_buf );
    } 

    Match(CLASS_TOKEN);

    if (currentToken == STRING_TOKEN)
    {
       /*
	* Add comment to class name.
	*/
	AddComment(yytext + 1, class);
	Match(STRING_TOKEN);
    }

    return(class);
}
							/* ^^^ S002 ^^^ */





/*************************************************************************
 *
 *  ParseMemory()
 *
 *  Description:
 *  -----------
 *  Get memory range for specified class.
 *
 *  Arguments:
 *  ---------
 *  class - current class being configured
 *
 *  Output:
 *  ------
 *
 *  Bugs:
 *  ----
 *  From the doc it isn't clear how value ranges are specified.
 *  For the time being we assume only a list of memory values is
 *  provided.
 *
 *  M000 - Memory is specified as base address plus length.  Changes
 *         are made here.
 *
 *************************************************************************/

							/* vvv S003 vvv */
int
ParseMemory(class)

char *class;

{
    unsigned long value;
    char memclass[BUF_LENGTH];

    dbg("\n*ParseMemory*\n");
    if (class == NULL)
    {
	warning("missing CLASS name specifier for MEMORY addresses");
	return(BAD);
    }

    strcpy(memclass, class);

    if (currentToken == IDENT_TOKEN)
    {
	strcat(memclass, yytext);
	AddClass(memclass);
	Match(IDENT_TOKEN);
    }

    if (currentToken == NUMBER_TOKEN)
    {
       /*
	* Set base of memory address.
	*/
	sscanf(yytext, "%i", &value);
	SetBaseAddress(value, memclass);
	Match(NUMBER_TOKEN);

        while (currentToken == NUMBER_TOKEN)
        {
           /*
	    * Save memory length.
	    */
	    sscanf(yytext, "%i", &value);
	    SetMemoryLength(value, memclass);
	    Match(NUMBER_TOKEN);
        }
    }

    return(GOOD);
}
							/* ^^^ S003 ^^^ */


/*************************************************************************
 *
 *  ParsePort()
 *
 *  Description:
 *  -----------
 *  Get port addresses for specified class.
 *
 *  Arguments:
 *  ---------
 *  class - current class being configured
 *
 *  Output:
 *  ------
 *
 *  Bugs:
 *  ----
 *
 *************************************************************************/

							/* vvv S002 vvv */
int
ParsePort(class)

char *class;
{
    unsigned long value, endvalue, temp;
    long count;

    dbg("\n*ParsePort*\n");
    if (class == NULL)
    {
	warning("missing CLASS name specifier for PORT addresses");
	return(BAD);
    }

    while ((currentToken == NUMBER_TOKEN) || (currentToken == IDENT_TOKEN))
    {
	if (currentToken == NUMBER_TOKEN)
	{
	    sscanf(yytext, "%i", &value);
	    Match(NUMBER_TOKEN);

	    switch (currentToken)
	    {
		case DASH_TOKEN:
		{
		    Match(DASH_TOKEN);

		    if (currentToken != NUMBER_TOKEN)
		    {
			warning("badly formed PORT range");
			return(BAD);
		    }
		    sscanf(yytext, "%i", &endvalue);
		    Match(NUMBER_TOKEN);

		    if (value > endvalue)
		    {
			temp = value;
			value = endvalue;
			endvalue = temp;
		    }
		    while (value <= endvalue)
			AddPort(value++, class);
		    break;
		}
		case COLON_TOKEN:
		{
		    Match(COLON_TOKEN);

		    if (currentToken != NUMBER_TOKEN)
		    {
			warning("badly formed PORT range");
			return(BAD);
		    }
		    sscanf(yytext, "%i", &count);
		    Match(NUMBER_TOKEN);

		    while (--count >= 0)
			AddPort(value++, class);
		    break;
		}
		default:
		{
		    AddPort(value, class);
		    break;
		}
	    }
	}
	else	/* IDENT_TOKEN */
	{
	    if (strcmp(yytext, "VGA") == 0)
	    {
		value = 0x3B0;
		count = 0x30;
		while (--count >= 0)
		    AddPort(value++, class);
	    }
	    else if (strcmp(yytext, "EFF") == 0)
	    {
		/*
		 *  CHEAT  CHEAT  CHEAT
		 *  since most of the 8514 ports are above MAXPORT,
		 *  just add the ones below.
		 */
		value = 0x2E8;
		count = 6;
		while (--count >= 0)
		    AddPort(value++, class);
	    }

	    Match(IDENT_TOKEN);
	}

    }	/* end while */
    return(GOOD);
}
							/* ^^^ S002 ^^^ */



/*************************************************************************
 *
 *  ParseVendor()
 *
 *  Description:
 *  -----------
 *  This function is just being used for error checking.  Want to 
 *  verify that the vendor name specified in the file (VENDOR vendor_name)
 *  matches that specified in the name of the file.
 *
 *  Arguments:
 *  ---------
 *
 *  Output:
 *  ------
 *  If vendor names are different then prints an error message.
 *
 *************************************************************************/

							/* vvv S002 vvv */
int
ParseVendor()

{
    register int i = 0;

    while (vmcm_buf[i] != NULL_C)
    {
	vmcm_buf[i] = tolower(vmcm_buf[i]);
	i++;
    }

    if (strcmp(vendor, vmcm_buf) != 0)
    {
	warning("vendor name specified incorrectly");
    }

    if( strcmp( currentStr->vendor, vmcm_buf ) != 0 )
    {
         currentStr->vendor = strdup( vmcm_buf );
    } 

    Match(VENDOR_TOKEN);
}
							/* ^^^ S002 ^^^ */



/*************************************************************************
 *
 *  ParseModel()
 *
 *  Description:
 *  -----------
 *  This function is just being used for error checking.  Want to 
 *  verify that the model name specified in the file (MODEL model_name)
 *  matches that specified in the name of the file.
 *
 *  Arguments:
 *  ---------
 *
 *  Output:
 *  ------
 *  If model names are different then prints an error message.
 *
 *************************************************************************/

							/* vvv S002 vvv */
int
ParseModel()

{
    register int i = 0;

    while (vmcm_buf[i] != NULL_C)
    {
	vmcm_buf[i] = tolower(vmcm_buf[i]);
	i++;
    }

    if (strcmp(model, vmcm_buf) != 0)
    {
	warning("model name specified incorrectly");
    }

    if( strcmp( currentStr->model, vmcm_buf ) != 0 )
    {
         currentStr->model = strdup( vmcm_buf );
    } 

    Match(MODEL_TOKEN);
}

/*************************************************************************
 *
 *  ParseMode()
 *
 *  Description:
 *  -----------
 *
 *  Arguments:
 *  ---------
 *
 *  Output:
 *  ------
 *  Store mode_name in global structure 
 *
 *************************************************************************/

int
ParseMode()

{
    register int i = 0;
    char * mode;

    while (vmcm_buf[i] != NULL_C)
    {
	vmcm_buf[i] = tolower(vmcm_buf[i]);
	i++;
    }

    if( strcmp( currentStr->mode, vmcm_buf ) != 0 )
    {
         currentStr->mode = strdup( vmcm_buf );
    } 

    Match(MODE_TOKEN);
}
							/* ^^^ S002 ^^^ */

ParseProcedure( fpc )                                   /* vvv S005 vvv */
FILE * fpc;
{
	char *cp;
        int i, procCount;
        char procName[BUF_LENGTH];
        char funName[BUF_LENGTH];
        char modeString[BUF_LENGTH];
        procStr * procList;

        if( currentToken != IDENT_TOKEN ) 
        {
              warning("procedure identifier missing");
              return;
        } 
        strcpy( procName , yytext);
        Match(IDENT_TOKEN);       /* go on to the next token */

        /*
         *   Save strings  to create global structure at end of file 
         */
        if( fpc != NULL )
        {
              procCount = 0;                            /* vvv S011 vvv*/
              procList = procListHead;
              while( procList != NULL )
              {
                    if( strcmp( procList->proc_name, procName ) == 0 )
                          procCount++;
                    procList = procList->next;
              }
              /* if duplicate procName make it unique by adding a number */
              if( procCount > 0 )
                   sprintf( funName, "%s_%d", procName, procCount);
              else
                   strcpy(  funName, procName);          /* ^^^ S011 ^^^*/
 
              sprintf( modeString, "%s.%s.%s.%s", 
                currentStr->vendor, currentStr->model, currentStr->class, currentStr->mode );

              fprintf( fpc, "\n");
              fprintf( fpc, "/*\n");
              fprintf( fpc, " *     %s()  for mode %s\n", procName, currentStr->mode );
              fprintf( fpc, " */\n");
              fprintf( fpc, "static int\n");
              fprintf( fpc, "%s", funName);
              /* 
               *  If the next token is a brace the PROCEDURE clause 
               *  does'nt have the argument parenthesis, insert them now
               */
              if( currentToken == OPBRACE_TOKEN )
              {
                   fprintf( fpc, "()\n\t{\n");
                   printDeclaration( fpc );
              }
              else if( currentToken == OPPARENT_TOKEN )
                   fprintf( fpc, "(");   

              procList = ( procStr * )malloc( sizeof( procStr ));
              procList->mode_string = strdup( modeString );
              procList->proc_name = strdup( procName );
              procList->fun_name = strdup( funName );
              procList->next = NULL;
              if( procListHead == NULL) procListHead = procListLast = procList;
              else {
                     procListLast->next = procList;
                     procListLast = procList;
              }
        }

        ExtractCcode( fpc );    /* Scans all characters past open brace/parenthesis  */
                                /* until and including the matching close brace      */

        if( fpc != NULL )                                /*     S006     */
              fprintf( fpc, "\n");
        
}                                                        /* ^^^ S005 ^^^ */


/*************************************************************************
 *
 *  Match()
 *
 *  Description:
 *  -----------
 *  Matches current token
 *
 *  Arguments:
 *  ---------
 *  token - name of token to match
 *
 *  Output:
 *  ------
 *
 *************************************************************************/

int
Match(token)

int token;

{
    if ((token == currentToken) || (token == NO_TOKEN))
    {
	currentToken = yylex();
	return(GOOD);
    }
    else
    {
	errexit("bad token match");
    }
}





							/* vvv S002 vvv */
SetVendor(v)

register char *v;

{
    register char *vend = vendor;
    register int c;

    do
    {
	c = *v++;
	*vend++ = tolower(c);
    } while (c != NULL_C);
}


SetModel(m)

register char *m;

{
    register char *mod = model;
    register int c;

    do
    {
	c = *m++;
	*mod++ = tolower(c);
    } while (c != NULL_C);
}

							/* ^^^ S002 ^^^ */
static void                                             /* vvv S005 vvv */
initCFile( fileName , fpc )
char * fileName;
FILE * fpc;
{
        fprintf( fpc, "\n");
        fprintf( fpc, "/*\n");
        fprintf( fpc, " *           !!! Do not change !!!\n");
        fprintf( fpc, " *        automatically generated file \"%s\"\n", fileName );
        fprintf( fpc, " */\n\n");

        fprintf( fpc, "#include \"mkcfiles.h\"\n");
}                                                        /* ^^^ S005 ^^^ */


/*
 *      DECLARATION is defined in mkcfiles.h
 *      mkcfiles also redefines the variables r0 ... r63 to
 */ 
void
printDeclaration( fpc )
FILE *fpc;
{
        fprintf( fpc, "\n            DECLARATION\n");
}

/*
 *             Print procList as a global structure
 *      *** Do not change the format of the created structure          ***
 *      *** Has to match typedef functionStruct in ./include/grafinfo  ***
 */ 
printGlobals( fpc )
FILE * fpc;
{
        procStr * nextProc = procListHead;

        fprintf( fpc, "/*\n");
        fprintf( fpc, " *     CfunctionTab is the default entry point\n");
        fprintf( fpc, " *              for this object\n");
        fprintf( fpc, " */\n\n");
        fprintf( fpc, "typedef struct _cFunctionStruct\n");
        fprintf( fpc, "    {\n");
        fprintf( fpc, "           char * mode;\n");
        fprintf( fpc, "           char * procedureName;\n");
        fprintf( fpc, "           int ( * functionPnt)();\n");
        fprintf( fpc, "    } cFunctionStruct;\n\n");
        fprintf( fpc, "cFunctionStruct CfunctionTab[] = \n    {\n");
		
        while( ( nextProc = procListHead) != 0 )
        {
               fprintf( fpc, "          { \"%s\",\t\"%s\",\t%s\t},\n",
                                            nextProc->mode_string,
                                            nextProc->proc_name,
                                            nextProc->fun_name );

               free(  nextProc->mode_string );
               free(  nextProc->proc_name );
               free(  nextProc->fun_name );
               procListHead = nextProc->next;
               free(  nextProc );
        }
        fprintf( fpc, "          { 0,\t0\t}\n");
        fprintf( fpc, "    };\n");

        fclose( fpc );
}
