#ident	"@(#)dtadmin:fontmgr/fontxlfd.c	1.8"

#include <stdio.h>
#include <Intrinsic.h>
#include <StringDefs.h>
#include <fontmgr.h>
#include <fontxlfd.h>

extern view_type *view;
extern Widget app_shellW;
static int medium_cnt, reg_italic_cnt, bold_cnt, bold_italic_cnt = 0;
void SetFontInFamily();
void FreeFontsInFamily();
int FontIsPartOfFamily();
int GetFontWeightSlant();


static weight_type weight_table[] = {
{ "regular", 1 },
{ "medium", 1 },
{ "normal", 1 },
{ "demi", 2 },
{ "roman", 1 },
{ "book", 1},
{ "light",  1 },
{ "bold" , 2 },
{ "heavy", 2 },
{ "demibold", 2 },
{ "semibold", 2 },
{ "extrabold", 2 },
{ "black", 2 },
{ "display", 2 },
{ "poster", 2 },
{ "ultra bold", 2 }
};

GetSlant(slant)
String slant;
{
	if (strcmp(slant, "r") == STR_MATCH) return 1;
	else
	if (strcmp(slant, "i") == STR_MATCH) return 2;
	else
	if (strcmp(slant, "o") == STR_MATCH) return 2;
	else
	return 0;
}

static int
LookUpWeight( weight)
    String weight;
{
    int i;

    for (i=0; i<XtNumber(weight_table); i++) {
	if (strstr( weight, weight_table[i].code)) {
		return weight_table[i].value;
	}
	}
    return 0;
} /* end of LookUpWeight */





Bool
SetupWeightSlantPattern(fname, pattern, r_weight, r_slant)
    char       *fname;
    char 	*pattern;
    char 	*r_weight;
    char	*r_slant;
{
    register char *ptr;
    register char *weight, *slant, *family, *rest_xlfd;
    char tmpbuf[256];
    int len;

#ifdef DEBUG
fprintf(stderr,"fname=%s\n", fname);
#endif
    if (!(*fname == '-') ||     /* foundry */
            !(family=ptr=(char *)strchr(fname + 1, '-')) ||   /* family_name */
            !(weight=ptr=(char *)strchr(ptr + 1, '-')) ||     /* weight_name */
            !(slant=ptr=(char *)strchr(ptr + 1, '-')) ||     /* slant */
            !(rest_xlfd=ptr=(char *)strchr(ptr + 1, '-')) ||     /* setwidth_name */
            !(ptr=(char *)strchr(ptr + 1, '-')) ||     /* add_style_name */
            !(ptr =(char *)strchr(ptr + 1, '-')) ||      /* pixel_size */
            !(ptr =(char *)strchr(ptr + 1, '-')) ||      /* point */
            !(ptr = (char *)strchr(ptr + 1, '-')) ||      /* res x */
            !(ptr = (char *)strchr(ptr + 1, '-')) ||      /* res y */
            !(ptr = (char *)strchr(ptr + 1, '-')) ||      /* spacing */
            !(ptr = (char *)strchr(ptr + 1, '-')) ||      /* average width */
            !(ptr = (char *)strchr(ptr + 1, '-')) ||     /* charset_registry */
            !(char *)strchr(ptr + 1, '-'))/* charset_encoding */
        return FALSE;


#ifdef DEBUG
fprintf(stderr, "family=%s\n",family);
fprintf(stderr," weight=%s \n",weight);
fprintf(stderr,"slant=%s\n", slant);
fprintf(stderr,"rest_xlfd=%s\n", rest_xlfd);
#endif

	len = weight - fname;
	strncpy(tmpbuf, fname, len);
	strcpy (tmpbuf+len, "-*-*");
 	strcpy(tmpbuf+len+4, rest_xlfd);
#ifdef DEBUG
	fprintf(stderr,"tmpbuf=%s\n", tmpbuf);	
#endif
	strcpy(pattern, tmpbuf);
#ifdef DEBUG
	fprintf(stderr,"pattern=%s\n", pattern);
#endif
	len =  slant - weight-1;

#ifdef DEBUG
fprintf(stderr,"len=%d\n", len);
#endif
	strncpy(r_weight, weight+1, len);
#ifdef DEBUG
	fprintf(stderr,"r_weight=%s\n",r_weight);
#endif
	r_weight[len] = 0;
	len = rest_xlfd - slant-1;
#ifdef DEBUG
	fprintf(stderr,"len=%d\n", len);
#endif
	strncpy(r_slant, slant+1, len);
	r_slant[len] = 0;
#ifdef DEBUG
	fprintf(stderr,"r_slant=%s\n",r_slant);
#endif
    return TRUE;
}


