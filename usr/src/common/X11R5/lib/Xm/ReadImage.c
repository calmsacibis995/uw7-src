#pragma ident	"@(#)m1.2libs:Xm/ReadImage.c	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.2
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
#include <stdio.h>
#include <ctype.h>
#include <Xm/XmP.h>
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#endif
#include <string.h>
#include <Xm/VendorSP.h>  /* for the default display */

#define MAX_SIZE 255

/* shared data for the image read/parse logic */

static short hexTable[256];		/* conversion value */
static Boolean initialized = False;	/* easier to fill in at run time */


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void initHexTable() ;
static int NextInt() ;
static Boolean ReadBitmapDataFromFile() ;

#else

static void initHexTable( void ) ;
static int NextInt( 
                        FILE *fstream) ;
static Boolean ReadBitmapDataFromFile( 
                        char *filename,
                        int *width,
                        int *height,
                        char **data,
                        int *x_hot,
                        int *y_hot) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


/*  Table index for the hex values. Initialized once, first time.  */
/*  Used for translation value or delimiter significance lookup.   */
static void 
#ifdef _NO_PROTO
initHexTable()
#else
initHexTable( void )
#endif /* _NO_PROTO */
{
    /*
     * We build the table at run time for several reasons:
     *
     *     1.  portable to non-ASCII machines.
     *     2.  still reentrant since we set the init flag after setting table.
     *     3.  easier to extend.
     *     4.  less prone to bugs.
     */
    hexTable['0'] = 0;	hexTable['1'] = 1;
    hexTable['2'] = 2;	hexTable['3'] = 3;
    hexTable['4'] = 4;	hexTable['5'] = 5;
    hexTable['6'] = 6;	hexTable['7'] = 7;
    hexTable['8'] = 8;	hexTable['9'] = 9;
    hexTable['A'] = 10;	hexTable['B'] = 11;
    hexTable['C'] = 12;	hexTable['D'] = 13;
    hexTable['E'] = 14;	hexTable['F'] = 15;
    hexTable['a'] = 10;	hexTable['b'] = 11;
    hexTable['c'] = 12;	hexTable['d'] = 13;
    hexTable['e'] = 14;	hexTable['f'] = 15;

    /* delimiters of significance are flagged w/ negative value */
    hexTable[' '] = -1;	hexTable[','] = -1;
    hexTable['}'] = -1;	hexTable['\n'] = -1;
    hexTable['\t'] = -1;
	
    initialized = True;
}




/************************************************************************
 *
 *  NextInt
 *	Read next hex value in the input stream, return -1 if EOF
 *
 ************************************************************************/
static int 
#ifdef _NO_PROTO
NextInt( fstream )
        FILE *fstream ;
#else
NextInt(
        FILE *fstream )
#endif /* _NO_PROTO */
{
    int	ch;
    int	value = 0;
    int gotone = 0;
    int done = 0;
    

    /*  Loop, accumulate hex value until find delimiter    */
    /*  skip any initial delimiters found in read stream.  */

    while (!done)
    {
	ch = getc(fstream);

	if (ch == EOF) 
        {
	    value = -1;
	    done++;
	}
        else
        {
	    /* trim high bits, check type and accumulate */

	    ch &= 0xff;

	    if (isascii(ch) && isxdigit(ch)) 
            {
		value = (value << 4) + hexTable[ch];
		gotone++;
	    }
            else if ((hexTable[ch]) < 0 && gotone)
	        done++;
	}
    }
    return value;
}




/************************************************************************
 *
 *  ReadBitmapDataFromFile
 *	The data returned by the following routine is always in left-most 
 *	byte first and left-most bit first.  If it doesn't return 
 *	BitmapSuccess then its arguments won't have been touched.
 *
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
ReadBitmapDataFromFile( filename, width, height, data, x_hot, y_hot)
        char *filename ;
        int *width ;
        int *height ;
        char **data ;
        int *x_hot, *y_hot;			/* RETURNED */
#else
ReadBitmapDataFromFile(
        char *filename,
        int *width,    /* RETURNED */
        int *height,   /* RETURNED */
        char **data,   /* RETURNED */
        int *x_hot,    /* RETURNED */
	int *y_hot)    /* RETURNED */
