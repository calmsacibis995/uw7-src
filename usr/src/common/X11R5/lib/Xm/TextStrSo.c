#pragma ident	"@(#)m1.2libs:Xm/TextStrSo.c	1.5"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3 - CR 6806 fixed
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

#include <ctype.h>
#include <string.h> /* Needed for memcpy() */
#include <limits.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>
#include <Xm/AtomMgr.h>
#include <Xm/TextP.h>
#include <Xm/TextSelP.h>
#include <Xm/TextStrSoP.h>
#include <Xm/XmosP.h>

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static int _XmTextBytesToCharacters() ;
static int _XmTextCharactersToBytes() ;
static void AddWidget() ;
static char * _XmStringSourceGetChar() ;
static int CountLines() ;
static void RemoveWidget() ;
static void _XmStringSourceMoveMem() ;
static void _XmStringSourceReadString() ;
static XmTextPosition ReadSource() ;
static void Validate() ;
static XmTextStatus Replace() ;
static void ScanParagraph() ;
static XmTextPosition Scan() ;
static Boolean GetSelection() ;
static void SetSelection() ;

#else

static int _XmTextBytesToCharacters( 
                        char *characters,
                        char *bytes,
                        int num_chars,
#if NeedWidePrototypes
                        int add_null_terminator,
#else
                        Boolean add_null_terminator,
#endif /* NeedWidePrototypes */
                        int max_char_size) ;
static int _XmTextCharactersToBytes( 
                        char *bytes,
                        char *characters,
                        int num_chars,
                        int max_char_size) ;
static void AddWidget( 
                        XmTextSource source,
                        XmTextWidget widget) ;
static char * _XmStringSourceGetChar( 
                        XmSourceData data,
                        XmTextPosition position) ;
static int CountLines( 
                        XmTextSource source,
                        XmTextPosition start,
                        unsigned long length) ;
static void RemoveWidget( 
                        XmTextSource source,
                        XmTextWidget widget) ;
static void _XmStringSourceMoveMem( 
                        char *from,
                        char *to,
                        int length) ;
static void _XmStringSourceReadString( 
                        XmTextSource source,
                        int start,
                        XmTextBlock block) ;
static XmTextPosition ReadSource( 
                        XmTextSource source,
                        XmTextPosition position,
                        XmTextPosition last_position,
                        XmTextBlock block) ;
static void Validate( 
                        XmTextPosition *start,
                        XmTextPosition *end,
                        int maxsize) ;
static XmTextStatus Replace( 
                        XmTextWidget initiator,
			XEvent *event,
                        XmTextPosition *start,
                        XmTextPosition *end,
                        XmTextBlock block,
#if NeedWidePrototypes
                        int call_callbacks);
#else
    		        Boolean call_callbacks);
#endif /* NeedsWidePrototypes */
static void ScanParagraph( 
                        XmSourceData data,
                        XmTextPosition *new_position,
                        XmTextScanDirection dir,
                        int ddir,
                        XmTextPosition *last_char) ;
static XmTextPosition Scan( 
                        XmTextSource source,
                        XmTextPosition pos,
                        XmTextScanType sType,
                        XmTextScanDirection dir,
                        int count,
#if NeedWidePrototypes
                        int include) ;
#else
                        Boolean include) ;
#endif /* NeedWidePrototypes */
static Boolean GetSelection( 
                        XmTextSource source,
                        XmTextPosition *left,
                        XmTextPosition *right) ;
static void SetSelection( 
                        XmTextSource source,
                        XmTextPosition left,
                        XmTextPosition right,
                        Time set_time) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

#define TEXT_INCREMENT 1024
#define TEXT_INITIAL_INCREM 64

/* Convert a stream of bytes into a char*, BITS16*, or BITS32* array. 
 * Return number of characters created.
 *
 * If num_chars == 1, don't add a null terminator, else if a null terminator
 * is present on the byte stream, convert it and add it to the character
 * array;  Count returned does not include NULL terminator (just like strlen).
 *
 * This routine assumes that a BITS16 is two-bytes and an BITS32 is four-bytes;
 * the routine must be modified if these assumptions are incorrect.
 */

/* ARGSUSED */
static int
#ifdef _NO_PROTO
_XmTextBytesToCharacters(characters, bytes, num_chars, add_null_terminator,
                         max_char_size)
	char * characters;
	char * bytes;
	int num_chars;
	Boolean add_null_terminator;
	int max_char_size;
#else /* _NO_PROTO */
_XmTextBytesToCharacters(
	char * characters, 
	char * bytes,
	int num_chars, 
#if NeedWidePrototypes
	int add_null_terminator,
#else
	Boolean add_null_terminator,
#endif /* NeedWidePrototypes */
	int max_char_size
)
#endif /* _NO_PROTO */
{
   unsigned char * tmp_bytes;
   int num_bytes; 
   int count=0;
   int i;
   BITS16 *bits16_ptr, temp_bits16;
   BITS32 *bits32_ptr, temp_bits32;


   /* If 0 characters requested, or a null pointer passed, dont do
    * anything... just return 0 characters converted.
    */

   if (num_chars == 0 || bytes == NULL) return 0;

   switch (max_char_size){
      case 1: {
         (void) memcpy((void*)characters, (void*)bytes, num_chars);
	 count = num_chars;
	 break;
      } /* end case 1 */
      case 2: {
	 bits16_ptr = (BITS16 *) characters;
	 tmp_bytes = (unsigned char*) bytes;
         for (num_bytes = mblen((char*)tmp_bytes, max_char_size), 
	      temp_bits16 = 0; 
	      num_chars > 0 && num_bytes > 0;
	      num_chars--, num_bytes = mblen((char*)tmp_bytes, max_char_size), 
	      temp_bits16 = 0, bits16_ptr ++){
            if (num_bytes == 1){
                temp_bits16 = (BITS16) *tmp_bytes++;
	    } else {
		temp_bits16 = (BITS16) *tmp_bytes++;
                temp_bits16 = temp_bits16 << 8;
                temp_bits16 |= (BITS16) *tmp_bytes++;
	    }
            *bits16_ptr = temp_bits16;
            count++;
	 }
	            /* if bytes is NULL terminated, characters should be too */
         if (add_null_terminator == True)
	    *bits16_ptr = (BITS16) 0;  
         break;
      } /* end case 2 */
      case 3: case 4: default: {
	 bits32_ptr = (BITS32 *) characters;
	 tmp_bytes = (unsigned char*) bytes;
         for (num_bytes = mblen((char*)tmp_bytes, max_char_size), 
	      temp_bits32 = 0; 
	      num_chars > 0 && num_bytes > 0;
	      num_chars--, num_bytes = mblen((char*)tmp_bytes, max_char_size), 
	      temp_bits32 = 0, bits32_ptr ++){
	    if (num_bytes == 1){
	       temp_bits32 = (BITS32) *tmp_bytes++;
	    } else {
	       for (i = 0; i < num_bytes; i++) {
	          temp_bits32 = (BITS32) temp_bits32 << 8;
	          temp_bits32 |= (BITS32) *tmp_bytes++;
               }
            } 

            *bits32_ptr = temp_bits32;
            count++;
	 }
	            /* if bytes is NULL terminated, characters should be too */
         if (add_null_terminator == True)
	    *bits32_ptr = (BITS32) 0;  
         break;
      } /* end case 3, 4, and default */
   } /* end switch */

   return count;
}

/* Convert an array of char*, BITS16*, or BITS32* into a stream of bytes.
 * Return the number of bytes placed into 'bytes'
 *
 * Null terminate the byte stream - caller better have alloc'ed enough space!
 */

/* ARGSUSED */
static int
#ifdef _NO_PROTO
_XmTextCharactersToBytes(bytes, characters, num_chars, max_char_size)
	char * bytes;
	char * characters;
	int num_chars;
	int max_char_size;
#else /* _NO_PROTO */
_XmTextCharactersToBytes(
	char * bytes,
	char * characters,
	int num_chars,
	int max_char_size
)
#endif /* _NO_PROTO */
{
   unsigned char *temp_char;
   unsigned char *byte_ptr;
   int count = 0;
   int i, j;
   BITS16 *bits16_ptr, temp_bits16;
   BITS32 *bits32_ptr, temp_bits32;

   if (num_chars == 0 || characters == 0) {
      *bytes = '\0';
      return 0;
   }

   switch (max_char_size) {
      case 1: {
         (void) memcpy((void*)bytes, (void*)characters, num_chars);
         count = num_chars;
	 break;
      } /* end case 1 */
      case 2: {
	 bits16_ptr = (BITS16 *) characters;
	 byte_ptr = (unsigned char*) bytes;
         temp_char = (unsigned char*) XtMalloc (max_char_size);
         for (i = 0; i < num_chars && *bits16_ptr != 0; i++, bits16_ptr++){
	    temp_bits16 = *bits16_ptr;
	    /* create an array of chars; char[max_char_size - 1] contains the 
	     * low order byte */
            for (j = max_char_size - 1; j >= 0; j--) {
               temp_char[j] = (unsigned char)(temp_bits16 & 0377);
	       temp_bits16 = temp_bits16 >> 8;
            }
	    /* start with high order byte.  If any byte is 0, skip it. */
            for (j = 0; j < max_char_size; j++) { 
               if (temp_char[j] > 0) {
	          *byte_ptr = temp_char[j];
	          byte_ptr++; count++;
	       }
	    }
	 }
         XtFree ((char*)temp_char);
         if (count < num_chars) *byte_ptr = '\0';
	 break;
      } /* end case 2 */
      case 3: case 4: default: {
	 bits32_ptr = (BITS32 *) characters;
	 byte_ptr = (unsigned char*) bytes;
         temp_char = (unsigned char *) XtMalloc (max_char_size);
         for (i = 0; i < num_chars && *bits32_ptr != 0; i++, bits32_ptr++){
	    temp_bits32 = *bits32_ptr;
	    /* create an array of chars; char[max_char_size - 1] contains the 
	     * low order byte */
            for (j = max_char_size - 1; j >= 0; j--) {
               temp_char[j] = (unsigned char)(temp_bits32 & 0377);
	       temp_bits32 = temp_bits32 >> 8;
            }
	    /* start with high order byte.  If any byte is 0, skip it. */
            for (j = 0; j < max_char_size; j++) { 
               if (temp_char[j] > 0) {
	          *byte_ptr = temp_char[j];
	          byte_ptr++; count++;
	       }
	    }
	 }
         XtFree ((char*)temp_char);
         *byte_ptr = '\0';  /* Null terminate the string */
	 break;
      } /* end case 3, 4, and default */
   } /* end switch */
   return (count);    /* return the number of bytes placed in bptr */
}

char * 
#ifdef _NO_PROTO
_XmStringSourceGetString( tw, from, to, want_wchar )
        XmTextWidget tw ;
        XmTextPosition from ;
        XmTextPosition to ;
	Boolean want_wchar;
#else
_XmStringSourceGetString(
        XmTextWidget tw,
        XmTextPosition from,
        XmTextPosition to,
#if NeedWidePrototypes
	int want_wchar)
#else
	Boolean want_wchar)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
   char *buf;
   wchar_t *wc_buf;
   XmTextBlockRec block;
   int destpos;
   XmTextPosition pos, ret_pos;
   int return_val = 0;

   destpos = 0;
   if (!want_wchar) {
     /* NOTE: to - from could result in a truncated long. */
      buf = XtMalloc(((int)(to - from) + 1) * (int)tw->text.char_size);
      for (pos = from; pos < to; ){
          pos = ReadSource(tw->text.source, pos, to, &block);
          if (block.length == 0)
             break;

          (void)memcpy((void*)&buf[destpos], (void*)block.ptr, block.length);
          destpos += block.length;
      }
      buf[destpos] = 0;
      return buf;
   } else { /* want buffer of wchar_t * data */
     /* NOTE: to - from could result in a truncated long. */
      buf = XtMalloc(((int)(to - from) + 1) * sizeof(wchar_t));
      wc_buf = (wchar_t *)buf;
      for (pos = from; pos < to; ){
          ret_pos = ReadSource(tw->text.source, pos, to, &block);
          if (block.length == 0)
             break;

        /* NOTE: ret_pos - pos could result in a truncated long. */
	  return_val = mbstowcs(&wc_buf[destpos], block.ptr,
					(unsigned int) (ret_pos - pos));
	  if (return_val > 0) destpos += return_val;
	  pos = ret_pos;
      }
      wc_buf[destpos] = (wchar_t)0L;
      return ((char*)wc_buf);
   }
}


