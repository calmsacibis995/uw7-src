#ident	"@(#)wksh:xpm.c	1.3"

/* Copyright 1989 GROUPE BULL -- See licence conditions in file COPYRIGHT */
/*****************************************************************************
 ****  Read/Write package for XPM file format (X PixMap) 
 ****
 ****  Pixmap XCreatePixmapFromData(dpy, d, cmap, w, h, depth, n, c, col, pix)
 ****  int    XReadPixmapFile(dpy, d, cmap, filename, w_return, h_return,
 ****                                                     depth, pixmap_return)
 ****  int    XWritePixmapFile(dpy, cmap, filename, pixmap, w, h)
 ****
 ****  Daniel Dardailler - Bull RC (89/02/22) e-mail: daniel@mirsa.inria.fr
 *****************************************************************************
 ****  Version 1.1:  extended chars_per_pixel support... [ Read, Create ]
 ****
 ****  Richard Hess - Consilium    (89/11/06) e-mail: ..!uunet!cimshop!rhess
 *****************************************************************************
 ****  Version 1.2:  improved file and storage mgmt, support for writing 1 cpp
 ****
 ****  James Bash - AT&T Bell Labs (89/11/25) e-mail: jmb@attunix.att.com
 *****************************************************************************
 ****  Version "2.0":  Read support for NAME_mono[] option...
 ****
 ****  Richard Hess - Consilium    (89/11/30) e-mail: ..!uunet!cimshop!rhess
 *****************************************************************************/

/*****************************************************************************
 **** Look of XPM file : .. something like X11 'C includable' format

#define drunk_format 1
#define drunk_width 18
#define drunk_height 21
#define drunk_ncolors 4
#define drunk_chars_per_pixel 2
static  char * drunk_colors[] = {
"  " , "#FFFFFFFFFFFF",
". " , "#A800A800A800",
"X " , "White",
"o " , "#540054005400"  
} ;
static char * drunk_pixels[] = {
"                                    ",
"                                    ",
"            . . . . . . .           ",
"          X         . . . .         ",
"        X     X       . . . .       ",
"      o         X       . . .       ",
"    o o     X           . . . .     ",
"  o o o               . . . . .     ",
"o o o               . . . . . .     ",
"o o o                   . . . .     ",
"  X                 X   . . .       ",
"  X   X               . . . .       ",
"    X               . . . .         ",
"    X                 . .           ",
"      X                   X X X     ",
"        X X X               X   X   ",
"              X           X X       ",
"            X X X       X X         ",
"          X       X X X             ",
"      X X                           ",
"                                    " 
} ;

******************************************************************************
*  Version 1.1 can handle either 1 or 2 chars per pixel
*  - for each different color : n chars can represent the pixel value 
*    and is associed with red, green and blue intensity or colorname.
******************************************************************************/

/*
 * From arpa!mirsa.inria.fr!Daniel.Dardailler Wed Nov 15 16:27:25 +0100 1989
 * Date: Wed, 15 Nov 89 16:27:25 +0100
 * From: Daniel Dardailler <Daniel.Dardailler@mirsa.inria.fr>
 * To: jmb@attunix.att.com
 * Subject: XPM
 * 
 * What is really meaned by this copyright is that:
 * 
 * Like the MIT distribution of the X Window System,it is publicly available,
 * but is NOT in the public domain (my fault).
 * The difference is that copyrights granting rights
 * for unrestricted use and redistribution have been placed on all of the
 * software to identify its authors.
 * You are allowed and encouraged to take
 * this software and build commercial products.
 * 
 * GROUPE BULL will let you use XPM as long as you do not pretend to have
 * written it.
 * 
 * You are also encouraged to re-distribute it freely with the same behaviour.
 * bye
 * 
 *    Daniel Dardailler                   |      Email : daniel@mirsa.inria.fr
 *    BULL  Centre de Sophia Antipolis    |      Phone : (33) 93 65 77 71
 *          2004, Route des Lucioles      |      Telex :      97 00 50 F
 *          06565 Valbonne CEDEX  France  |      Fax   : (33) 93 65 77 66
 */

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>

