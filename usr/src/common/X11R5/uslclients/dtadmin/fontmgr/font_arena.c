#ident	"@(#)dtadmin:fontmgr/font_arena.c	1.16.1.10"
/*
 * Module:     dtadmin:fontmgr   Graphical Administration of Fonts
 * File:       font_array.c
 */

#include <stdio.h>
#include <Xatom.h>
#include <Intrinsic.h>
#include <fontmgr.h>
#include <Gizmos.h>

extern Widget       app_shellW;		  /* application shell widget       */

static char *bad_size[] = { "0" };
static char *bad_boys[] = { "open look cursor ", "open look glyph " };

static slant_type slant_table[] = {
{ "R", "" },
{ "I", "Italic" },
{ "O", "Oblique" },
{ "RI", "Reverse Italic"},
{ "RO", "Reverse Oblique" }
};


int CalculateTruePointSize(view,pixel,point)
view_type *view;
int pixel;
int point;
{
int calc_size;

	if (pixel == 0) return point;

		/* scalable fonts return actual pointsize */

	if (view->resy) {
	calc_size = pixel * 72.27;
	calc_size = calc_size / view->resy;
	}
return (calc_size);
}

/* returns FALSE if font is a derived instance font */
Boolean
ParseXLFD(view_type *view, String xlfd_name, xlfd_type **info)
{
    static xlfd_type xlfd_info;
    char field_str[MAX_STRING];
    char *p;
    int f, len;
    Boolean derived = FALSE;
    int scale_count;
    int res, pixel, point, calc_size;
    

    String pixel_size_p;

    *info = &xlfd_info;
		/* set bitmap to true as default case */
     scale_count = 0;
     view->bitmap = TRUE;
     xlfd_info.bitmap = TRUE;
  
    /* for each field in the XLFD */
    for (f = 0, p = xlfd_name; (f<=FIELD_COUNT); f++) {
	char *fieldP;

	p = (char *) GetNextField( DELIM, p, &fieldP, &len);
	strncpy(field_str, fieldP, len);
	field_str[len]=0;  /* string terminator */

	switch(f) {
	case 1: /* family name */
	    strcpy(xlfd_info.family, field_str);
	    CapitalizeStr(xlfd_info.family);
	    strcat(xlfd_info.family, " ");
	    break;
	case 2: /* weight */
	    strcpy(xlfd_info.weight, field_str);
	    CapitalizeStr(xlfd_info.weight);
	    break;
	case 3: /* slant */
	    CapitalizeStr(field_str);
	    strcpy(xlfd_info.slant, field_str);
	    break;
	case 4: /* setwidth name */
	    strcpy(xlfd_info.set_width, field_str);
	    CapitalizeStr(xlfd_info.set_width);
	    if (strcmp(xlfd_info.set_width, "Normal")==STR_MATCH)
		xlfd_info.set_width[0]=0;
	    break;
	case 5: /* add style name */
	    strcpy(xlfd_info.add_style, field_str);
	    CapitalizeStr(xlfd_info.add_style);
	    break;
	case 6: /* pixel size */
		pixel_size_p = fieldP;
		pixel = atoi(field_str);
		if (pixel == 0)  {
			scale_count++;
			break;
		}
	    	strcpy(xlfd_info.pixel, field_str);
	    	break;
	case 7: /* point size */
	    	field_str[len-1]=0; 
	    	point = atoi(field_str);
	
	    	if ((pixel == 0) && (point == 0)) {
			scale_count++;
			strcpy(xlfd_info.size, "0");
			break;
		}
		strcpy(xlfd_info.size, field_str);
		strcpy(xlfd_info.orig_size, xlfd_info.size);
		/*unit is in tenths of a point so convert it */
		break;
	case 8:
 		calc_size=CalculateTruePointSize(view, pixel, point);
		if (calc_size) {
			strcpy(xlfd_info.orig_size, xlfd_info.size);
			sprintf(xlfd_info.size, "%d", calc_size);
		}
	
		
		res = atoi(field_str);
		if ((res == 0) || (res == view->resx)) scale_count++;
	
		if (atoi(field_str) != 0)   
			strcpy(xlfd_info.resolutionX, field_str);
	    	else
			strcpy(xlfd_info.resolutionX,"");
		break;
	case 9:
		res = atoi(field_str);
		if ((res == 0) || (res == view->resy)) scale_count++;
		if (res == 0)  {
			strcpy(xlfd_info.resolutionY, "");
			break;
		}
		if ((strcmp(xlfd_info.resolutionX, field_str))!=STR_MATCH) {
			strcpy(xlfd_info.resolutionY, "/");
			strcat(xlfd_info.resolutionY, field_str);
			strcat(xlfd_info.resolutionY, GetGizmoText(FORMAT_DPI));
	   	} else {
			strcpy(xlfd_info.resolutionX, GetGizmoText(FORMAT_COMMA));
			strcat(xlfd_info.resolutionX, field_str);
			strcpy(xlfd_info.resolutionY, GetGizmoText(FORMAT_DPI));
		}
		break;
	case 10: /* spacing */
		if (strcmp("c", field_str)==STR_MATCH) {
			strcpy(xlfd_info.spacing, GetGizmoText(FORMAT_CELLSPACED));
			break;
	    	} 
	    	if (strcmp("m", field_str)==STR_MATCH)  {
			strcpy(xlfd_info.spacing, GetGizmoText(FORMAT_MONOSPACED));
			break;
		}
		strcpy(xlfd_info.spacing, "");
    		break;
	case 11: /* average width */
		if ((atoi(field_str) != 0)) {
			strcpy(xlfd_info.average_width, GetGizmoText(FORMAT_AVERAGEWIDTH));
			strcat(xlfd_info.average_width, field_str);
			break;
		} 
		scale_count++;
		strcpy(xlfd_info.average_width, "");
		break;
	case 12:
		/*CapitalizeStr(field_str);*/
		strcpy(xlfd_info.charset, field_str);
		break;
	case 13:
		if ((strcmp(xlfd_info.charset, "iso8859"))== STR_MATCH) {
		if (strcmp("1", field_str) == STR_MATCH) {
			strcpy(xlfd_info.charset, "");
			strcpy(xlfd_info.encoding, "");
			break;
			} else 
			/* iso8859-2 thru iso8859-9 */
		if ((field_str[0] >= '2') && (field_str[0] <= '9')) {
			strcpy(xlfd_info.charset, GetGizmoText(FORMAT_CHARSET_ISO1));
			strcpy(xlfd_info.encoding, field_str);
			break;
			} else 
			{
				/* iso8859-adobe */
			strcpy(xlfd_info.charset, "");
			strcpy(xlfd_info.encoding, "");
			break;
			} 
		}
			/* not iso8859 */
			/* temp to move around string*/
		if ((strcmp("fontspecific", field_str)) == STR_MATCH) {
			strcpy(xlfd_info.charset, GetGizmoText(FORMAT_CHARSET));
			CapitalizeStr(field_str);
			strcpy(xlfd_info.encoding, field_str);
			break;
		}
	
		CapitalizeStr(xlfd_info.charset);
		strcpy(xlfd_info.encoding, xlfd_info.charset); 
		strcpy(xlfd_info.charset, GetGizmoText(FORMAT_CHARSET));
		strcat(xlfd_info.charset, xlfd_info.encoding);
		strcat(xlfd_info.charset, GetGizmoText(FORMAT_DASH));
		CapitalizeStr(field_str);
		strcpy(xlfd_info.encoding, field_str);
		break;
	default:
		break;
	} /* switch */
    } /* for f */
     if (scale_count >= 5)  {
	view->bitmap = FALSE;
	xlfd_info.bitmap = FALSE;
	} else 
	if ((scale_count == 4) && (point != 0)) {
			/* derived instance case */
			view->bitmap = FALSE;
			xlfd_info.bitmap = FALSE;
	}
		
    return !derived;

} /* end of ParseXLFD */


