#pragma ident	"@(#)m1.2libs:Xm/TextFSel.c	1.5"
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
*  (c) Copyright 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include <Xm/TextFSelP.h>
#include <Xm/TextFP.h>
#include <X11/Xatom.h>
#include <Xm/AtomMgr.h>
#include <Xm/XmosP.h>
#include <Xm/DragC.h>


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void InsertSelection() ;
static Boolean ConvertInsertSelection() ;
static void HandleInsertTargets() ;

#else

static void InsertSelection( 
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
static void HandleInsertTargets(
                        Widget w,
                        XtPointer closure,
                        Atom *seltype,
                        Atom *type,
                        XtPointer value,
                        unsigned long *length,
                        int *format );
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
    _XmInsertSelect *insert_select = (_XmInsertSelect *) closure;
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    XmTextPosition left = 0;
    XmTextPosition right = 0;
    Boolean replace_res = False;
    Boolean dest_disjoint = False;
    wchar_t * wc_value;
    char * temp;
    int num_chars = 0;
    Atom COMPOUND_TEXT = XmInternAtom(XtDisplay(w), "COMPOUND_TEXT", False);
    char * total_tmp_value = NULL;
    XTextProperty tmp_prop;
    char **tmp_value;
    int num_vals;
    int malloc_size = 0;
    int i, status = 0;
    XmAnyCallbackStruct cb;

    if (!value) {
       insert_select->done_status = True;
       return;
    }

   /* Don't do replace if there is not text to add */
    if (*(char *) value == (char)'\0' || *length == 0){
       XtFree((char *)value);
       insert_select->done_status = True;
       return;
    }

    if (insert_select->select_type == XmPRIM_SELECT) {
       if (!XmTextFieldGetSelectionPosition (w, &left, &right) ||
	   left == right) {
          XBell(XtDisplay(w), 0);
          XtFree((char *)value);
          insert_select->done_status = True;
          insert_select->success_status = False;
          return;
       } 
    } else if (insert_select->select_type == XmDEST_SELECT) {
      if (XmTextFieldGetSelectionPosition (w, &left, &right) && left != right) {
         if ( TextF_CursorPosition(tf) < left ||
	      TextF_CursorPosition(tf) > right ||
              !tf->text.pending_delete) {
            left = right = TextF_CursorPosition(tf);
            dest_disjoint = True;
         }
      } else
         left = right = TextF_CursorPosition(tf);
    }


    if (*type == COMPOUND_TEXT || *type == XA_STRING) {
       tmp_prop.value = (unsigned char *) value;
       tmp_prop.encoding = *type;
       tmp_prop.format = *format;
       tmp_prop.nitems = *length;
       num_vals = 0;
       status = XmbTextPropertyToTextList(XtDisplay(w), &tmp_prop, &tmp_value, 
					  &num_vals);
      /* if no conversion, num_vals won't change */
      /* status >0 if some characters can't be converted; continue anyway */
       if (num_vals && (status == Success || status > 0)) {
          for (i = 0; i < num_vals ; i++)
              malloc_size += strlen(tmp_value[i]);

          total_tmp_value = XtMalloc ((unsigned) malloc_size + 1);
          total_tmp_value[0] = '\0';
          for (i = 0; i < num_vals ; i++)
             strcat(total_tmp_value, tmp_value[i]);
	  XFreeStringList(tmp_value);
       }


       if (tf->text.max_char_size == 1) {
          replace_res = _XmTextFieldReplaceText(tf, 
                                                (XEvent *) insert_select->event,
					        left, right, total_tmp_value, 
                                                malloc_size, True);
       } else { /* must convert to wchar_t before passing to Replace */
          wc_value = (wchar_t *)XtMalloc((unsigned) 
                               (1 + malloc_size) * sizeof(wchar_t));
          num_chars = mbstowcs(wc_value, total_tmp_value, 1 + malloc_size);
	  if (num_chars < 0)
	     num_chars = 0;
	  else
             replace_res = _XmTextFieldReplaceText(tf,
                                                (XEvent *) insert_select->event,
                                                left, right, (char*) wc_value,
                                                num_chars, True);
          XtFree((char *)wc_value);
       }
       XtFree(total_tmp_value);
    } else { /* it must be either TEXT or codeset of the locale */
       if (tf->text.max_char_size == 1) {
         /* NOTE: casting *length could result in a truncated long. */
          replace_res = _XmTextFieldReplaceText(tf, 
					        (XEvent *)insert_select->event,
						left, right, (char *)value,
					        (unsigned)*length, True);
       } else {
          temp = XtMalloc((unsigned) *length + 1);
         /* NOTE: casting *length could result in a truncated long. */
          (void)memcpy((void*)temp, (void*)value, (unsigned)*length);
          temp[*length] = '\0';
          wc_value = (wchar_t*)XtMalloc((unsigned)
                                               (*length + 1) * sizeof(wchar_t));
      
         /* NOTE: casting *length could result in a truncated long. */
          num_chars = mbstowcs(wc_value, temp, (unsigned)*length + 1);
	  if (num_chars < 0)
	     num_chars = 0;
	  else
             replace_res = _XmTextFieldReplaceText(tf, 
                                                (XEvent *) insert_select->event,
					        left, right, (char*) wc_value,
					        num_chars, True);
          XtFree(temp);
          XtFree((char *)wc_value);
       }
    }

    if (!replace_res) {
       insert_select->success_status = False;
    } else {
       insert_select->success_status = True;

       if (!tf->text.add_mode) tf->text.prim_anchor = left;

       tf->text.pending_off = True;
       if (tf->text.max_char_size == 1) {
         _XmTextFieldSetCursorPosition(tf, NULL, left + *length, False, True);
       } else {
         _XmTextFieldSetCursorPosition(tf, NULL, left + num_chars, 
				       False, True);
       }
       (void) _XmTextFieldSetDestination(w, TextF_CursorPosition(tf),
					 insert_select->event->time);
       if (insert_select->select_type == XmDEST_SELECT) {
          if (left != right) {
             if (!dest_disjoint) {
                _XmTextFieldStartSelection(tf, TextF_CursorPosition(tf),
					   TextF_CursorPosition(tf),
                                           insert_select->event->time);
             } else {
                if (!tf->text.add_mode) {
                   _XmTextFieldStartSelection(tf, TextF_CursorPosition(tf),
				  	      TextF_CursorPosition(tf),
                                  	      insert_select->event->time);
                }
             }
          }
       }
       cb.reason = XmCR_VALUE_CHANGED;
       cb.event = (XEvent *)insert_select->event;
       XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
                          (XtPointer) &cb);
    }

    XtFree((char *)value);
    value = NULL;
    insert_select->done_status = True;
}

