#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/utilities.c	1.13.1.1"
#endif

/*
 * Module:      dtadmin: nfs  Graphical Administration of Network File Sharing
 * File:        utilities.c: utility functions.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <wctype.h>
#include <sys/vfstab.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <DtI.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/OpenLookP.h>
#include <Xol/Error.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/ListGizmo.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/ModalGizmo.h>

#include "nfs.h"
#include "text.h"
#include "sharetab.h"
#include "local.h"

extern ErrorNotice();
extern Boolean is_multi_byte;

XtArgVal GetValue(wid, resource)
Widget	wid;
String	resource;
{
	static XtArgVal	value;
	Arg	arg[1];

	XtSetArg(arg[0], resource, &value);
	XtGetValues(wid, arg, 1);
	return(value);
}

void SetValue(wid, resource, value)
	Widget		wid;
	String		resource;
	XtArgVal	value;
{
	Arg	arg[1];

	XtSetArg(arg[0], resource, value);
	XtSetValues(wid, arg, 1);
	return;
}

struct vfstab *
Copy_vfstab(struct vfstab *vp)
{
    struct vfstab *duplicate;

    if (vp == NULL)
	return (struct vfstab *)NULL;

    duplicate = (struct vfstab *)CALLOC(1, sizeof(struct vfstab));
    if (duplicate == NULL)
	NO_MEMORY_EXIT();
 
    duplicate->vfs_special  = (vp->vfs_special == NULL) ? NULL :
	STRDUP(vp->vfs_special); 
    duplicate->vfs_fsckdev  = (vp->vfs_fsckdev == NULL) ? NULL :
	STRDUP(vp->vfs_fsckdev); 
    duplicate->vfs_mountp   = (vp->vfs_mountp == NULL) ? NULL :
	STRDUP(vp->vfs_mountp); 
    duplicate->vfs_fstype   = (vp->vfs_fstype == NULL) ? NULL :
	STRDUP(vp->vfs_fstype); 
    duplicate->vfs_fsckpass = (vp->vfs_fsckpass == NULL) ? NULL :
	STRDUP(vp->vfs_fsckpass); 
    duplicate->vfs_automnt  = (vp->vfs_automnt == NULL) ? NULL :
	STRDUP(vp->vfs_automnt); 
    duplicate->vfs_mntopts  = (vp->vfs_mntopts == NULL) ? NULL :
	STRDUP(vp->vfs_mntopts); 

    return duplicate;
}


void
Free_vfstab(vp)
struct vfstab * vp;
{
    if (vp == NULL) return;
    
    if (vp-> vfs_special  != NULL) FREE(vp-> vfs_special);
    if (vp-> vfs_fsckdev  != NULL) FREE(vp-> vfs_fsckdev);
    if (vp-> vfs_mountp   != NULL) FREE(vp-> vfs_mountp);
    if (vp-> vfs_fstype   != NULL) FREE(vp-> vfs_fstype);
    if (vp-> vfs_fsckpass != NULL) FREE(vp-> vfs_fsckpass);
    if (vp-> vfs_automnt  != NULL) FREE(vp-> vfs_automnt);
    if (vp-> vfs_mntopts  != NULL) FREE(vp-> vfs_mntopts);
    FREE((char *)vp);
}

void
NoMemoryExit(char * file, char * line)
{
    extern NFSWindow *nfsw;
    char * errorText;

    SetMessage(nfsw, TXT_NoMemory, Notice);
    sleep(3);
    errorText = GetGizmoText(TXT_NoMemory);
    fprintf(stderr,"%s: %s %s\n", errorText, file, line );
    exit(NOMEMORY);
}

void
DeleteFromObjectList(ObjectListPtr slp, ObjectListPtr prev)
{
    extern NFSWindow *nfsw;

    if (slp == NULL)
	return;
    if (prev == NULL)
	*nfsw-> selectList = slp-> next;
    else
	prev -> next = slp-> next;
    FREE((char *)slp);
    return;
}

void
FreeObjectList(ObjectListPtr list)
{
    DEBUG0("FreeObjectList Entry\n");
    if (list == NULL)
	return;
    FreeObjectList(list-> next);
    FREE((char *)list);
    return;
}

extern void
setInitialValue(Setting * what, char * value)
{
    DEBUG2("old initial_value = %s, new = %s\n",  what->initial_value, value);
    if (what == NULL)
    {
	DEBUG0("NULL Setting pointer passed to setInitialValue()\n");
	return;
    }
    if (what-> initial_value)
	FREE( what-> initial_value );
    if (value == NULL)
	what-> initial_value = STRDUP(""); /* gizmo may try to free */
    else
	what-> initial_value = STRDUP(value);
}

