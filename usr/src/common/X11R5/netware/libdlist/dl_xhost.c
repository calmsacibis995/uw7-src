#ident	"@(#)dl_xhost.c	1.2"
#ident	"@(#)dl_xhost.c	2.2 "
#ident  "$Header$"

/*--------------------------------------------------------------------
** Filename : dl_xhost.c
**
** Description : This file contains a function to update the xhost
**               file.
**
** Functions : AccessXhost
**             AppendToNameList
**------------------------------------------------------------------*/
  
/*--------------------------------------------------------------------
**                         I N C L U D E S
**------------------------------------------------------------------*/
#include    <X11/Intrinsic.h>
#include    <X11/StringDefs.h>
#include    <Xol/OpenLook.h>

#include    <sys/types.h>
#include    <sys/stat.h>
#include    <stdio.h>

#include    "dl_common.h"

/*--------------------------------------------------------------------
**             F U N C T I O N    P R O T O T Y P E S 
**------------------------------------------------------------------*/
void AppendToNameList( nameList **, unsigned char * ); 

/*--------------------------------------------------------------------
** Function : AccessXhost
**
** Description : This function does all the manipulation of the Xhost
**               file.
**
** Parameters : unsigned char *server - the serverName to add.
**              int service      - the service to perform.
**                                 ADD_XHOST or CLEANUP_XHOST 
**                                 
**
** Return : SUCCESS
**          FAILURE if there was a problem.
**------------------------------------------------------------------*/
int AccessXhost( Widget w, unsigned char *server, int service )
{
   static nameList  *serverList  = NULL;
   static nameList  *save;
   char             *commandStr  = NULL; 
   char             *command     = "/usr/X/bin/xhost %c %s &"; 
   char             *viewCommand = "/usr/X/bin/xhost > %s 2> %s";
   char             *outFile     = NULL;
   char             *errFile     = NULL;
   char             *xhostLine   = NULL;
   unsigned char    *errStr      = NULL;
   struct stat       errStat;
   int               retCode     = SUCCESS;
   FILE             *outFD; 
   Boolean           foundFlag   = FALSE;
   
   switch( service )
   {
       case ADD_XHOST:
         GenRandomTempFName( &outFile );
         mktemp( outFile ); 
         GenRandomTempFName( &errFile );
         mktemp( errFile );

         commandStr = XtMalloc( strlen( viewCommand ) + 
                                   strlen( outFile )     + 
                                    strlen( errFile ) + 1 );
         sprintf( commandStr, viewCommand, outFile, errFile );
         system( commandStr );
         XtFree( commandStr );
     
         /*--------------------------------------------------  
         ** xhost puts the list of xhosts in stderr, and  
         ** Host Access Control enabled/disabled in stdout.
         ** Therefore to check for an error we check for
         ** a NULL stdout.
         --------------------------------------------------*/
         GetErrorMsg( outFile, &errStr );
         if ( errStr == NULL )
         {
             GetErrorMsg( errFile, &errStr );
             displayErrorMsg( w, errStr );
             retCode = FAILURE; 
         }
         else
         {
             xhostLine =  XtMalloc( MAX_XHOST_LINE );
             outFD = fopen(  errFile, "r" ); 
             while (( fgets(  xhostLine, MAX_XHOST_LINE - 1, outFD ))!= NULL )
             {
                xhostLine[strlen( xhostLine ) - 1] = '\0';
                if ( strcmp( xhostLine, SC server ) == 0 )
                {
                    foundFlag = TRUE;
                    break;
                }
             }
             if ( foundFlag == FALSE )
             {
                   commandStr = XtMalloc( strlen(  command ) + 
                                             strlen( SC server ) + 1 );
                   sprintf( commandStr, command, '+', server );
                   system( commandStr );
                   XtFree( commandStr );
                   AppendToNameList( &serverList, server ); 
             }
             XtFree( xhostLine ); 
         }
       break;

       case CLEANUP_XHOST:
           while( serverList != NULL )
           {
               commandStr = XtMalloc( strlen( command ) + 
                                      strlen( SC serverList->name ) + 1 );
               sprintf( commandStr, command, '-', serverList->name );  
               system( commandStr );
               XtFree( commandStr );
               save = serverList;
               serverList = serverList->next;
               XtFree( SC save->name );
               XtFree( ( XtPointer ) save );
           }
       break;
   }

   /* Cleanup and return */
   if ( errFile != NULL )
   {
      unlink( errFile );
      XtFree( errFile );
   }
   if ( outFile != NULL )
   {
       unlink( outFile ); 
       XtFree( outFile );
   }
   if ( errStr )
       XtFree( SC errStr );
   return( retCode );
}


/*--------------------------------------------------------------------
** Function : AppendToNameList
**
** Description : This function inserts a name in a nameList structure.
**
** Parameters :  nameList **list - list to insert name
**               unsigned char *name - name to insert into the list 
**
** Return : None
**------------------------------------------------------------------*/
void AppendToNameList( nameList **list, unsigned char *name ) 
{
    nameList *temp;
    nameList *step;

    temp = ( nameList * ) XtMalloc( sizeof( nameList ) );
    temp->name = ( XtPointer ) XtMalloc( strlen( SC name ) + 1 );
    strcpy( SC temp->name, SC name ); 
    temp->next = NULL;

    if ( *list == NULL )
        *list = temp;
    else
    {
        step = *list;
        while ( step->next != NULL )
            step = step->next;
        step->next = temp;
    }
}

