#pragma ident	"@(#)m1.2libs:Xm/TextSel.c	1.5"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/* Private definitions. */

#include <X11/Xatom.h>
#include <Xm/TextP.h>
#include <Xm/TextStrSoP.h>
#include <Xm/TextSelP.h>
#include <Xm/AtomMgr.h>
#include <Xm/DragC.h>


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void InsertSelection() ;
static void HandleInsertTargets() ;
static Boolean ConvertInsertSelection() ;

#else

static void InsertSelection( 
                        Widget w,
                        XtPointer closure,
                        Atom *seltype,
                        Atom *type,
                        XtPointer value,
                        unsigned long *length,
                        int *format) ;
static void HandleInsertTargets( 
                        Widget w,
                        XtPointer closure,
                        Atom *seltype,
                        Atom *type,
                        XtPointer value,
                        unsigned long *length,
                        int *format) ;
static Boolean ConvertInsertSelection( 
                        Widget w,
                        Atom *selection,
                        Atom *type,
                        XtPointer *value,
                        unsigned long *length,
                        int *format,
                        Atom locale_atom) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


/* ARGSUSED */
static void
#ifdef _NO_PROTO
InsertSelection( w, closure, seltype, type, value, length, format )
        Widget w ;
        XtPointer closure ;
        Atom *seltype ;
        Atom *type ;
        XtPointer value ;
        unsigned long *length ;
        int *format ;
#else
InsertSelection(
        Widget w,
        XtPointer closure,
        Atom *seltype,
        Atom *type,
        XtPointer value,
        unsigned long *length,
        int *format )
#endif /* _NO_PROTO */
{
    _XmInsertSelect *insert_select = (_XmInsertSelect *)closure;
    XmTextWidget tw = (XmTextWidget) w;
    XmTextPosition left = 0;
    XmTextPosition right = 0;
    Boolean dest_disjoint = False;
    Atom COMPOUND_TEXT = XmInternAtom(XtDisplay(w), "COMPOUND_TEXT", False);
    char * total_tmp_value = NULL;
    XTextProperty tmp_prop;
    char **tmp_value;
    int num_vals;
    int malloc_size = 0;
    int i, status;
    XmTextBlockRec block, newblock;
    XmTextPosition cursorPos;
    Boolean freeBlock;

    if (!value) {
        insert_select->done_status = True;
        return;
    }

  /* Don't do replace if there is no text to add */
    if (*(char *)value == '\0' || *length == 0){
       XtFree((char*)value);
       value = NULL;
       insert_select->done_status = True;
       return;
    }

    if (insert_select->select_type == XmPRIM_SELECT) {
       if (!(*tw->text.source->GetSelection)(tw->text.source, &left, &right) ||
	   left == right) {
	  XBell(XtDisplay(w), 0);
          XtFree((char*)value);
          value = NULL;
          insert_select->done_status = True;
          insert_select->success_status = False;
          return;
       }
    } else if (insert_select->select_type == XmDEST_SELECT) {
      if ((*tw->text.source->GetSelection)(tw->text.source, &left, &right) && 
         left != right) {
         if (tw->text.cursor_position < left ||
	     tw->text.cursor_position > right ||
             tw->text.pendingoff) {
	    left = right = tw->text.cursor_position; 
	    dest_disjoint = True;
         }
      } else
           left = right = tw->text.cursor_position;
    }

    (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position, off);

    if (*type == COMPOUND_TEXT || *type == XA_STRING) {
       tmp_prop.value = (unsigned char *) value;
       tmp_prop.encoding = *type;
       tmp_prop.format = *format;
       tmp_prop.nitems = *length;
       num_vals = 0;
       status = XmbTextPropertyToTextList(XtDisplay(w), &tmp_prop,
				        &tmp_value, &num_vals);
      /* if total failure, num_vals won't change */
       if (num_vals && (status == Success || status > 0)) { 
          for (i = 0; i < num_vals ; i++)
              malloc_size += strlen(tmp_value[i]);
       
          total_tmp_value = XtMalloc ((unsigned) malloc_size + 1);
          total_tmp_value[0] = '\0';
          for (i = 0; i < num_vals ; i++)
             strcat(total_tmp_value, tmp_value[i]);
          block.ptr = total_tmp_value;
          block.length = strlen(total_tmp_value);
          block.format = XmFMT_8_BIT;
          XFreeStringList(tmp_value);
       } else {
          insert_select->done_status = True;
          insert_select->success_status = False;
          (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position,
						 on);
          return;
       }
    } else {  /* it must be either CS_OF_LOCALE or TEXT */
       block.ptr = (char*)value;
      /* NOTE: casting *length could result in a truncated long. */
       block.length = (unsigned) *length;
       block.format = XmFMT_8_BIT;
    }

    if (_XmTextModifyVerify(tw, NULL, &left, &right,
                            &cursorPos, &block, &newblock, &freeBlock)) {
       if ((*tw->text.source->Replace)(tw, NULL, &left, &right,
				       &newblock, False) != EditDone) {
	  if (tw->text.verify_bell) XBell(XtDisplay(w), 0);
	  insert_select->success_status = False;
       } else {
	  insert_select->success_status = True;

	  if (!tw->text.add_mode) tw->text.input->data->anchor = left;

	  if (tw->text.add_mode && cursorPos >= left && cursorPos <= right)
	     tw->text.pendingoff = FALSE;
	  else
	     tw->text.pendingoff = TRUE;

	  _XmTextSetCursorPosition(w, cursorPos);
	  _XmTextSetDestinationSelection(w, tw->text.cursor_position, False,
					 insert_select->event->time);

	  if (insert_select->select_type == XmDEST_SELECT) {
	      if (left != right) {
		 if (!dest_disjoint) {
		    (*tw->text.source->SetSelection)(tw->text.source,
					      tw->text.cursor_position,
					      tw->text.cursor_position,
					      insert_select->event->time);
		 } else {
		    if (!tw->text.add_mode) {
		       (*tw->text.source->SetSelection)(tw->text.source,
					      tw->text.cursor_position,
					      tw->text.cursor_position,
					      insert_select->event->time);
		    }
		 }
	      }
	  }
	  _XmTextValueChanged(tw, (XEvent *)insert_select->event);
       }
       if (freeBlock && newblock.ptr) XtFree(newblock.ptr);
    }

    (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position, on);
    if (malloc_size != 0) XtFree(total_tmp_value);
    XtFree((char*)value);
    value = NULL;
    insert_select->done_status = True;
}

