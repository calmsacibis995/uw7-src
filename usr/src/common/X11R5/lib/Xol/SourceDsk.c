/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)oldtext:SourceDsk.c	1.15"
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
 * Description: This file contains the source code for the disk source
 *	used in the TextPane widget.
 *		
 *
 *******************************file*header*******************************
 */

/*************************************<+>*************************************
 *****************************************************************************
 **
 **   Project:     X Widgets
 **
 **   Description: Code for TextPane widget disk source
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

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/TextPaneP.h>


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
static int DummyReplaceText();
static int FillBuffer();
static unsigned char Look();
static OlDefine OlDiskSrcEditType();
static OlTextPosition OlDiskSrcGetLastPos();
int OlDiskSrcRead();
static int OlDiskSrcReplace();
static OlTextPosition OlDiskSrcScan();
static int OlDiskSrcSetLastPos();
static Boolean OlDiskSourceCheckData();

					/* class procedures		*/

					/* action procedures		*/

					/* public procedures		*/
OlTextSource * OlDiskSourceCreate();
void OlDiskSourceDestroy();

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */


extern char *tmpnam();

#define bcopy(src, dst, len)	OlMemMove(char, dst, src, len)

/** private DiskSource definitions **/

typedef struct _DiskSourceData {
	unsigned char       *fileName;
	FILE *file;
	OlTextPosition position, 	/* file position of first char in buffer */
	length; 	/* length of file */
	unsigned char *buffer;	/* piece of file in memory */
	int charsInBuffer;		/* number of bytes used in memory */
	OlDefine editMode;	/* append, read */
} DiskSourceData, *DiskSourcePtr;

#define bufSize 1000

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

