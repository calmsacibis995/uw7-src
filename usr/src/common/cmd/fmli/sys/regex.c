/***********************************************************************
* Copyright (C): 1990 SIEMENS NIXDORF INFORMATION SYSTEMS
* All Right Reserved
************************************************************************
*/
#ident "Copyright (C): 1990 SIEMENS NIXDORF INFORMATION SYSTEMS All Rights Reserved"

#ident	"@(#)fmli:sys/regex.c	1.2"

#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <regexpr.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "moremacros.h"

/*
#define DEBUG
*/

#define SUBPATTERNS	10


struct subreg {
	char *pat1;	/* compile pattern for this subpattern    */
	char *pat2;	/* compile pattern that follows pat1	    */
	int  index;	/* return index: is n for (...)$n	    */
    };

struct reg {
	char *comp;	    /* the overall compiled pattern	    */
	int num_pattern;    /* number of subpatterns in this pattern*/
	struct subreg re[2 * SUBPATTERNS + 1];
	};


char *_loc1;

extern char *malloc();

struct reg *
my_regcmp ( str )
char *str;
{
register int  i;
register char *p, *q, *s;
char          *new;
int           inbracket;
int	      inrange = 0;
unsigned char c;
struct reg *reg;
struct subreg re[2 * SUBPATTERNS + 1];
int num_patterns = 0;


if ( (new = malloc(6 * strlen(str) + 1)) == NULL )
  {
    return (NULL);
  }

re[0].pat1 = new;
for ( p = str, q = new ; *p != '\0' ; )
  {
    if ( *p != '$' || !isdigit(p[1]) || *(p-1) != ')' )
      {
	if ( (*p == '(' || *p == ')' || *p == '{' || *p == '}') 
	     && *(p-1) != '\\' )
	    *q++ = '\\';
	else if ( *p == '+'  && !inrange )
	  {
	    *q++ = '\\';
	    *q++ = '{';
	    *q++ = '1';
	    *q++ = ',';
	    *q++ = '\\';
	    *q++ = '}';
	    ++p;
	    continue;
	  }
	else if ( *p == '[' )
	    inrange = 1;
	else if ( *p == ']' && *(p-1) != '[' )
	    inrange = 0;
	*q++ = *p++;
      }
    else
      {
	s = q - 2;
	inbracket = 1;
	while ( inbracket && s >= re[num_patterns].pat1 )
	  {
	    switch (*s)
	      {
		case '(':
		    --inbracket;
		    break;
		case ')':
		    ++inbracket;
		    break;
	      }
	    --s;
	  }
	if ( s < re[num_patterns].pat1 )
	  {
	    (void) free (new);
	    return (NULL);
	  }
	if ( s == re[num_patterns].pat1 )
	  {
	    re[num_patterns].pat2 = q;
	    re[num_patterns].index = p[1] - '0';
	    ++num_patterns;
	    re[num_patterns].pat1 = q;
	  }
	else
	  {
	    re[num_patterns].pat2 = s;
	    re[num_patterns].index = -1;
	    ++num_patterns;
	    re[num_patterns].pat1 = s;
	    re[num_patterns].pat2 = q;
	    re[num_patterns].index = p[1] - '0';
	    ++num_patterns;
	    re[num_patterns].pat1 = q;
	  }

	p += 2;
      }
  } /* END OF FOR LOOP */
*q = '\0';

re[num_patterns].pat1 = NULL;

#ifdef DEBUG
fprintf (stderr, "new  pattern: \"%s\"\n", new);
fprintf (stderr, "num_patterns=%d\n", num_patterns);
for ( i = 0; i < num_patterns ; ++i )
  {
    c = *(re[i].pat2);
    *(re[i].pat2) = '\0';
    fprintf (stderr, "re[%d] pat1=\"%s\" ", i, re[i].pat1);
    *(re[i].pat2) = c;
    fprintf (stderr, " pat2=\"%s\"\n", re[i].pat2);
  }
#endif

if ( (reg = (struct reg*)malloc (sizeof (struct reg))) == NULL )
    return (NULL);

reg->comp = compile (new, (char *)0, (char *)0);

if(reg->comp == NULL)
   {
      free(reg);
      return(NULL);
   }

reg->num_pattern = num_patterns;

for ( i = 0 ; i < num_patterns ; ++i )
  {
    c = *(re[i].pat2);
    *(re[i].pat2) = '\0';
    reg->re[i].pat1 = compile (re[i].pat1, (char*)NULL, (char*)NULL);
    *(re[i].pat2) = c;
    reg->re[i].pat2 = compile (re[i].pat2, (char*)NULL, (char*)NULL);
    reg->re[i].index = re[i].index;
  }
while ( i < 2 * SUBPATTERNS + 1 )
  {
    reg->re[i].pat1 = NULL;
    reg->re[i].pat2 = NULL;
    reg->re[i].index = -1;
    ++i;
  }

if ( new != NULL )
    (void) free (new);
return (reg);
}


