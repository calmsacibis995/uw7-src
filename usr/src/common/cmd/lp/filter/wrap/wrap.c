#ident	"@(#)wrap.c	1.2"

#ident	"$Header$"
/*
 * wrap.c
 *
 */

#include <stdio.h>
#include <locale.h>
#include <sys/euc.h>
#include <getwidth.h>

#define DEFAULT_TAB_AMOUNT     8
#define DEFAULT_LINE_LIMIT    72

typedef enum { SOFT, HARD } WrapType;

char   buffer[2 * BUFSIZ];

eucwidth_t   euc;

#define multibyte    (euc._multibyte)
#define eucw1        (euc._eucw1)
#define eucw2        (euc._eucw2)
#define eucw3        (euc._eucw3)
#define scrw1        (euc._scrw1)
#define scrw2        (euc._scrw2)
#define scrw3        (euc._scrw3)

/*
 * main
 *
 * This module wraps lines read from standard input at
 * a given line limit.  In doing so, tabs, blanks, and
 * newlines are considered white space.  Wraps are made
 * at the last white space before the word that extends
 * beyond the line limit.  Tabs are expanded to a given
 * tab stop.  Defaults for the parameters governing the
 * algorithm are 72 character lines and 8 character tab
 * stops.  These can be overridden on the command line.
 *
 * Input is buffered until the line limit is exceeded.
 * The variable \fIwrap_point\fP is used to hold the
 * last known position where a wrap seems reasonable.
 * This is defined as whenever a blank or tab is found
 * or when a non-white space character is found after
 * a white space character.  This latter case handles
 * the situation when the character that forces the
 * wrap is white space.  In this case, we want to avoid
 * emitting the space at the beginning of the next line.
 *
 * This algorithm does not handle the case where the
 * blank or tab that forces a wrap is followed by more
 * blanks or tabs.  In this case the blank or tab that
 * forced the wrap is not printed, but the following
 * blanks or tabs are and they appear at the beginning
 * of the next line.
 *
 * Whenever a newline is encountered, the buffer is flushed.
 * In the event that the file ends with characters in the
 * buffer (i.e., the last character in the input is not
 * a newline), the buffer is flushed as well.
 *
 * To increase the efficiency of the module, in-line loops
 * are used to write the output and to copy strings.
 *
 * Usage:
 *
 * wrap [-w line_width] [-t tab_stop_width] < input
 *
 */

main(argc, argv)
int argc;
char * argv[];
{
   char *        p            = buffer;
   char *        wrap_point   = buffer;
   int           c;

   int           tab_amount   = DEFAULT_TAB_AMOUNT;
   int           line_limit   = DEFAULT_LINE_LIMIT;

   int           space_to_add;
   WrapType      type_of_wrap = HARD;

   int           col          = 0;
   int           bytes;
   int           width;
   char *        end_point;

   int           optval;
   extern char * optarg;

   (void) setlocale(LC_ALL, "");

   (void) getwidth(&euc);

   while ((optval = getopt(argc, argv, "w:t:")) != EOF)
   {
      switch (optval)
      {
         case 'w': line_limit = atoi(optarg);
                   break;

         case 't': tab_amount = atoi(optarg);
                   break;
         default:
            break;
      }
   }

   if (line_limit > BUFSIZ)
      line_limit = BUFSIZ;

   if (tab_amount >= line_limit)
      tab_amount = DEFAULT_TAB_AMOUNT;

   while ((c = getchar()) != EOF)
   {
      *p = c;
      width = 0;

      switch (c)
      {
         case '\t': if ((space_to_add = tab_amount - (col % tab_amount)))
                       while (--space_to_add)
		       {
                          *p++ = ' ';
			  col++;
		       }
                    *p = ' ';
                    /* FALL THROUGH */
         case  ' ': wrap_point = p;
		    width = 1;
                    break;

         case '\n': wrap_point = p;
                    if (p == buffer && type_of_wrap == SOFT)
                    {
                       p--;
                       col--;
                       type_of_wrap = HARD;
                    }
                    else
		       col = line_limit + 1;
                    break;

         default:   if (wrap_point == buffer && col >= line_limit)
                       wrap_point = p;
                    else
                       if (*wrap_point == ' ')
                          wrap_point = p;
		    if (c < 0x80 || !multibyte)
		    {
onebyte1:;
		       width = 1;
		    }
		    else
		    {
		       if (c == SS2)
		       {
			  if (eucw2 == 0)
			     goto onebyte1;
			  bytes = eucw2 + 1;
			  width = scrw2;
		       }
		       else if (c == SS3)
		       {
			  if (eucw3 == 0)
			     goto onebyte1;
			  bytes = eucw3 + 1;
			  width = scrw3;
		       }
		       else if (c < 0xa0)
			  goto onebyte1;
		       else
		       {
			  if (eucw1 == 0)
			     goto onebyte1;
			  bytes = eucw1;
			  width = scrw1;
		       }

		       wrap_point = p;

		       while (--bytes > 0)
		       {
			  if ((c = getchar()) == EOF)
			     goto out;
			  *++p = c;
		       }
		    }
                    break;
      }
      *(p+1) = '\0';

      if (*wrap_point != ' ' && (col + width) > line_limit)
      {
         type_of_wrap = *wrap_point == '\n' ? HARD : SOFT;

         for (end_point = wrap_point; end_point > buffer; end_point--)
            if (*(end_point-1) != ' ')
               break;

         for (p = buffer; p < end_point; p++)
            putchar(*p);
         putchar('\n');
         if (*wrap_point == '\n' || *wrap_point == ' ')
         {
            p = buffer;
            col = 0;
         }
         else
         {
	    p = buffer;
	    col = 0;
	    while (*wrap_point && *wrap_point != ' ')
	    {
	       if (c < 0x80 || !multibyte)
	       {
onebyte2:
                  *p++ = *wrap_point++;
		  col++;
	       } else {
		  if (c == SS2)
		  {
		     if (eucw2 == 0)
			goto onebyte2;
		     bytes = eucw2 + 1;
		     width = scrw2;
		  }
		  else if (c == SS3)
		  {
		     if (eucw3 == 0)
			goto onebyte2;
		     bytes = eucw3 + 1;
		     width = scrw3;
		  }
		  else if (c < 0xa0)
		     goto onebyte2;
		  else
		  {
		     if (eucw1 == 0)
			goto onebyte2;
		     bytes = eucw1;
		     width = scrw1;
		  }
		  while (bytes-- > 0)
                     *p++ = *wrap_point++;
		   col += width;
               }
	    }
            *p = '\0';
         }
         wrap_point = buffer;
      }
      else
      {
         ++p;
	 col += width;
      }
   }

out:
   if (p != buffer)
   {
      *p = '\0';
      printf("%s", buffer);
   }

   exit (0);
} /* end of main */

