/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)oldtext:SourceStr.c	1.19"
#endif

/*
 *************************************************************************
 * 
 * 
 ****************************procedure*header*****************************
 */
/* () {}
 *	The Above template should be located at the top of each file
 * to be easily accessable by the file programmer.  If this is included
 * at the top of the file, this comment should follow since formatting
 * and editing shell scripts look for special delimiters.		*/

/*
 *************************************************************************
 *
 * Date:	December 20, 1988
 *
 * Description:	This file contains the source code for a string soure
 *	for the TextPane widget.
 *		
 *
 *******************************file*header*******************************
 */

/*************************************<+>*************************************
 *****************************************************************************
 **
 **   Project:     X Widgets
 **
 **   Description: Code for TextPane widget string source
 **
 *****************************************************************************
 **   
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1987, 1988 by Digital Equipment Corporation, Maynard,
 **             Massachusetts, and the Massachusetts Institute of Technology,
 **             Cambridge, Massachusetts
 **   
 *****************************************************************************
 *************************************<+>*************************************/

						/* #includes go here	*/

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/PrimitiveP.h>
#include <Xol/TextPaneP.h>
#include <Xol/SourceP.h>
#include <sys/stat.h>


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

unsigned char Look();
static OlDefine OlStringSrcEditType();
static OlTextPosition OlStringSrcGetLastPos();
static int OlStringSrcRead();
static int OlStringSrcReplace();
static OlTextPosition OlStringSrcScan();
static int OlStringSrcSetLastPos();
static Boolean OlStringSourceCheckData();

					/* class procedures		*/

					/* action procedures		*/

					/* public procedures		*/

OlTextSource * OlStringSourceCreate();
void OlStringSourceDestroy();

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */


#define DEFAULTBUFFERSIZE        512


#define Increment(data, position, direction)\
{\
    if (direction == OlsdLeft) {\
	if (position > 0) \
	    position -= 1;\
    }\
    else {\
	if (position < data->length)\
	    position += 1;\
    }\
}

static int magic_value = MAGICVALUE;


/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions**********************
 */


/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource stringResources[] = {
	{XtNstring,
		XtCString,
		XtRString,
		sizeof (char *),
		XtOffset(StringSourcePtr, initial_string),
		XtRString,
		NULL},
	{XtNmaximumSize,
		XtCMaximumSize,
		XtRInt,
		sizeof (int),
		XtOffset(StringSourcePtr, max_size),
		XtRInt,
		(XtPointer)&magic_value},
	{XtNeditType,
		XtCEditType,
		XtROlDefine,
		sizeof(OlDefine), 
		XtOffset(StringSourcePtr, editMode),
		XtRImmediate,
		(XtPointer) ((OlDefine) OL_TEXT_EDIT)},
	};


/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */


/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */


/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * 
 *  Look - 
 * 
 ****************************procedure*header*****************************
 */
unsigned char
Look(data, position, direction)
	StringSourcePtr data;
	OlTextPosition  position;
	OlScanDirection direction;
{
	/* Looking left at pos 0 or right at position data->length returns newline */
	if (direction == OlsdLeft) {
		if (position == 0)
			return(0);
		else
			return(data->buffer[position-1]);
	}
	else {
		if (position == data->length)
			return(0);
		else
			return(data->buffer[position]);
	}
}	/*  Look  */


/*
 *************************************************************************
 * 
 *  OlStringSrcEditType - 
 * 
 ****************************procedure*header*****************************
 */
static OlDefine
OlStringSrcEditType(src)
	OlTextSource *src;
{
	StringSourcePtr data;
	data = (StringSourcePtr) src->data;
	return(data->editMode);
}	/*  OlStringSrcEditType  */


/*
 *************************************************************************
 * 
 *  OlStringSrcGetLastPos - 
 * 
 ****************************procedure*header*****************************
 */
static OlTextPosition
OlStringSrcGetLastPos (src)
	OlTextSource *src;
{
	return( ((StringSourceData *) (src->data))->length );
}	/*  OlStringSrcGetLastPos  */


/*
 *************************************************************************
 * 
 *  OlStringSrcRead - 
 * 
 ****************************procedure*header*****************************
 */
static int
OlStringSrcRead (src, pos, text, maxRead)
	OlTextSource *src;
	int pos;
	OlTextBlock *text;
	int maxRead;
{
	int     charsLeft;
	StringSourcePtr data;

	data = (StringSourcePtr) src->data;
	text->firstPos = pos;
	text->ptr = data->buffer + pos;
	charsLeft = data->length - pos;
	text->length = (maxRead > charsLeft) ? charsLeft : maxRead;
	return pos + text->length;
}	/*  OlStringSrcRead  */


