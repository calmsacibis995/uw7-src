/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)olmisc:OlImStub.c	1.9"
#endif

/*
 *************************************************************************
 *
 * Description:
 * 		This file contains stub function definitions for
 *		Input Method functions
 * 
 ****************************file*header**********************************
 */

						/* #includes go here	*/
#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <OpenLookP.h>


/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures
 *
 **************************forward*declarations***************************
 */



/* Stub functions useful with SVR4.  Can't have them with 3.2 or
 * get duplicate references.
 * Addendum - for now, the #ifdef SVR4 must be commented out so
 * we can build C locale applications.
 */
/*
#if defined(SVR4_0) || defined(SVR4)
 */

extern OlIm *
OlOpenIm OLARGLIST((dpy, rdb, res_name, res_class))
OLARG(Display, *dpy)
OLARG(XrmDatabase, rdb)
OLARG(char *,      res_name)
OLGRA(char *,      res_class)
{

#ifdef DEBUG
if (OlImFuncs)
	printf("Calling dynamically linked OlOpenIm\n");
else
	printf("No dynamic library: Stub OlOpenIm\n");
#endif

	if (OlImFuncs )
		return(OlImFuncs->OlOpenIm(dpy, rdb, res_name, res_class));
	else
	return (NULL);

} /* end of OlOpenIm() */

extern void
OlCloseIm OLARGLIST((im))
OLGRA(OlIm *, im)
{
#ifdef DEBUG
if (OlImFuncs)
	printf("Calling dynamically linked OlCloseIm\n");
else
	printf("No dynamic library: Stub OlCloseIm\n");
#endif

	if (OlImFuncs )
		(void)(OlImFuncs->OlCloseIm(im));

} /* end of OlCloseIm() */

extern OlIc *
OlCreateIc OLARGLIST((im, icvalues))
OLARG(OlIm *, im)
OLGRA(OlIcValues *, icvalues)
{
#ifdef DEBUG
if (OlImFuncs)
	printf("Calling dynamically linked OlCreateIc\n");
else
	printf("No dynamic library: Stub OlCreateIc\n");
#endif

	if (OlImFuncs )
		return(OlImFuncs->OlCreateIc(im, icvalues));
	else
	return (NULL);

} /* end of OlCreateIc() */

extern void
OlDestroyIc OLARGLIST((ic))
OLGRA(OlIc *, ic)
{
#ifdef DEBUG
if (OlImFuncs)
	printf("Calling dynamically linked OlDestroyIc\n");
else
	printf("No dynamic library: Stub OlDestroyIc\n");
#endif

if (OlImFuncs )
	(void)(OlImFuncs->OlDestroyIc(ic));

} /* end of OlDestroyIc() */

extern void
OlSetIcFocus OLARGLIST((ic))
OLGRA(OlIc *, ic)
{
#ifdef DEBUG
if (OlImFuncs)
	printf("Calling dynamically linked OlSetIcFocus\n");
else
	printf("No dynamic library: Stub OlSetIcFocus\n");
#endif

	if (OlImFuncs )
		(void)(OlImFuncs->OlSetIcFocus(ic));

} /* end of OlSetIcFocus() */
extern void
OlUnsetIcFocus OLARGLIST((ic))
OLGRA(OlIc *, ic)
{
#ifdef DEBUG
if (OlImFuncs)
	printf("Calling dynamically linked OlUnsetIcFocus\n");
else
	printf("No dynamic library: Stub OlUnsetIcFocus\n");
#endif

	if (OlImFuncs )
		(void)(OlImFuncs->OlUnsetIcFocus(ic));

} /* end of OlUnsetIcFocus() */

