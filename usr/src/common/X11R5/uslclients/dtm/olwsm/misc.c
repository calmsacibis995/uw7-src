#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/misc.c	1.10"
#endif

#include <stdio.h>
#include <string.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xm/Xm.h>
#include "misc.h"
#include "error.h"
#include "wsm.h"  

int bcopy (
           register unsigned char * b1, 
           register unsigned char * b2, 
	   register length)
{
    if ((b1 + length) <= b2 || (b2 + length) <= b1) 
      {
        memcpy(b2, b1, length);
      }
    else if (b1 < b2) 
      {
	b2 += length;
	b1 += length;
	while (length--) 
	  {
	    *--b2 = *--b1;
	  }
      }
    else 
      {
	while (length--) 
	  {
	    *b2++ = *b1++;
	  }
      }
}

char *GetText(char *s)
{
        char *p, c;
        static char *not_found = "Message not found";

        if (s == NULL) {
                return (not_found);
	}

        for (p = s; *p; p++) {
                if (*p == '\001') {
                        c = *p;
                        *p++ = 0;
                        s = (char *)gettxt(s, p);
                        *--p = c;
                        return(s);
                }
        }

        /* Sometimes the Desktop Mgr uses a '000` delemeter with no prefix */

        if (strlen(s) > 0 && !strchr(s, ':') && isdigit(*s)) {
                static char msgid[6 + 10] = "dtmgr:";

                (void) strcpy(msgid + 6, s);
                return((char *) gettxt(msgid, s + strlen(s) + 1));
        }

        return(s);
}