int
FontIsPartOfFamily ( display, xlfd_name,  otherFontsInFamily)
   Display *display;
    String xlfd_name;
    motif_font_resources *otherFontsInFamily;
{
  int numOtherFonts = 0;
  char ** font_list;
  int font_list_count;
  char pattern[256];
  char weight[25];
  char slant[10];
  Boolean res;
  int result, indx;
  int max_names =  32767;
  int i;

#ifdef DEBUG
fprintf(stderr,"xlfd_name=%s\n",xlfd_name);
fprintf(stderr,"FontIsPartOfFamily weight=%s slant=%s\n",weight,slant);
#endif
		/* need the pattern here to use for finding the rest
			of the family */
		/* put the current font that they are applying in the list first 
		so that fonts that sort before it do not take precedence */


  res = SetupWeightSlantPattern(xlfd_name, pattern, weight, slant);
  if (!res) return 0; /* if no match then assum plain font */
  font_list =  XListFonts(display, pattern, max_names,
			&font_list_count);
#ifdef DEBUG
fprintf(stderr,"font_list_count=%d\n", font_list_count);
#endif
#ifdef DEBUG
  for (i = 0; i < font_list_count; i++)  
    fprintf(stderr,"font_list[%d]=%s\n",i,font_list[i]);
#endif

  /* for each XLFD */
  for (i = 0; i < font_list_count; i++)  { 
    if (!_IsXLFDFontName(font_list[i])) {
      continue;
    }

	/* get the type of font */
	SetFontInFamily(font_list[i], otherFontsInFamily, False);

	}


#ifdef DEBUG
	fprintf(stderr,"medium=%s\n",otherFontsInFamily->plain_font);
	fprintf(stderr,"medium_italic=%s\n",otherFontsInFamily->italic_font);
	fprintf(stderr,"bold=%s\n",otherFontsInFamily->bold_font);
	fprintf(stderr,"bold_italic=%s\n",otherFontsInFamily->italic_bold_font);
	fprintf(stderr,"numOtherFonts=%d\n", numOtherFonts);
#endif
	numOtherFonts = bold_cnt + bold_italic_cnt + medium_cnt + reg_italic_cnt;
  	XFreeFontNames(font_list);

	if (numOtherFonts == 4) {
		otherFontsInFamily->complete_family = True;
	} else {
		otherFontsInFamily->complete_family = False;
	}
	return numOtherFonts;
}  /* end of GetFamilyFonts() */


int
GetFontWeightSlant(char * xlfd_name)
{
   int indx=0;		/* set indx to 0 if nothing matchs */
   int result;
   char slant[25]; 
   char weight[25];
   char pattern[256];
   Boolean res;
   int i;
		/* get the weight & slant for the current xlfd_name */
		/* pattern not needed in this routine */
   res = SetupWeightSlantPattern(xlfd_name, pattern, weight, slant);
   result = LookUpWeight(weight);
   switch (result) {
	default:
	case 1:
		/* lighter weight */

		result = GetSlant(slant);
		switch (result) {
		case 1:
			indx = 0;
			break;
		case 2:
			indx = 1;
			break;
		default:
			indx = 0;		/* set indx to 0 if nothing matchs */;
			break;
		}
		break;

	case 2:

		/* bold weight */

		result = GetSlant(slant);
		switch (result) {
		default:
		case 1:
			indx = 2;
			break;
		case 2:
			indx = 3;
			break;
		}

		break;

	}
   return indx;
}


void
FreeFontsInFamily(others)
motif_font_resources *others;
{

#ifdef DEBUG
fprintf(stderr,"freeing others\n");
#endif
	if (others->plain_font) XtFree(others->plain_font);
	if (others->italic_font) XtFree(others->italic_font);
	if (others->bold_font) XtFree(others->bold_font);
	if (others->italic_bold_font) XtFree(others->italic_bold_font);
	others->plain_font = 0;
	others->italic_font = 0;
	others->bold_font = 0;
	others->italic_bold_font=0;
}


