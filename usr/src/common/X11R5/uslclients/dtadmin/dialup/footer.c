#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/footer.c	1.10"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include <StaticText.h>
#include "uucp.h"

void
ClearLeftFooter(footer)
Widget        footer;                 /* Footer widget */
{
#ifdef TRACE
	fprintf(stderr,"ClearLeftFooter footer=%0x\n",footer);
#endif

	if (footer)
		XtVaSetValues(footer, XtNleftFoot, "", 0);
}

void
ClearFooter(footer)
Widget        footer;                 /* Static text widget */
{
	static Arg    arg[] = {{XtNstring, (XtArgVal)"                            "}};


#ifdef TRACE
	fprintf(stderr,"ClearFooter footer=%0x\n",footer);
#endif
	if (footer)
		XtSetValues(footer, arg, XtNumber(arg));

} /* ClearFooter */

/*
** Write message into `footer' using printf(3)-style template (`tmpl') and
** an optional string `str' that may be referenced in `tmpl'.
** The resulting message can be at most MAXLINE chars.
** If the user asked for beeping footers, we obey.
*/
void
LeftFooterMsg(footer, tmpl, str)
Widget		footer;			/* Static text widget */
char		*tmpl;
char		*str;
{
	Arg	arg[1];
	char 	msg[MAXLINE];


	(void)sprintf(msg, tmpl, str);	/* Construct message */

	if (footer) {
		/*
		** Display message
		*/
		XtSetArg(arg[0], XtNleftFoot, (XtArgVal)msg);
		XtSetValues(footer, arg, (Cardinal)1);

		/*
		** Beep if the user asked for beeping footers
		*/
		_OlBeepDisplay(footer, 1);
	} else { /* temporary, to be replaced by notice popup */
		fprintf(stderr, msg);
	}

} /* LeftFooterMsg */

void
FooterMsg(footer, tmpl, str)
Widget		footer;			/* Static text widget */
char		*tmpl;
char		*str;
{
	Arg	arg[1];
	char 	msg[MAXLINE];

#ifdef TRACE
	fprintf(stderr,"FooterMsg footer=%0x\n",footer);
#endif

	(void)sprintf(msg, tmpl, str);	/* Construct message */

	if (footer) {
		/*
		** Display message
		*/
		XtSetArg(arg[0], XtNstring, (XtArgVal)msg);
		XtSetValues(footer, arg, (Cardinal)1);

		/*
		** Beep if the user asked for beeping footers
		*/
		_OlBeepDisplay(footer, 1);
	} else { /* temporary, to be replaced by notice popup */
		fprintf(stderr, msg);
	}

} /* FooterMsg */

void
PropFooterMsg(footer, tmpl, str)
Widget		footer;			/* Static text widget */
char		*tmpl;
char		*str;
{
	Arg	arg[1];
	char 	msg[MAXLINE];
#ifdef TRACE
	fprintf(stderr,"PropFooterMsg footer=%0x\n",footer);
#endif


	(void)sprintf(msg, tmpl, str);	/* Construct message */

#ifdef DEBUG
fprintf(stderr,"footer=%0x msg=%s\n", footer,msg);
#endif
	if (footer) {
		/*
		** Display message
		*/
		XtVaSetValues(footer,
				XtNleftFoot,
				msg,
				0);


	}

} /* PropFooterMsg */
