/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)olmisc:regexp.c	1.12"
#endif

/*
 * regcmp.c
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strutil.h>
#include <X11/memutil.h>
#include <Xol/buffutil.h>
#include <Xol/regexp.h>
#if defined(SYSV) || defined(USL)
#if defined(SVR4_0) || defined(SVR4)
#include <limits.h>
#else
#define MB_LEN_MAX 5
#endif
#else
#define MB_LEN_MAX 5
#endif

/* From stdlib.h, but defined only for STDC */
#ifndef __STDC__

#ifndef sun
typedef unsigned int size_t;
#endif

#endif

typedef Bufferof(char *) StringBuffer;

typedef struct _Match
   {
   StringBuffer * buffer;
#if !defined(I18N)
   char           matchany;
   char           once;
#else
   int           matchany;
   int           once;
#endif
   } Match;

#define ANYCHAR    ""
#define ANYCHARS   NULL
#define EXPINCRE   10
#define MATCHINCRE 10

static char *AnyChar = ANYCHAR;
static char *AnyChars = ANYCHARS;

#if defined(__STDC__)

static Match * expcmp(char *);
static char * mbstrchr(char *, char *);

extern char * streexp();
extern char * strexp(char *, char *,char * );
extern char * strrexp(char *, char *, char *);

#else

static Match * expcmp();
static char * mbstrchr();
extern char * streexp();
extern char * strexp(); 
extern char * strrexp();

#endif

static char * match_begins;


/*
 * expcmp
 * mlp (I18N) - accept a multibyte char string; convert where applicable
 * to wide characters.  The string may consist of any EUC code of characters,
 * but the special matching characters must be ASCII.
 *
 */

static Match *
expcmp(exp)
char * exp;
{
static Match * match = NULL;
static char * prev  = NULL;

register int i = 0;
register int s;
char * current;
#ifdef I18N
char *mbptr = &exp[0];
char *tmp_ptr;
int bytestep;
#endif

if ((prev == NULL) || (strcmp(prev, exp) != 0))
   {
#if !defined(I18N)
   int l = strlen(exp);
#else
   int l = _mbstrlen(exp);
#endif

   if (prev != NULL)
      {
      FREE(prev);
      prev = (char *) NULL;
      for (i = 0; i < match-> buffer-> used; i++)
         if (match-> buffer-> p[i] != AnyChar &&
             match-> buffer-> p[i] != AnyChars ) {
            FREE(match-> buffer-> p[i]);
	}
      }
   else
      {
      match = (Match *) MALLOC(sizeof(Match));
      match-> buffer = (StringBuffer *)AllocateBuffer(sizeof(char *), EXPINCRE);
      }

   prev                  = strcpy(MALLOC((unsigned)strlen(exp) + 1), exp);
   match-> buffer-> used = 0;
#if !defined(I18N)
   match-> once          = (*exp == '^') ? 1 : 0;
#else
   match-> once          = (mblen(exp, (size_t)MB_LEN_MAX) == 1) &&
					(*exp == '^') ? mbptr++,1 : 0;
#endif
   match-> matchany      = 0;

#ifdef I18N
/* We'll keep track of 2 variables:
 * i = the number of iterations of the outer for loop, unless it
 * gets adjusted inside (then it may be reduced);
 * mbptr points to expr, and it gets continuously adjusted.
 */
#endif

   for (i = match-> once; i < l; i++, mbptr += bytestep)
      {
      if (BufferFilled(match-> buffer))
         GrowBuffer((Buffer *)match-> buffer, EXPINCRE);
#ifdef I18N
	bytestep = mblen(mbptr, (size_t)MB_LEN_MAX);
	if (bytestep == 1) {
      switch (*mbptr)
         {
         case '[':
		/* l = the number of multibyte chars in exp.
		 * Move i to the next character sequence, and
		 * set tmp_ptr to the next character (bump up
		 * mbptr first.
		 */ 
		for( ++i, tmp_ptr = ++mbptr; i < l; i++, mbptr += bytestep) {
			bytestep = mblen(mbptr, MB_LEN_MAX);
			if (bytestep == 1 && *mbptr == ']')
				break;
		}
		/* Copy the interval string, between the brackets, into
		 * current.
		 */
		current = strndup(tmp_ptr, mbptr - tmp_ptr);
		break;
         case '*':
            if (BufferEmpty(match-> buffer))
               match-> matchany = 1;
/*
            else
*/
               current = AnyChars;
            break;
	 case '?':
            current = AnyChar;
            break;
         default:
            current = strndup(mbptr, 1);
            break;
         }
	} /* end if (bytestep == 1) */
	else { 
		/* bytestep > 1 - we don't do special characters in
		 * foreign languages - nothing personal.
		 */
		current = strndup(mbptr, bytestep);
	} /* end else (bytestep != 1) */

#else
      switch (exp[i])
         {
         case '[':
	    /* l = the number of multibyte chars in exp */
            for (s = ++i; i < l && exp[i] != ']';i++)         ;
            current = strndup(&exp[s], i-s);
            break;
         case '*':
            if (BufferEmpty(match-> buffer))
               match-> matchany = 1;
/*
            else
*/
               current = AnyChars;
            break;
	 case '?':
            current = AnyChar;
            break;
         default:
            current = strndup(&exp[i], 1);
            break;
         }
#endif
      match-> buffer-> p[match-> buffer-> used++] = current;
      }

   /* Do the following statement because the previous match request may
    * be the same string as the previous one, and match->matchany may
    * still be set to 1 !
    */
   while (match-> buffer-> used != 0 && 
          match-> buffer-> p[match-> buffer-> used - 1] == AnyChars)
      match-> buffer-> used--;
   }
   if ( (match->buffer->used > 0) && (match->buffer->p[0] != AnyChars) )
	match-> matchany = 0;

return (match);

} /* end of expcmp */