/* ARGSUSED */
static void
#ifdef _NO_PROTO
HandleInsertTargets( w, closure, seltype, type, value, length, format )
        Widget w ;
        char * closure ;
        Atom *seltype ;
        Atom *type ;
        XtPointer value ;
        unsigned long *length ;
        int *format ;
#else
HandleInsertTargets(
        Widget w,
        XtPointer closure,
        Atom *seltype,
        Atom *type,
        XtPointer value,
        unsigned long *length,
        int *format )
#endif /* _NO_PROTO */
{
   _XmInsertSelect *insert_select = (_XmInsertSelect *) closure;
   Atom TEXT = XmInternAtom(XtDisplay(w), "TEXT", False);
   Atom COMPOUND_TEXT = XmInternAtom(XtDisplay(w),"COMPOUND_TEXT", False);
   Atom target = TEXT;
   Atom *atom_ptr;
   int i;

   if (!length) {
      XtFree((char *)value);
      insert_select->done_status = True;
      return; /* Supports no targets, so don't bother sending anything */
   }

   atom_ptr = (Atom *)value;

    for (i = 0; i < *length; i++, atom_ptr++) {
      if (*atom_ptr == COMPOUND_TEXT) {
         target = *atom_ptr;
         break;
      } else if (*atom_ptr == XA_STRING)
         target = *atom_ptr;
    }

   XtGetSelectionValue(w, *seltype, target,
                       InsertSelection,
                       (XtPointer) insert_select,
                       insert_select->event->time);

}

/*
 * Converts requested target of insert selection.
 */