#endif /* _NO_PROTO */
{
    FILE *fstream;			/* handle on file  */
    char *new_data = NULL;	        /* working variable */
    char line[MAX_SIZE];		/* input line from file */
    int size;				/* number of bytes of data */
    char name_and_type[MAX_SIZE];	/* an input line */
    char *type;				/* for parsing */
    int value;				/* from an input line */
    int version10p;			/* boolean, old format */
    int padding;			/* to handle alignment */
    int bytes_per_line;			/* per scanline of data */
    unsigned int ww = 0;		/* width */
    unsigned int hh = 0;		/* height */
    int hx = 0;				/* x hotspot */
    int hy = 0;				/* y hotspot */


    /* first time initialization */

    if (initialized == False) initHexTable();

    if ((fstream = fopen(filename, "r")) == NULL) 
	return (False);

    while (fgets(line, MAX_SIZE, fstream)) 
    {
	if (strlen(line) == MAX_SIZE - 1)
        {
           if (new_data) free (new_data);
           fclose (fstream);
	   return (False);
        }

	if (sscanf(line, "#define %s %d",name_and_type, &value) == 2) 
        {
	    if (!(type = (char *)strrchr(name_and_type, '_')))
	      type = name_and_type;
	    else
	      type++;

	    if (!strcmp("width", type))
	      ww = (unsigned int) value;

	    if (!strcmp("height", type))
	      hh = (unsigned int) value;

	    if (!strcmp("hot", type)) 
            {
		if (type-- == name_and_type || type-- == name_and_type)
		  continue;
		if (!strcmp("x_hot", type))
		  hx = value;
		if (!strcmp("y_hot", type))
		  hy = value;
	    }
	    continue;
	}
    
	if (sscanf(line, "static short %s = {", name_and_type) == 1)
	  version10p = 1;
	else if (sscanf(line,"static unsigned char %s = {",name_and_type) == 1)
	  version10p = 0;
	else if (sscanf(line, "static char %s = {", name_and_type) == 1)
	  version10p = 0;
	else
	  continue;

	if (!(type = strrchr(name_and_type, '_')))
	  type = name_and_type;
	else
	  type++;

	if (strcmp("bits[]", type))
	  continue;
    
	if (!ww || !hh)
        {
           if (new_data) free (new_data);
           fclose (fstream);
	   return (False);
        }

	if ((ww % 16) && ((ww % 16) < 9) && version10p)
	  padding = 1;
	else
	  padding = 0;

	bytes_per_line = (ww+7)/8 + padding;

	size = bytes_per_line * hh;
	new_data = XtMalloc ((Cardinal) size);

	if (version10p) 
        {
	    unsigned char *ptr;
	    int bytes;

	    for (bytes=0, ptr=(unsigned char *)new_data; bytes<size; (bytes += 2)) 
            {
		if ((value = NextInt(fstream)) < 0)
                {
                   if (new_data) XtFree (new_data);
                   fclose (fstream);
                   return (False);
                }

		*(ptr++) = value;
		if (!padding || ((bytes+2) % bytes_per_line))
		  *(ptr++) = value >> 8;
	    }
	} 
        else
        {
	    unsigned char *ptr;
	    int bytes;

	    for (bytes=0, ptr=(unsigned char *)new_data; bytes<size; bytes++, ptr++) 
            {
		if ((value = NextInt(fstream)) < 0) 
                {
                   if (new_data) XtFree (new_data);
                   fclose (fstream);
                   return (False);
                }
		*ptr=value;
	    }
	}
    }

    if (new_data == NULL)
    {
       fclose (fstream);
       return (False);
    }

    *data = new_data;
    *width = ww;
    *height = hh;
    if (x_hot) *x_hot = hx;
    if (y_hot) *y_hot = hy;

    fclose (fstream);
    return (True);
}


/************************************************************************
 *
 *  _XmGetImageAndHotSpotFromFile
 *	Given a filename, extract and create an image from the file data.
 *
 ************************************************************************/
XImage * 
#ifdef _NO_PROTO
_XmGetImageAndHotSpotFromFile( filename, hot_x, hot_y )
        char *filename ;
        int *hot_x, *hot_y;
#else
_XmGetImageAndHotSpotFromFile(
        char *filename,
	int *hot_x, 
	int *hot_y)
#endif /* _NO_PROTO */
{
   int width; 
   int height;
   char * data;
   XImage * image;
   Display * display = _XmGetDefaultDisplay() ; /* we don't have one here */

   if (ReadBitmapDataFromFile (filename, &width, &height, &data,
			       hot_x, hot_y))
   {
      _XmCreateImage(image, display, data, width, height, LSBFirst);

      return (image);
   }

   return (NULL);
}

/************************************************************************
 *
 *  _XmGetImageFromFile
 *	Given a filename, extract and create an image from the file data.
 *
 ************************************************************************/
XImage * 
#ifdef _NO_PROTO
_XmGetImageFromFile( filename )
        char *filename ;
#else
_XmGetImageFromFile(
        char *filename )
#endif /* _NO_PROTO */
{
   int hot_x, hot_y;
   return _XmGetImageAndHotSpotFromFile(filename, &hot_x, &hot_y) ;
}