void 
SetFontInFamily ( xlfd_name,  otherFontsInFamily, first_font)
    String xlfd_name;
    motif_font_resources *otherFontsInFamily;
	Boolean first_font;
{
  int result, indx;
  int i;


#ifdef DEBUG
fprintf(stderr,"xlfd_name=%s\n",xlfd_name);
#endif
	if (first_font) {
		medium_cnt = 0;
		reg_italic_cnt = 0;
		bold_cnt = 0;
		bold_italic_cnt = 0;
	}

	/* get the type of font */

   indx = GetFontWeightSlant(xlfd_name);
   switch (indx) {
	default:
	case 0:
		if (medium_cnt == 1) break;
		otherFontsInFamily->plain_font=XtNewString(xlfd_name);
		break;
	case 1:
		if (reg_italic_cnt == 1) break;
		reg_italic_cnt = 1;
		otherFontsInFamily->italic_font=XtNewString(xlfd_name);
		break;
	case 2:
		if (bold_cnt == 1) break;
		bold_cnt = 1;
		otherFontsInFamily->bold_font = XtNewString(xlfd_name);
		break;
	case 3:
		if (bold_italic_cnt == 1) break;
		bold_italic_cnt = 1;
		otherFontsInFamily->italic_bold_font = XtNewString(xlfd_name);
		break;
	}


#ifdef DEBUG
	fprintf(stderr,"medium=%s\n",otherFontsInFamily->plain_font);
	fprintf(stderr,"medium_italic=%s\n",otherFontsInFamily->italic_font);
	fprintf(stderr,"bold=%s\n",otherFontsInFamily->bold_font);
	fprintf(stderr,"bold_italic=%s\n",otherFontsInFamily->italic_bold_font);
#endif
}  /* end of SetFontInFamily() */



Boolean
GetBitmapFontName (xlfd_name,  apply_xlfd_name)
    String xlfd_name;
    char *apply_xlfd_name;
{
  char ** font_list;
  int font_list_count;
  char pattern[256];
  int resx, resy, pixel, width;
  int save_resx, save_resy, computex, computey;
    Display *display;
  Boolean result;
  int max_names =  32767;
  int i;

#ifdef DEBUG
fprintf(stderr,"xlfd_name=%s\n",xlfd_name);
#endif
		/* need the pattern here to use for finding the rest
			of the family */
		/* put the current font that they are applying in the list first 
		so that fonts that sort before it do not take precedence */


  	/* if no match then return no bitmap found */
  if (!SetupBitmapPattern(xlfd_name, pattern,  1, &pixel, &resx, &resy)) return 0;
  if (pixel != 0) /* we already have a bitmap */
	return 0;
  display = XtDisplay(app_shellW);

  font_list =  XListFonts(display, pattern, max_names,
			&font_list_count);
#ifdef DEBUG
fprintf(stderr,"font_list_count=%d\n", font_list_count);
#endif
  for (i = 0, result=0; i < font_list_count; i++)   {
    if (!_IsXLFDFontName(font_list[i])) continue;
    if (!SetupBitmapPattern(font_list[i], pattern, 2, &pixel, &resx, &resy)) continue;


	/* see if the font is a bitmap */
	if (pixel !=0)  {
		/* this is a bitmap */
		if (result == True)  {
			/* another bitmap also exists so we
			need to choose the one with the best
			resolution match */
			if (resy < view->resy) {
				computey = view->resy - resy;
			} else {
				computey = resy - view->resy;
			}
			if (resx < view->resx) {
				computex = view->resx - resx;
			} else {
				computex = resx - view->resx;
			}
			if ((computex < save_resx) && (computey < save_resy)){
				/* this one is closer to screen resolution */
				strcpy(apply_xlfd_name, font_list[i]);
				save_resy = computey;
				save_resx = computex;
			} else {
				continue;
			}

			
		} else {

			strcpy(apply_xlfd_name, font_list[i]);
			if (resx < view->resx) {
				save_resx = view->resx - resx;
			} else {
				save_resx = resx - view->resx;
			}
			if (resy < view->resy) {
				save_resy = view->resy - resy;
			} else {
				save_resy = resy - view->resy;
			}
			result = True;
		}
	}

   }

  	XFreeFontNames(font_list);

	return result;
}  /* end of GetBitmapFontName() */