/*--------------------------------------------------------------------------+*/
static Boolean
#ifdef _NO_PROTO
ConvertInsertSelection( w, selection, type, value, length, format, locale_atom )
        Widget w ;
        Atom *selection ;
        Atom *type ;
        XtPointer *value ;
        unsigned long *length ;
        int *format ;
        Atom locale_atom;
#else
ConvertInsertSelection(
        Widget w,
        Atom *selection,
        Atom *type,
        XtPointer *value,
        unsigned long *length,
        int *format,
        Atom locale_atom)
#endif /* _NO_PROTO */
{
   XtAppContext app = XtWidgetToApplicationContext(w);
   XSelectionRequestEvent * req_event;
   static unsigned long old_serial = 0;
   Atom TARGETS = XmInternAtom(XtDisplay(w), "TARGETS", False);
   Atom MOTIF_DESTINATION = XmInternAtom(XtDisplay(w), "MOTIF_DESTINATION",
                                         False);
   Atom actual_type;
   int actual_format;
   unsigned long nitems;
   unsigned long bytes;
   unsigned char *prop = NULL;
   _XmInsertSelect insert_select;
   _XmTextInsertPair *pair;

   insert_select.done_status = False;
   insert_select.success_status = False;

   if (*selection == MOTIF_DESTINATION) {
      insert_select.select_type = XmDEST_SELECT;
   } else if (*selection == XA_PRIMARY) {
      insert_select.select_type = XmPRIM_SELECT;
   }

   req_event = XtGetSelectionRequest(w, *selection, NULL);

   insert_select.event = req_event;

  /* Work around for intrinsics selection bug */
   if (old_serial != req_event->serial)
      old_serial = req_event->serial;
   else
      return False;

   if (XGetWindowProperty(req_event->display, req_event->requestor,
                      req_event->property, 0L, 10000000, False,
                      AnyPropertyType, &actual_type, &actual_format,
                      &nitems, &bytes, &prop) != Success)
      return FALSE;

   pair = (_XmTextInsertPair *)prop;

   if (pair->target != locale_atom) {
     /*
      * Make selection request to find out which targets
      * the selection can provide.
      */
      XtGetSelectionValue(w, pair->selection, TARGETS,
                          HandleInsertTargets,
                          (XtPointer) &insert_select,
                          req_event->time);
   } else {
     /*
      * Make selection request to replace the selection
      * with the insert selection.
      */
      XtGetSelectionValue(w, pair->selection, pair->target,
                          InsertSelection,
                          (XtPointer) &insert_select,
                          req_event->time);
   }

  /*
   * Make sure the above selection request is completed
   * before returning from the convert proc.
   */
   for (;;) {
       XEvent event;

       if (insert_select.done_status)
          break;
       XtAppNextEvent(app, &event);
       XtDispatchEvent(&event);
   }

   *type = XmInternAtom(XtDisplay(w), "NULL", False);
   *format = 8;
   *value = NULL;
   *length = 0;

   if(    prop != NULL    )
     {
       XFree((void *)prop);
     }
   return (insert_select.success_status);
}

/* ARGSUSED */
Boolean
#ifdef _NO_PROTO
_XmTextConvert( w, selection, target, type, value, length, format )
        Widget w ;
        Atom *selection ;
        Atom *target ;
        Atom *type ;
        XtPointer *value ;
        unsigned long *length ;
        int *format ;
#else
_XmTextConvert(
        Widget w,
        Atom *selection,
        Atom *target,
        Atom *type,
        XtPointer *value,
        unsigned long *length,
        int *format )