#include "xpm.h"      /* PixmapOpenFailed, PixmapSuccess .. MAXPRINTABLE */

#ifdef SYSV
#define rename(from, to)	(link(from, to) ? -1 : (unlink(from) ? -1 : 0))
#endif /* SYSV */

#define MAX_LINE_LENGTH		4096

static char * getline();
static char * StripName();
static char * BackupName();
static char * TmpName();


/**[ XCreatePixmapFromData ]**************************************************
 *
 *  This function allows you to include in your C program (using #include)
 *  a pixmap file in XPM format that was written by XWritePixmapFile
 * (THIS VERSION SUPPORTS : '_chars_per_pixel 1', '_chars_per_pixel 2')
 *****************************************************************************/

Pixmap XCreatePixmapFromData(display, d, colormap, width, height, depth, 
			     ncolors, chars_per_pixel, colors, pixels)
     Display *display;
     Drawable d;
     Colormap colormap ;
     unsigned int width, height;       
     unsigned int depth ;   
     unsigned int ncolors ;
     unsigned int chars_per_pixel ;
     char ** colors ;           /* array of colormap entries  "cc","#RRGGBB" */
     char ** pixels ;           /* array of pixels lines    "cc..cc00cccc.." */
{
  Pixmap pixmap ;
  GC Gc = NULL;
  XGCValues xgcv;

  CmapEntry * cmap = NULL;
  int * Tpixel = NULL;

  char c1, c2, c;
  int red, green, blue ;
  XColor xcolor ;
  int i,j,p;
  

  /* cleanup and return macro */
#undef RETURN
#define RETURN(code, flag) \
	{ if (cmap) XtFree (cmap); if (Tpixel) XtFree (Tpixel); \
		if (flag && pixmap) XFreePixmap(display, pixmap); \
		if (Gc) XFreeGC(display, Gc); return (code); }

  if (ncolors > (MAXPRINTABLE*MAXPRINTABLE)) 
    RETURN (fatal("Too many different colors, version 1"), 0);

  if ((chars_per_pixel < 1) || (chars_per_pixel > 2))
    RETURN (fatal("version 1.1 handles only 1 or 2 chars_per_pixel"), 0);

  /* now we construct cmap and Tpixel from colors array parameter */
  cmap = (CmapEntry *) XtMalloc(ncolors*sizeof(CmapEntry)) ;
  Tpixel = (int *) XtMalloc(ncolors*sizeof(int)) ;
  if ((cmap == NULL) || (Tpixel == NULL))
    RETURN (PixmapNoMemory, 0) ;

  if (!colors) RETURN (fatal("colors not defined"), 0);

  switch (chars_per_pixel)
    {
    case 1:
      for (i=0; i<2*ncolors ; i+=2)
	{
	  if (sscanf(colors[i],"%c", &c1) != 1)
	    RETURN
	     (fatal("bad colormap entry : must be '\"c\" , \"colordef\",'"), 0);
	  if (index(printable,c1))
	    {
	      cmap[i/2].cixel.c1 = c1 ;
	      if (!XParseColor(display,colormap,colors[i+1],&xcolor))
		RETURN
		  (fatal("bad colordef specification : #RGB or colorname"), 0);
	      XAllocColor(display,colormap,&xcolor);
	      Tpixel[i/2] = xcolor.pixel ;
	    } else
	      RETURN (fatal("bad cixel value : must be printable"), 0);
	}
      break;
    case 2:
      for (i=0; i<2*ncolors ; i+=2)
	{
	  if (sscanf(colors[i],"%c%c", &c1,&c2) != 2)
	    RETURN
	    (fatal("bad colormap entry : must be '\"cC\" , \"colordef\",'"), 0);
	  if ((index(printable,c1)) &&
	      (index(printable,c2))) {
	    cmap[i/2].cixel.c1 = c1 ;
	    cmap[i/2].cixel.c2 = c2 ;
	    if (!XParseColor(display,colormap,colors[i+1],&xcolor))
	      RETURN
		(fatal("bad colordef specification : #RGB or colorname"), 0);
	    XAllocColor(display,colormap,&xcolor);
	    Tpixel[i/2] = xcolor.pixel ;
	  } else
	    RETURN (fatal("bad cixel value : must be printable"), 0);
	}
      break;
    }
  
  pixmap = XCreatePixmap(display,d,width,height,depth);
  Gc = XCreateGC(display,pixmap,0,&xgcv);
  
  if (!pixels) RETURN (fatal("pixels not defined"), 1);
  j = 0 ;
  while (j < height)
    {  
      if (strlen(pixels[j]) != (chars_per_pixel*width))
	RETURN (fatal("bad pixmap line length %d (widths out of sync)",
							strlen(pixels[j])), 1);
      switch (chars_per_pixel)
	{
	case 1:
	  for (i=0; i< (width) ; i++)
	    {
	      c1 = pixels[j][i] ;
	      for (p = 0 ; p < ncolors ; p++)
		if (cmap[p].cixel.c1 == c1) break ;
	      if (p != ncolors)
		XSetForeground(display,Gc,Tpixel[p]);
	      else 
		RETURN (fatal("cixel \"%c\" not in previous colormap",c1), 1);
	      XDrawPoint(display,pixmap,Gc,i,j) ;
	    }
	  break;
	case 2:
	  for (i=0; i< (2*width) ; i+=2)
	    {
	      c1 = pixels[j][i] ;
	      c2 = pixels[j][i+1] ;
	      for (p = 0 ; p < ncolors ; p++)
		if ((cmap[p].cixel.c1 == c1)&&(cmap[p].cixel.c2 == c2)) break ;
	      if (p != ncolors)
		XSetForeground(display,Gc,Tpixel[p]);
	      else 
		RETURN
		  (fatal("cixel \"%c%c\" not in previous colormap",c1,c2), 1);
	      XDrawPoint(display,pixmap,Gc,i/2,j) ;
	    }
	  break;
	}
      j++ ;
    }
  
  RETURN (pixmap, 0) ;
}

