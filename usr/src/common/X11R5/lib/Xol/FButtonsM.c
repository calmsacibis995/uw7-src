#ifndef NOIDENT
#ident	"@(#)flat:FButtonsM.c	1.55"
#endif

#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include "Xol/FButtonsP.h"
#include <Xol/PopupMenuP.h>

#define ClassName FlatButtonsM
#include <Xol/NameDefs.h>

#define HILITETHICKNESS	((PrimitiveWidget)w)->primitive.highlight_thickness

	/* Motif certification checklist #3-6, ul92-13355,
	 * default ring should follow focus item...
	 */
#define IS_DEFAULT(wid,fitem)	\
		( (FEPART(wid)->default_item != OL_NO_ITEM &&		     \
		   FPART(wid)->focus_item == (fitem)->flat.item_index)	   ||\
		  (FPART(wid)->focus_item == OL_NO_ITEM &&		     \
		   (FEPART(wid)->default_item == (fitem)->flat.item_index || \
		    FEIPART(fitem)->is_default == True)) )


static void _FBTweakGCs OL_ARGS((FlatPart *, FlatItem, OlgTextLbl *,
				 OlgPixmapLbl *));

static void HandleButton OL_ARGS((Widget, Position, Position, 
					 OlVirtualName, Cardinal, Boolean));


/* _OlmFBItemLocCursorDims - figure out the place to draw location cursor
 */
extern Boolean
_OlmFBItemLocCursorDims OLARGLIST((w, item, di))
	OLARG( Widget,			w)
	OLARG( FlatItem,		item)	/* expanded item */
	OLGRA( OlFlatDrawInfo *,	di)	/* store dimensions here */
{
	if (FEPART(w)->button_type != OL_OBLONG_BTN)
		return(True);

		/* no need to draw location if this is a menu item */
	if (FEPART(w)->menu_descendant)
		return(False);

	if ( IS_DEFAULT(w, item) )
	{
		;	/* default item - EMPTY */
	}
	else
	{
		Dimension	diff, diff2;

#define max(a,b)	((a)<(b) ? (b) : (a))
#define PSIZE(z,s)	(int)max(OlScreenPointToPixel(OL_HORIZONTAL,z,s),\
				 OlScreenPointToPixel(OL_VERTICAL,z,s))

			/* See ItemDimension for why */
		diff		= PSIZE(4, di->screen);
		diff2		= 2 * diff;

		di->x		+= diff;
		di->y		+= diff;
		di->width	-= diff2;
		di->height	-= diff2;
#undef max
#undef PSIZE
	}
	return(True);

} /* end of _OlmFBItemLocCursorDims */

/* _OlmFBDrawItem - this routine draws a single item for the container.
 */
