/* ident	"@(#)libDtI:sizeicon.c	1.4" */

#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <OpenLook.h>
#include <DtI.h>

void
DmSizeIcon(DmItemPtr ip, OlFontList * font_list, XFontStruct * font)
{
    int		char_ht = OlFontHeight(font, font_list);
    int		len;
    Dimension	width;

    len = strlen(ITEM_LABEL(ip));
    width = DM_TextWidth(font, font_list, ITEM_LABEL(ip), len);

    ip->icon_width = (XtArgVal)((GLYPH_PTR(ip) ?
      DM_Max(GLYPH_PTR(ip)->width, width) : width) +
                              2 * ICON_PADDING);

    /*
     * The factor 6 is 2 for the frame around the label, 2 for the frame around
     * the glyph, and 2 for the shortcut glyph between the glyph and the label.
     * The half factor is the spacing below the shortcut glyph.
     */
    ip->icon_height = (XtArgVal)((GLYPH_PTR(ip) ?
                              GLYPH_PTR(ip)->height : 0) + char_ht +
                               6 * ICON_PADDING + ICON_PADDING / 2);

}				/* end of DmSizeIcon() */
