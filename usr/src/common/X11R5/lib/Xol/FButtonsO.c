#ifndef NOIDENT
#ident	"@(#)flat:FButtonsO.c	1.43"
#endif

#include <ctype.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include "Xol/FButtonsP.h"
#include <Xol/PopupMenuP.h>

#define ClassName FlatButtonsO
#include <Xol/NameDefs.h>

static void _FBTweakGCs OL_ARGS((FlatPart *, FlatItem, OlgTextLbl *,
				 OlgPixmapLbl *));

static void HandleButton OL_ARGS((Widget, Position, Position, 
				    OlVirtualName, Cardinal, Boolean));

/* _OloFBItemLocCursorDims - a dummy func for O/L mode...
 */
extern Boolean
_OloFBItemLocCursorDims OLARGLIST((w, item, di))
	OLARG( Widget,			w)
	OLARG( FlatItem,		item)	/* expanded item */
	OLGRA( OlFlatDrawInfo *,	di)	/* store dimensions here */
{
	return(False);
} /* end of _OloFBItemLocCursorDims */

/* ARGSUSED */
static Boolean
FBSetUpRect OLARGLIST((w, item_rec, check_all, di, flags))
  OLARG( Widget,		w)		/* flat widget id	*/
  OLARG( FlatItem,	 	item_rec)	/* expanded item        */
  OLARG( Boolean,		check_all)	/* F: draw_selected only */
  OLARG( OlFlatDrawInfo *, 	di)		/* Drawing information  */
  OLGRA( unsigned *,		flags)		/* property flags	*/
{
  FlatButtonsPart *	fexcp        = FEPART(w);
  FlatButtonsItemPart *	item_part    = FEIPART(item_rec);
  Cardinal		item_index   = item_rec->flat.item_index;
  Boolean		draw_selected;
  
  if (fexcp->exclusive_settings == True)
    {
      draw_selected = (Boolean)
	((item_index == fexcp->current_item &&
	  !(fexcp->none_set == True &&
	    fexcp->current_item == fexcp->set_item))
	 ||
	 (fexcp->current_item == (Cardinal) OL_NO_ITEM &&
	  item_part->set == True) ? True : False);
    }
  else
    {
      draw_selected = (Boolean)
	((item_index == fexcp->current_item &&
	  item_part->set == False)
	 ||
	 (fexcp->current_item != item_index &&
	  item_part->set == True) ? True : False);
    }

  if (!check_all)
	return(draw_selected);
  
  if (fexcp->preview != (Widget)NULL)
    {
      draw_selected = (draw_selected == True ? False : True);
    }
  
  if (draw_selected == True)
    {
      *flags |= RB_SELECTED;
    }
  
  if (fexcp->current_item == (Cardinal)OL_NO_ITEM &&
      (fexcp->default_item == item_index || 
       item_part->is_default == True))
    {
      *flags |= RB_DEFAULT;
    }
  
  if (fexcp->dim == True)
    {
      *flags |= RB_DIM;
    }
  
  if (item_rec->flat.sensitive == (Boolean) False || 
      !XtIsSensitive (w))
    {
      *flags |= RB_INSENSITIVE;
    }

  return(draw_selected);
} /* end of FBSetUpRect */