/* ARGSUSED */
static void
#ifdef _NO_PROTO
HandleInsertTargets( w, closure, seltype, type, value, length, format )
        Widget w ;
        XtPointer closure ;
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
_XmTextFieldConvert( w, selection, target, type, value, length, format )
        Widget w ;
        Atom *selection ;
        Atom *target ;
        Atom *type ;
        XtPointer *value ;
        unsigned long *length ;
        int *format ;
#else
_XmTextFieldConvert(
        Widget w,
        Atom *selection,
        Atom *target,
        Atom *type,
        XtPointer *value,
        unsigned long *length,
        int *format )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf;
    Atom MOTIF_DESTINATION = XmInternAtom(XtDisplay(w),
                                        "MOTIF_DESTINATION", False);
    Atom INSERT_SELECTION = XmInternAtom(XtDisplay(w),
                                           "INSERT_SELECTION", False);
    Atom DELETE = XmInternAtom(XtDisplay(w), "DELETE", False);
    Atom TARGETS = XmInternAtom(XtDisplay(w), "TARGETS", False);
    Atom MULTIPLE = XmInternAtom(XtDisplay(w), "MULTIPLE", False);
    Atom TEXT = XmInternAtom(XtDisplay(w), "TEXT", False);
    Atom COMPOUND_TEXT = XmInternAtom(XtDisplay(w), "COMPOUND_TEXT", False);
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
    int ret_status = 0;
    int MAX_TARGS = 10;
    char * tmp_value;
    char * tmp_string = "ABC";  /* these are characters in XPCS, so... safe */
    XtPointer c_ptr;
    Arg args[1];
    XmAnyCallbackStruct cb;

    if (*selection == MOTIF_DROP) {
       XtSetArg (args[0], XmNclientData, &c_ptr);
       XtGetValues (w, args, 1);
       tf = (XmTextFieldWidget) c_ptr;
    } else
       tf = (XmTextFieldWidget) w;

    if (tf == NULL) return False;
