/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)oldtext:TextPos.c	1.11"
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
 * Date:	February 21, 1989
 *
 * Description:  This file contains the _OlPt* code for handling the
 * position table in the TextPaneWidget.
 *
 *******************************file*header*******************************
 */

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include <Xol/PrimitiveP.h>
#include <Xol/TextPaneP.h>

#include <Xol/TextPosP.h>


/*
 *************************************************************************
 * _OlPtBuild - This routine builds a position table for 'self' starting
 * at 'pos'.
 ****************************procedure*header*****************************
 */
void
_OlPtBuild(self, pos)
  TextPaneWidget		self;
  register OlTextPosition	pos;
{
  OlTextPosition		drawPos;
  OlTextPosition		fitPos;
  int				fromx;
  unsigned char			*lastChar;
  register OlTextPosition	lastPos;
  register unsigned		line;
  OlTextPosition		nextPos;
  OlPositionTable		*pt;
  int				resHeight;
  int				resWidth;
  TextFit			(*textFitFn)();
  int				width;
  int				wrap;
  Boolean			wrapWhiteSpace;



  fromx = self->text.leftmargin;
  lastPos = (*(self->text.source->getLastPos))(self->text.source);
  line = _OlPtLineFromPos(self, pos);
  pt = &(self->text.pt);
  textFitFn = self->text.sink->textFitFn;
  width = self->core.width - self->text.leftmargin - self->text.rightmargin;
  wrap = self->text.wrap_mode;

  wrapWhiteSpace = wrap && self->text.wrap_break == OL_WRAP_WHITE_SPACE;

  if (pos == (OlTextPosition)0)
    _OlPtSetLine(pt, (unsigned)0, (OlTextPosition)0);

  while (pos < lastPos)
    {
      (*textFitFn)(self, pos, fromx, width, wrap, wrapWhiteSpace,
		   &fitPos, &drawPos, &nextPos, &resWidth, &resHeight);
      pos = nextPos;
      ++line;
      _OlPtSetLine(pt, line, pos);
    }
 
  lastChar = _OlTextCopySubString(self, lastPos - 1, lastPos);
  if (lastChar[0] != '\n')
    --line;
  XtFree((char *)lastChar);
  
  pt->lines = line + 1;
  if (self->text.verticalSB)
    _OlUpdateVerticalSB(self);

}	/* _OlPtBuild() */


/*
 *************************************************************************
 * _OlPtDestroy - This routine frees the position array.
 *************************************************************************
 */
void
_OlPtDestroy(pt)
  OlPositionTable	*pt;
{

  pt->lines = 0;
  XtFree((char *)pt->pos);
  pt->pos = (OlTextPosition *)NULL;
  pt->size = 0;

}	/* _OlPtDestroy() */


/*
 *************************************************************************
 * _OlPtInitialize - This routine initializes the position table pointed
 * to by 'pt'.
 ****************************procedure*header*****************************
 */
void
_OlPtInitialize(pt)
  OlPositionTable	*pt;
{

  pt->lines = 0;
  pt->pos = (OlTextPosition *)XtRealloc(NULL,
					_OL_POS_TAB_INIT_SIZE *
					sizeof(OlTextPosition));
  pt->size = _OL_POS_TAB_INIT_SIZE;

}	/* _OlPtInitialize() */


/*
 *************************************************************************
 * _OlPtLineFromPos - This routine returns a (wrapped) line number given
 * a TextPaneWidget ('self') and a position 'pos'.
 ****************************procedure*header*****************************
 */

unsigned
_OlPtLineFromPos(self, pos)
	TextPaneWidget			self;
	register OlTextPosition		pos;
{
	register OlTextPosition	*	pmin = self->text.pt.pos;
	register OlTextPosition	*	pmax = pmin + self->text.pt.lines - 1;
	register OlTextPosition	*	save = pmin;
	register OlTextPosition	*	p;

	if (pmax <= pmin || pos < 0 || pos > *pmax) {
		return 0;
	}
	for (;;) {
		p = pmin + (pmax - pmin)/2;

		if (pos < *p) {
			pmax = p;
		} else if (pos >= *(p+1)) {
			pmin = p+1;
		} else {
			return p - save;
		}
	}
}

/*
 *************************************************************************
 * _OlPtPosFromLine - This routine returns the first position on a
 * (wrapped) line ('line') for the givet TextPaneWidget ('self').
 ****************************procedure*header*****************************
 */
OlTextPosition
_OlPtPosFromLine(self, line)
  TextPaneWidget	self;
  unsigned		line;
{

  return self->text.pt.pos[line];

}	/* _OlPtPosFromLine() */


/*
 *************************************************************************
 * _OlPtSetLine - This routine sets the entry in 'pt' indexed by 'line'
 * to 'pos', doing realloc()'s if necessary.
 ****************************procedure*header*****************************
 */
void
_OlPtSetLine(pt, line, pos)
  OlPositionTable	*pt;
  unsigned		line;
  OlTextPosition	pos;
{

  while (line >= pt->size)
    {
      pt->size += (unsigned)_OL_POS_TAB_REALLOC_CHUNK;
      pt->pos = (OlTextPosition *)XtRealloc((char *)pt->pos,
					    (Cardinal)pt->size *
					    sizeof(OlTextPosition));
    }

  pt->pos[line] = pos;

}	/* _OlPtSetLine() */