static void 
#ifdef _NO_PROTO
AddWidget( source, widget )
        XmTextSource source ;
        XmTextWidget widget ;
#else
AddWidget(
        XmTextSource source,
        XmTextWidget widget )
#endif /* _NO_PROTO */
{
    XmSourceData data = source->data;
    data->numwidgets++;
    data->widgets = (XmTextWidget *)
	XtRealloc((char *) data->widgets,
		  (unsigned) (sizeof(XmTextWidget) * data->numwidgets));
    data->widgets[data->numwidgets - 1] = widget;

    if (data->numwidgets == 1)
       XmTextSetHighlight((Widget) widget, 0, widget->text.last_position, 
			  XmHIGHLIGHT_NORMAL);
    else {
      widget->text.highlight.list = (_XmHighlightRec *)
		XtRealloc((char *) widget->text.highlight.list, 
	      	 data->widgets[0]->text.highlight.maximum *
		 sizeof(_XmHighlightRec));
      widget->text.highlight.maximum = data->widgets[0]->text.highlight.maximum;
      widget->text.highlight.number = data->widgets[0]->text.highlight.number;
      memmove((void *) data->widgets[0]->text.highlight.list,
	       (void *) widget->text.highlight.list,
	       (size_t) data->widgets[0]->text.highlight.number *
						   sizeof(_XmHighlightRec));
    }


    if (data->hasselection && data->numwidgets == 1) {
        Time select_time = XtLastTimestampProcessed(XtDisplay((Widget)widget));
	if (!XtOwnSelection((Widget) data->widgets[0], XA_PRIMARY, select_time,
			    _XmTextConvert, _XmTextLoseSelection,
			    (XtSelectionDoneProc) NULL)) {
	   (*source->SetSelection)(source, 1, 0, select_time);
        } else {
            XmAnyCallbackStruct cb;

            data->prim_time = select_time;
            cb.reason = XmCR_GAIN_PRIMARY;
            cb.event = NULL;
            XtCallCallbackList ((Widget) data->widgets[0],
				data->widgets[0]->text.gain_primary_callback,
				(XtPointer) &cb);
        }
    }
}

/********************************<->***********************************/
static char * 
#ifdef _NO_PROTO
_XmStringSourceGetChar( data, position )
        XmSourceData data ;
        XmTextPosition position ;
#else
_XmStringSourceGetChar(
        XmSourceData data,
        XmTextPosition position )       /* starting position */
#endif /* _NO_PROTO */
{
    /* gap_size is the number of character in the gap, not number of bytes */
    register int gap_size;
    register XmTextPosition char_pos;
    XmTextWidget tw = (XmTextWidget) data->widgets[0];

    if (tw->text.char_size > 1) {
       char_pos = position * (int)tw->text.char_size;

   /* regardless of what it contains, data->ptr is treated as a char * ptr */
       if (data->ptr + char_pos < data->gap_start)
          return (&data->ptr[char_pos]);

       gap_size = (data->gap_end - data->gap_start) / (int)tw->text.char_size;
       if (position + gap_size >= data->maxlength)
          return ("");
       return (&data->ptr[(position + gap_size) * (int)tw->text.char_size]);
    } else {
       char_pos = position;
   /* regardless of what it contains, data->ptr is treated as a char * ptr */
       if (data->ptr + char_pos < data->gap_start)
          return (&data->ptr[char_pos]);

       gap_size = (data->gap_end - data->gap_start);
       if (char_pos + gap_size >= data->maxlength)
          return ("");
       return (&data->ptr[(char_pos + gap_size)]);
    }
}


Boolean
#ifdef _NO_PROTO
_XmTextFindStringBackwards( w, start, search_string, position)
        Widget w;
        XmTextPosition start;
        char *search_string;
        XmTextPosition *position;
#else
_XmTextFindStringBackwards(
        Widget w,
        XmTextPosition start,
        char* search_string,
        XmTextPosition *position)