extern void
_OloFBDrawItem OLARGLIST((w, item_rec, di))
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
	/* for OL_CHECKBOX */
	Dimension		padding;
	Dimension		label_width;
	int			int_x;
	Position		check_y;
	OlFlatScreenCache *	sc;

	if (item_rec->flat.mapped_when_managed == False)
	{
		return;
	}

					/* Determine if we should draw
					 * the selected box	*/

	switch(fexcp-> button_type)
	{
	case OL_OBLONG_BTN:

		draw_selected = item_index == current_item;
		/*
 		 * Add (potential) menu button component to visual 
		 * and, if current,
 		 * Post the associated menu.
 		 */
		if (fexcp->menu_descendant)
			flags |= OB_MENUITEM;

		if (item_part->popupMenu)
         		if (RCPART(w)->layout_type == OL_FIXEDCOLS)
				/*  && flatp-> measure == 1) */
				flags |= OB_MENU_R;
			else	
				flags |= OB_MENU_D;

		if (item_index == fexcp-> set_item || item_part->is_busy)
			flags |= OB_BUSY;

					/* Draw the default box		*/

#if 0
		if (current_item == (Cardinal)OL_NO_ITEM &&
#else
		if (fexcp->default_item != current_item &&
#endif
	    	   (fexcp->default_item == item_index || 
		    item_part->is_default == True))
		{
	        	flags |= OB_DEFAULT;
		}


		if (item_rec->flat.sensitive == (Boolean) False || 
		    !XtIsSensitive (w))
		{
	        	flags |= OB_INSENSITIVE;
		}

		/* Get GCs and label information */
		OlFlatSetupAttributes (w, item_rec, di, 
		   &item_attrs, (XtPointer *)&lbl, &drawProc);

			/* Postpone this setup because we need
			 * to override lbl->text.flags...
			 *
			 * Note that we probably should put
			 * changes in OlFlatSetupAttributes
			 * and/or OlFlatSetupLabelSize but
			 * this is 4m/P12 and I don't think
			 * it's wise to change interface now.
			 */
		if (draw_selected)
		{
			flags |= OB_SELECTED;

			if (!OlgIs3d())
				lbl->text.flags |= TL_SELECTED;
		}


		/* Draw the button */
		OlgDrawOblongButton (di->screen, di->drawable, item_attrs, 
		   di->x, di->y, di->width, di->height, (caddr_t)lbl,
		   (OlgLabelProc) drawProc, flags);

		if (flags & OB_SELECTED)
			_OlFBPostMenu(w, item_index, di, item_part);
		break;

	case OL_RECT_BTN:

		/* Get GCs and label information */
		OlFlatSetupAttributes (w, item_rec, di, 
				       &item_attrs, (XtPointer *)&lbl,
				       &drawProc);

		(void)FBSetUpRect(w, item_rec, True, di, &flags);
		
		/* Draw the button */
		OlgDrawRectButton (di->screen, di->drawable, item_attrs, 
				   di->x, di->y, di->width, di->height,
				   lbl, drawProc, flags);
		break;

	case OL_CHECKBOX:
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

		padding		= sc->check_width +
				OlgGetHorizontalStroke (item_attrs) * PADDING;
		label_width	= di->width - padding;
		int_x		= (int) di->x;

		if (fexcp-> position == (OlDefine)OL_LEFT)
		{
			(*drawProc) (di->screen, di->drawable, fp->pAttrs,
			      int_x, di->y, label_width, di->height, lbl);
			int_x = (int)di->x + (int) (di->width - sc->check_width);
		}

		if (FBSetUpRect(w, item_rec, False, di, &flags))
			flags = CB_CHECKED;
#if 0
		if ((item_rec->flat.item_index == fexcp->current_item &&
		     item_part->set == False) ||
		   (fexcp->current_item != item_rec->flat.item_index &&
		     item_part->set == True))
		{
	    		flags = CB_CHECKED;
		}
#endif

		if (item_rec->flat.sensitive == (Boolean)False ||
	    	    !XtIsSensitive(w) || 
		    fexcp->dim == (Boolean)True)
		{
	    		flags |= CB_DIM;
		}

		check_y = di->y + (int) (di->height - sc->check_height) / 2;

		OlgDrawCheckBox (di->screen, di->drawable, item_attrs, 
				 int_x, check_y, flags);

			/* Now put in the label or the image if
			 * it's to the right of the check box.	*/

		if (fexcp-> position != (OlDefine)OL_LEFT)
		  {
		    int_x += (int)padding;
		    (*drawProc) (di->screen, di->drawable, fp->pAttrs,
				 int_x, di->y, label_width, di->height, lbl);
		  }

		break;
	      }

} /* END OF FBDrawItem() */

