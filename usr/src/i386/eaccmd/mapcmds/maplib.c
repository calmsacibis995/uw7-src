#ident	"@(#)maplib.c	1.2"
/*	@(#) maplib.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1985.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 *	maplib - common routines for the console mapping utilities.
 *
 */

#include <stdio.h>
#include <ctype.h>

char *printc(), *prints();


/*
 *  process characters.  Non-printing characters
 * are converted from printable strings using the
 * standard '\'ing conventions.
 */
readc(fp)
FILE *fp;
{
int c, num, cnt;
    c = getc(fp);
    if ('\\' != c)
	return(c);
    else
    {   switch (c=getc(fp))
	{
	case '\\' : return('\\'); break;
	case '\'' : return('\''); break;
	case '\"' : return('\"'); break;
	case 'n' : return('\n'); break;
	case 't' : return('\t'); break;
	case 'b' : return('\b'); break;
	case 'r' : return('\r'); break;
	case 'f' : return('\f'); break;
	case '7' : case '6' : case '5' : case '4' :
	case '3' : case '2' : case '1' :
	case '0' :  num = 0; cnt=1;
		    do {
			if (c>= '0' && c<='7' && cnt<=3)
			    num = num*8 + (c-'0');
			else
			{   ungetc(c, fp);
			    return(num);
			}
			c=getc(fp);
			cnt++;
		    }while(1);
	default	: return(c);
	}
    }
}

/*
 *  process strings.  Non-printing characters
 * are converted to printable strings using the
 * standard '\'ing conventions.
 */
char *
prints(sp)
char * sp;
{
    static char ret[1024];
    char *r=ret;

    while(*sp)					/* S001 */
    {	sprintf(r, "%s", printc(*(sp++)) );
	r += strlen(r);
    }
    *r='\0';
    return(ret);
}

/*
 *  process characters.  Non-printing characters
 * are converted to printable strings using the
 * standard '\'ing conventions.
 */
char *
printc(k)
int k;
{
    static char ret[256];

    k &= 0xff;
    switch(k)
    {
    case '\\' : sprintf(ret, "\\\\");  break;
    case '\n' : sprintf(ret, "\\n"); break;
    case '\t' : sprintf(ret, "\\t"); break;
    case '\b' : sprintf(ret, "\\b"); break;
    case '\r' : sprintf(ret, "\\r"); break;
    case '\f' : sprintf(ret, "\\f"); break;
    case '\'' : sprintf(ret, "\\'"); break;
    case '\"' : sprintf(ret, "\\\""); break;
    default   : if (isprint(k))
		    sprintf(ret,"%c",k);
		else
		{   if (k < 010)
		    {   sprintf(ret, "\\00%o", k); break; }
		    if (k < 0100)
		    {   sprintf(ret, "\\0%o", k); break; }
		    sprintf(ret, "\\%o",k); break;
		}
    }
    return(ret);
}