static int
FamilyInfoCmp (pA, pB)
    family_info * pA;
    family_info * pB;
{
	return(strcmp(pA->name, pB->name));
}  /*  end of FamilyInfoCmp() */


static int
PSInfoCmp (pA, pB)
    ps_info *pA;
    ps_info *pB;
{
	int a = atoi(pA->ps);
	int b = atoi(pB->ps);

	if (a < b)
		return(-1);
	else
		return(!(a == b));

}  /*  end of PSInfoCmp() */


static int
StyleInfoCmp (pA, pB)
    style_info *pA;
    style_info *pB;
{
	return(strcmp(pA->style_name, pB->style_name));
}  /*  end of StyleInfoCmp() */


Boolean _IsXLFDFontName(fontName)
    String fontName;
{
    int f;
    for (f = 0; *fontName;) if (*fontName++ == DELIM) f++;
    return (f == FIELD_COUNT);
} /* end of _IsXLFDFontName() */


static _OlArrayType(StyleArray) *
	InitStyleArray ()
{
    _OlArrayType(StyleArray) * style_array;
    
    style_array = XtNew(_OlArrayType(StyleArray));
    _OlArrayInitialize(style_array, 4, 4, StyleInfoCmp);
    return(style_array);
    
}  /* end of InitStyleArray() */



