/*		copyright	"%c%" 	*/

#ident	"@(#)tcpio:vallvl.c	1.1.1.2"
#ident "$Header$"
/* Title:	vallvl.c
   Validates an level, in the form of a LID, alias, or fully qualified name.
   Called by rtcpio shell script.
*/
#include <sys/types.h>
#include <mac.h>
#include <stdlib.h>

main(argc,argv)
int argc;
char **argv;
{
  int result;
  level_t levelid;
  char *levelnamep = argv[1];
  if (isalpha(*levelnamep))
      result=lvlin(levelnamep,&levelid);
  else 
    { 
      levelid=(level_t)atol(levelnamep);
      result=lvlvalid(&levelid);
    }
exit (result);
}

	

	    
