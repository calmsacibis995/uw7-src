#ident	"@(#)xdm:ggt.c	1.8"

#include "unistd.h"		/* for gettxt() */
#include "X11/Intrinsic.h"
#include "Xm/Xm.h"
#include <locale.h>
#if defined(__STDC__)
#define CONST   const
#else
#define CONST
#endif

static char	msgid[16];	/* Assume that only nondesktop and xdm
				 * use this buffer */
static int	len_filename;

extern void
SetMsgFileName(CONST char * filename)
{
	strcpy(msgid, filename);
	len_filename = strlen(filename);
}

extern char *
CallGetText(CONST char * label)
{
#define DFT	label + strlen(label) + 1

	strcpy(msgid + len_filename, label);	/* get msg # */
	return((char *)gettxt((CONST char *)msgid, DFT));

#undef DFT
}

extern void
GetMax(Widget w, Dimension * max_wd, Dimension * max_hi)
{
	Arg		args[2];
	Dimension 	wd;
	Dimension 	hi;

	XtSetArg(args[0], XmNwidth,  &wd);
	XtSetArg(args[1], XmNheight, &hi);
	XtGetValues(w, args, 2);

	if (wd > *max_wd)
		*max_wd = wd;

	if (hi > *max_hi)
		*max_hi = hi;
}