static void
LookUpSlant( slant)
    String slant;
{
    int i;

    /*UppercaseStr( slant);*/
    for (i=0; i<XtNumber(slant_table); i++)
	if (strcmp( slant, slant_table[i].code)==STR_MATCH) {
	    strcpy( slant, slant_table[i].translation);
	    break;
	}
} /* end of LookUpSlant */


static void
ConcatStyle(String style, String cat)
{
    if (*cat) {
	strcat(style, " ");
	strcat(style, cat);
	/*strcat(style, " ");*/
    }    
} /* end of ConcatStyle */


static void
GetStyle(xlfd_type *info, String style_str)
{
    int style_len;

    LookUpSlant( info->slant);
    *style_str = 0;
    ConcatStyle(style_str, info->set_width);
    ConcatStyle(style_str, info->add_style);
    ConcatStyle(style_str, info->weight);
    ConcatStyle(style_str, info->slant);
    style_len = strlen(style_str);
    if (style_len && style_str[style_len-1] == ' ')
	style_str[style_len-1] = 0; /* remove trailing space */

} /* end of GetStyle */

static _OlArrayType(PSArray) *
AddStyle (xlfd_type *info, _OlArrayType(StyleArray) *style_array)
{
    char style_str[MAX_STRING];
    style_info tmp;
    int spot, hint;

    GetStyle(info, style_str);
    tmp.style_name = style_str;
    if ((spot = _OlArrayFindHint(style_array, &hint, tmp)) ==
	_OL_NULL_ARRAY_INDEX)  {
	tmp.style_name = XtNewString(style_str);
	tmp.l = XtNew(_OlArrayType(PSArray));
	_OlArrayInitialize(tmp.l, 10, 10, PSInfoCmp);
	spot = _OlArrayHintedOrderedInsert(style_array, hint, tmp);
    }
    return _OlArrayElement(style_array, spot).l;
}  /*  end of AddStyle() */


static void
AddPointSize (point_str, ps_array, xlfd_name, bitmap)
    String point_str;
    _OlArrayType(PSArray) * ps_array;
    String xlfd_name;
    Boolean bitmap;
{
    ps_info tmp;
    int hint;

	
    tmp.ps = point_str;
	
   if (bitmap == TRUE) {
	/* for bitmaps allow duplicates in pointsize array */
	tmp.ps = XtNewString(point_str);
	tmp.l = XtNew(font_type);
	tmp.l->xlfd_name = XtNewString(xlfd_name);
	tmp.l->bitmap = bitmap;
	_OlArrayHintedOrderedInsert(ps_array, hint, tmp);
    }
	 
    else
    {
    tmp.ps = "0";
    if (_OlArrayFindHint(ps_array, &hint, tmp) == _OL_NULL_ARRAY_INDEX)   {
	tmp.ps = XtNewString(point_str);
	tmp.l = XtNew(font_type);
	tmp.l->xlfd_name = XtNewString(xlfd_name);
	tmp.l->bitmap = bitmap;
	_OlArrayHintedOrderedInsert(ps_array, hint, tmp);
	}
    }
	

}  /* end of AddPointSize() */



/* add family */
static _OlArrayType(StyleArray) *
AddFamily(family_str, family_array)
    char *family_str;
    _OlArrayType(FamilyArray) *family_array;
{
    family_info tmp;
    int hint;
    int spot;

    tmp.name = family_str;
    if ((spot = _OlArrayFindHint(family_array, &hint,
				 tmp)) == _OL_NULL_ARRAY_INDEX)  {
	tmp.name = XtNewString(family_str);
	tmp.l = InitStyleArray();
	spot = _OlArrayHintedOrderedInsert(family_array, hint, tmp);
    }
    return _OlArrayElement(family_array, spot).l;
} /* end of AddFamily */


