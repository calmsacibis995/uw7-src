#ifndef NOIDENT
#ident	"@(#)textedit:TextEPos.h	1.2"
#endif

/*
 * TextPos.h
 *
 */

#ifndef _TextPos_h
#define _TextPos_h

extern void       _SetDisplayLocation();
extern void       _SetTextXOffset();
extern void       _CalculateCursorRowAndXOffset();
extern int        _MoveDisplayPosition();
extern void       _MoveDisplay();
extern void       _MoveDisplayLaterally();
extern void       _MoveCursorPosition();
extern void       _MoveCursorPositionGlyph();
extern Boolean    _MoveSelection();
extern int        _TextEditOwnPrimary();

extern int        _PositionFromXY();
extern XRectangle _RectFromPositions();

#endif
