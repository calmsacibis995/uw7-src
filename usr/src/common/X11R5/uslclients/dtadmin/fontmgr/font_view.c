#ident	"@(#)dtadmin:fontmgr/font_view.c	1.23"
/*
 * Module:     dtadmin:fontmgr   Graphical Administration of Fonts
 * File:       font_view.c
 */

#include <IntrinsicP.h>
#include <Xatom.h>
#include <Caption.h>
#include <OpenLookP.h>
#include <Intrinsic.h>
#include <StringDefs.h>
#include <FExclusive.h>
#include <FNonexclus.h>
#include <StaticText.h>
#include <TextEdit.h>
#include <ControlAre.h>
#include <TextField.h>
#include <IntegerFie.h>
#include <FList.h>
#include <Gizmos.h>

#include <fontmgr.h>

extern Widget       app_shellW;		  /* application shell widget       */
extern Boolean C_locale;

static void UpdatePS();
extern String GetFontName(view_type *,XFontStruct *, String);

Boolean show_xlfd;


void
DisplayFont( view_type *view)
{
    XFontStruct * font;
    XFontStruct * new_font;
    Arg args[5];
    Cardinal n;
    char buf[MAX_PATH_STRING];
    static Boolean first_time=TRUE;

    /*  Load the font and set it in the text widget.  */
#ifdef SERVER_CANT_CACL_RESOL
    new_font = _OlGetDefaultFont(app_shellW, view->cur_xlfd);
#else
    new_font = XLoadQueryFont(XtDisplay(app_shellW), view->cur_xlfd);
#endif
#ifdef DEBUG
fprintf(stderr,"view->cur_xlfd=%s\n",view->cur_xlfd);
#endif
    if (new_font == NULL)  {
	if (show_xlfd)
	    sprintf(buf, GetGizmoText(TXT_FORMAT_CAN_NOT_DISPLAY), view->cur_xlfd);
	else
	    sprintf(buf, GetGizmoText(TXT_FORMAT_CAN_NOT_DISPLAY), GetFontName(view, new_font, view->cur_xlfd));
	InformUser(buf);
	if (!view->bitmap)
	    StandardCursor(0);
	strcpy(view->cur_xlfd, view->prev_xlfd);
	return;
    }

/***********
 * don't free any fonts, let the X-server cache it
    if (first_time)
	first_time = FALSE;
    else {
	n = 0;
	XtSetArg(args[n], XtNfont, &font);	n++;
	XtGetValues(view->sample_text, args, n);
	XFreeFont(XtDisplay(app_shellW), font);
    }
************/

    /*  Change the sample_text to show the current font.  */
    n = 0;
    XtSetArg(args[n], XtNfont, new_font);	n++;
    XtSetValues(view->sample_text, args, n);
	  
    if (show_xlfd)
	InformUser(view->display_xlfd);
    else
	InformUser(GetFontName(view, new_font, view->cur_xlfd));

StandardCursor(0);

} /* end of DisplayFont */


static void
UpdateSample (view_type *view, int item_index)
{
    static char buf[MAX_PATH_STRING];
    String p;
    int i=0;
    font_type *font_data = _OlArrayElement(view->ps_array, item_index).l;
    String xlfd_name;

    xlfd_name = font_data->xlfd_name;
#ifdef DEBUG
	fprintf(stderr,"UpdateSample: xlfd_name=%s\n",xlfd_name);
#endif
    strcpy(view->prev_xlfd, view->cur_xlfd);
#ifdef DEBUG
    fprintf(stderr,"UpdateSample: view->bitmap=%d\n",view->bitmap);
#endif
    
    if (view->bitmap) {
    	strcpy(view->display_xlfd, xlfd_name);
    	strcpy(view->cur_xlfd, xlfd_name);
    }
    else {
	BusyCursor(0);

	/* search for the pointsize field */
	for(p=xlfd_name; *p && (i < 8); p++) {
	    if (*p == DELIM)
		i++;
	}
	strncpy(buf, xlfd_name, p-xlfd_name);
	buf[p-xlfd_name] = 0; /* terminator */
	
	/* skip the resolution fields */
	for(i=0; *p && (i < 3); p++) {
	    if (*p == DELIM)
		i++;
	}

	/* fill in the pointsize and wild-card the resolution fields */
	sprintf(view->cur_xlfd, "%s%d0-0-0-%s", buf, view->cur_size, p);
	sprintf(view->display_xlfd, "%s0-0-0-%s", buf,  p);
    }

    XSync(XtDisplay(app_shellW), FALSE);
    ScheduleWork(DisplayFont, view, 10);

}  /* end of UpdateSample() */


