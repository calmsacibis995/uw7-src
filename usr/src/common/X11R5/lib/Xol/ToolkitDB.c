#ifndef	NOIDENT
#ident	"@(#)olmisc:ToolkitDB.c	1.5"
#endif

/*
 *************************************************************************
 *
 * Description:
 * 		The default toolkit database contains resources that
 *		are global to the toolkit, strings for toolkit created
 *		buttons, and other resources.  It is important to keep
 *		this database as small as possible.  The function to
 *		load the database is here, too.  It is called from the
 *		toolkit post initialize function in OlCommon.c.
 * 
 ****************************file*header**********************************
 */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

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

					/* private procedures		*/
extern void _OlCombineToolkitDB OL_ARGS((Widget));

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

/*
 * The default toolkit database.
 *
 * WARNING: As with any resource database, be very careful with trailing
 * whitespace (don't include any, except in comments).
 */
static OLconst char	OlDefaultToolkitDB[] = "\
!								\n\
! ModalShell:							\n\
!								\n\
*ModalShell.StaticText.font:		olDefaultNoticeFont\n\
*NoticeShell.StaticText.font:		olDefaultNoticeFont\n\
!								\n\
! Scrollbar:							\n\
!								\n\
*olScrollBarHMenu.title:		Scrollbar\n\
*olScrollBarHMenu*items:\\\n\
	{ Here to Left,	true },\\\n\
	{ Left to Here,	true },\\\n\
	{ Previous,	true }\n\
*olScrollBarVMenu.title:		Scrollbar\n\
*olScrollBarVMenu*items:\\\n\
	{ Here to Top,	true },\\\n\
	{ Top to Here,	true },\\\n\
	{ Previous,	true }\n\
!								\n\
! TextEdit:							\n\
!								\n\
*olTextEditMenu.title:			Edit\n\
*olTextEditMenu*items:\\\n\
	{ Undo,   U, true },\\\n\
	{ Cut,    X, true },\\\n\
	{ Copy,   C, true },\\\n\
	{ Paste,  P, true },\\\n\
	{ Delete, D, true }\n\
!								\n\
! PopupWindow:							\n\
!								\n\
*olPopupWindowMenu.title:		Settings\n\
*olPopupWindowMenu*items:\\\n\
	{ OK,               O, true, 0 },\\\n\
	{ Apply,            A, true, 0 },\\\n\
	{ Set Defaults,     S, true, 0 },\\\n\
	{ Reset,            R, true, 0 },\\\n\
	{ Reset to Factory, F, true, 0 },\\\n\
	{ Cancel,           C, true, 0 }\n\
!								\n\
! Menus:							\n\
!								\n\
*PopupMenuShell.FlatButtons.layoutType:	fixedcols\n\
*olMenuShellStayUp*items:\\\n\
	{ Stay Up },\\\n\
	{ Dismiss }\n\
!								\n\
! Category widget:						\n\
!								\n\
*olCategoryNextPage*items:\\\n\
	{ Next Page }\n\
";

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * _OlCombineToolkitDB - combines the toolkit database into each of the
 *	screen databases.  The toolkit database is called "moolit" and
 *	is found in the app-defaults directory with XResolvePathname.
 *	If this file does not exist, then the OlDefaultToolkitDB string
 *	defined above is used.
 ****************************procedure*header*****************************
 */
extern void
_OlCombineToolkitDB OLARGLIST((w))
	OLGRA(Widget, w)
{
	Display * display = XtDisplay(w);
#if defined(XtSpecificationRelease) && XtSpecificationRelease >= 5
	int num_screens = ScreenCount(display);
#else
	int num_screens = 1;
#endif
	Screen * screen;
	int i;
	XrmDatabase db;

	for (i = 0; i < num_screens; i++)  {
		char * filename;
		screen =  ScreenOfDisplay(display, i);
#if defined(XtSpecificationRelease) && XtSpecificationRelease >= 5
    		db = XtScreenDatabase(screen);
#else
    		db = XtDatabase(display);
#endif

		if (filename = XtResolvePathname(display, "app-defaults",
			"moolit", NULL, NULL, NULL, 0, NULL))  {
#if defined(XtSpecificationRelease) && XtSpecificationRelease >= 5
			XrmCombineFileDatabase(filename, &db, False);
#else
			XrmDatabase fdb;

			fdb = XrmGetFileDatabase(filename);
			XrmMergeDatabases(fdb, &db);
#endif
			XtFree(filename);
		}
		else {
			XrmDatabase default_db;

			default_db = XrmGetStringDatabase(OlDefaultToolkitDB);
			/*  XrmCombineDatabase destroys the default_db. */
#if defined(XtSpecificationRelease) && XtSpecificationRelease >= 5
			XrmCombineDatabase(default_db, &db, False);
#else
			XrmMergeDatabases(default_db, &db);
#endif
		}
	}

}  /* end of _OlCombineToolkitDB() */