/*
 * strexp
 *
 * The \fIstrexp\fR function is used to perform a regular expression forward
 * scan of \fIstring\fR for \fIexpression\fR starting at \fIcurp\fR.
 * The regular expression language used is:
 *
 * .so CWstart
 *        c - match c
 *  [<set>] - match any character in <set>
 * [!<set>] - match any character not in <set>
 *        * - match any character(s)
 *        ^ - match must start at curp
 * .so CWend
 *
 * Return value:
 *
 * NULL is returned if expression cannot be found in string; otherwise
 * a pointer to the first character in the substring which matches
 * expression is returned.  The function streexp(3) can be used to get
 * the pointer to the last character in the match.
 *
 * See also:
 *
 * strrexp(3), streexp(3)
 *
 * Synopsis:
 *
 *#include <expcmp.h>
 * ...
 */

extern char *
strexp(string, curp, expression)
char * string;
char * curp;
char * expression;
{
Match * match = NULL;
char * found = NULL;

if ( curp == NULL  ||  expression == NULL ||
    *curp == '\0'  || *expression == '\0')
   return(NULL);

if ((match = expcmp(expression)) != NULL)
   {
   register int matchpos; /* position in the string we're looking at */
   char * pos;		  /* ptr into the expression (match pattern) */
   char * matchstart;
   char * stringend = &curp[strlen(curp)];
#ifdef I18N
   char *tmp_ptr;
   int bytestep;
#endif

   if (match-> once && string != curp)
      return (NULL);

   matchstart = pos = curp;
   matchpos = 0;

   if (match-> buffer-> used != 0)
   do
      {
#if !defined(I18N)
      if (matchpos < match-> buffer-> used &&
  ( *match-> buffer-> p[matchpos] == '\0' || 
  (*match-> buffer-> p[matchpos] != '!' && strchr(match-> buffer-> p[matchpos], *pos) != NULL) ||
  (*match-> buffer-> p[matchpos] == '!' && strchr(match-> buffer-> p[matchpos], *pos) == NULL)))
#else
      if (matchpos < match-> buffer-> used &&
		( match->buffer->p[matchpos] == AnyChar ||
		(*match-> buffer-> p[matchpos] != '!' &&
			mbstrchr(match-> buffer-> p[matchpos], pos) != NULL) ||
		(*match-> buffer-> p[matchpos] == '!' &&
			mbstrchr(match-> buffer-> p[matchpos], pos) == NULL)) )
#endif
         {
         match-> matchany = 0;
         if (matchpos == match-> buffer-> used - 1)
            match_begins = pos;
         if (++matchpos == match-> buffer-> used)
            found = matchstart;
         }
      else
         if (match-> buffer-> p[matchpos] == AnyChars)
            {
            for (; matchpos < match-> buffer-> used; matchpos++)
               if (match-> buffer-> p[matchpos] != AnyChars)
                  break;
            match-> matchany = 1;
            }
         else
            if (!match-> matchany)
               {
		int savepos = matchpos;
               matchpos = 0;
#if !defined(I18N)
               matchstart = pos + 1;
#else
		if (savepos != 1) /* we previously matched 0 or > 1 position */ 
			matchstart = pos + mblen(pos, MB_LEN_MAX);
#endif
               if (match-> once) break;
		/* if the matching position was at one, then just
		 * redo it from that spot, to be on the safe side.
		 * we were missing patterns where we had 2 consecutive
		 * alike characters, where the pattern started on the
		 * 2nd character.
		 */
		if (savepos == 1) {
			matchstart = pos;
			continue;
		}
               }
#if !defined(I18N)
      pos++;
#else
	pos += mblen(pos, MB_LEN_MAX);
#endif
      } while (found == NULL && pos != stringend);
   }

return (found);

} /* end of strexp */