/**[ XReadPixmapFile ]********************************************************
 *
 *  Read a Pixmap file in a X Pixmap with specified depth and colormap...
 * (THIS VERSION SUPPORTS : '_mono[]' [ optional ] )
 * (THIS VERSION SUPPORTS : '_chars_per_pixel 1', '_chars_per_pixel 2')
 *****************************************************************************/

int XReadPixmapFile (display,d,
		     colormap,filename,width,height,depth,pixmap)
     Display *display;
     Drawable d;
     Colormap colormap ;
     char *filename;
     unsigned int *width, *height;       /* RETURNED */
     unsigned int depth ;   
     Pixmap *pixmap;                     /* RETURNED */
{
  GC Gc = NULL;
  XGCValues xgcv;

  FILE *fstream = NULL;			/* handle on file  */
  char linebuf[MAX_LINE_LENGTH] ;
  char namebuf[80];
  char name[80];
  char type[40];

  int ncolors ;

  CmapEntry * cmap = NULL;
  int * Tpixel = NULL;

  char c1, c2, c;
  int red, green, blue ;
  XColor xcolor ;
  int i,j,p,n;
  int cpp;
  int mono = 0;
  

  /* cleanup and return macro */
#undef RETURN
#define RETURN(code, flag) \
	{ if (cmap) XtFree (cmap); if (Tpixel) XtFree (Tpixel); \
		if (Gc) XFreeGC(display, Gc); \
		if (flag && *pixmap) { XFreePixmap(display, *pixmap); \
							*pixmap = NULL; } \
		if (fstream) fclose (fstream); return (code); }

  if ((fstream = fopen(filename, "r")) == NULL) {
    RETURN (PixmapOpenFailed, 0);
  }

  getline(linebuf,fstream);
  if ((sscanf(linebuf, "#define %[^_]%s %d", namebuf, type, &p) != 3) 
      || ((strcmp("_format",type)) && (strcmp("_paxformat",type)))
      || (p != XPM_FORMAT)) {
    RETURN (PixmapFileInvalid, 0);    /* be silent about it at first */
  } else
    strcpy(name,namebuf);

  getline(linebuf,fstream);
  if ((sscanf(linebuf, "#define %[^_]%s %d", namebuf, type, width) != 3)
      || (strcmp(name,namebuf)) 
      || (strcmp("_width",type))) 
	RETURN (fatal("bad '#define NAME_width n'"), 0);

  getline(linebuf,fstream);
  if ((sscanf(linebuf, "#define %[^_]%s %d", namebuf, type, height) != 3)
      || (strcmp(name,namebuf)) 
      || (strcmp("_height",type))) 
	RETURN (fatal("bad '#define NAME_height n'"), 0);

  getline(linebuf,fstream);
  if ((sscanf(linebuf, "#define %[^_]%s %d", namebuf, type, &ncolors) != 3)
      || (strcmp(name,namebuf)) 
      || (strcmp("_ncolors",type))) 
	RETURN (fatal("bad '#define NAME_ncolors n'"), 0);

  if (ncolors > (MAXPRINTABLE*MAXPRINTABLE)) 
    RETURN (fatal("Too many different colors, version 1"), 0);

  getline(linebuf,fstream);
  if ((sscanf(linebuf, "#define %[^_]%s %d", namebuf, type, &cpp) != 3)
      || (strcmp(name,namebuf)) || (cpp < 1) || (cpp > 2)
      || (strcmp("_chars_per_pixel",type)))
	RETURN (fatal("bad '#define NAME_chars_per_pixel n' [1][2]"), 0);

  for (n=0; n<2 ; n++) {
    if (n == 0) {
      getline(linebuf,fstream);
      if ((sscanf(linebuf, "static char * %[^_]%s = {",namebuf,type) != 2)
	  || (strcmp(name,namebuf))
	  || (strcmp("_mono[]",type))) continue;
      if (depth != 1) {
	for (i=0; i<=ncolors ; i++) {
	  getline(linebuf,fstream);
	}
	if (strncmp(linebuf, "} ;",3))
	  RETURN (fatal("missing '} ;' in NAME_mono[]"), 0);
	getline(linebuf,fstream);
	continue;
      }
      mono = 1;
    }
    else {
      if (mono) getline(linebuf, fstream);
      if ((sscanf(linebuf, "static char * %[^_]%s = {",namebuf,type) != 2)
	  || (strcmp(name,namebuf))
	  || (strcmp("_colors[]",type))) 
	RETURN (fatal("bad 'static char * NAME_colors[] = {'"), 0);
      if (mono) {
	for (i=0; i<=ncolors ; i++) {
	  getline(linebuf,fstream);
	}
	if (strncmp(linebuf, "} ;",3))
	  RETURN (fatal("missing '} ;'"), 0);
	continue;
      }
    }
    cmap = (CmapEntry *) XtMalloc(ncolors*sizeof(CmapEntry)) ;
    Tpixel = (int *) XtMalloc(ncolors*sizeof(int)) ;
    if ((cmap == NULL) || (Tpixel == NULL))
      RETURN (PixmapNoMemory, 0) ;

    getline(linebuf,fstream);
    for (i=0; i<ncolors ; i++) {
      switch (cpp)
	{
	case 1:
	  if (sscanf(linebuf, "\"%c\" , \"%[^\"]%s",
		     &c1,namebuf,type) != 3)
	    RETURN
	    (fatal("bad colormap entry : must be '\"c\" , \"colordef\",'"), 0);
	  if (index(printable,c1)) {
	    cmap[i].cixel.c1 = c1 ;
	  } else
	    RETURN (fatal("bad cixel value : must be printable"), 0);
	  break;
	case 2:
	  if (sscanf(linebuf, "\"%c%c\" , \"%[^\"]%s",
		     &c1,&c2,
		     namebuf,type) != 4)
	    RETURN
	    (fatal("bad colormap entry : must be '\"cC\" , \"colordef\",'"), 0);
	  if ((index(printable,c1)) &&
	      (index(printable,c2))) {
	    cmap[i].cixel.c1 = c1 ;
	    cmap[i].cixel.c2 = c2 ;
	  } else
	    RETURN (fatal("bad pixel value : must be printable"), 0);
	  break;
	}
      if (!XParseColor(display,colormap,namebuf,&xcolor))
	RETURN (fatal("bad colordef specification : #RGB or colorname"), 0);
      XAllocColor(display,colormap,&xcolor);
      Tpixel[i] = xcolor.pixel ;
      getline(linebuf,fstream);
    }
    if (strncmp(linebuf, "} ;",3))
      RETURN (fatal("missing '} ;'"), 0);
  }

  getline(linebuf,fstream);
  if ((sscanf(linebuf, "static char * %[^_]%s = {",namebuf,type) != 2)
      || (strcmp(name,namebuf))
      || (strcmp("_pixels[]",type))) 
	RETURN (fatal("bad 'static char * NAME_pixels[] = {'"), 0);

  *pixmap = XCreatePixmap(display,d,*width,*height,depth);
  Gc = XCreateGC(display,*pixmap,0,&xgcv);
  
  getline(linebuf,fstream);
  j = 0 ;
  while((j < *height) && strncmp(linebuf, "} ;",3))
    {  
      if (strlen(linebuf) < (cpp*(*width)+2)) 
	RETURN (fatal("pixmap line length %d exceeds maximum of %d",
					cpp*(*width)+2, strlen(linebuf)), 1);
      switch (cpp)
	{
	case 1:
	  for (i=1; i<=(*width) ; i++)
	    {
	      c1 = linebuf[i] ;
	      for (p = 0 ; p < ncolors ; p++)
		if (cmap[p].cixel.c1 == c1) break ;
	      if (p != ncolors)
		XSetForeground(display,Gc,Tpixel[p]);
	      else 
		RETURN (fatal("cixel \"%c\" not in previous colormap",c1), 1);
	      XDrawPoint(display,*pixmap,Gc,i-1,j) ;
	    }
	  break;
	case 2:
	  for (i=1; i<(2*(*width)) ; i+=2)
	    {
	      c1 = linebuf[i] ;
	      c2 = linebuf[i+1] ;
	      for (p = 0 ; p < ncolors ; p++)
		if ((cmap[p].cixel.c1 == c1)&&(cmap[p].cixel.c2 == c2)) break ;
	      if (p != ncolors)
		XSetForeground(display,Gc,Tpixel[p]);
	      else 
		RETURN
		  (fatal("cixel \"%c%c\" not in previous colormap",c1,c2), 1);
	      XDrawPoint(display,*pixmap,Gc,i/2,j) ;
	    }
	  break;
	}
      j++ ;
      getline(linebuf,fstream);    
    }

  if (strncmp(linebuf, "} ;",3))
    RETURN (fatal("missing '} ;'"), 1);

  if (j != *height)
    RETURN (fatal("%d too few pixmap lines", *height - j), 1);

  RETURN (PixmapSuccess, 0) ;
}