/*
 *************************************************************************
 * 
 *  OlStringSrcReplace - 
 * 
 ****************************procedure*header*****************************
 */
static int
OlStringSrcReplace (src, startPos, endPos, text, delta)
	OlTextSource *src;
	OlTextPosition startPos, endPos;
	OlTextBlock *text;
	int *delta;
{
	StringSourcePtr data;
	int     i, length;

	data = (StringSourcePtr) src->data;
	switch (data->editMode) {
/*
	case OL_TEXT_APPEND:
		if (startPos != endPos || endPos!= data->length)
			return (OleditPosError);
		break;
*/
	case OL_TEXT_READ:
		return (OleditError);
	case OL_TEXT_EDIT:
		break;
	default:
		return (OleditError);
	}
	length = endPos - startPos;
	*delta = text->length - length;

	if (data->max_size_flag && data->length + *delta > data->max_size)
		return (OleditError);

	/*
	** The "+ 1" is for the \0 written at the end of the buffer
	*/
	if ((data->length + *delta + 1) > data->buffer_size) {
		while ((data->length + *delta + 1) > data->buffer_size)
			data->buffer_size += DEFAULTBUFFERSIZE;
		data->buffer = (unsigned char *)
			    XtRealloc((char *)data->buffer, data->buffer_size);
	};

	/*
	 *  update the number_of_lines field by adding the number of
	 *  new lines minus the number of lines deleted.
	 */
	{
	int register new_lines = 0;
	unsigned char *ptr = (unsigned char *) text->ptr;
	int register deleted_lines = 0;

	for (i = 0; i < *delta; i++)  {
		if (ptr[i] == '\n')
			new_lines++;
		}
		
	ptr = (unsigned char *) data->buffer;
	for (i = startPos; i < endPos; i++)  {
		if (ptr[i] == '\n')
			deleted_lines++;
		}

	src->number_of_lines += new_lines - deleted_lines;
	}

	if (*delta < 0)		/* insert shorter than delete, text getting
						   shorter */
		for (i = startPos; i < data->length + *delta; ++i)
			data->buffer[i] = data->buffer[i - *delta];
	else
		if (*delta > 0)	{	/* insert longer than delete, text getting
						   longer */
			for (i = data->length; i > startPos-1; --i)
				data->buffer[i + *delta] = data->buffer[i];
		}
	if (text->length != 0)	/* do insert */
		for (i = 0; i < text->length; ++i)
			data->buffer[startPos + i] = text->ptr[i];
	data->length = data->length + *delta;
	data->buffer[data->length] = 0;
	return (OleditDone);
} 	/*  OlStringSrcReplace  */


/*
 *************************************************************************
 * 
 *  OlStringSrcScan - 
 * 
 ****************************procedure*header*****************************
 */