#endif /* _NO_PROTO */
{
    XmTextWidget widget;
    XmTextSource source;
    Atom MOTIF_DESTINATION = XmInternAtom(XtDisplay(w),
                                        "MOTIF_DESTINATION", False);
    Atom INSERT_SELECTION = XmInternAtom(XtDisplay(w),
                                           "INSERT_SELECTION", False);
    Atom DELETE = XmInternAtom(XtDisplay(w), "DELETE", False);
    Atom TARGETS = XmInternAtom(XtDisplay(w), "TARGETS", False);
    Atom MULTIPLE = XmInternAtom(XtDisplay(w), "MULTIPLE", False);
    Atom TEXT = XmInternAtom(XtDisplay(w), "TEXT", False);
    Atom COMPOUND_TEXT = XmInternAtom(XtDisplay(w),
					  "COMPOUND_TEXT", False);
    Atom TIMESTAMP = XmInternAtom(XtDisplay(w), "TIMESTAMP", False);
    Atom MOTIF_DROP = XmInternAtom(XtDisplay(w), "_MOTIF_DROP", False);
    Atom CS_OF_LOCALE; /* to be initialized by XmbTextListToTextProperty */
    XSelectionRequestEvent * req_event;
    Boolean has_selection = False;
    XmTextPosition left = 0;
    XmTextPosition right = 0;
    Boolean is_primary;
    Boolean is_secondary;
    Boolean is_destination;
    Boolean is_drop;
    int target_count = 0;
    XTextProperty tmp_prop;
    int status = 0;
    int MAX_TARGS = 10;
    char * tmp_value;
    char * tmp_string = "ABC";  /* these are characters in XPCS, so... safe */
    XtPointer c_ptr;
    Arg args[1];

    if (*selection == MOTIF_DROP) {
       XtSetArg(args[0], XmNclientData, &c_ptr);
       XtGetValues(w, args, 1);
       widget = (XmTextWidget) c_ptr;
    } else
       widget = (XmTextWidget) w;

    if (widget == NULL) return False;

    source = widget->text.source;

#ifdef NON_OSF_FIX
    tmp_prop.value = NULL;
#endif
    status = XmbTextListToTextProperty(XtDisplay(widget), &tmp_string, 1,
				(XICCEncodingStyle)XTextStyle, &tmp_prop);
    if (status == Success)
       CS_OF_LOCALE = tmp_prop.encoding;
    else
       CS_OF_LOCALE = (Atom) 9999; /* XmbTextList... SHOULD never fail for
				       * XPCS character.  But if it does, this
				       * prevents a core dump.
				       */

#ifdef NON_OSF_FIX
    XFree((char*) tmp_prop.value);
#endif

    if (*selection == XA_PRIMARY) {
       has_selection = (*widget->text.source->GetSelection)(source, &left,
							    &right);
       is_primary = True;
       is_secondary = is_destination = is_drop = False;
    } else if (*selection == MOTIF_DESTINATION) {
       has_selection = widget->text.input->data->has_destination;
       is_destination = True;
       is_secondary = is_primary = is_drop = False;
    } else if (*selection == XA_SECONDARY) {
       has_selection = _XmTextGetSel2(widget, &left, &right);
       is_secondary = True;
       is_destination = is_primary = is_drop = False;
    } else if (*selection == MOTIF_DROP) {
       has_selection = (*widget->text.source->GetSelection)(source, &left,
							    &right);
       is_drop = True;
       is_destination = is_primary = is_secondary = False;
    } else
       return False;

    
  /*
   * TARGETS identifies what targets the text widget can
   * provide data for.
   */
    if (*target == TARGETS) {
      Atom *targs = (Atom *)XtMalloc((unsigned) (MAX_TARGS * sizeof(Atom)));

      *value = (XtPointer) targs;
      *targs++ = CS_OF_LOCALE;  target_count++;
      *targs++ = TARGETS;  target_count++;
      *targs++ = MULTIPLE;  target_count++;
      *targs++ = TIMESTAMP;  target_count++;
      if (is_primary || is_destination)
         *targs++ = INSERT_SELECTION;  target_count++;
      if (is_primary || is_secondary || is_drop) {
         *targs++ = COMPOUND_TEXT;  target_count++;
         *targs++ = TEXT;  target_count++;
         *targs++ = XA_STRING;  target_count++;
      }
      if (is_primary || is_drop) {
         *targs++ = DELETE; target_count++;
      }
      *type = XA_ATOM;
      *length = (target_count*sizeof(Atom)) >> 2; /*convert to work count */
      *format = 32;
   } else if (*target == TIMESTAMP) {
      Time *timestamp;
      timestamp = (Time *) XtMalloc(sizeof(Time));
      if (is_primary)
         *timestamp = source->data->prim_time;
      else if (is_destination)
         *timestamp = widget->text.input->data->dest_time;
      else if (is_secondary)
         *timestamp = widget->text.input->data->sec_time;
      else if (is_drop)
         *timestamp = widget->text.input->data->sec_time;
      *value = (XtPointer) timestamp;
      *type = XA_INTEGER;
      *length = sizeof(Time);
      *format = 32;
  /* Provide data for XA_STRING requests */
    } else if (*target == XA_STRING) {
      *type = (Atom) XA_STRING;
      *format = 8;
      if (is_destination || !has_selection) return False;
      tmp_value = _XmStringSourceGetString(widget, left, right, False);
#ifdef NON_OSF_FIX
      tmp_prop.value = NULL;
#endif
      status = XmbTextListToTextProperty(XtDisplay(widget), &tmp_value, 1,
					  (XICCEncodingStyle)XStringStyle, 
					  &tmp_prop);
      XtFree(tmp_value);
      if (status == Success || status > 0){
        /* NOTE: casting tmp_prop.nitems could result in a truncated long. */
         *value = (XtPointer) XtMalloc((unsigned)tmp_prop.nitems);
	 memcpy((void*)*value, (void*)tmp_prop.value,(unsigned)tmp_prop.nitems);
#ifdef NON_OSF_FIX
	 if (tmp_prop.value != NULL) XFree((char*)tmp_prop.value);
#endif
	 *length = tmp_prop.nitems;
      } else {
	 *value = NULL;
	 *length = 0;
	 return False;
      }

    } else if (*target == TEXT) {
      if (is_destination || !has_selection)
	return False;
#ifdef NON_OSF_FIX
      tmp_prop.value = NULL;
#endif
      tmp_value =_XmStringSourceGetString(widget, left, right, False);
      status = XmbTextListToTextProperty(XtDisplay(widget), &tmp_value, 1,
					 (XICCEncodingStyle)XStdICCTextStyle,
					 &tmp_prop);
      *type = tmp_prop.encoding;	/* STRING or COMPOUND_TEXT */
      *format = tmp_prop.format;
      XtFree(tmp_value);

      if (status == Success || status > 0) {
	/* NOTE: casting tmp_prop.nitems could result in a truncated long. */
	*value = (XtPointer) XtMalloc((unsigned) tmp_prop.nitems);
	memcpy((void*)*value, (void*)tmp_prop.value,(unsigned)tmp_prop.nitems);
#ifdef NON_OSF_FIX
	 if (tmp_prop.value != NULL) XFree((char*)tmp_prop.value);
#endif
	*length = tmp_prop.nitems;

      } else {
	*value =  NULL;
	*length = 0;
	return False;
      }

    } else if (*target == CS_OF_LOCALE) {
      *type = CS_OF_LOCALE;
      *format = 8;
      if (is_destination || !has_selection) return False;
      *value = (XtPointer)_XmStringSourceGetString(widget, left, right, False);
      *length = strlen((char*) *value);
    } else if (*target == COMPOUND_TEXT) {
      *type = COMPOUND_TEXT;
      *format = 8;
      if (is_destination || !has_selection) return False;
#ifdef NON_OSF_FIX
      tmp_prop.value = NULL;
#endif
      tmp_value =_XmStringSourceGetString(widget, left, right, False);
      status = XmbTextListToTextProperty(XtDisplay(widget), &tmp_value, 1,
					  (XICCEncodingStyle)XCompoundTextStyle,
					  &tmp_prop);
      XtFree(tmp_value);
      if (status == Success || status > 0) {
        /* NOTE: casting tmp_prop.nitems could result in a truncated long. */
         *value = (XtPointer) XtMalloc((unsigned) tmp_prop.nitems);
	 memcpy((void*)*value, (void*)tmp_prop.value,(unsigned)tmp_prop.nitems);
#ifdef NON_OSF_FIX
	 if (tmp_prop.value != NULL) XFree((char*)tmp_prop.value);
#endif
	 *length = tmp_prop.nitems;
      } else {
         *value =  NULL;
	 *length = 0;
	 return False;
      }
  /*
   * Provide data for INSERT_SELECTION requests, used in
   * swaping selections.
   */
    } else if (*target == INSERT_SELECTION) {
      if (is_secondary) return False;
      return (ConvertInsertSelection(w, selection, type,
                                     value, length, format, CS_OF_LOCALE));

  /* Delete the selection */
    } else if (*target == DELETE) {
      XmTextBlockRec block, newblock;
      XmTextPosition cursorPos;
      Boolean freeBlock;

      if (!(is_primary || is_drop)) return False;

      block.ptr = "";
      block.length = 0;
      block.format = XmFMT_8_BIT;

      if (is_drop) {
         Atom real_selection_atom; /* DND hides the selection atom from us */

         XtSetArg(args[0], XmNiccHandle, &real_selection_atom);
         XtGetValues(w, args, 1); /* 'w' is the drag context */
         req_event = XtGetSelectionRequest(w, real_selection_atom, NULL);
      } else {
         req_event = XtGetSelectionRequest(w, *selection, NULL);
      }
      if (_XmTextModifyVerify(widget, NULL, &left, &right,
                              &cursorPos, &block, &newblock, &freeBlock)) {
	 if ((*widget->text.source->Replace)(widget, NULL, &left, &right, 
					     &newblock, False) != EditDone) {
            if (freeBlock && newblock.ptr) XtFree(newblock.ptr);
	    return False;
	 } else {
	    if (is_drop) {
	      if (_XmTextGetDropReciever((Widget)widget) != (Widget) widget)
		_XmTextSetCursorPosition((Widget)widget, cursorPos);
	    } else {
	      if (req_event->requestor != XtWindow((Widget) widget))
		_XmTextSetCursorPosition(w, cursorPos);
	    }
	    _XmTextValueChanged(widget, (XEvent *) req_event);
	 }
         if (freeBlock && newblock.ptr) XtFree(newblock.ptr);
      } 
      if (!widget->text.input->data->has_destination)
         widget->text.input->data->anchor = widget->text.cursor_position;

         (*widget->text.source->SetSelection)(widget->text.source,
				        widget->text.cursor_position,
                                        widget->text.cursor_position,
        			        XtLastTimestampProcessed(XtDisplay(w)));
      *type = XmInternAtom(XtDisplay(w), "NULL", False);
      *value = NULL;
      *length = 0;
      *format = 8;
  /* unknown selection type */
    } else
       return FALSE;
    return TRUE;
}