/**[ XWritePixmapFile ]*******************************************************
 *
 *  Write a X Color Pixmap through a specified colormap
 * (THIS VERSION PRODUCES 1 CHARS PER PIXEL FORMAT : '_chars_per_pixel 1')
 * (THIS VERSION PRODUCES 2 CHARS PER PIXEL FORMAT : '_chars_per_pixel 2')
 *****************************************************************************/

int XWritePixmapFile(display,
		     colormap, filename, pixmap, width, height)
     Display * display;
     Colormap  colormap ;
     char *filename;
     Pixmap pixmap;
     unsigned int width, height;
{
   XImage *image = NULL;         /* client image */
   
   FILE *fstream = NULL;
   char *stripname = NULL, *backupname = NULL, *tmpname = NULL;

   int ncolors = 0 ;             /* number of different colors */

   CmapEntry * cmap = NULL;      /* indexed by ncolors, 
				    dynamically allocated by width */

   Cixel * Map = NULL;           /* should be [width][height] */
   int i,j,p ;

   XColor * colors = NULL;

   int MaxCixel ;               /* theorytical max */
   int cpp;


  /* cleanup and return macro */
#undef RETURN
#define RETURN(code) \
	{ if (stripname) XtFree (stripname); if (backupname) XtFree (backupname); \
		if (tmpname) XtFree (tmpname); if (cmap) XtFree (cmap); \
		if (Map) XtFree (Map); if (colors) XtFree (colors); \
		if (image) XDestroyImage(image); \
		if (fstream) { fclose (fstream); unlink (tmpname); } \
		return (code); }

   if (!filename) RETURN(PixmapOpenFailed);

   if ((stripname = StripName(filename)) == NULL ||
       (backupname = BackupName(filename)) == NULL ||
       (tmpname = TmpName(filename)) == NULL)
     RETURN (PixmapNoMemory) ;

   if (!(fstream = fopen(tmpname, "w")))
     RETURN(PixmapOpenFailed);            

   /* Convert pixmap to an image in client memory */
   image = XGetImage(display, pixmap, 0,0,width, height, AllPlanes, ZPixmap);

   /* calcul of ncolors, cmap, Map in a while */
   colors = (XColor *) calloc(width,sizeof(XColor));
   cmap = (CmapEntry *) XtMalloc(MAXPRINTABLE*sizeof(CmapEntry)); 
   Map = (Cixel *) XtMalloc(sizeof(Cixel)*width*height);

   if ((colors == NULL) || (cmap == NULL) || (Map == NULL))
     RETURN (PixmapNoMemory) ;

   MaxCixel = (MAXPRINTABLE*MAXPRINTABLE)-1 ;

   for (j=0 ; j<height ; j++) {
     for (i=0; i<width ; i++) colors[i].pixel = XGetPixel(image,i,j) ;
     /* ask server for rgb values line by line */
     XQueryColors(display,colormap,colors,width);  /* return : colors */
     for (i=0; i<width ; i++) {
       for (p=0; p<ncolors; p++)
	 if ((colors[i].red   == cmap[p].red) &&
	     (colors[i].green == cmap[p].green) &&
	     (colors[i].blue  == cmap[p].blue)) {  /* already present */
	   *(Map + j*width + i) = cmap[p].cixel ;
	   break ;                     /* stop with p < ncolors */
	 }
       if (p == ncolors) {             /* new rgb value in cmap */
	 cmap[ncolors].red = colors[i].red ;
	 cmap[ncolors].green = colors[i].green ;
	 cmap[ncolors].blue = colors[i].blue ;
	 cmap[ncolors].cixel.c1 = printable[ncolors % MAXPRINTABLE];
	 cmap[ncolors].cixel.c2 = printable[ncolors / MAXPRINTABLE];
	 *(Map + j*width + i) = cmap[ncolors].cixel ;  
	 if (ncolors < MaxCixel) ncolors++ ;
	 if (ncolors % MAXPRINTABLE) {        /*  => much memory for cmap */
	   cmap = (CmapEntry *) XtRealloc((char*)cmap,
				(ncolors+MAXPRINTABLE)*sizeof(CmapEntry)); 
	   if (cmap == NULL)
	     RETURN (PixmapNoMemory) ;
	 }
       }
     }
   }       
   if (ncolors == MaxCixel) ncolors++ ;
   cpp = (ncolors <= MAXPRINTABLE) ? 1 : 2;

   /* Write out standard header */
   fprintf(fstream, "#define %s_format %d\n",stripname,XPM_FORMAT);
   fprintf(fstream, "#define %s_width %d\n",stripname,width);
   fprintf(fstream, "#define %s_height %d\n",stripname,height);
   fprintf(fstream, "#define %s_ncolors %d\n",stripname,ncolors);
   fprintf(fstream, "#define %s_chars_per_pixel %d\n",stripname,cpp);

   fprintf(fstream, "static char * %s_colors[] = {\n",stripname);

   for (i=0; i<ncolors ; i++) {
     switch (cpp) {
     case 1:
       fprintf(fstream, "\"%c\" , \"#%04X%04X%04X\"",
	       cmap[i].cixel.c1,
	       cmap[i].red,cmap[i].green,cmap[i].blue);
       break;
     case 2:
       fprintf(fstream, "\"%c%c\" , \"#%04X%04X%04X\"",
	       cmap[i].cixel.c1,cmap[i].cixel.c2,
	       cmap[i].red,cmap[i].green,cmap[i].blue);
       break;
     }
     if (i != (ncolors - 1)) fprintf(fstream,",\n"); else
                             fprintf(fstream,"\n"); 
   }     
   fprintf(fstream, "} ;\n");
   fprintf(fstream, "static char * %s_pixels[] = {\n",stripname);

   for (j=0; j<height ; j++) {
     fprintf(fstream, "\"");
     for (i=0; i<width ; i++) 
       switch (cpp) {
       case 1:
         fprintf(fstream, "%c", (*(Map + j*width + i)).c1);
         break;
       case 2:
         fprintf(fstream, "%c%c", (*(Map + j*width + i)).c1,
	         (*(Map + j*width + i)).c2);
         break;
       }
     if (j != (height - 1)) fprintf(fstream,"\",\n"); else
                            fprintf(fstream,"\"\n"); 
   }

   if (fprintf(fstream, "} ;\n") != 4)     /* enough disk space for it all? */
     RETURN (PixmapOpenFailed);

   fclose(fstream);
   unlink(backupname);
   rename(filename, backupname);
   if (rename(tmpname, filename))
     RETURN (PixmapOpenFailed);

   RETURN (PixmapSuccess) ;
}

