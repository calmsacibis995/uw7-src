#ident	"@(#)cDebug.C	1.2"
//	cDebug.C

#include	<stdio.h>
#include	<Xm/Xm.h>		//  for True/False definition.

#define CDEBUG_DEFINITION		// define storage for debugging
#include	"cDebug.h"


int	cDebugInit (int debugLevel, char *debugFileName, int maxLevel);
void	xError (Display *display, XErrorEvent *errorEvent);




int
cDebugInit (int debugLevel, char *debugFileName, int maxLevel)
{
	FILE	*fp;


	if (debugLevel > 0 && debugLevel <= maxLevel)
	{
		if (debugFileName)
		{
			printf ("debugFileName=%s.\n", debugFileName);

			if ((fp = fopen (debugFileName, "w+")) == NULL)
				return (False);
			else
				log = fp;
		}

		if (setvbuf (log, "", _IOLBF, (size_t)0))
		{
			printf ("ERROR: setvbuf() failed for %s.\n",
							debugFileName);
		}

		(void)XSetErrorHandler ((XErrorHandler)xError);
		(void)XSetIOErrorHandler ((XIOErrorHandler)xError);

		return (True);
	}
#ifdef DEBUG
	(void)XSetErrorHandler ((XErrorHandler)xError);
	(void)XSetIOErrorHandler ((XIOErrorHandler)xError);
#endif DEBUG

	return (False);


}	//  End cDebugInit ()



/* /////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void xError (Display *display, XErrorEvent *errorEvent)
//
//  DESCRIPTION:
//	Catch X errors and print out the error code and message, instead
//	of letting X exit on us.  In order to see where the problem came
//	from, set a breakpoint on this function, then look at the debug
//	stack.
//
//  RETURN:
//	Nothing.
//
///////////////////////////////////////////////////////////////////////////// */

void
xError (Display *display, XErrorEvent *errorEvent)
{
	char	buff[256];

	XGetErrorText (display, errorEvent->error_code, buff, sizeof(buff));
	fprintf (stderr, "X Error: Display=%x, Code=%d, Msg=%s\n",
			display, errorEvent->error_code, buff);

}	//  End  xError ()