/*
 * FBTweakGCs -- for checkboxes.  The Olg*Lbl structures must change
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
} /* end of _FBTweakGCs */

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

	switch(virtual_name)
	{
	case OL_SELECT:
		if (_OlSelectDoesPreview(w))
		{
			_OlResetStayupMode(w);
			_OlSetPreviewMode(w);
			_OlFBSetToCurrent(
				w, False, x, y, new_current, is_motion);
			break;
		}
		/* FALLTHROUGH */
	case OL_MENU:
		_OlResetPreviewMode(w);
		_OlFBSetToCurrent(
			w,
			virtual_name == OL_MENU ? True : False,
			x, y, new_current, is_motion);
		break;
	case OL_MENUDEFAULT:
#if 0
			/* no need to call this routine because
			 * it's a no-op anyway...
			 */
		if (XtIsSubclass(w, flatButtonsWidgetClass))
		{
			_OlFBUnsetCurrent(w);
		}
#endif
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
 * _OloFBButtonHandler - this routine handles all button press and release
 * events.
 ****************************procedure*header*****************************
 */
extern void
_OloFBButtonHandler OLARGLIST((w, ve))
	OLARG( Widget,		w)
	OLGRA( OlVirtualEvent,	ve)
{
	/* used by CALL_SELECTPROC...	*/
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
	       CALL_SELECTPROC(w, ve->virtual_name, ve->item_index)
	    }

            HandleButton(w, x, y, ve-> virtual_name, ve->item_index, True);
            break;
         case ButtonPress:
            x = ve->xevent->xbutton.x;
            y = ve->xevent->xbutton.y;
	    last_item_index = RESET_FLAG;

	    if ( ve->virtual_name == OL_SELECT )
	    {
#define SET_FOCUS	if (ve->item_index != OL_NO_ITEM)	\
			  OlFlatCallAcceptFocus(w, ve->item_index, time); \
			else XtCallAcceptFocus(w, &time)

#define NOT_SAME_ITEM	(ve->item_index != OL_NO_ITEM &&	\
			 FPART(w)->focus_item != ve->item_index)

				/* I have to use CurrentTime because
				 * olwm always uses CurrentTime when
				 * sending WM_TAKE_FOCUS...
				 */
		Boolean	done = False;
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

			/* Menu operation... */
	       if ( (ve->virtual_name == OL_MENU ||
		     ve->virtual_name == OL_SELECT &&
		     !_OlSelectDoesPreview(w)) )
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
	       CALL_SELECTPROC(w, ve->virtual_name, ve->item_index)
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
            if (_OlIsPendingStayupMode(shell))
               _OlResetStayupMode(shell);
            HandleButton(w, x, y, ve-> virtual_name, ve->item_index,
			_OlIsNotInStayupMode(w) ? True : False);
            break;
         case ButtonRelease:
	    last_item_index = RESET_FLAG;

            x = ve->xevent->xbutton.x;
            y = ve->xevent->xbutton.y;
	    if (ve->item_index == OL_NO_ITEM)
	    {
		_OlFBUnsetCurrent(w);
		break;
	    }
            switch(ve->virtual_name)
               {
               case OL_SELECT:
		if (ve->item_index != OL_NO_ITEM)
		{
                  _OlFBSetToCurrent(w, False, x, y, ve->item_index, False);
                  _OlFBSelectItem(w);
		}
                  break;
               case OL_MENU:
		if (ve->item_index != OL_NO_ITEM)
		{
                  _OlFBSetToCurrent(w, True, x, y, ve->item_index, False);
                  _OlFBSelectItem(w);
		}
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
	 _OlFBResetParentSetItem(w, True);
         _OlPopdownCascade(_OlRootOfMenuStack(w), False);
         }
} /* END OF _OloFBButtonHandler() */
