/*		copyright	"%c%" 	*/

#ident	"@(#)tcpio:lvldom.c	1.1.1.2"
#ident "$Header$"
/* Title:	lvldom.c
   Compares 2 levels.  If the first level dominates the second, returns 0
   Called by rtcpio shell script.
*/
#include <sys/types.h>
#include <mac.h>
#include <stdlib.h>

main(argc,argv)
int argc;
char **argv;
{
  level_t lidarray[2];
  int i;
  for (i=0;i<2;i++)
    {
    if (isalpha(*(argv[i+1]))) 
        lvlin(argv[i+1],&lidarray[i]);
    else 
        lidarray[i]=atol(argv[i+1]);
   }
   if (lvldom(&lidarray[0],&lidarray[1]) > 0)
	exit (0);
   else exit (1);
}

	

	    