extern void
_OlmFBDrawItem OLARGLIST((w, item_rec, di))
	OLARG( Widget,		 w)		/* flat widget id	*/
	OLARG( FlatItem,	 item_rec)	/* expanded item        */
	OLGRA( OlFlatDrawInfo *, di)		/* Drawing information  */
{
	FlatPart *		fp           = FPART(w);
	FlatButtonsPart *	fexcp        = FEPART(w);
	FlatButtonsItemPart *	item_part    = FEIPART(item_rec);
	Cardinal		item_index   = item_rec->flat.item_index;
	Cardinal		current_item = fexcp-> current_item;
	unsigned		flags        = 0;
	OlgAttrs *		item_attrs;
	OlgLabelProc		drawProc;
	union {
		OlgTextLbl	text;
		OlgPixmapLbl	pixmap;
	} *lbl;
	Boolean			draw_selected;
		/* The following variables are not for OL_OBLONG_BTN */
	Dimension		padding;
	Dimension		label_width;
	Dimension		hStroke;
	int			int_x;
	Position		check_y;
	OlFlatScreenCache *	sc;
	XGCValues		values;

	if (item_rec->flat.mapped_when_managed == False)
	{
		return;
	}

		/* Use superclass draw_item to draw Motif Location Cursor */
	if (fp->focus_item == item_index)
	{
#define SUPERCLASS \
	  ((FlatButtonsClassRec *)flatButtonsClassRec.core_class.superclass)

		(*SUPERCLASS->flat_class.draw_item)(w, item_rec, di);

#undef SUPERCLASS
	}

	switch(fexcp-> button_type)
	{
	case OL_OBLONG_BTN:

			/* Determine if we should draw the selected box	*/
		draw_selected = item_index == current_item;

			/*
			 * Add (potential) menu button component to visual 
			 * and, if current,
			 * Post the associated menu.
			 */
		if (fexcp->menu_descendant)
			flags |= OB_MENUITEM;

		if (item_part->popupMenu)
		{
         		if (RCPART(w)->layout_type == OL_FIXEDCOLS)
				/*  && flatp-> measure == 1) */
				flags |= OB_MENU_R;
			else	
				flags |= OB_MENU_D;
		}

		if (item_index == fexcp-> set_item || item_part->is_busy)
			flags |= OB_BUSY;
		
			/* Draw the default box		*/
		if ( IS_DEFAULT(w, item_rec) )
		{
	        	flags |= OB_DEFAULT;
		}

		if (item_rec->flat.sensitive == (Boolean)False || 
		    !XtIsSensitive (w))
		{
	        	flags |= OB_INSENSITIVE;
		}

		if (fexcp->fill_on_select == (Boolean) True)
			flags |= OB_FILLONSELECT;

			/* Get GCs and label information */
		OlFlatSetupAttributes (w, item_rec, di, 
			&item_attrs, (XtPointer *)&lbl, &drawProc);

			/* draw selected also if have focus on a menu item */
			/* See comments in FButtonsO.c...	*/
		{
			Widget		shell = _OlGetShellOfWidget(w);
			Boolean		shell_is_popup_menu;

			if ( shell_is_popup_menu = XtIsSubclass(
					shell, popupMenuShellWidgetClass) )
			{
				Arg	arg[1];
				XtSetArg(
					arg[0],
					XtNoverrideRedirect,
					&shell_is_popup_menu);

				XtGetValues(shell, arg, 1);
			}

			if ( draw_selected ||
			    ((flags & OB_MENUITEM) &&
			     (item_index == fp->focus_item) &&
			     (!shell_is_popup_menu ||
			      _OlIsInStayupMode(w))) )
			{
				flags |= OB_SELECTED;

				if (!OlgIs3d())
					lbl->text.flags |= TL_SELECTED;
			}
		}

			/* Old code steals "shadow_thickness" field from
			 * superclass, it's wrong and it's preventing
			 * from having border shadow in a Motif menubar.
			 * An item resource, XtNshadowThickness, is used
			 * for the replacement. Also see FBInitialize...
			 */

		item_attrs->pDev->shadow_thickness =
			item_part->shadow_thickness;

		if ((flags & OB_FILLONSELECT)) {

			/* store old bg2 foreground, change to
			 * select_color, draw the button, restore
			 */
		  XGetGCValues(DisplayOfScreen(di->screen),
			       item_attrs->bg2, GCForeground, &values);
		  
		  XSetForeground(DisplayOfScreen(di->screen),
				 item_attrs->bg2, fexcp->select_color);
		}

			/* Draw the button */
		{
			Position	x, y;
			Dimension	ww, hh;

			if (flags & OB_MENUITEM)
			{
				x = di->x;
				y = di->y;
				ww = di->width;
				hh = di->height;
			}
			else
			{
				x = di->x + HILITETHICKNESS;
				y = di->y + HILITETHICKNESS;
				ww = di->width - 2 * HILITETHICKNESS;
				hh = di->height - 2 * HILITETHICKNESS;
			}
			OlgDrawOblongButton (
				di->screen, di->drawable, item_attrs, 
				x, y, ww, hh,
				(caddr_t)lbl, (OlgLabelProc)drawProc, flags);
		}

		if (flags & OB_FILLONSELECT) {
			/* reset gc */
		  XSetForeground(DisplayOfScreen(di->screen),
				 item_attrs->bg2, values.foreground);
		}
		
			/*
			 * Note that, we can't use "flags" in this case
			 * otherwise it may pop up a redundant menu
			 * (see P8 load notes). This is because additonal
			 * checking will be made in Motif mode when
			 * setting "OB_SELECTED" and these checking don't
			 * apply to the decision whether _OlFBPostMenu
			 * should be invoked. You should compared to
			 * FButtonsO.c for additional info.
			 */
		if (draw_selected)
			_OlFBPostMenu(w, item_index, di, item_part);
		break;

	case OL_CHECKBOX:
		/* FALLTHROUGH */
	case OL_RECT_BTN:

		if (fexcp->exclusive_settings == True)
		  flags |= CB_DIAMOND;

		sc = OlFlatScreenManager(w, OL_DEFAULT_POINT_SIZE, 
						OL_JUST_LOOKING);

			/* Get GCs and label information */
		OlFlatSetupAttributes (w, item_rec, di, 
		   &item_attrs, (XtPointer *)&lbl, &drawProc);

			/* Now Tweak the GCs in the returned label
			 * structure since the label never reflects the
			 * background or input focus color of the
			 * checkbox.					*/
		_FBTweakGCs(fp, item_rec, &(lbl->text), &(lbl->pixmap));

			/* Now put in the label or the image if
			 * it's to the left of the check box.	*/
		hStroke		= OlgGetHorizontalStroke (item_attrs);
		
		padding		= sc->check_width + hStroke * MPADDING;
		label_width	= di->width - padding - 2 * HILITETHICKNESS;
		int_x		= (int) di->x + HILITETHICKNESS;

		if (fexcp-> position == (OlDefine)OL_LEFT)
		{
			(*drawProc) (di->screen, di->drawable, fp->pAttrs,
			      int_x, di->y,label_width,
				     di->height, lbl); 
			int_x += (int) (di->width - sc->check_width);
		}

			/* I'm confused with the logic but this's
			 * how is handled in OPEN LOOK mode...
			 */
		{
			Boolean draw_selected;

			if (fexcp->exclusive_settings == True)
			{
				draw_selected = (Boolean)
					( (item_rec->flat.item_index ==
						fexcp->current_item &&
					  !(fexcp->none_set == True &&
					    fexcp->current_item ==
						fexcp->set_item))
					||
					  (fexcp->current_item ==
						(Cardinal)OL_NO_ITEM &&
					   item_part->set == True)
							? True : False );
			}
			else
			{
				draw_selected = (Boolean)
					( (item_rec->flat.item_index ==
						fexcp->current_item &&
					   item_part->set == False)
					||
					  (fexcp->current_item !=
						item_rec->flat.item_index &&
					   item_part->set == True)
							? True : False );
			}

			if (draw_selected)
				flags |= CB_CHECKED;
		}

		if (item_rec->flat.sensitive == (Boolean)False ||
	    	    !XtIsSensitive(w) || 
		    fexcp->dim == (Boolean)True)
		{
	    		flags |= CB_DIM;
		}

		if (fexcp->fill_on_select == (Boolean) True)
			flags |= CB_FILLONSELECT;

		if (flags & CB_FILLONSELECT) {

			/* store old bg2 foreground, change to
			 * select_color, draw the checkbox,
			 * restore afterward
			 */
		  XGetGCValues(DisplayOfScreen(di->screen),
			       item_attrs->bg2, GCForeground, &values);
		  
		  XSetForeground(DisplayOfScreen(di->screen),
				 item_attrs->bg2, fexcp->select_color);
		}

		check_y	= di->y + (int)(di->height - sc->check_height) / 2;
						 
		OlgDrawCheckBox (di->screen, di->drawable, item_attrs, 
				 int_x, check_y, flags);

		if (flags & CB_FILLONSELECT) {
			/* reset gc... */
		  XSetForeground(DisplayOfScreen(di->screen),
				 item_attrs->bg2, values.foreground);
		}

			/* Now put in the label or the image if
			 * it's to the right of the check box.	*/

		if (fexcp-> position != (OlDefine)OL_LEFT)
		{
			int_x += (int)padding;
			(*drawProc) (di->screen, di->drawable, fp->pAttrs,
				     int_x,di->y, label_width,
				     di->height, lbl);  
#undef HILITETHICKNESS
		}

	break;
	}

} /* END OF _OlFBDrawItem() */