/****[ UTILITIES ]************************************************************
 * following routines are used in XReadPixmapFile() function 
 *****************************************************************************/

/*
 * read the next line and jump blank lines 
 */
static char *
getline(s,pF)
     char * s ;
     FILE * pF ;
{
    s = fgets(s,MAX_LINE_LENGTH,pF);

    while (s) {
	int len = strlen(s);
	if (len && s[len-1] == '\015')
	    s[--len] = '\0';
	if (len==0) s = fgets(s,MAX_LINE_LENGTH,pF);
	else break;
    }
    return(s);
}
	    

/*
 * fatal message : return code, no exit 
 */
static int fatal(msg, p1, p2, p3, p4)
    char *msg;
{
    fprintf(stderr,"\n");
    fprintf(stderr, msg, p1, p2, p3, p4);
    fprintf(stderr,"\n");
    return PixmapFileInvalid ;
}


static char *
StripName(name)
char *name;
{
	char	*begin = rindex(name, '/');
	char	*end, *result;
	int	length;

	begin = (begin ? begin+1 : name);
	end = index(begin, '.');    /* change to rindex to allow longer names */	length = (end ? (end - begin) : strlen(begin));
	result = (char *) XtMalloc(length + 1);
	if (result != NULL) {
		strncpy(result, begin, length);
		result[length] = '\0';
	}
	return (result);
}

static char *
BackupName(name)
char *name;
{
	int	length = name ? strlen(name) : 0;
	char	*result = (char *) XtMalloc(length + 2);

	if (result != NULL) {
		strncpy(result, name, length);
		result[length] = '~';
		result[length+1] = '\0';
	}
	return (result);
}

static char *
TmpName(name)
char *name;
{
	char	*stop = rindex(name, '/');
	char	*tmp = "_tmp_pixmap_";
	int	pathname_length = (stop ? (stop - name + 1) : 0);
	int	tmp_length = strlen(tmp);
	int	result_length = pathname_length + tmp_length;
	char	*result = (char *) XtMalloc(result_length + 1);

	if (result != NULL) {
		strncpy(result, name, pathname_length);
		strncpy(result+pathname_length, tmp, tmp_length);
		result[result_length] = '\0';
	}
	return (result);
}

/****<eof>********************************************************************/