/*
 * strrexp
 *
 * The \fIstrrexp\fR function is used to perform a regular expression backward
 * scan of \fIstring\fR for \fIexpression\fR starting at \fIcurp\fR.
 * The regular expression language used is:
 *
 * .so CWstart
 *        c - match c
 *  [<set>] - match any character in <set>
 * [!<set>] - match any character not in <set>
 *        * - match any character(s)
 *        ^ - match must start at curp
 * .so CWend
 *
 * Return value:
 *
 * NULL is returned if expression cannot be found in string; otherwise
 * a pointer to the first character in the substring which matches
 * expression is returned.  The function streexp(3) can be used to get
 * the pointer to the last character in the match.
 *
 * See also:
 *
 * strexp(3), streexp(3)
 *
 * Synopsis:
 *
 *#include <expcmp.h>
 * ...
 */

extern char *
strrexp(string, curp, expression)
char * string;
char * curp;
char * expression;
{
Match * match = NULL;
char * found = NULL;
int bytecnt;

if (expression == NULL || *expression == '\0')
   return(NULL);

if (curp == NULL) 
   curp = &string[strlen(string) - 1];

if (curp < string)
   return(NULL);

#ifdef I18N
/* if curp does not point to a valid multibyte character, move it back until
 * it is at one.
 */
for (; curp > string; curp--)
	if (mblen(curp, (size_t)MB_LEN_MAX) != -1)
		break;
#endif

if ((match = expcmp(expression)) != NULL)
   {
   register int matchpos;
   char * pos;
   char * stringend = string;
#ifdef I18N
   char *tmp_ptr;
   int bytestep;
#endif

   if (match-> once)
      {
      if (string != curp)
	 return (strexp(string, string, expression));
      }
   else
      if ((strexp(curp, curp, expression)) == curp)
         return(curp);

   pos = curp;
   matchpos = match-> buffer-> used - 1;

   if (match-> buffer-> used != 0)
   do
      {
#if !defined(I18N)
      if (matchpos < match-> buffer-> used &&
  (*match-> buffer-> p[matchpos] == '\0' ||
  (*match-> buffer-> p[matchpos] != '!' && strchr(match-> buffer-> p[matchpos], *pos) != NULL) ||
  (*match-> buffer-> p[matchpos] == '!' && strchr(match-> buffer-> p[matchpos], *pos) == NULL)))
#else
	if (matchpos < match-> buffer-> used &&
		(match-> buffer-> p[matchpos] == AnyChar ||
		(*match-> buffer-> p[matchpos] != '!' &&
			mbstrchr(match-> buffer-> p[matchpos], pos) != NULL) ||
		(*match-> buffer-> p[matchpos] == '!' &&
			mbstrchr(match-> buffer-> p[matchpos], pos) == NULL)) )
#endif
         {
         match-> matchany = 0;
         if (matchpos == match-> buffer-> used - 1)
            match_begins = pos;
         if (--matchpos == -1)
            found = pos;
         }
      else
         if (match-> buffer-> p[matchpos] == AnyChars)
            {
            for (; matchpos >= 0; matchpos--)
               if (match-> buffer-> p[matchpos] != AnyChars)
                  break;
            match-> matchany = 1;
            }
         else
            if (!match-> matchany)
               {
               matchpos = match-> buffer-> used - 1;
               if (match-> once) break;
               }
#if !defined(I18N)
      pos--;
#else
	/* We can step back one byte at a time (decrement pos), and check to
	 * see if pos points to a valid character with mblen() -
	 * if mblen() returns -1, then it's not valid;  but that doesn't
	 * work;  consider the string "\273\260\311\251", the
	 * japanese characters for "mitsubishi"; these are actually two
	 * wide characters, \273\260 and \311\251.  When stepping backward,
	 * \311\251 points to a valid character, but so does \260\311 -
	 * we really want to jump two full bytes backward.  So we have
	 * to work around this by keeping track of where we last left off.
	 * Note the syntax at the far right of the for statement (after the
	 * comma) - it's saying to increment bytecnt until it is equal to
	 * MB_LEN_MAX (no effect if bytecnt == MB_LEN_MAX).
	 */
	for(pos--, bytecnt = 1; pos > stringend; pos--, bytecnt ==
					 MB_LEN_MAX ? MB_LEN_MAX : ++bytecnt )
		if (mblen(pos, (size_t)bytecnt ) != -1)
			/* found valid multibyte character */
			break;
#endif
      } while (found == NULL && pos >= stringend);
   }

return (found);

} /* end of strrexp */