extern void
setPreviousValue(Setting * what, char * value)
{
    DEBUG2("old previous_value = %s, new = %s\n",  what->previous_value, value);
    if (what == NULL)
    {
	DEBUG0("NULL Setting pointer passed to setPreviousValue()\n");
	return;
    }
    if (what-> previous_value)
	FREE( what-> previous_value );
    if (value == NULL)
	what-> previous_value = STRDUP(""); /* gizmo may try to free */
    else
	what-> previous_value = STRDUP(value);
}

extern void
setChoicePreviousValue(Setting * what, int value)
{
    DEBUG2("old iniitial_value = %d, new = %d\n",
	   (int)what->previous_value, value);
    if (what == NULL)
    {
	DEBUG0("NULL Setting pointer passed to setChoicePreviousValue()\n");
	return;
    }
	what-> previous_value = (char *)value;
}

extern void
free_dfstab(dfstab * ptr)
{
    extern void sharefree();

    if (ptr == NULL)
	return;
    if (ptr-> sharep)
	sharefree(ptr-> sharep);
    FREE((char *)ptr);
    return;
}
    

extern int
direq(dir1, dir2)
	char *dir1, *dir2;
{
	struct stat st1, st2;

	if (strcmp(dir1, dir2) == 0)
		return (1);
	if (stat(dir1, &st1) < 0 || stat(dir2, &st2) < 0)
		return (0);
	return (st1.st_ino == st2.st_ino && st1.st_dev == st2.st_dev);
}


extern Boolean 
sharecmp(struct share *s1, struct share *s2)
{
    if (s1 == s2)
	return True;
    if (s1 == NULL || s2 == NULL)
	return False;
    if ( strcmp(s1-> sh_path, s2->sh_path) ) /* FIX: use direq() ?? */
	return False;
    if (strcmp(s1-> sh_res, s2-> sh_res)       ||
	strcmp(s1-> sh_fstype, s2-> sh_fstype) ||
	strcmp(s1-> sh_opts, s2-> sh_opts)    /** ||
	strcmp(s1-> sh_descr, s2-> sh_descr)   not important **/
       )
	 return False;
    return True;
}
	
/*
 * UpdateList
 *
 */

extern void
UpdateList(ListGizmo * gizmo, ListHead * hp)
{
   Arg        arg[10];

   DEBUG0("UpdateList Entry\n");
   XtSetArg(arg[0], XtNnumItems,     hp->size);
   XtSetArg(arg[1], XtNitems,        hp->list);
   XtSetArg(arg[2], XtNviewHeight,   gizmo->height);
   XtSetArg(arg[3], XtNitemsTouched, True);
   XtSetValues(GetList(gizmo), arg,  4);
   DEBUG0("UpdateList Exit\n");

} /* end of UpdateList */


extern void
MoveFocus(Widget w)
{
    Time time;

    time = XtLastTimestampProcessed(XtDisplay(w));
    if (OlCanAcceptFocus(w, time))
	XtCallAcceptFocus(w, &time);
}
    
/*
 * SetMessage
 *
 */

extern void 
SetMessage(XtPointer win, char * message, msgTarget target)
{
    extern NFSWindow *nfsw;

    switch (target)
    {
    case Popup:
	SetPopupMessage((Gizmo)win, GetGizmoText(message));
	break;
    case Notice:
        ErrorNotice(GetGizmoText(message));
	break;
    case Base:
	/*FALLTHROUGH*/
    default:
	SetBaseWindowMessage(nfsw-> baseWindow, GetGizmoText(message)); 
	break;
    }
    return;

} /* end of SetMessage */


/* remove leading and trailing whitespace without moving the pointer */
/* so that the pointer may still be free'd later.                    */
/* returns True if the string was modified; False otherwise          */

Boolean
removeWhitespace(char * string)
{
    register char *ptr = string;
    size_t   len;
    Boolean  changed = False;

    if (string == NULL)
	return False;
   
    if (is_multi_byte) {
	String end;
	int ch_len;
	unsigned short  len;

	len =  _OlStrlen(string);

	for (end = string; (len != 0) && (*end != '\0');
		end += ch_len, len--) 
	{
		wchar_t ch;

		if ((ch_len = mbtowc(&ch, end, MB_LEN_MAX)) == -1)
			return False; /*invalid multi-byte char*/

    		if (iswspace(ch))
			changed = True;
		else
			break;
	}
	if ((len = _OlStrlen(end)) == 0)
	{
		*string = EOS;
		return changed;
	}
	
	if (changed)
		(void)memmove((wchar_t *)string, (wchar_t *)end, len+1);
	
	return changed;
    } /*is_multi_byte*/

    while (isspace(*ptr))
    {
	ptr++;
	changed = True;
    }
    if ((len = strlen(ptr)) == 0)
    {
	*string = EOS;
	return changed;
    }

    if (changed)
	(void)memmove((void *)string, (void *)ptr, len+1); /* +1 to */
							   /* move EOS */
    ptr = string + len - 1;    /* last character before EOS */
    while (isspace(*ptr))
    {
	ptr--;
	changed = True;
    }
    *(++ptr) = EOS;
    
    return changed;
}