Bool
SetupBitmapPattern(fname, pattern, action, r_pixel, r_resx, r_resy)
    char       *fname;
    char 	*pattern;
    int 	action;
    int 	*r_pixel;
    int 	*r_resx;
    int 	*r_resy;
{
    register char *ptr;
    register char *resx, *resy, *awidth, *rest_xlfd, *encoding;
    register char *pixel, *point, *spacing, *style;
    char tmpbuf[256];
    int len, len2;

#ifdef DEBUG
fprintf(stderr,"fname=%s\n", fname);
#endif
    if (!(*fname == '-') ||     /* foundry */
            !(ptr=(char *)strchr(fname + 1, '-')) ||   /* family_name */
            !(ptr=(char *)strchr(ptr + 1, '-')) ||     /* weight_name */
            !(ptr=(char *)strchr(ptr + 1, '-')) ||     /* slant */
            !(ptr=(char *)strchr(ptr + 1, '-')) ||     /* setwidth_name */
            !(style=ptr=(char *)strchr(ptr + 1, '-')) ||     /* add_style_name */
            !(pixel=ptr =(char *)strchr(ptr + 1, '-')) ||      /* pixel_size */
            !(point=ptr =(char *)strchr(ptr + 1, '-')) ||      /* point */
            !(resx=ptr = (char *)strchr(ptr + 1, '-')) ||      /* res x */
            !(resy=ptr = (char *)strchr(ptr + 1, '-')) ||      /* res y */
            !(spacing=ptr = (char *)strchr(ptr + 1, '-')) ||      /* spacing */
            !(awidth=ptr = (char *)strchr(ptr + 1, '-')) ||      /* average width */
            !(rest_xlfd=ptr = (char *)strchr(ptr + 1, '-')) ||     /* charset_registry */
            !(char *)strchr(ptr + 1, '-'))/* charset_encoding */
        return FALSE;


#ifdef DEBUG
fprintf(stderr,"rest_xlfd=%s\n", rest_xlfd);
#endif

	switch (action) {

	case 1:		/* format pattern for XListFonts */
		len = pixel - fname;
			/* copy name up to pixel size */
		strncpy(tmpbuf, fname, len);
			/* wildcard pixel size */
		strcpy (tmpbuf+len, "-*");
		len += 2;
		len2 = resx - point;
			/* copy pointsize */
		strncpy(tmpbuf+len, point, len2);
		len += len2;
			/* wildcard resx and resy */
		strcpy(tmpbuf+len, "-*-*");
		len +=4;
		len2 = awidth - spacing ;
			/* copy spacing */
		strncpy(tmpbuf+len, spacing, len2);
		len += len2;
			/* wildcard average width */
		strcpy(tmpbuf+len, "-*");
		len +=2;
			/* copy rest of fontname */
		len2 = rest_xlfd -awidth;
		strcpy(tmpbuf+len, rest_xlfd);
		len += len2;
		len2 = strlen(fname);
		len = strlen(fname);
		tmpbuf[len] = 0;
		strcpy(pattern, tmpbuf);
#ifdef DEBUG
		fprintf(stderr,"pattern=%s\n", pattern);
#endif
		/* get pixelsize */
		len = point - pixel  -1;
		strncpy(tmpbuf, pixel+1, len);
		tmpbuf[len] = 0;
		*r_pixel = atoi(tmpbuf);
		r_resx = 0;
		r_resy = 0;

		break;

	case 2:

		/* get pixelsize */
		len = point - pixel  -1;
		strncpy(tmpbuf, pixel+1, len);
		tmpbuf[len] = 0;
		*r_pixel = atoi(tmpbuf);

		/* get resolution x */
		len = resy - resx -1;
		strncpy(tmpbuf, resx+1, len);
		tmpbuf[len] = 0;
		*r_resx = atoi(tmpbuf);

		/* get resolution y */
		len = spacing - resy -1;
		strncpy(tmpbuf, resy+1, len);
		tmpbuf[len] = 0;
		*r_resy = atoi(tmpbuf);

		break;

	}

    return TRUE;
}