/*
 * _FBTweakGCs -- for checkboxes.  The Olg*Lbl structures must change
 * because the label never reflcets the bg or input focus color of the
 * checkbox.
 */

static void
_FBTweakGCs OLARGLIST((fp, item_rec, text, pixmap))
  OLARG( FlatPart *,	fp )
  OLARG( FlatItem,	item_rec )
  OLARG( OlgTextLbl *,	text )
  OLGRA( OlgPixmapLbl *, pixmap )
{
  if (item_rec-> flat.label != (String)NULL)
    {
      text->normalGC = fp->label_gc;
      text->inverseGC = fp->inverse_gc;
    }
  else 
    if (item_rec-> flat.label_image != (XImage *) NULL ||
	(item_rec-> flat.background_pixmap != (Pixmap)None &&
	 item_rec-> flat.background_pixmap != (Pixmap)ParentRelative &&
	 item_rec-> flat.background_pixmap != XtUnspecifiedPixmap))
      {
	pixmap->normalGC = fp->label_gc;
      }
}

/*
 * HandleButton
 *
 * This procedure processes the Press/Motion button events.
 * It's written separately to clarify the FBButtonHandler code.
 *
 */

static void
HandleButton OLARGLIST((w, x, y, virtual_name, new_current, is_motion))
  OLARG( Widget,        w)
  OLARG( Position,      x)
  OLARG( Position,      y)
  OLARG( OlVirtualName, virtual_name)
  OLARG( Cardinal,	new_current)
  OLGRA( Boolean,	is_motion)
{
  FlatButtonsPart *	fexcp = FEPART(w);


	switch(virtual_name)
	{
	case OL_SELECT:
	  _OlResetPreviewMode(w);
	  _OlFBSetToCurrent(
		w, fexcp->button_type == OL_OBLONG_BTN, x, y,
		new_current, is_motion);
	  break;
	case OL_MENU:
	{
	  Widget	shell = _OlGetShellOfWidget(w);

	  if (XtIsSubclass(shell, popupMenuShellWidgetClass))
	  {
	      _OlResetPreviewMode(w);
	      _OlFBSetToCurrent(w, True, x, y, new_current, is_motion);
          }
	  break;
	}
	case OL_MENUDEFAULT:
	  _OlFBSetDefault(w, False, x, y, new_current);
	  break;
	default:
		if (!_OlIsInStayupMode(w))
			_OlFBUnsetCurrent(w);
	  break;
	}

} /* end of HandleButton */