/* ARGSUSED */
void
#ifdef _NO_PROTO
_XmTextLoseSelection( w, selection )
        Widget w ;
        Atom *selection ;
#else
_XmTextLoseSelection(
        Widget w,
        Atom *selection )
#endif /* _NO_PROTO */
{
   XmTextWidget tw = (XmTextWidget) w;
   XmTextSource source = tw->text.source;
   Atom MOTIF_DESTINATION = XmInternAtom(XtDisplay(w),
                                        "MOTIF_DESTINATION", False);
/* Losing Primary Selection */
   if (*selection == XA_PRIMARY && _XmStringSourceHasSelection(source)) {
      XmAnyCallbackStruct cb;
      (*source->SetSelection)(source, 1, -999,
                                     XtLastTimestampProcessed(XtDisplay(w)));
      cb.reason = XmCR_LOSE_PRIMARY;
      cb.event = NULL;
      XtCallCallbackList(w, tw->text.lose_primary_callback, (XtPointer) &cb);
/* Losing Destination Selection */
   } else if (*selection == MOTIF_DESTINATION) {
      tw->text.input->data->has_destination = False;
      if (!tw->text.output->data->has_rect) _XmTextAdjustGC(tw);
      else _XmTextToggleCursorGC(w);
      (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position, off);
      tw->text.output->data->blinkstate = on;
      (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position, on);
/* Losing Secondary Selection */
   } else if (*selection == XA_SECONDARY && tw->text.input->data->hasSel2){
      _XmTextSetSel2(tw, 1, -999, XtLastTimestampProcessed(XtDisplay(w)));
   }
}