char *
my_regex (reg, subject, ret0, ret1, ret2, ret3, ret4, ret5, ret6, ret7, ret8, ret9)
struct reg *reg;	/* the regular expression pattern	    */
char *subject;	
char *ret0, *ret1, *ret2, *ret3, *ret4, *ret5, *ret6, *ret7, *ret8, *ret9;
{
register int i;
register char *start; 	/* begin of match of sub pattern	*/
register char *end;	/* end of match of a sub pattern	*/
register char *q;
	 char *ret;
unsigned char c;

#ifdef DEBUG
fprintf (stderr, "my_regex: subject=\"%s\"\n", subject);
#endif

_loc1 = NULL;
if ( !step (subject, reg->comp) )
  {
    return (NULL);
  }

_loc1 = loc1;
ret = loc2;

#ifdef DEBUG
c = *loc2;
*loc2 = '\0';
fprintf (stderr, "matched string is: \"%s\"\n", loc1);
*loc2 = c;
#endif

for ( i = 0, start = loc1 ; i < reg->num_pattern ; ++i)
  {
    if ( !advance (start, reg->re[i].pat1) )
      {
#ifdef DEBUG
	fprintf (stderr, "no match for re[%d].pat1\n", i);
#endif
	break;
      }
    /* end points to the last character of the current match */
    end = loc2 - 1;
    if ( reg->re[i].pat2 )
      {
        while ( !advance(loc2, reg->re[i].pat2) )
          {
#ifdef DEBUG
	    fprintf (stderr, "could not match second pattern of re[%d] in \"%s\"\n",
		    i, end+1);
#endif
	    /* save the last character of current match	*/
	    c = *end;
	    /* change the last character of current match to be end of string */
	    *end = '\0';

	    /* now lets try to find a new smaller match for the first
	    ** pattern
	    */
	    if ( !advance (start, reg->re[i].pat1) )
	      {
#ifdef DEBUG
	        fprintf (stderr, "no match for re[%d].pat1 during retry\n", i);
#endif
	        *end = c;
	        goto RETURN;
	      }
	    /* restore the last character of previous match */
	    *end = c;
	    /* let end point to the last character of new match */
	    end = loc2 - 1;
#ifdef DEBUG
	    fprintf (stderr, "try second pattern for \"%s\"\n", loc2);
#endif
          } /* END OF WHILE ( !advance() ) */

	} /* END OF IF (reg->re[i].pat2) */
    
#ifdef DEBUG
    fprintf (stderr, "re[%d].index=%d\n", i, reg->re[i].index);
#endif
    if ( reg->re[i].index >= 0 )
      {
	switch (reg->re[i].index)
	  {
	    case 0:
		q = ret0;
		break;
	    case 1:
		q = ret1;
		break;
	    case 2:
		q = ret2;
		break;
	    case 3:
		q = ret3;
		break;
	    case 4:
		q = ret4;
		break;
	    case 5:
		q = ret5;
		break;
	    case 6:
		q = ret6;
		break;
	    case 7:
		q = ret7;
		break;
	    case 8:
		q = ret8;
		break;
	    case 9:
		q = ret9;
		break;
	  } /* END OF SWITCH */
	
	strncpy (q, start, end - start + 1);
	q[end - start + 1] = '\0';
#ifdef DEBUG
	fprintf (stderr, "ret%d=\"%s\"\n", reg->re[i].index, q);
#endif
      } /* END OF IF (re[i].index >= 0) */

    /* use the rest of the string for the next patterns */
    start = end + 1;
#ifdef DEBUG
    fprintf (stderr, "string for next loop is \"%s\"\n", start);
#endif
    
  } /* END OF FOR i LOOP */
RETURN:
return (ret);
}

void
regfree (reg)
struct reg *reg;
{
register int i;

if ( reg->comp )
    (void) free (reg->comp);
for ( i = 0 ; i < reg->num_pattern ; ++i )
  {
    if ( reg->re[i].pat1 )
	(void) free (reg->re[i].pat1);
    if ( reg->re[i].pat2 )
	(void) free (reg->re[i].pat2);
  }
(void) free (reg);
}