static void
UpdateStyle (view_type *view, int family_index)
{
    int i;
    int j;
    Widget style_exclusive = view->style_exclusive;
    _OlArrayType(PSArray) * ps_array;
    style_info tmp;
    _OlArrayType(StyleArray) * style_array;

    style_array = _OlArrayElement(view->family_array, family_index).l;
    tmp.style_name = view->cur_style;
    _OlArrayFindHint(style_array, &i, tmp);
    if (i >= _OlArraySize(style_array)) {
	i = _OlArraySize(style_array) - 1;
    }

    /* update the style list */
    XtVaSetValues(style_exclusive, 
		  XtNitems, style_array->array,
		  XtNnumItems, _OlArraySize(style_array),
		  0);
    OlVaFlatSetValues(style_exclusive, i, XtNset, True,(String) 0);
    /* can't set XtNviewItemIndex with other resources */
    XtVaSetValues(style_exclusive, XtNviewItemIndex, i, 0);

    /*  Display the selected font.  */
    ps_array = _OlArrayElement(style_array, i).l;
    UpdatePS(view, ps_array, 0);

}  /* end of UpdateStyle() */


static void
UpdatePS (view, ps_array, delta)
    view_type *view;
    _OlArrayType(PSArray) * ps_array;
    int delta;
{
    Cardinal org_item_index, item_index;
    char cur_size_str[MAX_STRING];
    Arg args[1];
    Widget size_exclusive = view->size_exclusive;
    int hint;
    font_type *font_data;
    ps_info tmp;


    view->ps_array = ps_array;

    /* outline always have index of zero */
    font_data = _OlArrayElement(ps_array, 0).l;
    view->bitmap = font_data->bitmap;

#ifdef DEBUG
fprintf(stderr,"in UpdatePs ");
#endif
    XtSetArg(args[0], XtNuserData, &org_item_index);
    XtGetValues(size_exclusive, args, 1);

    /*  Find the current point size in the new ps_array, so that the
	current point size is maintained.  The hinted search returns
	the position that the current point size would have been
	inserted.  If the current size is not found, use the element
	at that index as the closest choice.  */
#ifdef DEBUG
fprintf(stderr,"view->bitmap =%d view->cur_size=%d\n", view->bitmap, view->cur_size);
#endif
    if (!view->bitmap) {
	if (view->cur_size == 0) view->cur_size = DEFAULT_PS_VALUE;
	else
	if (view->cur_size < MIN_PS_VALUE) view->cur_size = MIN_PS_VALUE;
		/* scalable font need to supply value */
	}
	sprintf(cur_size_str, "%d", view->cur_size);

    tmp.ps = cur_size_str;
    _OlArrayFindHint(ps_array, &hint, tmp);
    hint += delta;
    if (hint >= (int) _OlArraySize(ps_array))
	item_index = _OlArraySize(ps_array)-1;
    else if (hint < 0)
	item_index = 0;
    else
	item_index = hint;
    
    /* if needed, update the index */
    if (org_item_index != item_index) {
	/*  Make sure that the userData contains the point size.  */
	XtVaSetValues(size_exclusive, XtNuserData, item_index, 0);
    }

    /*  Update the size_exclusive to have the valid point sizes in
	the ps_array.  */
    XtVaSetValues (size_exclusive,
		   XtNitems, (XtArgVal)ps_array->array,
		   XtNnumItems, (XtArgVal)_OlArraySize(ps_array),
		   (String)0);
    OlVaFlatSetValues(size_exclusive, item_index, XtNset, True, (String)0);
    /* can't set XtNviewItemIndex with other resources */
    XtVaSetValues(size_exclusive, XtNviewItemIndex, item_index, 0);

    if (view->bitmap) {
	XtUnmanageChild(view->ps_text);
	XtManageChild(view->size_window);
    }
    else {
	XtUnmanageChild(view->size_window);
	XtManageChild(view->ps_text);
	XtVaSetValues( view->ps_text, XtNvalue, view->cur_size, NULL);
	item_index = 0;
    }

    /*  Display the selected font.  */
    UpdateSample(view, item_index);
}  /* end of UpdatePS() */


