#ifndef	NOIDENT
#ident	"@(#)oldtext:DisplayP.h	1.8"
#endif

/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        DisplayP.h
 **
 **   Project:     X Widgets
 **
 **   Description: Private include file for TextPane widget ascii sink
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


#ifndef _OlDisplayP_h
#define _OlDisplayP_h

#include <Xol/TextPane.h>

/*****************************************************************************
*
* Constants
*
*****************************************************************************/

#define INFINITE_WIDTH 32767

#define applyDisplay(method) (*(self->text.sink->method))

/*****************************************************************************
*
* Displayable text management data structures (LineTable)
*
*****************************************************************************/

#define RequiredCursorMargin 3

typedef struct {
    OlTextPosition	position, drawPos;
    Position		x, y, endX;
    TextFit		fit ;
    } OlLineTableEntry, *OlLineTableEntryPtr;

/* Line Tables are n+1 long - last position displayed is in last lt entry */
typedef struct {
    OlTextPosition  top;	/* Top of the displayed text.		*/
    OlTextPosition  lines;	/* How many lines in this table.	*/
    OlLineTableEntry  *info;	/* A dynamic array, one entry per line  */
    } OlLineTable, *OlLineTablePtr;

typedef enum {OlisOn, OlisOff} OlInsertState;

#ifndef USE_EXT_VARS
typedef enum {OlselectNull, OlselectPosition, OlselectChar, OlselectWord,
    OlselectLine, OlselectParagraph, OlselectAll} OlSelectType;
#endif /* USE_EXT_VARS */

typedef enum {OlsmTextSelect, OlsmTextExtend} OlSelectionMode;

typedef enum {OlactionStart, OlactionAdjust, OlactionEnd} OlSelectionAction;

typedef struct {
    OlTextPosition left, right;
    OlSelectType  type;
} OlTextSelection;

#define IsPositionVisible(ctx, pos)\
  (pos >= ctx->text.lt.info[0].position && \
   pos <  ctx->text.lt.info[ctx->text.lt.lines].position)

#endif
/* DON'T ADD STUFF AFTER THIS #endif */