extern char *
OlSetIcValues OLARGLIST((ic, icvalues))
OLARG(OlIc *, ic)
OLGRA(OlIcValues *, icvalues)
{
#ifdef DEBUG
if (OlImFuncs)
	printf("Calling dynamically linked OlSetIcValues\n");
else
	printf("No dynamic library: Stub OlSetIcValues\n");
#endif

	if (OlImFuncs )
		return(OlImFuncs->OlSetIcValues(ic, icvalues));
	else
	return (NULL);

} /* end of OlSetIcValues() */
extern char *
OlGetIcValues OLARGLIST((ic, icvalues))
OLARG(OlIc *, ic)
OLGRA(OlIcValues *, icvalues)
{
#ifdef DEBUG
if (OlImFuncs)
	printf("Calling dynamically linked OlGetIcValues\n");
else
	printf("No dynamic library: Stub OlGetIcValues\n");
#endif

	if (OlImFuncs )
		return(OlImFuncs->OlGetIcValues(ic, icvalues));
	else
	return (NULL);

} /* end of OlGetIcValues() */
extern int
OlLookupImString OLARGLIST((event, ic, buffer, len, keysym, status))
OLARG(XKeyEvent *, event)
OLARG(OlIc *, ic)
OLARG(String, buffer)
OLARG(int, len)
OLARG(KeySym *, keysym)
OLGRA(OlImStatus *, status)
{
#ifdef DEBUG
if (OlImFuncs)
	printf("Calling dynamically linked OlLookupImString\n");
else
	printf("No dynamic library: Stub OlLookupImString\n");
#endif

	if (OlImFuncs )
		return(OlImFuncs->OlLookupImString(event, ic, buffer, len, keysym, status));
	else
	return (NULL);

} /* end of OlLookupImString() */

extern char *
OlResetIc OLARGLIST((ic))
OLGRA(OlIc *, ic)
{
#ifdef DEBUG
if (OlImFuncs)
	printf("Calling dynamically linked OlResetIc\n");
else
	printf("No dynamic library: Stub OlResetIc\n");
#endif

	if (OlImFuncs )
		return(OlImFuncs->OlResetIc(ic));
	else
	return (NULL);

} /* end of OlResetIc() */

extern OlIm *
OlImOfIc OLARGLIST((ic))
OLGRA(OlIc *,	   ic)
{
#ifdef DEBUG
if (OlImFuncs)
        printf("Calling dynamically linked OlImOfIc\n");
else
    	printf("No dynamic library: Stub OlImOfIc\n");
#endif

        if (OlImFuncs )
		return(OlImFuncs->OlImOfIc(ic));
	else
	return (NULL);

} /* end of OlImOfIc() */

extern Display *
OlDisplayOfIm OLARGLIST((im))
OLGRA(OlIm *,	   im)
{
#ifdef DEBUG
if (OlImFuncs)
        printf("Calling dynamically linked OlDisplayOfIm\n");
else
    	printf("No dynamic library: Stub OlDisplayOfIm\n");
#endif

        if (OlImFuncs )
		return(OlImFuncs->OlDisplayOfIm(im));
	else
	return (NULL);

} /* end of OlDisplayOfIm() */

extern char *
OlLocaleOfIm OLARGLIST((im))
OLGRA(OlIm *,	   im)
{
#ifdef DEBUG
if (OlImFuncs)
        printf("Calling dynamically linked OlLocaleOfIm\n");
else
    	printf("No dynamic library: Stub OlLocaleOfIm\n");
#endif

        if (OlImFuncs )
		return(OlImFuncs->OlLocaleOfIm(im));
	else
	return (NULL);

} /* end of OlLocaleOfIm() */

extern void
OlGetImvalues OLARGLIST((im, imvalues))
OLARG(OlIm *, im)
OLGRA(OlImValues *,	imvalues)
{
#ifdef DEBUG
if (OlImFuncs)
	printf("Calling dynamically linked OlGetImValues\n");
else
    	printf("No dynamic library: Stub OlGetImValues\n");
#endif

        if (OlImFuncs )
		(void)(OlImFuncs->OlGetImValues(im, imvalues));

} /* end of OlGetImValues() */

/*
#endif
 */