static void
UpdateFamily (view_type *view, int family_index)
{
    XtVaSetValues(view->family_exclusive, 
		      XtNitems, view->family_array->array,
		      XtNnumItems, _OlArraySize(view->family_array),
		      (String) 0);
    OlVaFlatSetValues(view->family_exclusive, family_index, XtNset, True, 0);
    /* can't set XtNviewItemIndex with other resources */
    XtVaSetValues(view->family_exclusive, XtNviewItemIndex, family_index, 0);

    /*  Update the style and size exclusives */
    UpdateStyle(view, family_index);

}  /* end of UpdateFamily() */


void
FamilySelectCB (w, client_data, call_data)
	Widget w;
	XtPointer client_data;
	XtPointer call_data;
{
        OlFlatCallData *        fd      = (OlFlatCallData *)call_data;
	view_type *view = (view_type *) client_data;
	int family_index = fd->item_index;

	UpdateStyle(view, family_index);
	strcpy(view->cur_family, 
	       _OlArrayElement(view->family_array, family_index).name);

}  /* end of FamilySelectCB() */


void
StyleSelectCB (w, client_data, call_data)
	Widget w;
	XtPointer client_data;
	XtPointer call_data;
{
    OlFlatCallData *        fd      = (OlFlatCallData *)call_data;
    view_type *view = (view_type *) client_data;
    _OlArrayType(PSArray) * ps_array;
    String style_str;

    OlVaFlatGetValues (w, fd->item_index,
		       XtNuserData, (XtArgVal) &ps_array,
		       XtNlabel, &style_str,
		       (String)0);
    strcpy( view->cur_style, style_str);

    UpdatePS(view, ps_array, 0);
}  /* end of StyleSelectCB() */


void
PSSelectCB (w, client_data, call_data)
	Widget w;
	XtPointer client_data;
	XtPointer call_data;
{
    view_type *view = (view_type *) client_data;
    OlFlatCallData *        fd      = (OlFlatCallData *)call_data;
    char *cur_size_str;

    OlVaFlatGetValues (w, fd->item_index,
		       XtNlabel, &cur_size_str,
		       (String)0);
    view->cur_size = atoi( cur_size_str);

    /*  Make sure that the userData contains the set index.  */
    XtVaSetValues(w,
		XtNuserData, fd->item_index,
		(String)0);

    /*  Display the selected font.  */
    UpdateSample(view, fd->item_index);

}  /* end of PSSelectCB() */


static void
TimeOutHandler(view_type *view)
{
    /*  Display the selected font.  */
    UpdateSample(view, 0);

    /* mark timer as off */
    view->timer_id = 0;
} /* end of TimeOutHandler */


/*
 * This routine is used to handle multi-click to the integerField widget.
 * It will update the pointsize only to the last click and not all the
 * clicks in between
 */
void
OutlinePSCB(w, client_data, call_data)
    Widget w;
    XtPointer client_data;
    XtPointer call_data;
{
    OlIntegerFieldChanged *cbP = (OlIntegerFieldChanged *) call_data;
    int			value = cbP->value;
    view_type *view = (view_type *) client_data;

    view->bitmap = 0;
    view->cur_size = value;

    if (view->timer_id) 
	XtRemoveTimeOut(view->timer_id);
    view->timer_id = XtAddTimeOut(500, 
				  (XtTimerCallbackProc)TimeOutHandler, view);
} /* end of OutlinePSCB */


void
ResetFont (view)
	view_type *view;
{
    Arg args[1];
    _OlArrayType(StyleArray) * style_array;
    _OlArrayType(PSArray) * ps_array;
    family_info tmp;
    int family_index;
    int spot;

    tmp.name = view->cur_family;
    if ((spot = _OlArrayFindHint(view->family_array, &family_index, tmp)) ==
	_OL_NULL_ARRAY_INDEX)  {
	/* reset the view */
	strcpy(view->cur_family, DEFAULT_FAMILY);
	strcpy(view->cur_style, DEFAULT_LOOK);
	view->cur_size = atoi(DEFAULT_POINT_SIZE);
	tmp.name = view->cur_family;
	if (_OlArrayFindHint(view->family_array, &family_index, tmp) ==
	    _OL_NULL_ARRAY_INDEX)
	    family_index = 0;
    }
    UpdateFamily(view, family_index);

}  /* end of ResetFont() */