/*
 *************************************************************************
 * _OlmFBButtonHandler - this routine handles all button press and release
 * events.
 ****************************procedure*header*****************************
 */
void
_OlmFBButtonHandler OLARGLIST((w, ve))
	OLARG( Widget,		w)
	OLGRA( OlVirtualEvent,	ve)
{
	/* used by CALL_SELECTPROC...  */
#define RESET_FLAG	OL_NO_ITEM

   static Cardinal   last_item_index = RESET_FLAG;

   Position          x;
   Position          y;
   FlatButtonsPart * fexcp;
   Widget            widget = XtWindowToWidget(ve-> xevent-> xany.display, 
                                               ve-> xevent-> xany.window);
   Widget            shell = _OlGetShellOfWidget(widget);

   if (w != widget) /* remap events */
      {
      if (ve-> xevent-> type == ButtonPress)
         if (!_OlIsInMenuStack(shell))
            _OlResetStayupMode(shell);
      }
   else
      {
      Boolean call_selectproc = False;

      if ( ve->virtual_name == OL_SELECT ||
	  (ve->virtual_name == OL_MENU &&
	   XtIsSubclass(shell, popupMenuShellWidgetClass)) )
		call_selectproc = True;

      fexcp = FEPART(w);

      ve->consumed = True;
      if (fexcp-> button_type == OL_OBLONG_BTN && ve->item_index != OL_NO_ITEM)
      {
		Arg	args[1];
		Boolean	is_busy = False;

		XtSetArg(args[0], XtNbusy, (XtArgVal)&is_busy);
		OlFlatGetValues(w, ve->item_index, args, 1);
		if (is_busy)
		{
			return;
		}
      }
      switch(ve->xevent->type)
         {
         case MotionNotify:
            x = ve->xevent->xmotion.x;
            y = ve->xevent->xmotion.y;

		/* Make sure (x, y) really points to OL_NO_ITEM,
		 * not because it's an insensitive item (2nd check).
		 */
	    if (ve->item_index == OL_NO_ITEM &&
		OlFlatGetIndex(w, x, y, True) == OL_NO_ITEM)
	    {
		break;
	    }
	    if (fexcp-> button_type == OL_OBLONG_BTN)
	    {
	      FlatPart *	fp = FPART(w);

	      if (call_selectproc)
	      {
		CALL_SELECTPROC(w, ve->virtual_name, ve->item_index)
	      }

	      if (fexcp->menubar_behavior == True &&
		  ve->item_index != OL_NO_ITEM &&
		  fp->focus_item != OL_NO_ITEM)
		{
		  Cardinal	focus_item = fp->focus_item;
		  
		  fp->focus_item = ve->item_index;

		  if (focus_item != ve->item_index)
		    OlFlatRefreshItem(w, focus_item, True);
		}
	     }

            HandleButton(w, x, y, ve-> virtual_name, ve->item_index, True);
            break;
         case ButtonPress:
            x = ve->xevent->xbutton.x;
            y = ve->xevent->xbutton.y;
	    last_item_index = RESET_FLAG;

	    if (ve->virtual_name == OL_SELECT)
	    {
#define SET_FOCUS	if (ve->item_index != OL_NO_ITEM)	\
			  OlFlatCallAcceptFocus(w, ve->item_index, time); \
			else XtCallAcceptFocus(w, &time)

#define NOT_SAME_ITEM	(ve->item_index != OL_NO_ITEM &&	\
			 FPART(w)->focus_item != ve->item_index)

				/* I have to use CurrentTime because
				 * olwm uses CurrentTime when sending
				 * out WM_TAKE_FOCUS...
				 */
		Boolean done = False;
                Time    time = /*CurrentTime */ve->xevent->xbutton.time;
		Widget	cfw  = OlGetCurrentFocusWidget(w);

		if ( ((PrimitiveWidget)w)->primitive.traversal_on == True &&
		     (fexcp-> button_type != OL_OBLONG_BTN ||
		     (!fexcp->menubar_behavior && fexcp->focus_on_select)) )
		{
			if ( cfw == NULL ||
			     (cfw != w  ? True : NOT_SAME_ITEM) )
			{
				done = True;
				SET_FOCUS;
			}
		}

		if ( cfw == NULL && !done &&
		     !(_OlRootOfMenuStack(w) == w && _OlIsInStayupMode(w)) )
		{
			XtCallAcceptFocus(shell, &time);
		}

#undef SET_FOCUS
#undef NOT_SAME_ITEM
	    }

            if (fexcp-> button_type == OL_OBLONG_BTN)
	    {
	      if ( ve->virtual_name == OL_SELECT )
	      {
			if ( _OlRootOfMenuStack(w) == w &&
			     _OlIsInStayupMode(w) &&
				/* current_menu_item != last_menu_item */
			     ve->item_index != _OlFBFindMenuItem(w,
						_OlRoot1OfMenuStack(w)) )
			{
				_OlResetStayupMode(w);
				_OlPopdownCascade(_OlRootOfMenuStack(w), True);
			}
	      }

	      _OlMenuLock(shell, w, ve-> xevent);
	      if (call_selectproc)
	      {
		CALL_SELECTPROC(w, ve->virtual_name, ve->item_index)
	      }
	    }
            else /* exclusives/nonexclusives/checkbox */
               {
               _OlUngrabPointer(w);
               _OlResetStayupMode(shell);
               }
            HandleButton(w, x, y, ve-> virtual_name, ve->item_index, False);

            break;
         case EnterNotify:
	    if (ve->item_index == OL_NO_ITEM)
	    {
		break;
	    }
            x = ve->xevent->xcrossing.x;
            y = ve->xevent->xcrossing.y;
            if (_OlIsPendingStayupMode(w))
               _OlResetStayupMode(w);
            HandleButton(w, x, y, ve-> virtual_name, ve->item_index,
			_OlIsNotInStayupMode(w) ? True : False);
            break;
         case ButtonRelease:

		/* Menubar should give up focus once press-drag-release
		 * operation is performed. 
		 */
            if (fexcp-> button_type == OL_OBLONG_BTN &&
	        ve->virtual_name == OL_SELECT &&
                _OlIsNotInStayupMode(w) &&
	        OlGetCurrentFocusWidget(w) == _OlGetMenubarWidget(w))
	    {
                Time    time = /*CurrentTime */ve->xevent->xbutton.time;
		XtCallAcceptFocus(shell, &time);
	    }

	    last_item_index = RESET_FLAG;

            x = ve->xevent->xbutton.x;
            y = ve->xevent->xbutton.y;
            switch(ve->virtual_name)
               {
	       case OL_MENU:
		  if (XtIsSubclass(shell, popupMenuShellWidgetClass)) {
                    _OlFBSetToCurrent(w, True, x, y, ve->item_index, False);
                    _OlFBSelectItem(w);
                  }
                  break;
	       case OL_SELECT:
                  _OlFBSetToCurrent(w, (fexcp-> button_type == OL_OBLONG_BTN),
				 x, y, ve->item_index, False);
                  _OlFBSelectItem(w);
                  break;
               default:
                  _OlFBUnsetCurrent(w);
                  break;
               }
	    break;
         case LeaveNotify:
	    last_item_index = RESET_FLAG;

            if (_OlTopOfMenuStack(w) == _OlRootOfMenuStack(w) || 
                _OlTopOfMenuStack(w) == shell)
               _OlFBUnsetCurrent(w);
            break;
         default:
            break;
         }
      }
   /*
    * test for spring loaded release events
    */

   if (ve-> xevent-> type == ButtonRelease &&
       !_OlIsEmptyMenuStack(w) && _OlIsNotInStayupMode(w))
         {
         _OlPopdownCascade(_OlRootOfMenuStack(w), False);
         }

} /* END OF _OlmFBButtonHandler() */