static XtResource diskResources[] = {
	{XtNfile,
		XtCFile,
		XtRString,
		sizeof (char *),
		XtOffset(DiskSourcePtr, fileName),
		XtRString,
		""},
	{XtNeditType,
		XtCEditType,
		XtROlDefine,
		sizeof(OlDefine), 
		XtOffset(DiskSourcePtr, editMode),
		XtRImmediate,
		(XtPointer) ((OlDefine) OL_TEXT_READ)},
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
 *  DummyReplaceText - This is a dummy routine for read only disk sources.
 * 
 ****************************procedure*header*****************************
 */
static int
DummyReplaceText (src, startPos, endPos, text, delta)
	OlTextSource *src;
	OlTextPosition startPos, endPos;
	OlTextBlock *text;
	int *delta;
{
	return(OleditError);
}	/*  DummyReplace Text  */


/*
 *************************************************************************
 * 
 *  FillBuffer - This routine reads text starting at "pos" into memory.
 *	Contains heuristic for keeping the read position centered in
 *	the buffer.
 * 
 ****************************procedure*header*****************************
 */
static int
FillBuffer (data, pos)
	DiskSourcePtr data;
	OlTextPosition pos;
{
	long readPos;
	if ((pos < data->position ||
	    pos >= data->position + data->charsInBuffer - 100) &&
	    data->charsInBuffer != data->length) {
		if (pos < (bufSize / 2))
			readPos = 0;
		else
			if (pos >= data->length - bufSize)
				readPos = data->length - bufSize;
			else
				if (pos >= data->position + data->charsInBuffer - 100)
					readPos = pos - (bufSize / 2);
				else
					readPos = pos;
		(void) fseek(data->file, readPos, 0);
		data->charsInBuffer = fread(data->buffer, sizeof(char), bufSize,
		    data->file);
		data->position = readPos;
	}
}	/*  FillBuffer  */


/*
 *************************************************************************
 * 
 *  Look -
 * 
 ****************************procedure*header*****************************
 */
static unsigned char
Look(data, position, direction)
	DiskSourcePtr   data;
	OlTextPosition  position;
	OlScanDirection direction;
{

	if (direction == OlsdLeft) {
		if (position == 0)
			return('\n');
		else {
			FillBuffer(data, position - 1);
			return(data->buffer[position - data->position - 1]);
		}
	}
	else {
		if (position == data->length)
			return('\n');
		else {
			FillBuffer(data, position);
			return(data->buffer[position - data->position]);
		}
	}
}	/*  Look  */


/*
 *************************************************************************
 * 
 *  OlDiskSrcEditType - 
 * 
 ****************************procedure*header*****************************
 */
static OlDefine
OlDiskSrcEditType(src)
	OlTextSource *src;
{
	DiskSourcePtr data;
	data = (DiskSourcePtr) src->data;
	return(data->editMode);
}	/*  OlDiskSrcEditType  */


/*
 *************************************************************************
 * 
 *  OlDiskSrcGetLastPos - 
 * 
 ****************************procedure*header*****************************
 */
static OlTextPosition
OlDiskSrcGetLastPos (src)
	OlTextSource *src;
{
	return( ((DiskSourceData *)(src->data))->length );
}	/*  OlTextPosition  */


/*
 *************************************************************************
 * 
 *  OlDiskSrcRead - 
 * 
 ****************************procedure*header*****************************
 */
int
OlDiskSrcRead (src, pos, text, maxRead)
	OlTextSource *src;
	OlTextPosition pos;	/** starting position */
	OlTextBlock *text;	/** RETURNED: text read in */
	int maxRead;		/** max number of bytes to read **/
{
	OlTextPosition count;
	DiskSourcePtr data;

	data = (DiskSourcePtr) src->data;
	FillBuffer(data, pos);
	text->firstPos = pos;
	text->ptr = data->buffer + (pos - data->position);
	count = data->charsInBuffer - (pos - data->position);
	text->length = (maxRead > count) ? count : maxRead;
	return pos + text->length;
}	/*  OlDiskSrcRead  */


/*
 *************************************************************************
 * 
 *  OlDiskSrcReplace - This routine will only append to the end of a
 *	source.  If incorrect starting and ending positions are given,
 *	an error will be returned.
 * 
 ****************************procedure*header*****************************
 */
static int
OlDiskSrcReplace (src, startPos, endPos, text, delta)
	OlTextSource *src;
	OlTextPosition startPos, endPos;
	OlTextBlock *text;
	int *delta;
{
	long topPosition = 0;
	unsigned char *tmpPtr;
	DiskSourcePtr data;
	data = (DiskSourcePtr) src->data;
	if (startPos != endPos || endPos != data->length)
		return (OleditPosError);
	/* write the new text to the end of the file */
	if (text->length > 0) {
		(void) fseek(data->file, data->length, 0);
		(void) fwrite(text->ptr, sizeof(char), text->length, data->file);
	} else
		/* if the delete key was hit, blank out last char in the file */
		if (text->length < 0) {
			(void) fseek(data->file, data->length-1, 0);
			(void) fwrite(" ", sizeof(char), 1, data->file);
		}
	/* need this in case the application trys to seek to end of file. */
	(void) fseek(data->file, topPosition, 2);

	/* put the new text into the buffer in memory */
	data->length += text->length;
	if (data->charsInBuffer + text->length <= bufSize) {
		/**** NOTE: need to check if text won't fit in the buffer ***/
		if (text->length > 0) {
			tmpPtr = data->buffer + data->charsInBuffer;
			bcopy(text->ptr, tmpPtr, text->length);
		}
		data->charsInBuffer += text->length;
	} else
		FillBuffer(data, data->length - text->length);

	*delta = text->length;
	return (OleditDone);
}	/*  OlDiskSrcReplace  */


/*
 *************************************************************************
 * 
 *  OlDiskSrcScan - This routine will start at the "pos" position of the
 *	source and scan in the appropriate direction until it finds
 *	something of the right sType.  It returns the new position.
 *	If upon reading it hits the end of the buffer.
 * 
 ****************************procedure*header*****************************
 */
static OlTextPosition
OlDiskSrcScan (src, pos, sType, dir, count, include)
	OlTextSource 	   *src;
	OlTextPosition   pos;
	OlScanType 	   sType;
	OlScanDirection  dir;
	int     	   count;
	Boolean	   include;
{
	DiskSourcePtr data;
	OlTextPosition position;
	int     i, whiteSpace;
	unsigned char    c;

	data = (DiskSourcePtr) src->data;
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
			whiteSpace = 0;
			while (position >= 0 && position <= data->length) {
				FillBuffer(data, position);
				c = Look(data, position, dir);
				whiteSpace = (c == ' ') || (c == '\t') || (c == '\n');
				if (whiteSpace)
					break;
				Increment(data, position, dir);
			}
			if (i + 1 != count)
				Increment(data, position, dir);
		}
		if (include)
			Increment(data, position, dir);
		break;
	case OlstEOL:
		for (i = 0; i < count; i++) {
			while (position >= 0 && position <= data->length) {
				if (Look(data, position, dir) == '\n')
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
	return(position);
}	/*  OlDiskSrcScan  */


/*
 *************************************************************************
 * 
 *  OlDiskSrcSetLastPos - 
 * 
 ****************************procedure*header*****************************
 */
static int
OlDiskSrcSetLastPos (src, lastPos)
	OlTextSource *src;
	OlTextPosition lastPos;
{
	((DiskSourceData *)(src->data))->length = lastPos;
}	/*  OlDiskSrcSetLastPos  */


/*
 *************************************************************************
 * 
 *  OlDiskSourceCheckData - 
 * 
 ****************************procedure*header*****************************
 */
static Boolean
OlDiskSourceCheckData(self)
	Widget self;
{

}	/*  OlDiskSrcCheckData  */


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
 *  OlDiskSourceCreate - 
 * 
 ****************************procedure*header*****************************
 */
OlTextSource *
OlDiskSourceCreate(w, args, num_args)
	Widget	w;
	ArgList	args;
	Cardinal	num_args;
{
	OlTextSource *src;
	DiskSourcePtr data;
	long topPosition = 0;

	src = XtNew(OlTextSource);
	src->read = OlDiskSrcRead;
	src->setLastPos = OlDiskSrcSetLastPos;
	src->getLastPos = OlDiskSrcGetLastPos;
	src->scan = OlDiskSrcScan;
	src->editType = OlDiskSrcEditType;
	src->resources = diskResources;
	src->resource_num = XtNumber(diskResources);
	src->check_data = OlDiskSourceCheckData;
	src->destroy = OlDiskSourceDestroy;
	data = XtNew(DiskSourceData);
	src->data = (int *)data;

	/* Use the name given to the Text widget this source will go with.
       This could be a problem if we allow multiple views on one source */

	XtGetSubresources (w, (XtPointer)data, XtNdiskSrc , "DiskSrc",
	    diskResources, XtNumber(diskResources), args, num_args);

	/* NOTE:  Do not want to leave temp file around.  Must be unlinked
    somewhere */

	if (data->fileName == NULL || *(data->fileName) == '\0')
		data->fileName = (unsigned char*)tempnam(NULL,"OlDSRC");

	switch (data->editMode) {
	case OL_TEXT_READ:
		if ((data->file = fopen((OLconst char *)data->fileName, "r"))
			== 0)  {
			OlVaDisplayWarningMsg(	XtDisplay(w),
						OleNfileSourceDsk,
						OleTmsg1,
						OleCOlToolkitWarning,
						OleMfileSourceDsk_msg1,
						data->fileName);
			}
		src->replace = DummyReplaceText;
		break;
/*
	case OL_TEXT_APPEND:
		if ((data->file = fopen(data->fileName, "a+")) == 0)  {
			char error_msg[200];
			sprintf(error_msg,
				"OlDiskSourceCreate cannot open source file: %s",
				data->fileName);
			OlError(error_msg);
			}
		src->replace = OlDiskSrcReplace;
		break;
*/
	default:
			OlVaDisplayWarningMsg(	XtDisplay(w),
						OleNfileSourceDsk,
						OleTmsg2,
						OleCOlToolkitWarning,
						OleMfileSourceDsk_msg2);
		if ((data->file = fopen((OLconst char *)data->fileName, "r"))
			== 0)  {
			OlVaDisplayErrorMsg(	XtDisplay(w),
						OleNfileSourceDsk,
						OleTmsg1,
						OleCOlToolkitError,
						OleMfileSourceDsk_msg1,
						data->fileName);
			}
		src->replace = DummyReplaceText;
	}

	/*
	 *  Determine how many lines are in this file so that the
	 *  scrollbar can be set correctly.
	 */
	if (data->file != (FILE *)NULL) {
    		register unsigned char *p1, *p2;
		register unsigned int c;
		unsigned char	b[BUFSIZ];
		long	linect;
		FILE *fptr = data->file;

		p1 = p2 = b;
		linect = 0;
		for(;;) {
			if(p1 >= p2) {
                                p1 = b;
                                c = fread(p1, 1, BUFSIZ, fptr);
                                if(c <= 0)
                                        break;
                                p2 = p1+c;
			}
                        c = *p1++;
                        if(c=='\n')
                                linect++;
		}

		src->number_of_lines = linect;
		(void) fseek(data->file, topPosition, 2);
		data->length = ftell (data->file);
	}
	else {			/* File couldn't be opened */
		src->number_of_lines = 0;
		data->length = 0;
	}

	data->buffer = (unsigned char *) XtMalloc((unsigned)bufSize);
	data->position = 0;
	data->charsInBuffer = 0;
	src->data = (int *) (data);

	return src;

}	/*  OlDiskSourceCreate  */


/*
 *************************************************************************
 * 
 *  OlDiskSourceDestroy - 
 * 
 ****************************procedure*header*****************************
 */
void
OlDiskSourceDestroy (src)
	OlTextSource *src;
{
	DiskSourcePtr data;
	data = (DiskSourcePtr) src->data;
	fclose(data->file);
	XtFree((char *) data->buffer);
	XtFree((char *) src->data);
	XtFree((char *) src);
}	/*  OlDiskSourceDestroy  */