/*
 * streexp
 *
 * The \fIstreexp\fR function is used to retrieve the pointer of the last
 * character in a match found following a strexp/strrexp function call.
 *
 * See also:
 *
 * strexp(3), strrexp(3)
 *
 * Synopsis:
 *
 *#include <regexp.h>
 * ...
 */

extern char * streexp()
{

return (match_begins);

} /* end of streexp */

/*
 * mbstrchr
 *
 * The mbstrchr function searches string s for the first occurrenc of
 * the multibyte character pointed to by cptr.
 *
 */
static char *
mbstrchr(s, cptr)
char *s;
char *cptr;
{
/* Search multibyte character string s for the first occurrence of
 * the multipbyte character pointer to by c.
 */
	int i,j;
	unsigned int n = strlen(s);
	int kount, charcount;
	char *tmp_ptr = &s[0];

	charcount = mblen(cptr, MB_LEN_MAX);

	for (i = 0;  i < n;  i += kount, tmp_ptr += kount) {
		kount = mblen(tmp_ptr, MB_LEN_MAX);
		if (kount != charcount)
			continue;
		/* kount == charcount, what's to do?? */
		for (j=0; j < charcount; j++) {
			if (tmp_ptr[j] != cptr[j])
				break;
		}
		if (j == charcount)	/* found */
			return(tmp_ptr);
	} /* end outer for loop */
	return(NULL);
}