static Boolean
ValidSize(int bitmap, String str)
{
    int i;

  
	/* check for bitmapped with 0 pointsize, since we don't
	want to show these in the list */

    if (bitmap == FALSE) return TRUE;
    for (i=0; i<XtNumber(bad_size); i++)
      if (caseless_strcmp(str, bad_size[i], 0) == STR_MATCH)
          return FALSE;

    return TRUE;
}

static Boolean
ValidFamily(String str)
{
    int i;

    for (i=0; i<XtNumber(bad_boys); i++)
      if (caseless_strcmp(str, bad_boys[i], 0) == STR_MATCH)
          return FALSE;

    return TRUE;
}


static void
FillArray(view, family_array, xlfd_name)
    view_type *view;
    _OlArrayType(FamilyArray) *family_array;
    char *xlfd_name;
{
    _OlArrayType(StyleArray) *style_array;
    _OlArrayType(PSArray) *ps_array;
    xlfd_type *info;

	/* skip CDE fontname aliases that start with
			foundry -dt */
    if ((strncmp(xlfd_name, "-dt", 3)) == STR_MATCH) {
#ifdef DEBUG
		fprintf(stderr,"found dt %s\n",xlfd_name);
#endif
		return;
	}
    if (ParseXLFD(view, xlfd_name, &info) && ValidFamily(info->family)
		&& ValidSize(info->bitmap, info->size)) {
	style_array = AddFamily(info->family, family_array);
	ps_array = AddStyle(info, style_array);
	AddPointSize(info->size, ps_array, xlfd_name, info->bitmap);
    }

} /* end of FillArray */


String GetFontName( view_type *view, XFontStruct * newfont, String xlfd)
{
    char font_name[MAX_PATH_STRING];
    char style_str[MAX_STRING];
    xlfd_type *info;
    int bitmap;
	/* save bitmap setting before using current xlfd since
		current xlfd will have pointsize filled in for
		scalable */

    bitmap = view->bitmap;
    ParseXLFD( view, xlfd, &info);
    LookUpSlant( info->slant);
    GetStyle(info, style_str);

    sprintf(font_name, "%s%s ", info->family, style_str);
    if (!bitmap) {
		if ((GetPostScriptFontName(view,newfont,xlfd) == 1)) {
		sprintf(font_name,"%s ", view->postscript_name);
		/*else
		sprintf(font_name, "%s%s ", info->family, style_str);
	*/
    		}
    }

    if (bitmap) {
	sprintf(font_name, "%s%s%s", font_name, info->size,
		GetGizmoText(TXT_BITMAP));
  	sprintf(font_name, "%s%s%s%s%s", font_name, 
		info->spacing, info->resolutionX, info->resolutionY,
		info->average_width);
	}
	else {
		if (strcmp(info->spacing,"") == 0) {
			sprintf(font_name,"%s%s%s", font_name, info->size,
				GetGizmoText(TXT_OUTLINE));
		} else  {
			sprintf(font_name, "%s%s%s%s", font_name, 
				info->size, GetGizmoText(TXT_OUTLINE),
				info->spacing);

		}
	}
	if (strcmp(info->charset ,"") == 0)  {
		sprintf(font_name, "%s)", font_name);
	} else {
		sprintf(font_name, "%s%s%s)", font_name, info->charset,info->encoding);
	}	
    return font_name;

} /* end of GetFontName */



static void
AddFontFamilies ( view, display, family_array)
    view_type *view;
    Display * display;
    _OlArrayType(FamilyArray) *family_array;
{
  char ** font_list;
  String pattern = "*";
  int font_list_count;
  int max_names = 32767;
  int i;

  font_list =  XListFonts(display, pattern, max_names,
			  &font_list_count);

  /* for each XLFD */
  for (i = 0; i < font_list_count; i++)  { 
    if (!_IsXLFDFontName(font_list[i])) {
      continue;
    }

    FillArray(view, family_array, font_list[i]);
  } /* for i */

  XFreeFontNames(font_list);

}  /* end of AddFontFamilies() */


/* create and init family_array */
void
CreateFontDB( view,family_array)
    view_type *view;
    _OlArrayType(FamilyArray) *family_array;
{
    _OlArrayInitialize(family_array, 15, 15, FamilyInfoCmp);
    AddFontFamilies(view, XtDisplay(app_shellW), family_array);

} /* end of CreateFontDB */


