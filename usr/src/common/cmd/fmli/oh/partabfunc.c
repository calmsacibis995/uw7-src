/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */

#ident	"@(#)fmli:oh/partabfunc.c	1.7.3.4"

#include <stdio.h>
#include <string.h>
#include "inc.types.h"		/* abs s14 */
#include "but.h"
#include "wish.h"
#include "sizes.h"
#include "typetab.h"
#include "ifuncdefs.h"
#include "optabdefs.h"
#include "partabdefs.h"

/*extern size_t strlen();
**abs s14 */

char *
part_match(name, template)
char *name, *template;
{
    char pre[FILE_NAME_SIZ], suf[FILE_NAME_SIZ];
    char name1[FILE_NAME_SIZ];
    char *p;
    static char retstr[FILE_NAME_SIZ];
    int  matchlen = FILE_NAME_SIZ;
    int len;

    p = pre;

    while (*template && (*template != '%' || *(template+1) == '%')) {
	*p++ = *template++;
    }

    *p = '\0';

    if (*template == '%') {
	if (*(++template) == '.')
	    matchlen = atoi(++template);
	if (matchlen == 0)
	    matchlen = FILE_NAME_SIZ;
	while (*template && *template >= '0' && *template <= '9')
	    template++;
	if (*template == 's') {
	    template++;
	}
    }

    p = suf;

    while (*template && *template != '/')
	*p++ = *template++;

    *p = '\0';

    if (*template == '/') {
	if ((p = strchr(name, '/')) == NULL)
	    return(NULL);
	strncpy(name1, name, name-p);
	name1[name-p] = '\0';
    } else {
	strcpy(name1, name);
    }

    if (*pre)
    {
	if ((int)strlen(name1) < (int)strlen(pre) ||    /* EFT k16 */
	    strncmp(name1, pre, strlen(pre)) != 0)
	{
	    return(NULL);
	}
    }

    if (*suf)
    {
	if (!has_suffix(name1+(int)strlen(pre), suf))
	{
	    return(NULL);
	}
    }
				/* EFT k16.. */
    if ((int)strlen(name1) > matchlen + (int)strlen(pre) + (int)strlen(suf))
    {
	return(NULL);
    }

    strncpy(retstr, name+(int)strlen(pre), 
	    (len=(int)strlen(name)-(int)strlen(pre)-(int)strlen(suf)));
    retstr[len] = '\0';

    if (*template == '/') {
	sprintf(name1, ++template, retstr);
	if (strcmp(++p, name1) != 0)
	    return(NULL);
    }
		
    return(retstr);
}

char *
part_construct(name, template)
char *name, *template;
{
	static char result[FILE_NAME_SIZ];

	sprintf(result, template, name, name);
	return(result);
}

/***********************************************************************
* init_I18n_partab () function translates the "objdisp" entries in the
* "Partab" array.
************************************************************************
*/

void
init_I18n_partab ()
{
    register int i;
    extern struct opt_entry Partab[];
    extern char *gettxt();

    for ( i = 0 ; i < MAX_TYPES ; i++) 
      {
        if ( strcmp(Partab[i].objdisp,"") != 0 )
	  {
            strncpy (Partab[i].objdisp,
		     gettxt (Partab[i].i18n_msgid, Partab[i].objdisp),
		     OTYPESIZ);
	    Partab[i].objdisp[OTYPESIZ-1] = '\0';
	  }
      }
    return;
}
    