#ifdef NON_OSF_FIX
   tmp_prop.value = NULL;
#endif
    ret_status = XmbTextListToTextProperty(XtDisplay(w), &tmp_string, 1,
                                      (XICCEncodingStyle)XTextStyle, &tmp_prop);
    if (ret_status == Success) {
       CS_OF_LOCALE = tmp_prop.encoding;
#ifdef NON_OSF_FIX
       if (tmp_prop.value != NULL) XFree((char*)tmp_prop.value);
#endif
    } else
       CS_OF_LOCALE = (Atom) 9999; /* XmbTextList... should always be able
				       * to convert XPCS characters; but in
				       * case its broken, this prevents a core
				       * dump.
				       */

    if (*selection == XA_PRIMARY) {
       has_selection = tf->text.has_primary;
       left = tf->text.prim_pos_left;
       right = tf->text.prim_pos_right;
       is_primary = True;
       is_secondary = is_destination = is_drop = False;
    } else if (*selection == MOTIF_DESTINATION) {
       has_selection = tf->text.has_destination;
       is_destination = True;
       is_secondary = is_primary = is_drop = False;
    } else if (*selection == XA_SECONDARY) {
       has_selection = tf->text.has_secondary;
       left = tf->text.sec_pos_left;
       right = tf->text.sec_pos_right;
       is_secondary = True;
       is_destination = is_primary = is_drop = False;
    } else if (*selection == MOTIF_DROP) {
       has_selection = tf->text.has_primary;
       left = tf->text.prim_pos_left;
       right = tf->text.prim_pos_right;
       is_drop = True;
       is_destination = is_primary = is_secondary = False;
    } else
       return False;

  /*
   * TARGETS identifies what targets the textfield widget can
   * provide data for.
   */
    if (*target == TARGETS) {
      Atom *targs = (Atom *)XtMalloc((unsigned) (MAX_TARGS * sizeof(Atom)));

      *value = (XtPointer) targs;
      *targs++ = CS_OF_LOCALE; target_count++;
      *targs++ = TARGETS; target_count++;
      *targs++ = MULTIPLE; target_count++;
      *targs++ = TIMESTAMP; target_count++;
      if (is_primary || is_destination)
         *targs++ = INSERT_SELECTION; target_count++;
      if (is_primary || is_secondary || is_drop) {
         *targs++ = COMPOUND_TEXT; target_count++;
         *targs++ = TEXT; target_count++;
         *targs++ = XA_STRING; target_count++;
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
         *timestamp = tf->text.prim_time;
      else if (is_destination)
         *timestamp = tf->text.dest_time;
      else if (is_secondary)
         *timestamp = tf->text.sec_time;
      else if (is_drop)
         *timestamp = tf->text.prim_time;
      *value = (XtPointer) timestamp;
      *type = XA_INTEGER;
      *length = sizeof(Time);
      *format = 32;
    } else if (*target == XA_STRING) {
      *type = (Atom) XA_STRING;
      *format = 8;
      if (is_destination || !has_selection) return False;

      /* put a char* value into tmp_value, then convert to 8859.1 */
      if (tf->text.max_char_size != 1) {
	  int stat ;

	/* NOTE: casting (right - left) could result in a truncated long. */
         *length = _XmTextFieldCountBytes(tf, TextF_WcValue(tf) + left, 
                                          (int)(right - left));
         tmp_value = XtMalloc((unsigned) *length + 1);
         stat = wcstombs(tmp_value, TextF_WcValue(tf) + left,
		           (unsigned)*length); /* NOTE: casting *length could
					 	  result in a truncated long. */
	  if (stat < 0) /* wcstombs will return neg value on conv failure */
	      *length = 0;
	  else *length = (unsigned long) stat ;
      } else {
         *length = right - left;
         tmp_value = XtMalloc((unsigned) *length + 1);
        /* get the selection value */
         (void)memcpy((void*)tmp_value, (void*)(TextF_Value(tf) + left), 
		      (unsigned)*length); /* NOTE: casting *length could result
				             in a truncated long. */
      }
      tmp_value[*length] = '\0';
      /* convert tmp_value to 8859.1 */
      ret_status = XmbTextListToTextProperty(XtDisplay(w), &tmp_value, 1, 
                                             (XICCEncodingStyle)XStringStyle,
                                             &tmp_prop);
      XtFree(tmp_value);
      if (ret_status == Success || ret_status > 0){
         *value = (XtPointer) tmp_prop.value;
         *length = tmp_prop.nitems;
      } else {
         *value = NULL;
         *length = 0;
         return False;
      }
    } else if (*target == TEXT || *target == CS_OF_LOCALE) {
      *type = CS_OF_LOCALE;
      *format = 8;
      if (is_destination || !has_selection) return False;
      if (tf->text.max_char_size != 1) {
	  int stat ;

	  /* NOTE: casting (right - left) could result in a truncated long. */
	  *length = _XmTextFieldCountBytes(tf, TextF_WcValue(tf) + left,
					   (int)(right - left));
	  *value = XtMalloc((unsigned) *length + 1);
	  stat = wcstombs((char *)*value, TextF_WcValue(tf) + left,
			  (unsigned)*length); /* NOTE: casting *length could
						 result in a truncated long */
	  if (stat < 0) /* wcstombs return neg value on conv failure */
	      *length = 0;
	  else *length = (unsigned long) stat ;
      } else {
         *length = right - left;
         *value = XtMalloc((unsigned) *length + 1);
        /* get the selection value */
         (void)memcpy((void*)*value, (void*)(TextF_Value(tf) + left),
		      (unsigned)*length); /* NOTE: casting *length could result
				             in a truncated long. */
      }
      (*(char **)value)[*length]='\0';
    } else if (*target == COMPOUND_TEXT) {
      *type = COMPOUND_TEXT;
      *format = 8;
      if (is_destination || !has_selection) return False;
      if (tf->text.max_char_size != 1) { 
	int stat ;

	/* convert to char* before converting to CT.  NOTE: casting
         * (right - left) could result in a truncated long.
         */
         *length = _XmTextFieldCountBytes(tf, TextF_WcValue(tf) + left,
                                          (int)(right - left));
         tmp_value = XtMalloc((unsigned) *length + 1);
         stat = wcstombs(tmp_value, TextF_WcValue(tf) + left,
		           (unsigned)*length); /* NOTE: casting *length could
					 	  result in a truncated long. */
	if (stat < 0) /* wcstombs will return neg value on conv failure */
	    *length = 0;
	else *length = (unsigned long) stat ;
      } else { /* malloc the space and copy the data to be converted */
         *length = right - left;
         tmp_value = XtMalloc((unsigned) *length + 1);
        /* get the selection value */
         (void)memcpy((void*)tmp_value, (void*)(TextF_Value(tf) + left), 
		      (unsigned)*length); /* NOTE: casting *length could result
				             in a truncated long. */
      }
      tmp_value[*length] = '\0';

     /* Convert to compound text */
      ret_status = XmbTextListToTextProperty(XtDisplay(w), &tmp_value, 1,
                                          (XICCEncodingStyle)XCompoundTextStyle,
                                             &tmp_prop);
      XtFree(tmp_value);
      if (ret_status == Success || ret_status > 0){
         *length = tmp_prop.nitems;
	 *value = (XtPointer)tmp_prop.value;
      } else {
         *value = NULL;
         *length = 0;
         return False;
      }
  /*
   * Provide data for INSERT_SELECTION requests, used in
   * swaping selections.
   */
    } else if (*target == INSERT_SELECTION) {
      if (is_secondary) return False;
      return (ConvertInsertSelection((Widget) tf, selection, type,
                                     value, length, format, CS_OF_LOCALE));

  /* Delete the selection */
    } else if (*target == DELETE) {
      XmTextPosition left, right;
      Boolean move_cursor = True;

      if (!(is_primary || is_drop)) return False;

      if (is_drop) {
         Atom real_selection_atom; /* DND hides the selection atom from us */

         XtSetArg(args[0], XmNiccHandle, &real_selection_atom);
         XtGetValues(w, args, 1); /* 'w' is the drag context */
         req_event = XtGetSelectionRequest(w, real_selection_atom, NULL);
      } else {
         req_event = XtGetSelectionRequest((Widget) tf, *selection, NULL);
      }

      left = tf->text.prim_pos_left;
      right = tf->text.prim_pos_right;

      if (is_drop) {
           if (_XmTextFieldGetDropReciever((Widget)tf) == (Widget) tf)
              move_cursor = False;
      } else {
         if (req_event->requestor == XtWindow((Widget)tf))
	    move_cursor = False;
      }

      _XmTextFieldStartSelection(tf, tf->text.prim_anchor, tf->text.prim_anchor,
 				 req_event->time);

      if (!_XmTextFieldReplaceText(tf, (XEvent *) req_event,
				   left, right, NULL, 0, move_cursor)) {
           tf->text.has_primary = True;
           return False;
      }
      cb.reason = XmCR_VALUE_CHANGED;
      cb.event = (XEvent *) req_event;
      XtCallCallbackList((Widget) tf, TextF_ValueChangedCallback(tf),
			 (XtPointer) &cb);

      tf->text.has_primary = True;

      if (tf->text.has_destination)
          tf->text.prim_anchor = TextF_CursorPosition(tf);

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
_XmTextFieldLoseSelection( w, selection )
        Widget w ;
        Atom *selection ;
#else
_XmTextFieldLoseSelection(
        Widget w,
        Atom *selection )
#endif /* _NO_PROTO */
{
    XmTextFieldWidget tf = (XmTextFieldWidget) w;
    Atom MOTIF_DESTINATION = XmInternAtom(XtDisplay(w),
                                        "MOTIF_DESTINATION", False);
/* Losing Primary Selection */
    if (*selection == XA_PRIMARY && tf->text.has_primary) {
        XmAnyCallbackStruct cb;
        _XmTextFieldDeselectSelection(w, False, 0);

        cb.reason = XmCR_LOSE_PRIMARY;
        cb.event = NULL;
        XtCallCallbackList(w, tf->text.lose_primary_callback, (XtPointer) &cb);
/* Losing Destination Selection */
    } else if (*selection == MOTIF_DESTINATION) {
        tf->text.has_destination = False;
	if (tf->text.has_rect) _XmTextFieldSetClipRect(tf);
       /* if we have focus, we have a valid putback area.  If we don't have
	* focus, don't want to update the putback with the destination cursor
	* image.
	*/
	tf->text.refresh_ibeam_off = False;
	_XmTextFieldDrawInsertionPoint(tf, False);
	tf->text.blink_on = True;
	_XmTextFieldDrawInsertionPoint(tf, True);
/* Losing Secondary Selection */
    } else if (*selection == XA_SECONDARY && tf->text.has_secondary){
        _XmTextFieldSetSel2(w, 0, 0, True, 
			    XtLastTimestampProcessed(XtDisplay(w)));
    }
}