/*  destroy the style and size arrays and the associated strings.  */
void
DeleteFontDB( family_array)
    _OlArrayType(FamilyArray) *family_array;
{
    int i, j, k;
    _OlArrayType(PSArray) * ps_array;
    _OlArrayType(StyleArray) * style_array;
    font_type *font_info;

    for (i = 0; i < _OlArraySize(family_array); i++)  {
	XtFree(_OlArrayElement(family_array, i).name);
	style_array = _OlArrayElement(family_array, i).l;
	for (j = 0; j < _OlArraySize(style_array); j++)  {
	    XtFree(_OlArrayElement(style_array, j).style_name);
	    ps_array = _OlArrayElement(style_array, j).l;
	    for (k = 0; k < _OlArraySize(ps_array); k++)  {
		XtFree(_OlArrayElement(ps_array, k).ps);
		font_info = _OlArrayElement(ps_array, k).l;
		XtFree( font_info->xlfd_name);
		XtFree( (char *) font_info);
	    }
	    _OlArrayFree(ps_array);
	}
	_OlArrayFree(style_array);
    }
    _OlArrayFree(family_array);

} /* end of DeleteFontDB */

#define FONT_KEY "*sansSerifFamilyFontList:"
ResetToDefaultFont(view_type *view)
{
    int i, num_props;
    XFontProp *props;
    char buf[MAX_PATH_STRING], *p;
    FILE *file;
    XFontStruct *font;
    xlfd_type *info;
    char *home;
    int key_len = strlen(FONT_KEY);

    *view->cur_xlfd = 0;

    /* first try to get fontname from .Xdefaults */
    home = (char *) getenv("HOME");
    if (home) sprintf(buf, "%s/.Xdefaults", getenv("HOME"));
	else
	sprintf(buf,"./Xdefaults");
    file = fopen(buf, "r");
    if (FileOK(file)) {
	while (fgets(buf, MAX_PATH_STRING, file) != NULL) {
	    if (strncmp(buf, FONT_KEY, key_len) == STR_MATCH) {
		if ((p = strchr(buf, DELIM)) == NULL)
			continue;
		strcpy(view->cur_xlfd, p);
		view->cur_xlfd[strlen(view->cur_xlfd)-1]=0; /* get rid of \n */
		break;
	    }
	}
	fclose(file);
    }

    /* try to get name from X property */
    if (*view->cur_xlfd == 0) {
	XtVaGetValues(view->sample_text, XtNfont, &font, NULL);
	props = font->properties;
	num_props = font->n_properties;
	for (i = 0; i < num_props; i++, props++)  {
	    if (props->name == XA_FONT) {
		p = XGetAtomName(XtDisplay(app_shellW),props->card32);
		if (p)
		    strcpy(view->cur_xlfd, p);
		break;
	    }
	}
    }
    if (*view->cur_xlfd) {
	ParseXLFD(view, view->cur_xlfd, &info);
	strcpy(view->cur_family, info->family);
	GetStyle(info, view->cur_style);
	view->cur_size = atoi(info->size);
    }
} /* end of ResetToDefaultFont */



int 
GetPostScriptFontName(view_type *view, XFontStruct *newfont, String xlfd)
{
Atom adobePostScriptFontNameAtom, psfontNameAtom;
char *xlfdstring;
char *psfontName;

	Display * dpy = XtDisplay(app_shellW);

	adobePostScriptFontNameAtom = XInternAtom(dpy,
		"_ADOBE_POSTSCRIPT_FONTNAME", FALSE);
	if (adobePostScriptFontNameAtom == None) 
		adobePostScriptFontNameAtom = 0L;

	if (!adobePostScriptFontNameAtom) 
		return -1;

	if (!XGetFontProperty(newfont, adobePostScriptFontNameAtom,
			&psfontNameAtom)) {
	/*	psfontName is a (char *). According to the X 
	documentation, XGetAtomName is doing the allocating here, and
	the application is expected to XFree the psfontName when done. */

			return -1;
	}
	psfontName = XGetAtomName(dpy, psfontNameAtom);
	if (!psfontName) {
		return -1;
	}

	if ((strlen(psfontName)) > MAX_STRING) {
		XFree(psfontName);
		return -1;
	/* name too long */
	}
	strcpy(view->postscript_name, 	psfontName);
	XFree(psfontName);
	return 1;

}