#endif /* _NO_PROTO */
{
    register int i;
    XmTextWidget tw = (XmTextWidget) w;
    XmSourceData data = ((XmTextWidget)w)->text.source->data;
    Boolean return_val = False, match = False;
    int search_length = 0;
    char *ptr, *end_ptr, *tmp_ptr, *end_of_data;

    search_length = _XmTextCountCharacters(search_string,
                      (search_string ? strlen(search_string) : 0) );

    if (!search_length || !data->length || search_length > data->length)
       return FALSE;

   /* Search can be broken into three phases for fastest search:
    *
    *    - search from data_end - strlen(search_string) back until
    *      base search is at gap_end.  This is a fast simple compare 
    *      that doesn't worry about incursions into the gap.
    *
    *    - search from gap_start-strlen(search_string) until the base
    *      of the search is moved across the gap.
    *
    *    - search from start up to gap_start-strlen(search_string).
    *      This is a fast simple compare that doesn't worry about
    *      incursions into the gap.
    */

    switch ((int)tw->text.char_size) {
       case 1: {

         /* Make sure you don't search past end of data. End of data is...  */
          end_of_data = data->ptr + data->length +
                                              (data->gap_end - data->gap_start);

         /* actually, no need to search beyond end - search_length position */
          if (end_of_data - search_length >= data->gap_end)
             end_ptr = end_of_data - search_length;
          else
             end_ptr = data->gap_start -
                                (search_length - (end_of_data - data->gap_end));

         /* Phase one: search from start back to gap_end */
         /* Set the base for the search */
          if (data->ptr + start > data->gap_start) /* backside of gap */
             ptr = data->ptr + (data->gap_end - data->gap_start) + start;
          else /* we're starting before the gap */
             ptr = data->ptr + start;

          if (ptr > end_ptr)
             ptr = end_ptr; /* no need search where a match can't be found */

          while (!return_val && ptr >= data->gap_end) {
             if (*ptr == *search_string) { /* potential winner! */
                for (match = True, i = 1; match && (i < search_length); i++){
                   if (ptr[i] != search_string[i]) {
                      match = False;
		      i--;
		   }
                }
                if (i == search_length) { /* we have a winner! */
                   *position = ptr - data->ptr - 
                                              (data->gap_end - data->gap_start);
                   return_val = True;
                }
             }
             ptr--; /* decrement base of search */
             match = True;
         }

        /* Phase two: these searches span the gap and are SLOW!
         * Two possibilities: either I've just backed the base back to gap 
         * end (and must do searches that span the gap) or start puts
         * the base prior to the gap end.  Also, possibility that there
         * isn't enough room after the gap for a complete match
         * (so no need to search it).  This phase must be performed as
         * long as data->ptr + start places the base to the right of
         * gap_start - search_length.
         */

        /* Do this as nested for loops; the outer loop decrements the base for
         * the search, the inner loop compares character elements from 0 to
         * length(search_string).
         */

        /* If no match yet and if need to search prior to gap_start... */
        /* Set the base for the search. */

         if (!return_val && 
                 (data->ptr + start) > (data->gap_start - search_length)){
            if (data->ptr < data->gap_start)
               ptr = data->gap_start - 1;
           /* else, we're done... gap_start is at data->ptr and still no match*/

            for (match = True;
                 ptr >= data->ptr && (data->gap_start - ptr) +
                                 (end_of_data - data->gap_end) >= search_length;
                 ptr--, match = True){

               if (*ptr == *search_string){ /* we have a potential winner */
                  for (i = 1; i < search_length && match == True; i ++){
                     if (ptr + i >= data->gap_start) {
                        tmp_ptr = ptr +(data->gap_end - data->gap_start) + i;
                        if (*tmp_ptr != search_string[i]){
                           match = False;
			   i--;
			}
                     } else {
                        if (ptr[i] != search_string[i]){
                           match = False;
			   i--;
			}
                     }
                  } /* end inner for loop - searching from current base */
                  if (match && (i == search_length)){/* a winner! */
                     *position = ptr - data->ptr;
                     return_val = True;
                  }
               }
               if (return_val) break;
            } /* end outer for - restart search from a new base */
         }

      /* phase three: search backwards from base == gap_start - search_length 
       * through and including base == data->ptr.
       */

          if (!return_val) {
             if (data->ptr + start > data->gap_start - search_length)
                ptr = data->gap_start - search_length; 
             else
                ptr = data->ptr + start;

             while (!return_val && ptr >= data->ptr){
                if (*ptr == *search_string) { /* potential winner! */
                   for (match = True, i = 1; match && (i < search_length); i++){
                      if (ptr[i] != search_string[i]){
                         match = False;
			 i--;
		      }
                   }
                   if (i == search_length) { /* we have a winner! */
		      *position = ptr - data->ptr;
                      return_val = True;
                   }
                }
                ptr--; /* decrement base of search */
                match = True;
             }
          }
          break;
       } /* end case 1 */
       case 2: { 
          BITS16 *bits16_ptr, *bits16_search_string, *bits16_gap_start;
	  BITS16 *bits16_gap_end, *bits16_end_ptr, *bits16_tmp_ptr;
	  BITS16 *bits16_end_of_data;
          bits16_ptr = bits16_search_string = NULL;
	  bits16_gap_start = bits16_gap_end = NULL;

        /* search_length is number of characters (!bytes) in search_string */
          bits16_search_string =
                    (BITS16 *) XtMalloc((unsigned)
				 (search_length + 1) * (int)tw->text.char_size);
          (void) _XmTextBytesToCharacters((char *) bits16_search_string,
                                         search_string, search_length, True,
                                         (int)tw->text.char_size);

         /* setup the variables for the search */
          bits16_gap_start = (BITS16 *) data->gap_start;
          bits16_gap_end = (BITS16 *) data->gap_end;

         /* Make sure you don't search past end of data. End of data is...  */
#ifdef NON_OSF_FIX
          bits16_ptr = (BITS16 *) data->ptr;
#endif /* NON_OSF_FIX */
          bits16_end_of_data = bits16_ptr + data->length +
                                            (bits16_gap_end - bits16_gap_start);

         /* only need to search up to end - search_length position */
          if (bits16_end_of_data - search_length >= bits16_gap_end)
             bits16_end_ptr = bits16_end_of_data - search_length;
          else
             bits16_end_ptr = bits16_gap_start -
                 (search_length - (bits16_end_of_data - bits16_gap_end));

         /* Phase one: search from start back to gap_end */

          bits16_ptr = (BITS16 *)data->ptr;
          if (bits16_ptr + start > bits16_gap_start) /* backside of gap */
             bits16_ptr = (BITS16 *)data->ptr + 
                                    (bits16_gap_end - bits16_gap_start) + start;
          else /* we're starting before the gap */
             bits16_ptr = (BITS16 *)data->ptr + start;

	                       /* no need search where a match can't be found */
          if (bits16_ptr > bits16_end_ptr)
             bits16_ptr = bits16_end_ptr; 

          while (!return_val && bits16_ptr >= bits16_gap_end) {
             if (*bits16_ptr == *bits16_search_string) { /* potential winner! */
                for (match = True, i = 1; match && (i < search_length); i++){
                   if (bits16_ptr[i] != bits16_search_string[i]) {
                      match = False;
                      i--;
                   }
                }
                if (i == search_length) { /* we have a winner! */
                   *position = bits16_ptr - (BITS16 *)data->ptr -
                                            (bits16_gap_end - bits16_gap_start);
                   return_val = True;
                }
             }
             bits16_ptr--; /* decrement base of search */
             match = True;
         }

        /* Phase two: these searches span the gap and are SLOW!
         * Two possibilities: either I've just backed the base back to gap
         * end (and must do searches that span the gap) or start puts
         * the base prior to the gap end.  Also, possibility that there
         * isn't enough room after the gap for a complete match
         * (so no need to search it).  This phase must be performed as
         * long as data->ptr + start places the base to the right of
         * gap_start - search_length.
         */

        /* Do this as nested for loops; the outer loop decrements the base for
         * the search, the inner loop compares character elements from 0 to
         * length(search_string).
         */

        /* If no match yet and if need to search prior to gap_start... */
        /* Set the base for the search. */

         bits16_ptr = (BITS16 *) data->ptr;
         if (!return_val &&
             (bits16_ptr + start) > (bits16_gap_start - search_length)){
            if (bits16_ptr < bits16_gap_start)
               bits16_ptr = bits16_gap_start - 1;
           /* else, we're done... gap_start is at data->ptr and still no match*/

            for (match = True;
                 bits16_ptr >= (BITS16*)data->ptr && 
                  (bits16_gap_start - bits16_ptr) +
                         (bits16_end_of_data - bits16_gap_end) >= search_length;
                 bits16_ptr--, match = True){

               if (*bits16_ptr == *bits16_search_string){ /* potential winner */
                  for (i = 1; i < search_length && match == True; i ++){
                     if (bits16_ptr + i >= bits16_gap_start) {
                        bits16_tmp_ptr = bits16_ptr + 
                                        (bits16_gap_end - bits16_gap_start) + i;
                        if (*bits16_tmp_ptr != bits16_search_string[i]){
                           match = False;
                           i--;
                        }
                     } else {
                        if (bits16_ptr[i] != bits16_search_string[i]){
                           match = False;
                           i--;
                        }
                     }
                  } /* end inner for loop - searching from current base */
                  if (match && (i == search_length)){/* a winner! */
                     *position = bits16_ptr - (BITS16*)data->ptr;
                     return_val = True;
                  }
               }
               if (return_val) break;
            } /* end outer for - restart search from a new base */
         }

      /* phase three: search backwards from base == gap_start - search_length
       * through and including base == data->ptr.
       */

          if (!return_val) {
             bits16_ptr = (BITS16 *) data->ptr;
             if (bits16_ptr + start > bits16_gap_start - search_length)
                bits16_ptr = bits16_gap_start - search_length;
             else
                bits16_ptr = bits16_ptr + start;

             while (!return_val && bits16_ptr >= (BITS16 *)data->ptr){
                if (*bits16_ptr == *bits16_search_string){/* potential winner!*/
                   for (match = True, i = 1; match && (i < search_length); i++){
                      if (bits16_ptr[i] != bits16_search_string[i]){
                         match = False;
                         i--;
                      }
                   }
                   if (i == search_length) { /* we have a winner! */
		      *position = bits16_ptr - (BITS16 *)data->ptr;
                      return_val = True;
                   }
                }
                bits16_ptr--; /* decrement base of search */
                match = True;
             }
          }
         /* clean up before you go */
          if (bits16_search_string != NULL)
             XtFree((char*)bits16_search_string);
          break;
       } /* end case 2 */
       case 3: case 4: default: {
          BITS32 *bits32_ptr, *bits32_search_string, *bits32_gap_start;
	  BITS32 *bits32_gap_end, *bits32_end_ptr, *bits32_tmp_ptr;
	  BITS32 *bits32_end_of_data;
          bits32_ptr = bits32_search_string = NULL;
	  bits32_gap_start = bits32_gap_end = NULL;

          bits32_search_string =
                    (BITS32 *) XtMalloc((unsigned)
				 (search_length + 1) * (int)tw->text.char_size);
          (void)_XmTextBytesToCharacters((char *) bits32_search_string,
                                        search_string, search_length, True,
                                        (int)tw->text.char_size);

         /* setup the variables for the search of new lines before the gap */
          bits32_gap_start = (BITS32 *) data->gap_start;
          bits32_gap_end = (BITS32 *) data->gap_end;

         /* Make sure you don't search past end of data. End of data is...  */
#ifdef NON_OSF_FIX
          bits32_ptr = (BITS32 *) data->ptr;
#endif /* NON_OSF_FIX */
          bits32_end_of_data = bits32_ptr + data->length +
                                            (bits32_gap_end - bits32_gap_start);

         /* only need to search up to end - search_length position */
          if (bits32_end_of_data - search_length >= bits32_gap_end)
             bits32_end_ptr = bits32_end_of_data - search_length;
          else
             bits32_end_ptr = bits32_gap_start -
                        (search_length - (bits32_end_of_data - bits32_gap_end));

         /* Phase one: search from start back to gap_end */

          bits32_ptr = (BITS32 *)data->ptr;
          if (bits32_ptr + start > bits32_gap_start) /* backside of gap */
             bits32_ptr = (BITS32 *)data->ptr +
                                    (bits32_gap_end - bits32_gap_start) + start;
          else /* we're starting before the gap */
             bits32_ptr = (BITS32 *)data->ptr + start;

	                       /* no need search where a match can't be found */
          if (bits32_ptr > bits32_end_ptr)
             bits32_ptr = bits32_end_ptr; 

          while (!return_val && bits32_ptr >= bits32_gap_end) {
             if (*bits32_ptr == *bits32_search_string) { /* potential winner! */
                for (match = True, i = 1; match && (i < search_length); i++){
                   if (bits32_ptr[i] != bits32_search_string[i]) {
                      match = False;
                      i--;
                   }
                }
             if (i == search_length) { /* we have a winner! */
                   *position = bits32_ptr - (BITS32 *)data->ptr -
                                            (bits32_gap_end - bits32_gap_start);
                   return_val = True;
                }
             }
             bits32_ptr--; /* decrement base of search */
             match = True;
          }

         /* Phase two: these searches span the gap and are SLOW!
          * Two possibilities: either I've just backed the base back to gap
          * end (and must do searches that span the gap) or start puts
          * the base prior to the gap end.  Also, possibility that there
          * isn't enough room after the gap for a complete match
          * (so no need to search it).  This phase must be performed as
          * long as data->ptr + start places the base to the right of
          * gap_start - search_length.
          */

         /* Do this as nested for loops; the outer loop decrements the base for
          * the search, the inner loop compares character elements from 0 to
          * length(search_string).
          */

         /* If no match yet and if need to search prior to gap_start... */
         /* Set the base for the search. */

          bits32_ptr = (BITS32 *) data->ptr;
          if (!return_val &&
              (bits32_ptr + start) > (bits32_gap_start - search_length)){
             if (bits32_ptr < bits32_gap_start)
                bits32_ptr = bits32_gap_start - 1;
            /*else, we're done... gap_start is at data->ptr and still no match*/

             for (match = True;
                  bits32_ptr >= (BITS32*)data->ptr &&
                   (bits32_gap_start - bits32_ptr) +
                       (bits32_end_of_data - bits32_gap_end) >= search_length;
                  bits32_ptr--, match = True){

                if (*bits32_ptr == *bits32_search_string){/* potential winner */
                   for (i = 1; i < search_length && match == True; i ++){
                      if (bits32_ptr + i >= bits32_gap_start) {
                         bits32_tmp_ptr = bits32_ptr +
                                        (bits32_gap_end - bits32_gap_start) + i;
                         if (*bits32_tmp_ptr != bits32_search_string[i]){
                            match = False;
                            i--;
                         }
                      } else {
                         if (bits32_ptr[i] != bits32_search_string[i]){
                            match = False;
                            i--;
                         }
                      }
                   } /* end inner for loop - searching from current base */
                   if (match && (i == search_length)){/* a winner! */
                      *position = bits32_ptr - (BITS32*)data->ptr;
                      return_val = True;
                   }
                }
                if (return_val) break;
             } /* end outer for - restart search from a new base */
          }

      /* phase three: search backwards from base == gap_start - search_length
       * through and including base == data->ptr.
       */

       if (!return_val) {
          bits32_ptr = (BITS32 *) data->ptr;
          if (bits32_ptr + start > bits32_gap_start - search_length)
             bits32_ptr = bits32_gap_start - search_length;
          else
             bits32_ptr = bits32_ptr + start;

          while (!return_val && bits32_ptr >= (BITS32 *)data->ptr){
             if (*bits32_ptr == *bits32_search_string){/* potential winner!*/
                for (match = True, i = 1; match && (i < search_length); i++){
                   if (bits32_ptr[i] != bits32_search_string[i]){
                      match = False;
                      i--;
                   }
                }
                if (i == search_length) { /* we have a winner! */
		   *position = bits32_ptr - (BITS32 *)data->ptr;
                   return_val = True;
                }
             }
             bits32_ptr--; /* decrement base of search */
             match = True;
          }
       }
      /* clean up before you go */
       if (bits32_search_string != NULL)
          XtFree((char*)bits32_search_string);
       break;
       } /* end case 3, 4, and default */
    } /* end switch */

    return return_val;
}


Boolean
#ifdef _NO_PROTO
_XmTextFindStringForwards( w, start, search_string, position)
        Widget w;
        XmTextPosition start;
        char *search_string;
        XmTextPosition *position;
#else
_XmTextFindStringForwards(
        Widget w,
        XmTextPosition start,
        char* search_string,
        XmTextPosition *position)