static OlTextPosition
OlStringSrcScan (src, pos, sType, dir, count, include)
	OlTextSource	  *src;
	OlTextPosition  pos;
	OlScanType	  sType;
	OlScanDirection dir;
	int		  count;
	Boolean	  include;
{
	StringSourcePtr data;
	OlTextPosition position;
	int     i, whiteSpace;
	unsigned char c;
	int ddir = (dir == OlsdRight) ? 1 : -1;

	data = (StringSourcePtr) src->data;
	position = pos;
	switch (sType) {
	case OlstPositions:
		if (!include && count > 0)
			count -= 1;
		for (i = 0; i < count; i++) {
			Increment(data, position, dir);
		}
		break;
	case OlstWhiteSpace:

		for (i = 0; i < count; i++) {
			whiteSpace = -1;
			while (position >= 0 && position <= data->length) {
				c = Look(data, position, dir);
				if ((c == ' ') || (c == '\t') || (c == '\n')){
					if (whiteSpace < 0) whiteSpace = position;
				} else if (whiteSpace >= 0)
					break;
				position += ddir;
			}
		}
		if (!include) {
			if(whiteSpace < 0 && dir == OlsdRight) whiteSpace = data->length;
			position = whiteSpace;
		}
		break;
	case OlstEOL:
		for (i = 0; i < count; i++) {
			while (position >= 0 && position <= data->length) {
				if (Look(data, position, dir) == '\n')
					break;
				if(((dir == OlsdRight) && (position == data->length)) || 
				    (dir == OlsdLeft) && ((position == 0)))
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
	case OlstLast:
		if (dir == OlsdLeft)
			position = 0;
		else
			position = data->length;
	}
	if (position < 0) position = 0;
	if (position > data->length) position = data->length;
	return(position);
}	/*  OlStringSrcScan  */


/*
 *************************************************************************
 * 
 *  OlStringSrcSetLastPos - 
 * 
 ****************************procedure*header*****************************
 */
static int
OlStringSrcSetLastPos (src, lastPos)
	OlTextSource *src;
	OlTextPosition lastPos;
{
	((StringSourceData *) (src->data))->length = lastPos;
}	/*  OlStringSrcSetLastPos  */


/*
 *************************************************************************
 * 
 *  OlStringSourceCheckData - 
 * 
 ****************************procedure*header*****************************
 */
static Boolean
OlStringSourceCheckData(src)
	OlTextSource *src;
{   
	int  initial_size = NULL;
	StringSourcePtr data = (StringSourcePtr)src->data;

	data->max_size_flag = (data->max_size != MAGICVALUE);

	if (data->initial_string == NULL) {
		if (data->max_size_flag)
			data->buffer_size = data->max_size;
		else 
			data->buffer_size = DEFAULTBUFFERSIZE;
		if (!data->buffer) data->length = 0;
	}
	else {
		initial_size = _OlStrlen(data->initial_string);
		if (data->max_size_flag) {
			if (data->max_size < initial_size) {
			OlVaDisplayWarningMsg(	(Display *)NULL,
						OleNfileSourceStr,
						OleTmsg1,
						OleCOlToolkitWarning,
						OleMfileSourceStr_msg1);
				data->max_size = initial_size;
			}
			data->buffer_size = data->max_size;
		}
		else {
			data->buffer_size =
			    (initial_size < DEFAULTBUFFERSIZE) ? DEFAULTBUFFERSIZE :
			    ((initial_size / DEFAULTBUFFERSIZE) + 1) * DEFAULTBUFFERSIZE;
		};
		data->length = initial_size;
	};

	if (data->buffer && initial_size)
		data->buffer =
		    (unsigned char *) XtRealloc((char *)data->buffer, data->buffer_size);
	else if (!data->buffer)
		data->buffer = (unsigned char *) XtMalloc(data->buffer_size);

	if (initial_size) {
		(void)strcpy((char *)data->buffer,
			     (OLconst char *)data->initial_string);
		data->initial_string = NULL;
	}

	if (data->editMode != OL_TEXT_EDIT && data->editMode != OL_TEXT_READ) {
		data->editMode = OL_TEXT_READ;
			OlVaDisplayWarningMsg(	(Display *)NULL,
						OleNfileSourceStr,
						OleTmsg2,
						OleCOlToolkitWarning,
						OleMfileSourceStr_msg2);
		}
}	/*  OlStringSourceCheckData  */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */


/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */


/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */


/*
 *************************************************************************
 * 
 *  OlStringSourceCreate - 
 * 
 ****************************procedure*header*****************************
 */
OlTextSource *
OlStringSourceCreate (w, args, argCount)
	Widget w;
	ArgList args;
	int     argCount;
{
	OlTextSource *src;
	StringSourcePtr data;


	src = XtNew(OlTextSource);
	src->read = OlStringSrcRead;
	src->replace = OlStringSrcReplace;
	src->setLastPos = OlStringSrcSetLastPos;
	src->getLastPos = OlStringSrcGetLastPos;
	src->scan = OlStringSrcScan;
	src->editType = OlStringSrcEditType;
	src->resources = stringResources;
	src->resource_num = XtNumber(stringResources);
	src->check_data = OlStringSourceCheckData;
	src->destroy = OlStringSourceDestroy;
	src->number_of_lines = 0;
	data = XtNew(StringSourceData);
	data->editMode = OL_TEXT_READ;
	data->buffer = NULL;
	data->initial_string = NULL;
	data->length = 0;
	data->buffer_size = 0;
	data->max_size = 0;
	data->max_size_flag = 0;
	src->data = (int *)data;

	/* Use the name given to the Text widget this source will go with.
       This could be a problem if we allow multiple views on one source */

	XtGetSubresources (w, data, XtNstringSrc, "StringSrc",
	    stringResources, XtNumber(stringResources), args, argCount);

	src->data = (int *) (data);

	OlStringSourceCheckData(src);

	/*
	 *  Calculate how many lines are in this string for the scrollbar.
	 */
	{
	register int length = data->length;
	register int number_of_lines = 0;
	register unsigned char *buffer = (unsigned char *) data->buffer;
	int i;

	for (i=0; i < length; i++)  {
		if (buffer[i] == '\n')
			number_of_lines++;
		}
	src->number_of_lines = number_of_lines;
	}

	return src;
}	/*  OlStringSourceCreate  */


/*
 *************************************************************************
 * 
 *  OlStringSourceDestroy - 
 * 
 ****************************procedure*header*****************************
 */
void
OlStringSourceDestroy (src)
	OlTextSource *src;
{
 	XtFree((char *) ((StringSourceData *)src->data)->buffer);
	XtFree((char *) src->data);
	XtFree((char *) src);
}	/*  OlStringSourceDestroy  */
