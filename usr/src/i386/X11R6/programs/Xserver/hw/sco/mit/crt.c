/*
 *	S001	Wed Sep 02 17:00:03 PDT 1992	hiramc@sco.COM
 *	- Declare GetCrtName properly
 *	S002	Thu Oct 15 12:14:52 PDT 1992	mikep@sco.com
 *	- change strdup to Xstrdup 
 */
#include <string.h>

extern char * Xstrdup( char * );

static char *crtName;

char * GetCrtName()
    {
    return crtName;
    }

SetCrtName(name)
    char * name;
    {
    crtName=Xstrdup(name);
    }