#endif /* _NO_PROTO */
{
    register int i;
    XmTextWidget tw = (XmTextWidget) w;
    XmSourceData data = tw->text.source->data;
    Boolean return_val = False, match = False;
    int search_length = 0;
    char *ptr, *end_ptr, *tmp_ptr, *end_of_data;

    search_length = _XmTextCountCharacters(search_string,
                     (search_string ? strlen(search_string) : 0) );

    if (!search_length || !data->length || search_length > data->length) 
       return FALSE;

   /* Search can be broken into three phases for fastest search:
    *
    *    - search from start up to gap_start-strlen(search_string).
    *      This is a fast simple compare that doesn't worry about
    *      incursions into the gap.
    *
    *    - search from gap_start-strlen(search_string) until the base
    *      of the search is moved across the gap.
    *
    *    - search from gap_end to data_end - strlen(search_string).
    *      This is a fast, simple compare that doesn't worry about
    *      incursions into the gap or overrunning end of data.
    */

    switch ((int)tw->text.char_size) {
       case 1: {

	 /* Make sure you don't search past end of data. End of data is...  */
	  end_of_data = data->ptr + data->length + 
					      (data->gap_end - data->gap_start);

         /* actually, only need to search up to end - search_length position */
          if (end_of_data - search_length >= data->gap_end)
	     end_ptr = end_of_data - search_length;
          else 
	     end_ptr = data->gap_start - 
				(search_length - (end_of_data - data->gap_end));

	 /* Phase one: search from start to gap_start-strlen(search_string) */

	  ptr = data->ptr + start;
	  while (!return_val && ptr + search_length <= data->gap_start){
             if (*ptr == *search_string) { /* potential winner! */
	        for (match = True, i = 1; match && (i < search_length); i++){
	           if (ptr[i] != search_string[i]){
		      match = False;
		      i--;
		   }
	        }
	        if (i == search_length) { /* we have a winner! */
		   *position = ptr - data->ptr;
		   return_val = True;
                }
	     }
	     ptr++; /* advance base of search */
	     match = True;
	 }   

	/* Phase two: these searches span the gap and are SLOW! 
	 * Two possibilities: either I'm just short of the gap
	 * (and must do searches that span the gap) or start puts
	 * the base after gap end.  Also, possibility that there
         * isn't enough room after the gap for a complete match
         * (so no need to search it).
	 */

	/* Do this as nested for loops; the outer loop advances the base for
	 * the search, the inner loop compares character elements from 0 to
	 * length(search_string).  
         */

        /* if no match yet and if need to search prior to gap_start... */

	 if (!return_val && (data->ptr + start) < data->gap_start){
	    if (data->ptr + start < data->gap_start - search_length)
	       ptr = data->gap_start - search_length;
	    else
	       ptr = data->ptr + start;

            for (match = True; 
		 ptr < data->gap_start && (data->gap_start - ptr) + 
		                 (end_of_data - data->gap_end) >= search_length;
		 ptr++, match = True){

	       if (*ptr == *search_string){ /* we have a potential winner */
	          for (i = 1; i < search_length && match == True; i ++){
	             if (ptr + i >= data->gap_start) {
                        tmp_ptr = ptr +(data->gap_end - data->gap_start) + i;
	                if (*tmp_ptr != search_string[i]){
		           match = False;
			   i--;
			}
	             } else {
		        if (ptr[i] != search_string[i]){
		           match = False;
			   i--;
			}
                     }
		  } /* end inner for loop - searching from current base */
	          if (match && (i == search_length)){/* a winner! */
		     *position = ptr - data->ptr;
		     return_val = True;
	          }
	       } 
	       if (return_val) break;
	    } /* end outer for - restart search from a new base */
         }
      /* phase three: search after gap end upto end of data - search_length */

          if (!return_val) {
             if (data->ptr + start < data->gap_start)
                ptr = data->gap_end; /* we've already started - continue at
                                      * gap end. */
             else
                ptr = data->ptr + (data->gap_end - data->gap_start) + start;

             while (!return_val && ptr <= end_ptr){
                if (*ptr == *search_string) { /* potential winner! */
                   for (match = True, i = 1; match && (i < search_length); i++){
                      if (ptr[i] != search_string[i]){
                         match = False;
			 i--;
		      }
                   }
                   if (i == search_length) { /* we have a winner! */
		      *position = ptr - data->ptr -
                         (data->gap_end - data->gap_start);
                      return_val = True;
                   }
		}
                ptr++; /* advance base of search */
                match = True;
             }
          }
       } /* end case 1 */
       break;
       case 2: {
          BITS16 *bits16_ptr, *bits16_search_string, *bits16_gap_start;
	  BITS16 *bits16_gap_end, *bits16_end_ptr, *bits16_tmp_ptr; 
	  BITS16 *bits16_end_of_data;
          bits16_ptr = bits16_search_string = NULL;
	  bits16_gap_start = bits16_gap_end = NULL;

        /* search_length is number of characters (!bytes) in search_string */
         bits16_search_string =
                    (BITS16 *) XtMalloc((unsigned)
				 (search_length + 1) * (int)tw->text.char_size);
         (void) _XmTextBytesToCharacters((char *) bits16_search_string, 
                                         search_string, search_length, True,
                                         (int)tw->text.char_size);

         /* setup the variables for the search */
         bits16_gap_start = (BITS16 *) data->gap_start;
         bits16_gap_end = (BITS16 *) data->gap_end;

         /* Make sure you don't search past end of data. End of data is...  */
#ifdef NON_OSF_FIX
          bits16_ptr = (BITS16 *) data->ptr;
#endif /* NON_OSF_FIX */
          bits16_end_of_data = bits16_ptr + data->length +
                                            (bits16_gap_end - bits16_gap_start);

         /* only need to search up to end - search_length position */
          if (bits16_end_of_data - search_length >= bits16_gap_end)
             bits16_end_ptr = bits16_end_of_data - search_length;
          else
             bits16_end_ptr = bits16_gap_start -
                 (search_length - (bits16_end_of_data - bits16_gap_end));

         /* Phase one: search from start to gap start - search_length */

          bits16_ptr = (BITS16 *)data->ptr + start;
          while (!return_val && bits16_ptr + search_length <= bits16_gap_start){
             if (*bits16_ptr == *bits16_search_string) { /* potential winner! */
                for (match = True, i = 1; match && (i < search_length); i++){
                   if (bits16_ptr[i] != bits16_search_string[i]){
                      match = False;
		      i--;
		   }
                }
                if (i == search_length) { /* we have a winner! */
                   *position = bits16_ptr - (BITS16 *)data->ptr;
                   return_val = True;
                }
             }
             bits16_ptr++; /* advance base of search */
             match = True;
         }

        /* Phase two: these searches span the gap and are SLOW!
         * Two possibilities: either I'm just short of the gap
         * (and must do searches that span the gap) or start puts
         * the base after gap end.  Also, possibility that there
         * isn't enough room after the gap for a complete match
         * (so no need to search it).
         */

        /* Do this as nested for loops; the outer loop advances the base for
         * the search, the inner loop compares character elements from 0 to
         * length(search_string).
         */

        /* if no match yet and if need to search prior to gap_start... */

         bits16_ptr = (BITS16 *) data->ptr;
         if (!return_val && (bits16_ptr + start) < bits16_gap_start){
            if (bits16_ptr + start < bits16_gap_start - search_length)
               bits16_ptr = bits16_gap_start - search_length;
            else
               bits16_ptr = (BITS16*)data->ptr + start;

            for (match = True;
                 bits16_ptr < bits16_gap_start && 
                        (bits16_gap_start - bits16_ptr) +
		        (bits16_end_of_data - bits16_gap_end) >= search_length;
                 bits16_ptr++, match = True){

#ifdef NON_OSF_FIX
                if (*bits16_ptr == *bits16_search_string)
#else /* NON_OSF_FIX */
                if (*bits16_ptr == (BITS16)*search_string)
#endif /* NON_OSF_FIX */
	        { /* have a potential winner*/
                  for (i = 1; i < search_length && match == True; i ++){
                     if (bits16_ptr + i >= bits16_gap_start) {
                        bits16_tmp_ptr = bits16_ptr +
                                       (bits16_gap_end - bits16_gap_start) + i;
#ifdef NON_OSF_FIX
                        if (*bits16_tmp_ptr != bits16_search_string[i]){
#else /* NON_OSF_FIX */
                        if (*bits16_tmp_ptr != (BITS16)search_string[i]){
#endif /* NON_OSF_FIX */
                           match = False;
			   i--;
			}
                     } else {
#ifdef NON_OSF_FIX
                        if (bits16_ptr[i] != bits16_search_string[i]){
#else /* NON_OSF_FIX */
                        if (bits16_ptr[i] != (BITS16)search_string[i]){
#endif /* NON_OSF_FIX */
                           match = False;
			   i--;
			}
                     }
                  } /* end inner for loop - searching from current base */
                  if (match && (i == search_length)){ /* a winner! */
                     *position = bits16_ptr - (BITS16*)data->ptr;
                     return_val = True;
                  }
               }
               if (return_val) break;
            } /* end outer for - restart search from a new base */
         }

      /* phase three: search after gap end upto end of data - search_length */

          if (!return_val) {
             bits16_ptr = (BITS16 *) data->ptr;
             if (bits16_ptr + start < bits16_gap_start)
                bits16_ptr = bits16_gap_end; /* we've already started...
                                              * continue at gap end. */
             else
                bits16_ptr = (BITS16*)data->ptr + 
                                   (bits16_gap_end - bits16_gap_start) + start;

             while (!return_val && bits16_ptr <= bits16_end_ptr){
#ifdef NON_OSF_FIX
                if (*bits16_ptr == *bits16_search_string)
#else /* NON_OSF_FIX */
                if (*bits16_ptr == (BITS16)*search_string)
#endif /* NON_OSF_FIX */
		{ /* potential winner! */
                   for (match = True, i=1; match && (i < search_length); i++){
#ifdef NON_OSF_FIX
                      if (bits16_ptr[i] != bits16_search_string[i]){
#else /* NON_OSF_FIX */
                      if (bits16_ptr[i] != (BITS16)search_string[i]){
#endif /* NON_OSF_FIX */
                         match = False;
			 i--;
		      }
                   }
                   if (i == search_length) { /* we have a winner! */
                      *position = bits16_ptr - (BITS16 *)data->ptr -
                         (bits16_gap_end - bits16_gap_start);
                      return_val = True;
                   }
                }
                bits16_ptr++; /* advance base of search */
                match = True;
             }
          }
         /* clean up before you go */
          if (bits16_search_string != NULL)
             XtFree((char*)bits16_search_string);
          break;
       } /* end case 2 */
       case 3: case 4: default: {
          BITS32 *bits32_ptr, *bits32_search_string, *bits32_gap_start;
	  BITS32 *bits32_gap_end, *bits32_end_ptr, *bits32_tmp_ptr;
	  BITS32 *bits32_end_of_data;
          bits32_ptr = bits32_search_string = NULL;
	  bits32_gap_start = bits32_gap_end = NULL;

          bits32_search_string =
                    (BITS32 *) XtMalloc((unsigned)
				 (search_length + 1) * (int)tw->text.char_size);
          (void)_XmTextBytesToCharacters((char *) bits32_search_string, 
					search_string, search_length, True,
                                        (int)tw->text.char_size);

         /* setup the variables for the search of new lines before the gap */
          bits32_gap_start = (BITS32 *) data->gap_start;
          bits32_gap_end = (BITS32 *) data->gap_end;

         /* Make sure you don't search past end of data. End of data is...  */
#ifdef NON_OSF_FIX
          bits32_ptr = (BITS32 *) data->ptr;
#endif /* NON_OSF_FIX */
          bits32_end_of_data = bits32_ptr + data->length +
                                            (bits32_gap_end - bits32_gap_start);

         /* only need to search up to end - search_length position */
          if (bits32_end_of_data - search_length >= bits32_gap_end)
             bits32_end_ptr = bits32_end_of_data - search_length;
          else
             bits32_end_ptr = bits32_gap_start -
                 (search_length - (bits32_end_of_data - bits32_gap_end));

         /* Phase one: search from start to gap start - search_length */

          bits32_ptr = (BITS32 *)data->ptr + start;
          while (!return_val && bits32_ptr + search_length <= bits32_gap_start){
             if (*bits32_ptr == *bits32_search_string) { /* potential winner! */
                for (match = True, i = 1; match && (i < search_length); i++){
                   if (bits32_ptr[i] != bits32_search_string[i]){
                      match = False;
		      i--;
		   }
                }
                if (i == search_length) { /* we have a winner! */
                   *position = bits32_ptr - (BITS32 *)data->ptr;
                   return_val = True;
                }
             }
             bits32_ptr++; /* advance base of search */
             match = True;
         }

        /* Phase two: these searches span the gap and are SLOW!
         * Two possibilities: either I'm just short of the gap
         * (and must do searches that span the gap) or start puts
         * the base after gap end.  Also, possibility that there
         * isn't enough room after the gap for a complete match
         * (so no need to search it).
         */

        /* Do this as nested for loops; the outer loop advances the base for
         * the search, the inner loop compares character elements from 0 to
         * length(search_string).
         */

        /* if no match yet and if need to search prior to gap_start... */

         bits32_ptr = (BITS32 *) data->ptr;
         if (!return_val && (bits32_ptr + start) < bits32_gap_start){
            if (bits32_ptr + start < bits32_gap_start - search_length)
               bits32_ptr = bits32_gap_start - search_length;
            else
               bits32_ptr = (BITS32*)data->ptr + start;

            for (match = True;
                 bits32_ptr < bits32_gap_start &&
                        (bits32_gap_start - bits32_ptr) +
                         (bits32_end_of_data - bits32_gap_end) >= search_length;
                 bits32_ptr++, match = True){

#ifdef NON_OSF_FIX
               if (*bits32_ptr == *bits32_search_string){
                                                   /* have a potential winner */
#else /* NON_OSF_FIX */
               if (*bits32_ptr == *search_string){ /* have a potential winner */
#endif /* NON_OSF_FIX */
                  for (i = 1; i < search_length && match == True; i ++){
                     if (bits32_ptr + i >= bits32_gap_start) {
                        bits32_tmp_ptr = bits32_ptr +
                                        (bits32_gap_end - bits32_gap_start) + i;
#ifdef NON_OSF_FIX
                        if (*bits32_tmp_ptr != bits32_search_string[i]){
#else /* NON_OSF_FIX */
                        if (*bits32_tmp_ptr != search_string[i]){
#endif /* NON_OSF_FIX */
                           match = False;
			   i--;
			}
                     } else {
#ifdef NON_OSF_FIX
                        if (bits32_ptr[i] != bits32_search_string[i]){
#else /* NON_OSF_FIX */
                        if (bits32_ptr[i] != search_string[i]){
#endif /* NON_OSF_FIX */
                           match = False;
			   i--;
			}
                     }
                  } /* end inner for loop - searching from current base */
                  if (match && (i == search_length)){ /* a winner! */
                     *position = bits32_ptr - (BITS32*)data->ptr;
                     return_val = True;
                  }
               }
               if (return_val) break;
            } /* end outer for - restart search from a new base */
         }

      /* phase three: search after gap end upto end of data - search_length */

          if (!return_val) {
             bits32_ptr = (BITS32 *) data->ptr;
             if (bits32_ptr + start < bits32_gap_start)
                bits32_ptr = bits32_gap_end; /* we've already started...
                                              * continue at gap end. */
             else
                bits32_ptr = (BITS32*)data->ptr +
                                    (bits32_gap_end - bits32_gap_start) + start;

             while (!return_val && bits32_ptr <= bits32_end_ptr){
#ifdef NON_OSF_FIX
                if (*bits32_ptr == *bits32_search_string) {
                                                     /* potential winner! */
#else /* NON_OSF_FIX */
                if (*bits32_ptr == *search_string) { /* potential winner! */
#endif /* NON_OSF_FIX */
                   for (match = True, i = 1; match && (i < search_length); i++){
#ifdef NON_OSF_FIX
                      if (bits32_ptr[i] != bits32_search_string[i]){
#else /* NON_OSF_FIX */
                      if (bits32_ptr[i] != search_string[i]){
#endif /* NON_OSF_FIX */
                         match = False;
			 i--;
		      }
                   }
                   if (i == search_length) { /* we have a winner! */
                      *position = bits32_ptr - (BITS32 *)data->ptr -
                         (bits32_gap_end - bits32_gap_start);
                      return_val = True;
                   }
                }
                bits32_ptr++; /* advance base of search */
                match = True;
             }
          }
         /* clean up before you go */
          if (bits32_search_string != NULL)
             XtFree((char*)bits32_search_string);
          break;
       } /* end case 3, 4, and default */
    } /* end switch */
    return return_val;
}

/********************************<->***********************************/
Boolean
#ifdef _NO_PROTO
_XmStringSourceFindString( w, start, string, position)
        Widget w;
        XmTextPosition start;
        char *string;
        XmTextPosition *position;
#else
_XmStringSourceFindString(
        Widget w,
        XmTextPosition start,
        char* string,
        XmTextPosition *position)
#endif /* _NO_PROTO */
{
    return(XmTextFindString(w, start, string, XmTEXT_FORWARD, position));
}

/* DELTA: length IS NOW TREATED AS NUMBER OF CHARACTERS - CALLERS MUST CHANGE */
static int 
#ifdef _NO_PROTO
CountLines( source, start, length )
        XmTextSource source ;
        XmTextPosition start ;
        unsigned long length ;
#else
CountLines(
        XmTextSource source,
        XmTextPosition start,
        unsigned long length )
#endif /* _NO_PROTO */
{
    XmSourceData data = source->data;
    XmTextWidget tw = (XmTextWidget) data->widgets[0];
    int num_lines = 0;
    unsigned long seg_length;
    char *ptr;
    BITS16 *bits16_ptr, *bits16_gap_start, *bits16_gap_end;
    BITS32 *bits32_ptr, *bits32_gap_start, *bits32_gap_end;

  /* verify that the 'start' and 'length' parameters are reasonable */

    if (start + length > data->length)
       length = data->length - start;
    if (length == 0) return num_lines;

    seg_length = (data->gap_start - data->ptr) / (int)tw->text.char_size;

  /* make sure the segment length is not greater than the length desired */
    if (length < seg_length) seg_length = length;

    switch ((int)tw->text.char_size){
       case 1: {
          /* setup the variables for the search of new lines before the gap */
          ptr = data->ptr + start;

          /* search up to gap */
          while (seg_length--) {
             if (*ptr++ == *(data->PSWC_NWLN)) ++num_lines;
          }

          /* check to see if we need more data after the gap */
          if ((int)length > data->gap_start - (data->ptr + start)) {
             if (data->gap_start - (data->ptr + start) > 0) /* if we searched
                                                             * before gap,
                                                             * adjust length */
                length -= data->gap_start - (data->ptr + start);
             ptr = data->gap_end;

             /* continue search till length is completed */
             while (length--) {
                 if (*ptr++ == *(data->PSWC_NWLN)) ++num_lines;
             }
          }
          break;
       } /* end case 1 */
       case 2: {
          /* setup the variables for the search of new lines before the gap */
          bits16_ptr = (BITS16 *) data->ptr;
          bits16_gap_start = (BITS16 *) data->gap_start;
          bits16_gap_end = (BITS16 *) data->gap_end;
          bits16_ptr += start;

          /* search up to gap */
          while (seg_length--) {
             if (*bits16_ptr++ == *(BITS16 *)(data->PSWC_NWLN)) ++num_lines;
          }

          /* check to see if we need more data after the gap */
          if ((int)length > bits16_gap_start - ((BITS16 *)data->ptr + start)) {
	    /* if we searched before gap, adjust length */
             if (bits16_gap_start - ((BITS16 *)data->ptr + start) > 0)
                length -= bits16_gap_start - ((BITS16 *)data->ptr + start);
             bits16_ptr = bits16_gap_end;

             /* continue search till length is completed */
             while (length--) {
                 if (*bits16_ptr++ == *(BITS16 *)(data->PSWC_NWLN)) ++num_lines;
             }
          }
          break;
       } /* end case 2 */
       case 3: case 4: default: {
          /* setup the variables for the search of new lines before the gap */
          bits32_ptr = (BITS32 *) data->ptr;
          bits32_gap_start = (BITS32 *) data->gap_start;
          bits32_gap_end = (BITS32 *) data->gap_end;
          bits32_ptr += start;

          /* search up to gap */
          while (seg_length--) {
             if (*bits32_ptr++ == *(BITS32 *)(data->PSWC_NWLN)) ++num_lines;
          }

          /* check to see if we need more data after the gap */
          if ((int)length > bits32_gap_start - ((BITS32 *)data->ptr + start)) {
	    /* if we searched before gap, adjust length */
             if (bits32_gap_start - ((BITS32 *)data->ptr + start) > 0)
                length -= bits32_gap_start - ((BITS32 *)data->ptr + start);
             bits32_ptr = bits32_gap_end;

             /* continue search till length is completed */
             while (length--) {
                 if (*bits32_ptr++ == *(BITS32 *)(data->PSWC_NWLN)) ++num_lines;
             }
          }
          break;
       } /* end case 3, 4, and default */
    } /* end switch */
    return num_lines;
}

static void 
#ifdef _NO_PROTO
RemoveWidget( source, widget )
        XmTextSource source ;
        XmTextWidget widget ;
#else
RemoveWidget(
        XmTextSource source,
        XmTextWidget widget )
#endif /* _NO_PROTO */
{
    XmSourceData data = source->data;
    int i;
    for (i=0 ; i<data->numwidgets ; i++) {
	if (data->widgets[i] == widget) {
            XmTextPosition left, right;
            Boolean had_selection = False;
            Time select_time =
			 XtLastTimestampProcessed(XtDisplay((Widget)widget));

	    if (data->hasselection) {
                (*source->GetSelection)(source, &left, &right);
                (*source->SetSelection)(source, 1, -999, select_time);
                had_selection = True;
            }
	    data->numwidgets--;
	    data->widgets[i] = data->widgets[data->numwidgets];
	    if (i == 0 && data->numwidgets > 0 && had_selection)
	       (*source->SetSelection)(source, left, right, select_time);
            if (data->numwidgets == 0) _XmStringSourceDestroy(source);
	    return;
	}
    }
}

/*
 * Physically moves the memory.  length == number of BYTES to be moved.
 */
/********************************<->***********************************/
static void 
#ifdef _NO_PROTO
_XmStringSourceMoveMem( from, to, length )
        char * from ;
        char * to ;
        int length ;
#else
_XmStringSourceMoveMem(
        char * from,         /* from & to are low-address markers. */
        char * to,
        int length )
#endif /* _NO_PROTO */
{
    if (from < to) {
       /* move gap to the left */
        --length;
	to += length;
	from += length;
        ++length;
        while(length--) *to-- = *from --;
    } else
       /* move gap to the right */
        while(length--) *to++ = *from ++;
}

/*
 * Determines where to move the gap and calls _XmStringSourceMoveMem()
 * to do the physical move of the gap.
 */
/********************************<->***********************************/
void 
#ifdef _NO_PROTO
_XmStringSourceSetGappedBuffer( data, position )
        XmSourceData data ;
        XmTextPosition position ;
#else
_XmStringSourceSetGappedBuffer(
        XmSourceData data,
        XmTextPosition position )       /* starting position */
#endif /* _NO_PROTO */
{
    XmTextWidget tw = (XmTextWidget) data->widgets[0];
    int count;

   /* if no change in gap placement, return */
    if (data->ptr + (position * (int)tw->text.char_size) == data->gap_start)
	return;

    if (data->ptr + (position * (int)tw->text.char_size) < data->gap_start) {
       /* move gap to the left */
        count = data->gap_start - 
			     (data->ptr + (position * (int)tw->text.char_size));
	_XmStringSourceMoveMem((data->ptr + (position*(int)tw->text.char_size)),
			       (data->gap_end - count), count);
        data->gap_start -= count; /* ie, data->gap_start = position; */
        data->gap_end -= count;   /* ie, data->gap_end = position + gap_size; */
    } else {
       /* move gap to the right */
        count = (data->ptr + 
			(position * (int)tw->text.char_size)) - data->gap_start;
	_XmStringSourceMoveMem(data->gap_end, data->gap_start, count);
        data->gap_start += count; /* ie, data->gap_start = position; */
        data->gap_end += count;   /* ie, data->gap_end = position + gap_size; */
    }
}

/********************************<->***********************************/
/* The only caller of this routine expects to get char* in block */
static void 
#ifdef _NO_PROTO
_XmStringSourceReadString( source, start, block )
        XmTextSource source ;
        int start ;
        XmTextBlock block ;
#else
_XmStringSourceReadString(
        XmTextSource source,
        int start,
        XmTextBlock block )
#endif /* _NO_PROTO */
{
    XmSourceData data = source->data;
    XmTextWidget tw = (XmTextWidget) data->widgets[0];
    int gap_size = data->gap_end - data->gap_start;
    int byte_start = start * (int)tw->text.char_size; 

    if (data->ptr + byte_start + block->length <= data->gap_start)
        block->ptr = data->ptr + byte_start;
    else if (data->ptr + byte_start + gap_size >= data->gap_end)
            block->ptr = data->ptr + byte_start + gap_size;
	 else {
            block->ptr = data->ptr + byte_start;
            block->length = data->gap_start - (data->ptr + byte_start);
         }
}

/* Caller wants block to contain char*; _XmStringSourceReadString provides
 * char*, BITS16* or BITS32*; so we need to modify what it gives us.
 */
static XmTextPosition 
#ifdef _NO_PROTO
ReadSource( source, position, last_position, block )
        XmTextSource source ;
        XmTextPosition position ;
        XmTextPosition last_position ;
        XmTextBlock block ;
#else
ReadSource(
        XmTextSource source,
        XmTextPosition position,       /* starting position */
        XmTextPosition last_position,  /* The last position we're interested */
                                       /*   in.  Don't return info about any */
                                       /*   later positions. */
        XmTextBlock block )            /* RETURN: text read in */
#endif /* _NO_PROTO */
{
    XmTextPosition return_pos;
    int num_bytes;
    XmSourceData data = source->data;
    XmTextWidget tw = (XmTextWidget) data->widgets[0];


    if (last_position > data->length) last_position = data->length;
   /* NOTE: the length calculation could result in a truncated long */
    block->length = (int)((last_position - position) * (int)tw->text.char_size);
    if (block->length < 0 ) block->length = 0;
    block->format = XmFMT_8_BIT;
    _XmStringSourceReadString(source, (int)position, block);

    if (block->length > 0) {
       if (data->old_length == 0) {
          data->value = (char *) XtMalloc((unsigned)
				 (block->length + 1) * (int)tw->text.char_size);
          data->old_length = block->length;
       } else if (block->length > data->old_length) {
          data->value = XtRealloc(data->value, (unsigned)
			       ((block->length + 1) * (int)tw->text.char_size));
          data->old_length = block->length;
       }

       if ((int)tw->text.char_size == 1) {
          return_pos = position + block->length;
       } else {
          return_pos = position + (block->length / (int)tw->text.char_size);
          num_bytes = _XmTextCharactersToBytes(data->value, block->ptr, 
                                       block->length / (int)tw->text.char_size,
                                       (int)tw->text.char_size);
          block->length = num_bytes;
          block->ptr = data->value;
       }
       return return_pos;
    } else
       return 0;
}

static void
#ifdef _NO_PROTO
Validate(start, end, maxsize)
	XmTextPosition * start;
	XmTextPosition * end;
	int maxsize;
#else /* _NO_PROTO */
Validate(
	XmTextPosition * start,
	XmTextPosition * end,
	int maxsize)
#endif /* _NO_PROTO */
{
    if (*start < 0) *start = 0;
    if (*start > maxsize) {
       *start = maxsize;
    }
    if (*end < 0 ) *end = 0;
    if (*end > maxsize) {
       *end = maxsize;
    }

    if (*start > *end) { 
       XmTextPosition tmp; /* tmp variable for swapping positions */
       tmp = *end;    
       *end = *start;
       *start = tmp;
    }
}

Boolean 
#ifdef _NO_PROTO
_XmTextModifyVerify( initiator, event, start, end, cursorPos,
		     block, newblock, freeBlock )
        XmTextWidget initiator ;
        XEvent *event ;
        XmTextPosition *start ;
        XmTextPosition *end ;
        XmTextPosition *cursorPos ;
        XmTextBlock block ;
        XmTextBlock newblock ;
        Boolean *freeBlock ;
#else
_XmTextModifyVerify(
        XmTextWidget initiator,
        XEvent *event,
        XmTextPosition *start,
        XmTextPosition *end,
        XmTextPosition *cursorPos,
        XmTextBlock block,
        XmTextBlock newblock,
	Boolean *freeBlock)
#endif /* _NO_PROTO */
{
    register XmSourceData data = initiator->text.source->data;
    register long delta;
    register int block_num_chars;  /* number of characters in the block */
    XmTextPosition newInsert = initiator->text.cursor_position;
    XmTextVerifyCallbackStruct tvcb;
    XmTextVerifyCallbackStructWcs wcs_tvcb;
    XmTextBlockRecWcs wcs_newblock;

    *freeBlock = False;

    if (*start == *end && block->length == 0) return False;

    Validate(start, end, data->length);

    block_num_chars = _XmTextCountCharacters(block->ptr, block->length);
    *cursorPos = *start + block_num_chars;

    newblock->length = block->length; 
    newblock->format = block->format; 
    newblock->ptr = block->ptr; 

    if (!initiator->text.modify_verify_callback &&
        !initiator->text.wcs_modify_verify_callback){
       return True;
    }

    delta = block_num_chars - (*end - *start);

    if ( !data->editable ||
          (delta > 0 && data->length + delta > data->maxallowed))
        return False;

   /* If both modify_verify and modify_verify_wcs are registered:
    *    - first call the char* callback, then
    *    - pass the modified data from the char* callback to the
    *      wchar_t callback.
    * If programmers set both callback lists, they get's what they asked for.
    */

    wcs_newblock.wcsptr = (wchar_t *)NULL;
    wcs_newblock.length = 0;

   /* If there are char* callbacks registered, call them. */
    if (initiator->text.modify_verify_callback) {
      /* Fill in the block to pass to the callback. */
       if (block->length) {
	  newblock->ptr = (char *) XtMalloc(block->length + 1);
          *freeBlock = True;
	  (void) memcpy((void*) newblock->ptr, (void*) block->ptr,
			block->length);
	  newblock->ptr[block->length] = '\0';
       }

    /* Call Verification Callback to indicate that text is being modified */
	tvcb.reason = XmCR_MODIFYING_TEXT_VALUE;
	tvcb.event = event;
	tvcb.currInsert = (XmTextPosition) (initiator->text.cursor_position);
	tvcb.newInsert = (XmTextPosition) (initiator->text.cursor_position);
	tvcb.startPos = *start;
	tvcb.endPos = *end;
	tvcb.doit = True;
	tvcb.text = newblock;
	XtCallCallbackList ((Widget) initiator,
			    initiator->text.modify_verify_callback,
			    (XtPointer) &tvcb);
       /* If doit flag is false, application wants to negate the action,
	* so free allocate space and return False.
	*/
	if (!tvcb.doit) {
	  if (newblock->ptr) XtFree(newblock->ptr);
          *freeBlock = False;
	  return False;
	} else {
	  *start = tvcb.startPos;
	  *end = tvcb.endPos;
          newInsert = tvcb.newInsert;
	  Validate (start, end, data->length);
	  if (tvcb.text != newblock || tvcb.text->ptr != newblock->ptr) {
	     newblock->length = tvcb.text->length;
	     if (newblock->ptr) XtFree(newblock->ptr);
             *freeBlock = False;
	     if (newblock->length) {
		newblock->ptr = XtMalloc(newblock->length + 1);
                *freeBlock = True;
		(void)memcpy((void*)newblock->ptr, (void*)tvcb.text->ptr, 
			     tvcb.text->length);
	     } else newblock->ptr = NULL;
	  }
	  newblock->format = tvcb.text->format;
	  block_num_chars = _XmTextCountCharacters(newblock->ptr,
						   newblock->length);

	  delta = block_num_chars - (*end - *start);

	  if (delta > 0 && data->length + delta > data->maxallowed) {
	     if (newblock->ptr) XtFree(newblock->ptr);
             *freeBlock = False;
	     return False;
	  }
       }
    }  /* end if there are char* modify verify callbacks */

    if (initiator->text.wcs_modify_verify_callback){
       wcs_newblock.wcsptr = (wchar_t *)XtMalloc((unsigned)sizeof(wchar_t) *
						 (newblock->length + 1));
       wcs_newblock.length = mbstowcs(wcs_newblock.wcsptr, newblock->ptr,
				      block_num_chars);
       if (wcs_newblock.length < 0) wcs_newblock.length = 0;
       wcs_tvcb.reason = XmCR_MODIFYING_TEXT_VALUE;
       wcs_tvcb.event = event;
       wcs_tvcb.currInsert = initiator->text.cursor_position;
       wcs_tvcb.newInsert = initiator->text.cursor_position;
       wcs_tvcb.startPos = *start;
       wcs_tvcb.endPos = *end;
       wcs_tvcb.doit = True;
       wcs_tvcb.text = &wcs_newblock;

       XtCallCallbackList((Widget) initiator,
			  initiator->text.wcs_modify_verify_callback,
			  (XtPointer) &wcs_tvcb);
       if (!wcs_tvcb.doit) {
	  if (newblock->ptr && newblock->ptr != block->ptr)
             XtFree(newblock->ptr);
          *freeBlock = False;
	  if (wcs_newblock.wcsptr) XtFree((char*)wcs_newblock.wcsptr);
	  return False;
       } else {
	 *start = wcs_tvcb.startPos;
	 *end = wcs_tvcb.endPos;
         newInsert = wcs_tvcb.newInsert;
	 Validate (start, end, data->length);
	/* use newblock as a temporary holder and put the char*
	 * data there */
	 if (newblock->ptr && newblock->ptr != block->ptr) {
            XtFree(newblock->ptr);
            newblock->ptr = NULL;
         }
         *freeBlock = False;
	 if (wcs_tvcb.text->length){
	    newblock->ptr = (char*) XtMalloc((unsigned)
					    (1 + wcs_tvcb.text->length) *
					    (int)initiator->text.char_size);
           *freeBlock = True;
	    wcs_tvcb.text->wcsptr[wcs_tvcb.text->length] = (wchar_t) 0L;
	   /* NOTE: wcstombs returns a long which could be truncated */
	    newblock->length = (int) wcstombs(newblock->ptr, 
					     wcs_tvcb.text->wcsptr,
					     (wcs_tvcb.text->length + 1) *
					     (int)initiator->text.char_size);
	 } else {
	    newblock->ptr = NULL;
	    newblock->length = 0;
	 }

	 block_num_chars = wcs_tvcb.text->length;
	 delta = block_num_chars - (*end - *start);

	/* if the wcstombs found bad data, newblock->length is negative */
	 if ((delta > 0 && data->length + delta > data->maxallowed) ||
	     newblock->length < 0){

	    if (newblock->ptr && newblock->ptr != block->ptr)
               XtFree(newblock->ptr);
            *freeBlock = False;
	    if (wcs_newblock.wcsptr) XtFree((char*)wcs_newblock.wcsptr);
	    return False;
	 }
      }

     /* If we alloced space for the wcs_newblock, we need to clean it up */
	 if (wcs_newblock.wcsptr) XtFree((char*)wcs_newblock.wcsptr);

    }  /* end if there are wide char modify verify callbacks */

    if (initiator->text.cursor_position != newInsert)
       if (newInsert > data->length + delta) {
	  *cursorPos = data->length + delta;
       } else if (newInsert < 0) {
          *cursorPos = 0;
       } else {
          *cursorPos = newInsert;
       }
    else
       *cursorPos = *start + block_num_chars;

    return True;
}

static XmTextStatus 
#ifdef _NO_PROTO
Replace( initiator, event, start, end, block, call_callbacks )
        XmTextWidget initiator ;
        XEvent * event;
        XmTextPosition *start ;
        XmTextPosition *end ;
        XmTextBlock block ;
        Boolean call_callbacks ;
#else
Replace(XmTextWidget initiator,
        XEvent * event,
        XmTextPosition *start,
        XmTextPosition *end,
        XmTextBlock block,
#if NeedWidePrototypes
        int call_callbacks )
#else
        Boolean call_callbacks )
#endif
#endif /* _NO_PROTO */
{
    register XmSourceData data = initiator->text.source->data;
    register int i;
    register long delta;
    register int block_num_chars;  /* number of characters in the block */
    int gap_size;
    int old_maxlength;

    if (*start == *end && block->length == 0) return EditReject;

    Validate(start, end, data->length);

    block_num_chars = _XmTextCountCharacters(block->ptr, block->length);
    delta = block_num_chars - (*end - *start);

    if ( !data->editable ||
	  (delta > 0 && data->length + delta > data->maxallowed))
	return EditError;

/**********************************************************************/

    initiator->text.output->DrawInsertionPoint(initiator,
					 initiator->text.cursor_position, off);

   /* Move the gap to the editing position (*start). */
    _XmStringSourceSetGappedBuffer(data, *start);

   for (i=0 ; i<data->numwidgets ; i++) {
	_XmTextDisableRedisplay(data->widgets[i], TRUE);
	if (data->hasselection)
	    XmTextSetHighlight((Widget)data->widgets[i], data->left, 
			       data->right, XmHIGHLIGHT_NORMAL);
    }

    old_maxlength = data->maxlength;
    if (data->length + delta >= data->maxlength) {
       int gap_start_offset, gap_end_offset;

       while (data->length + delta >= data->maxlength) {
             if (data->maxlength < TEXT_INCREMENT)
	        data->maxlength *= 2;
             else
                data->maxlength += TEXT_INCREMENT;
       }

       gap_start_offset = data->gap_start - data->ptr;
       gap_end_offset = data->gap_end - data->ptr;
       data->ptr = XtRealloc(data->ptr, (unsigned) 
			  ((data->maxlength) * (int)initiator->text.char_size));
       data->gap_start = data->ptr + gap_start_offset;
       data->gap_end = data->ptr + gap_end_offset +
	               ((int)initiator->text.char_size *
	                (data->maxlength - old_maxlength));
       if (gap_end_offset != (old_maxlength * (int)initiator->text.char_size))
          _XmStringSourceMoveMem(data->ptr + gap_end_offset, data->gap_end,
	                     ((int)initiator->text.char_size * old_maxlength) -
			      gap_end_offset);
/* Do something to move the allocated space into the buffer */
    } 

   /* NOTE: the value of delta could be truncated by cast to int. */
    data->length += (int) delta;

    if (data->hasselection && *start < data->right && *end > data->left) {
       if (*start <= data->left) {
	  if (*end < data->right) {
	     data->left = *end; /* delete encompasses left half of the
				  selection so move left endpoint */
	  } else {
	     data->left = data->right; /* delete encompasses the selection
					  so set selection to NULL */
	  }
       } else {
	  if (*end > data->right) {
	     data->right = *start; /* delete encompasses the right half of the
				   selection so move right endpoint */
	  } else {
	     data->right = data->left; /* delete is completely within the
					selection so set selection to NULL */
	  }
       }
    }

/* delete data */
    gap_size = data->gap_end - data->gap_start;
   /* expand the end of the gap to the right */
    if ((data->ptr + gap_size + (*end *(int)initiator->text.char_size)) > data->gap_end)
       data->gap_end += ((*end - *start) * (int)initiator->text.char_size);

/* add data */
   /* copy the data into the gap_start and increment the gap start pointer */
   /* convert data from char* to characters and copy into the gapped buffer */
    if ((int)initiator->text.char_size == 1){
       for (i=0; i < block->length; i++) {
	  /* if (data->gap_start == data->gap_end) break; */
	  *data->gap_start++ = block->ptr[i];
       }
    
    } else {
       data->gap_start += (int)initiator->text.char_size * 
			     _XmTextBytesToCharacters(data->gap_start,
                                                &block->ptr[0], 
					        block_num_chars, False,
                                                (int)initiator->text.char_size);
    }
	
    if (data->hasselection && data->left != data->right) {
       if (*end <= data->left) {
	  data->left += delta;
	  data->right += delta;
       }
       if (data->left > data->right)
	  data->right = data->left;
    }

    for (i=0 ; i<data->numwidgets ; i++) {
	_XmTextInvalidate(data->widgets[i], *start, *end, delta);
        _XmTextUpdateLineTable((Widget) data->widgets[i], *start,
				  *end, block, True);
	if (data->hasselection)
	    XmTextSetHighlight((Widget)data->widgets[i], data->left, 
			       data->right, XmHIGHLIGHT_SELECTED);

	_XmTextEnableRedisplay(data->widgets[i]);
    }
    if (*end <= initiator->text.cursor_position)
       _XmTextResetClipOrigin(initiator, initiator->text.cursor_position,
			      False);
    initiator->text.output->DrawInsertionPoint(initiator,
					   initiator->text.cursor_position, on);

    if (data->maxlength != TEXT_INITIAL_INCREM &&
        ((data->maxlength > TEXT_INCREMENT &&
	 data->length <= data->maxlength - TEXT_INCREMENT) ||
	 data->length <= data->maxlength >> 1)) {
      /* Move the gap to the last position. */
       _XmStringSourceSetGappedBuffer(data, data->length);

       data->maxlength = TEXT_INITIAL_INCREM;

       while (data->length >= data->maxlength) {
          if (data->maxlength < TEXT_INCREMENT)
             data->maxlength *= 2;
          else
             data->maxlength += TEXT_INCREMENT;
       }

       data->ptr = XtRealloc(data->ptr, (unsigned) 
			  ((data->maxlength) * (int)initiator->text.char_size));
       data->gap_start = data->ptr + (data->length * initiator->text.char_size);
       data->gap_end = data->ptr + ((data->maxlength - 1) *
		       initiator->text.char_size);
    }

    return EditDone;
}

#define Increment(data, position, direction)\
{\
    if (direction == XmsdLeft) {\
	if (position > 0) \
	    position--;\
    }\
    else {\
	if (position < data->length)\
	    position++;\
    }\
}

#define Look(data, position, dir) \
    ((dir == XmsdLeft) \
       ? ((position) ? _XmStringSourceGetChar(data, position - 1) \
	             : NULL) \
       : ((position == data->length) ? NULL \
		                     : _XmStringSourceGetChar(data, position)))
   
static void 
#ifdef _NO_PROTO
ScanParagraph( data, new_position, dir, ddir, last_char )
        XmSourceData data ;
        XmTextPosition *new_position ;
        XmTextScanDirection dir ;
        int ddir ;
        XmTextPosition *last_char ;
#else
ScanParagraph(
        XmSourceData data,
        XmTextPosition *new_position,
        XmTextScanDirection dir,
        int ddir,
        XmTextPosition *last_char )
#endif /* _NO_PROTO */
{
   Boolean found = False;
   XmTextPosition position = *new_position;
   char mb_char[1 + MB_LEN_MAX];
   char * c;

   while (position >= 0 && position <= data->length) {
       /* DELTA: Look now returns a pointer */
/* DELTA: EFFECIENCY: LEAVE AS SHORT*, INT*, ... COMPARE TO PSWC_NWLN */
       c = Look(data, position, dir);
       (void) _XmTextCharactersToBytes(mb_char, c, 1, 
                                       (int)data->widgets[0]->text.char_size);
       if (mb_char && *mb_char == '\n') {
	  /* DELTA: Look now returns a pointer */
          c = Look(data, position + ddir, dir);
          (void) _XmTextCharactersToBytes(mb_char, c, 1,
                                         (int)data->widgets[0]->text.char_size);
          while (mb_char && isspace((unsigned char)*mb_char)) {
              if (*mb_char == '\n') {
                 found = True;
                 while (mb_char && isspace((unsigned char)*mb_char)) {
		     /* DELTA: Look now returns a pointer */
                     c = Look(data, position + ddir, dir);
                     (void) _XmTextCharactersToBytes(mb_char, c, 1,
                                         (int)data->widgets[0]->text.char_size);
                     Increment(data, position, dir);
                 }
                 break;
              }
	      /* DELTA: Look now returns a pointer */
              c = Look(data, position + ddir, dir);
              (void) _XmTextCharactersToBytes(mb_char, c, 1,
                                         (int)data->widgets[0]->text.char_size);
              Increment(data, position, dir);
/* BEGIN 3145 fix -- Do not bypass a nonspace character */
		if (!isspace((unsigned char)*c))
		  *last_char = (position) + ddir;
/* END 3145 */
          }
          if (found) break;
       } else if (mb_char && !isspace((unsigned char)*mb_char)) {
          *last_char = (position) + ddir;
       }
          
       if(((dir == XmsdRight) && (position == data->length)) ||
           ((dir == XmsdLeft) && (position == 0)))
           break;
       Increment(data, position, dir);
   }

   *new_position = position;
}

static XmTextPosition 
#ifdef _NO_PROTO
Scan( source, pos, sType, dir, count, include )
        XmTextSource source ;
        XmTextPosition pos ;
        XmTextScanType sType ;
        XmTextScanDirection dir ;
        int count ;
        Boolean include ;
#else
Scan(
        XmTextSource source,
        XmTextPosition pos,
        XmTextScanType sType,
        XmTextScanDirection dir,
        int count,
#if NeedWidePrototypes
        int include )
#else
        Boolean include )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    register long whiteSpace = -1;
    register XmTextPosition position = pos;
    register int i;
    XmTextPosition temp;
    XmSourceData data = source->data;
    XmTextWidget tw = (XmTextWidget)data->widgets[0];
    char * c;
    BITS16 * bits16_ptr;
    BITS32 * bits32_ptr;
    char mb_char[1 + MB_LEN_MAX];
    Boolean start_is_mb, cur_is_mb;  /* False == 1-byte char, else multi-byte */
    int num_bytes = 0;
    int ddir = (dir == XmsdRight) ? 1 : -1;

    switch (sType) {
	case XmSELECT_POSITION: 
	    if (!include && count > 0)
		count -= 1;
	    for (i = 0; i < count; i++) {
		Increment(data, position, dir);
	    }
	    break;
	case XmSELECT_WHITESPACE: 
	case XmSELECT_WORD:
            if (tw->text.char_size == 1) {
    	       char * c;
	       for (i = 0; i < count; i++) {
		   whiteSpace = -1;
		   while (position >= 0 && position <= data->length) {
                       c = Look(data, position, dir);
		       if (c && isspace((unsigned char)*c)){
		           if (whiteSpace < 0) whiteSpace = position;
		       } else if (whiteSpace >= 0)
			   break;
		       position += ddir;
		   }
                }
             } else {
	        for (i = 0; i < count; i++) {
		   whiteSpace = -1;
		   num_bytes = _XmTextCharactersToBytes(mb_char,
		                                     Look(data, position, dir),
                                                     1,(int)tw->text.char_size);
		   start_is_mb = (num_bytes < 2 ? False : True);
		   while (position >= 0 && position <= data->length) {
	               num_bytes = _XmTextCharactersToBytes(mb_char,
			                            Look(data, position, dir),
                                                    1, (int)tw->text.char_size);
		       cur_is_mb = (num_bytes < 2 ? False : True);
		       if (!cur_is_mb && mb_char && 
			   isspace((unsigned char)*mb_char)){
		           if (whiteSpace < 0) whiteSpace = position;
		       } else if ((sType == XmSELECT_WORD) &&
			   (start_is_mb ^ cur_is_mb)){
		           if (whiteSpace < 0) whiteSpace = position;
		           break;
		       } else if (whiteSpace >= 0)
			   break;
		       position += ddir;
		   }
                }
	    }
	    if (!include) {
	       	if(whiteSpace < 0 && dir == XmsdRight)
		     whiteSpace = data->length;
		position = whiteSpace;
	    }
	    break;
	case XmSELECT_LINE: 
	    for (i = 0; i < count; i++) {
		while (position >= 0 && position <= data->length) {
		    /* DELTA: Look now returns a pointer */
		    if ((int)tw->text.char_size == 1){
		       c = Look(data, position, dir);
		       if ((c == '\0') || (*c == *data->PSWC_NWLN))
			   break;
		    }
		    else if ((int)tw->text.char_size == 2){
		       bits16_ptr = (BITS16 *) Look(data, position, dir);
		       if ((bits16_ptr == NULL) ||
			   (*bits16_ptr == *(BITS16 *)data->PSWC_NWLN))
			   break;
		    }
		    else { /* MB_CUR_MAX == 3 or 4 or more */
		       bits32_ptr = (BITS32 *) Look(data, position, dir);
		       if ((bits32_ptr == NULL) ||
			   (*bits32_ptr == *(BITS32 *)data->PSWC_NWLN))
			   break;
                    }

		    if(((dir == XmsdRight) && (position == data->length)) || 
			((dir == XmsdLeft) && (position == 0)))
			break;
		    Increment(data, position, dir);
		}
		if (i + 1 != count)
		    Increment(data, position, dir);
	    }
	    if (include) {
	    /* later!!!check for last char in file # eol */
		Increment(data, position, dir);
	    }
	    break;
	case XmSELECT_PARAGRAPH: 
           /* Muliple paragraph scanning is not guarenteed to work. */
            for (i = 0; i < count; i++) {
                XmTextPosition start_position = position; 
                XmTextPosition last_char = position; 

               /* if scanning forward, check for between paragraphs condition */
                if (dir == XmsdRight) {
		   /* DELTA: Look now returns a pointer */
                   c = Look(data, position, dir);
		   (void) _XmTextCharactersToBytes(mb_char, 
                                                   Look(data, position, dir), 
                                                   1, (int)tw->text.char_size);
                  /* if is space, go back to first non-space */
                   while (mb_char && isspace((unsigned char)*mb_char)) {
	               if (position > 0)
	                  position--;
		       else if (position == 0)
			  break;
		       (void) _XmTextCharactersToBytes(mb_char, 
                                                    Look(data, position, dir),
                                                    1, (int)tw->text.char_size);
                   }
                }

                temp = position;
                ScanParagraph(data, &temp, dir, ddir, &last_char);
		position = temp;

               /*
                * If we are at the beginning of the paragraph and we are
                * scanning left, we need to rescan to find the character
                * at the beginning of the next paragraph.
                */  
		if (dir == XmsdLeft) {
                  /* If we started at the beginning of the paragraph, rescan */
                   if (last_char == start_position) {
                      temp = position;
                      ScanParagraph(data, &temp, dir, ddir, &last_char);
                   }
                  /*
                   * Set position to the last non-space 
                   * character that was scanned.
                   */
                   position = last_char;
                }

                if (i + 1 != count)
                    Increment(data, position, dir);
                
            }
            if (include) {
                Increment(data, position, dir);
            }
            break;
	case XmSELECT_ALL: 
	    if (dir == XmsdLeft)
		position = 0;
	    else
		position = data->length;
    }
    if (position < 0) position = 0;
    if (position > data->length) position = data->length;
    return(position);
}

static Boolean 
#ifdef _NO_PROTO
GetSelection( source, left, right )
        XmTextSource source ;
        XmTextPosition *left ;
        XmTextPosition *right ;
#else
GetSelection(
        XmTextSource source,
        XmTextPosition *left,
        XmTextPosition *right )
#endif /* _NO_PROTO */
{
    XmSourceData data = source->data;

    if (data->hasselection && data->left <= data->right && data->left >= 0) {
	*left = data->left;
	*right = data->right;
	return TRUE;
    } else {
        *left = *right = 0;
        data->hasselection = FALSE;
    }
    return FALSE;
}

static void 
#ifdef _NO_PROTO
SetSelection( source, left, right, set_time )
        XmTextSource source ;
        XmTextPosition left ;
        XmTextPosition right ;
        Time set_time ;
#else
SetSelection(
        XmTextSource source,
        XmTextPosition left,
        XmTextPosition right, /* if right == -999, then  we're in */
        Time set_time )   /* LoseSelection, so don't call XtDisownSelection.*/
#endif /* _NO_PROTO */
{
    XmSourceData data = source->data;
    XmTextWidget tw;
    int i;

    if (!XtIsRealized((Widget)data->widgets[0]) ||
	(left > right && !data->hasselection)) return;

    if (left < 0) left = right = 0;

    for (i=0 ; i<data->numwidgets; i++) {
	tw = (XmTextWidget)(data->widgets[i]);
        (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position,
                                               off);
	_XmTextDisableRedisplay(data->widgets[i], FALSE);
	if (data->hasselection)
	    XmTextSetHighlight((Widget)data->widgets[i], data->left, 
			       data->right, XmHIGHLIGHT_NORMAL);

        data->widgets[i]->text.output->data->refresh_ibeam_off = True;
    }

    data->left = left;
    data->right = right;
    if (data->numwidgets > 0) {
	Widget widget = (Widget) data->widgets[0];
        tw = (XmTextWidget)widget;

	if (left <= right) {
	   if (!data->hasselection) {
	      if (!XtOwnSelection(widget, XA_PRIMARY, set_time,
				  _XmTextConvert, 
				  _XmTextLoseSelection,
				  (XtSelectionDoneProc) NULL)) {
		(*source->SetSelection)(source, 1, 0, set_time);
              } else {
                XmAnyCallbackStruct cb;

                data->prim_time = set_time;
                data->hasselection = True;

                cb.reason = XmCR_GAIN_PRIMARY;
                cb.event = NULL;
                XtCallCallbackList ((Widget) data->widgets[0], 
			           data->widgets[0]->text.gain_primary_callback,
				   (XtPointer) &cb);
              }
           }
           if (data->hasselection && data->left < data->right) {
              for (i=0 ; i<data->numwidgets; i++) {
	            XmTextSetHighlight((Widget) data->widgets[i], data->left,
				       data->right, XmHIGHLIGHT_SELECTED);
                }
           }
	   if (left == right) XmTextSetAddMode(widget, False);
	} else {
	    if (right != -999)
	       XtDisownSelection(widget, XA_PRIMARY, set_time);
            data->hasselection = False;
            XmTextSetAddMode(widget, False);
	}
    }
    for (i=0 ; i<data->numwidgets; i++) {
	tw = (XmTextWidget)(data->widgets[i]);
	_XmTextEnableRedisplay(data->widgets[i]);
        (*tw->text.output->DrawInsertionPoint)(tw, tw->text.cursor_position,
                                               on);
    }
}

/* Public routines. */

void
#ifdef _NO_PROTO
_XmTextValueChanged(initiator, event)
     XmTextWidget initiator ;
     XEvent *event ;
#else
_XmTextValueChanged(XmTextWidget initiator,
		    XEvent *event)
#endif
{
  XmAnyCallbackStruct cb;
  
  cb.reason = XmCR_VALUE_CHANGED;
  cb.event = event;
  if (initiator->text.value_changed_callback)
    XtCallCallbackList((Widget)initiator,
		       initiator->text.value_changed_callback, (XtPointer)&cb);
}

XmTextSource 
#ifdef _NO_PROTO
_XmStringSourceCreate( value, is_wchar )
        char *value ;
        Boolean is_wchar ;
#else
_XmStringSourceCreate(
        char *value,
#if NeedWidePrototypes
	int is_wchar)
#else
	Boolean is_wchar)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmTextSource source;
    XmSourceData data;
    int num_chars;
    char newline = '\n';
    wchar_t *wc_value;
    char * tmp_value;
    int char_size;
    int ret_value = 0;

    source = (XmTextSource) XtMalloc((unsigned) sizeof(XmTextSourceRec));
    data = source->data = (XmSourceData)
	XtMalloc((unsigned) sizeof(XmSourceDataRec));
    source->AddWidget = AddWidget;
    source->CountLines = CountLines;
    source->RemoveWidget = RemoveWidget;
    source->ReadSource = ReadSource;
    source->Replace = (ReplaceProc) Replace;
    source->Scan = Scan;
    source->GetSelection = GetSelection;
    source->SetSelection = SetSelection;

    data->source = source;
    switch (MB_CUR_MAX) {
       case 1: case 2: case 4: {
          char_size = MB_CUR_MAX;
          break;
       }
       case 3: {
          char_size = 4;
          break;
       }
       default:
          char_size = 1;
    }

    if (is_wchar){
       for (num_chars = 0, wc_value = (wchar_t*)value; 
	    wc_value[num_chars] != 0L;) num_chars++;

       data->maxlength = TEXT_INITIAL_INCREM;

       while ((num_chars + 1) >= data->maxlength) {
          if (data->maxlength < TEXT_INCREMENT)
             data->maxlength *= 2;
          else
             data->maxlength += TEXT_INCREMENT;
       }

       data->old_length = 0;
       data->ptr = XtMalloc((unsigned)((data->maxlength) * char_size));
       tmp_value = XtMalloc((unsigned)((num_chars + 1) * char_size));
       ret_value = wcstombs(tmp_value, wc_value, (num_chars + 1) * char_size);
       data->value = NULL;  /* Scratch area for block->ptr conversions */
       if (ret_value < 0) {
	  data->length = 0;
       } else {
		                                      /* Doesnt include NULL */
       data->length = _XmTextBytesToCharacters(data->ptr, tmp_value, num_chars,
					       False, char_size);
	}
       XtFree(tmp_value);
    } else {
       num_chars = _XmTextCountCharacters(value, (value ? strlen(value) : 0));
       data->maxlength = TEXT_INITIAL_INCREM;

       while ((num_chars + 1) >= data->maxlength) {
          if (data->maxlength < TEXT_INCREMENT)
             data->maxlength *= 2;
          else
             data->maxlength += TEXT_INCREMENT;
       }

       data->old_length = 0;
       data->ptr = XtMalloc((unsigned)((data->maxlength) * char_size));

       data->value = NULL;  /* Scratch area for block->ptr conversions */
       data->length = _XmTextBytesToCharacters(data->ptr, value, num_chars,
					       False, char_size);
    }

    data->PSWC_NWLN = (char *) XtMalloc(char_size);
    _XmTextBytesToCharacters(data->PSWC_NWLN, &newline, 1, False, char_size);

    data->numwidgets = 0;
    data->widgets = (XmTextWidget *) XtMalloc((unsigned) sizeof(XmTextWidget));
    data->hasselection = FALSE;
    data->left = data->right = 0;
    data->editable = TRUE;
    data->maxallowed = MAXINT;
    data->gap_start = data->ptr + (data->length * char_size);
    data->gap_end = data->ptr + ((data->maxlength - 1) * char_size);
    data->prim_time = 0;
    return source;
}

void 
#ifdef _NO_PROTO
_XmStringSourceDestroy( source )
        XmTextSource source ;
#else
_XmStringSourceDestroy(
        XmTextSource source )
#endif /* _NO_PROTO */
{
    XtFree((char *) source->data->ptr);
    XtFree((char *) source->data->value);
    XtFree((char *) source->data->widgets);
    XtFree((char *) source->data->PSWC_NWLN);
    XtFree((char *) source->data);
    XtFree((char *) source);
    source = NULL;
}

char *
#ifdef _NO_PROTO
_XmStringSourceGetValue( source, want_wchar )
        XmTextSource source ;
        Boolean want_wchar;
#else
_XmStringSourceGetValue(
        XmTextSource source,
#if NeedWidePrototypes
        int want_wchar)
#else
        Boolean want_wchar)
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    XmSourceData data = source->data;
    XmTextWidget tw = (XmTextWidget) data->widgets[0];
    XmTextBlockRec block;
    int length = 0;
    XmTextPosition pos = 0;
    XmTextPosition ret_pos = 0;
    XmTextPosition last_pos = 0;
    char * temp;
    wchar_t * wc_temp;
    int return_val = 0;

    if (!want_wchar) {
       if (data->length > 0)
          temp = (char *) XtMalloc((unsigned)
				  (data->length + 1) * (int)tw->text.char_size);
       else
          return(XtNewString(""));

       last_pos = (XmTextPosition) data->length;

       while (pos < last_pos) {
          ret_pos = ReadSource(source, pos, last_pos, &block);
          if (block.length == 0)
             break;

          (void)memcpy((void*)&temp[length], (void*)block.ptr, block.length);
          length += block.length;
          pos = ret_pos;
       }
       temp[length] = '\0';
       return (temp);
    } else {

       if (data->length > 0)
          wc_temp = (wchar_t*)XtMalloc((unsigned)
			            (data->length+1) * sizeof(wchar_t));
       else {
          wc_temp = (wchar_t*)XtMalloc((unsigned) sizeof(wchar_t));
          wc_temp[0] = (wchar_t)0L;
          return (char*) wc_temp;
       }

       last_pos = (XmTextPosition) data->length;

       while (pos < last_pos) {
          ret_pos = ReadSource(source, pos, last_pos, &block);
          if (block.length == 0)
             break;

        /* NOTE: ret_pos - pos could result in a truncated long. */
	  return_val = mbstowcs(&wc_temp[length], block.ptr,
				(int)(ret_pos - pos));
	  if (return_val > 0) length += return_val;
          pos = ret_pos;
       }
       wc_temp[length] = (wchar_t)0L;
       return ((char*)wc_temp);
    }
}

void 
#ifdef _NO_PROTO
_XmStringSourceSetValue( widget, value )
        XmTextWidget widget ;
        char *value ;
#else
_XmStringSourceSetValue(
        XmTextWidget widget,
        char *value )
#endif /* _NO_PROTO */
{
    XmTextSource source = widget->text.source;
    XmSourceData data = source->data;
    Boolean editable, freeBlock;
    int maxallowed;
    XmTextBlockRec block, newblock;
    XmTextPosition fromPos = 0; 
    XmTextPosition toPos = data->length;
    XmTextPosition cursorPos;

    (*source->SetSelection)(source, 1, 0,
			    XtLastTimestampProcessed(XtDisplay(widget)));
    block.format = XmFMT_8_BIT;
    block.length = (value ? strlen(value) : 0);
    block.ptr = value;
    editable = data->editable;
    maxallowed = data->maxallowed;
    data->editable = TRUE;
    data->maxallowed = MAXINT;
    XmTextSetHighlight((Widget)widget, 0, widget->text.last_position, 
		       XmHIGHLIGHT_NORMAL);

    if (_XmTextModifyVerify(widget, NULL, &fromPos, &toPos,
		            &cursorPos, &block, &newblock, &freeBlock)) {
       (void)(source->Replace)(widget, NULL, &fromPos, &toPos,
			       &newblock, False);
       if (freeBlock && newblock.ptr) XtFree(newblock.ptr);
       _XmTextValueChanged(widget, NULL);
    }

    data->editable = editable;
    data->maxallowed = maxallowed;
}


Boolean 
#ifdef _NO_PROTO
_XmStringSourceHasSelection( source )
        XmTextSource source ;
#else
_XmStringSourceHasSelection(
        XmTextSource source )
#endif /* _NO_PROTO */
{
   return source->data->hasselection;
}

Boolean 
#ifdef _NO_PROTO
_XmStringSourceGetEditable( source )
        XmTextSource source ;
#else
_XmStringSourceGetEditable(
        XmTextSource source )
#endif /* _NO_PROTO */
{
    return source->data->editable;
}

void 
#ifdef _NO_PROTO
_XmStringSourceSetEditable( source, editable )
        XmTextSource source ;
        Boolean editable ;
#else
_XmStringSourceSetEditable(
        XmTextSource source,
#if NeedWidePrototypes
        int editable )
#else
        Boolean editable )
#endif /* NeedWidePrototypes */
#endif /* _NO_PROTO */
{
    source->data->editable = editable;
}

int 
#ifdef _NO_PROTO
_XmStringSourceGetMaxLength( source )
        XmTextSource source ;
#else
_XmStringSourceGetMaxLength(
        XmTextSource source )
#endif /* _NO_PROTO */
{
    return source->data->maxallowed;
}

void 
#ifdef _NO_PROTO
_XmStringSourceSetMaxLength( source, max )
        XmTextSource source ;
        int max ;
#else
_XmStringSourceSetMaxLength(
        XmTextSource source,
        int max )
#endif /* _NO_PROTO */
{
    source->data->maxallowed = max;
}
