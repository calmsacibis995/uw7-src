#ident	"@(#)loadfont.c	1.5"
/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 * Modification history
 *
 *	L000	5Mar97		rodneyh@sco.com
 *	- Changes for Gemini downloadadble graphics text fonts. Fonts are
 *	  downloaded to the font server
 *	- Added load_plane() function to read a BDF file that can be downloaded
 *	  to the font server, similar to load_font().
 *	- General tidy up, unmarked.
 *	L001	23Jun97		rodneyh@sco.com
 *	- Change load_plane() to verify the eucw input width is one in cases
 *	  where the ENCODING field of the BDF character is only one byte wide.
 *	  This is the case if we are loading a height one plane, ie a line.
 *	  Fix for ul97-17408
 *	L002	24Jun97		rodneyh@sco.com
 *	- L001 should have stampe the row to zero not one. This was initialy
 *	  masked by a bug in the font server driver.
 *	  Still trying to be a fix for ul97-17408
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1989 INTERACTIVE Systems Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */


#include <sys/types.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "pcfont.h"

extern rom_font_t font_map;
extern unsigned char active_map[];		/* Active row bitmap, L000 */
extern int errno;				/* L000 */
extern int output_width;			/* L000 */

int linenum = 0;    /* for error messages */

char *file_name;
char *getline();
unchar hexbyte();
void fatal();

/**
 *  This function has been taken with little change directly from the 
 *  ISC loadfont command.
 *
 *  The purpose of this function is to correctly read a BDF format
 *  font description file, and convert the information into the format
 *  used by the WS_PIO_ROMFONT ioctl.
 *
 *  NOTE:  Most of the BDF format information is ignored - we really
 *         only use the encoding number and the bit map.
 **/
void
load_font(font_file)
char *font_file;
{
	int dummy_x;
	int dummy_y;
    int	num_chars = 0;
    int	nchars;
	int nprops;
	int p_size;		/*  Point size for font  */

    char linebuf[BUFSIZ];
    char namebuf[100];
    char font_name[100];
	unchar *bmap_ptr;
	unchar *bmap_ptr2;
   
    FILE *fp;

	/*  Set file_name for the error routine  */
	file_name = font_file;

	/*  Open the font file for reading  */
    if ((fp = fopen(font_file, "r")) == NULL)
	    fatal("could not open BDF font file");

	/*  Format of BDF font file must start with
	 *      STARTFONT <version>
	 *      FONT <font_size>
	 *      SIZE <x> <y> <z>
	 *      FONTBOUNDINGBOX <a> <b> <c> <d>
	 *      STARTPROPERTIES <n>
	 *         .
	 *         .
	 *      ENDPROPERTIES
	 *      CHARS 256 (usually)
	 *
	 *  Most of this information is ignored, as it is not required for the
	 *  downloaded bitmaps.
	 */
    (void)getline(fp, linebuf);

    if ((sscanf(linebuf, "STARTFONT %s", namebuf) != 1) ||
		strcmp(namebuf, "2.1") != 0)
		fatal("bad 'STARTFONT' in font file");

    (void)getline(fp, linebuf);

    if (sscanf(linebuf, "FONT %[^\n]", font_name) != 1)
		fatal("bad 'FONT' in font file");

    (void)getline(fp, linebuf);

    if (!prefix(linebuf, "SIZE"))
		fatal("missing 'SIZE' in font file");

	/*  Extract the point size from the SIZE parameters  */
	if (sscanf(linebuf, "SIZE %d %d %d", &p_size, &dummy_x, &dummy_y) != 3)
		fatal("SIZE syntax not valid");

	/*  Only 8, 14 and 16 are valid sizes  */
	if (p_size != 8 && p_size != 14 && p_size != 16)
		fatal("Invalid pointsize");

    (void)getline(fp, linebuf);

    if (!prefix(linebuf, "FONTBOUNDINGBOX"))
		fatal("missing 'FONTBOUNDINGBOX' in font file");

    (void)getline(fp, linebuf);

    if (!prefix(linebuf, "STARTPROPERTIES"))
		fatal("missing 'STARTPROPERTIES' in font file");

	if (sscanf(linebuf, "STARTPROPERTIES %d", &nprops) != 1)
	   fatal("bad 'STARTPROPERTIES' in font file");

	(void)getline(fp, linebuf);

	/*  Read in the properties (no processing to be done)  */
	while((nprops-- > 0) && !prefix(linebuf, "ENDPROPERTIES"))
		(void)getline(fp, linebuf);

	if (!prefix(linebuf, "ENDPROPERTIES"))
		fatal("missing 'ENDPROPERTIES' in font file");

	/*  Check that we got as many properties as we were told to expect  */
	if (nprops != -1)
		fatal("%d too few properties in font file", nprops + 1);

    (void)getline(fp, linebuf);

	/*  First really useful bit of information  */
    if (sscanf(linebuf, "CHARS %d", &nchars) != 1)
		fatal("bad 'CHARS'");

	/*  Must be at least one character, otherwise what's the point of 
	 *  this file?
	 */
    if (nchars < 1)
		fatal("invalid number of CHARS");

	/*  Set the number of chars in the font map  */
	font_map.fnt_numchar = nchars;

    (void)getline(fp, linebuf);

	/*  The loop that does the real work of loading in the bit maps  */
    while ((nchars-- > 0) && prefix(linebuf, "STARTCHAR"))
	{
		register int row;	/*  Loop variable for bit-map rows  */
		int i;		/*  General purpose int  */
		int	bw;		/*  bounding-box width  */
		int	bh;		/*  bounding-box height  */
		int	bl;		/*  bounding-box left  */
		int	bb;		/*  bounding-box bottom  */
		int	enc;	/*  encoding value 1  */
		int enc2;	/*  encoding value 2  */

		char char_name[100];

		if (sscanf(linebuf, "STARTCHAR %s", char_name) != 1)
			fatal("bad character name");

		(void)getline(fp, linebuf);

		if ((i = sscanf(linebuf, "ENCODING %d %d", &enc, &enc2)) < 1)
			fatal("bad 'ENCODING'");

		/*  Check for invalid encoding values  */
		if ((enc < -1) || ((i == 2) && (enc2 < -1)))
			fatal("bad ENCODING value");

		if (i == 2 && enc == -1)
			enc = enc2;

		/*  If we don't have a valid encoding value, we print a warning 
		 *  and ignore the character
		 */
		if (enc == -1)
		{
			(void)fprintf(stderr, "pcfont: character '%s' on line %d ignored\n",
				char_name, linenum);

			do
			{
				if (!getline(fp,linebuf))
					fatal("Unexpected EOF");

			} while (!prefix(linebuf, "ENDCHAR"));

			(void)getline(fp, linebuf);
			continue;
		}

		/*  Check that the value is within the legal range  */
		if (enc > MAXENCODING)
			fatal("character '%s' has an encoding (%d) which is too large", char_name, enc);

		/*  Record the character encoding value  */
		font_map.fnt_chars[num_chars].cd_index = enc;

		/*  Skip over the SWIDTH and DWIDTH lines, which we don't need  */
		(void)getline(fp, linebuf);
		(void)getline(fp, linebuf);
		(void)getline(fp, linebuf);

		/*  Extract bitmap parameters  */
		if (sscanf( linebuf, "BBX %d %d %d %d", &bw, &bh, &bl, &bb) != 4)
			fatal("bad 'BBX'");

		/*  Check for invalid parameters  */
		if (bh < 0)
			fatal("character '%s' has invalid sized bitmap, %dx%d", 
				  char_name, bw, bh);

		if (bw != 8)	/*  Only size supported  */
			fatal("character '%s' has invalid bitmap width %d", bw);

		(void)getline(fp, linebuf);

		/*  Check for attributes, which again we ignore  */
		if (prefix(linebuf, "ATTRIBUTES"))
			(void)getline(fp, linebuf);

		if (!prefix(linebuf, "BITMAP"))
			fatal("missing 'BITMAP'");

		/*  Set the pointer for where to store the bitmap  */
		switch (p_size) {
			case 8:
				bmap_ptr =
			 	     font_map.fnt_chars[num_chars].cd_map_8x8;
				break;

			case 14:
				bmap_ptr =
				    font_map.fnt_chars[num_chars].cd_map_8x14;
				break;

			case 16:
				bmap_ptr =
				    font_map.fnt_chars[num_chars].cd_map_8x16;

				bmap_ptr2 =
				    font_map.fnt_chars[num_chars].cd_map_9x16;
				break;
		}

		for (row = 0; row < bh; row++)
		{
			(void)getline(fp,linebuf);

			if ((int)strlen(linebuf) != 2)
				fatal("Illegal number of characters in hex encoding");

			*(bmap_ptr++) = hexbyte(linebuf);

			/*  Fill in 9x16 as well for 16 point size  */
			if (p_size == 16)
				*(bmap_ptr2++) = hexbyte(linebuf);
		}

		(void)getline( fp,linebuf);

		/*  Check for ENDCHAR  */
		if (!prefix(linebuf, "ENDCHAR"))
            fatal("missing 'ENDCHAR'");

		(void)getline(fp, linebuf);
		++num_chars;
	}

    if (nchars != -1)
        fatal("%d too few characters", nchars + 1);

    if (prefix(linebuf, "STARTCHAR"))
		fatal("more characters than specified");

    if (!prefix(linebuf, "ENDFONT"))
        fatal("missing 'ENDFONT'");

	/*  Just for lint  */
    return;
}

/* L000 comment
 *
 * Function skips over new lines and BDF file comments.
 */
char *
getline(FILE *fp, char *s)
{
int len;

    s = fgets(s, 80, fp);
    linenum++;

    while (s){
		len = (int)strlen(s);

		/*
		 * Strip off new line
		 */
		if (len && s[len - 1] == '\n' || s[len - 1] == '\015')
			s[--len] = '\0';
		
		/*
		 * Skip over comments
		 */
		if (len == 0 || prefix(s, "COMMENT")) {
			s = fgets(s, 80, fp);	/*  Grab new line  */
			linenum++;
		}
		else
			break;
    }
    return(s);
}

/*VARARGS*/
void
fatal(msg, p1, p2, p3, p4)
char *msg, *p1;
{
	(void)fprintf(stderr, "pcfont: %s: ", file_name);
	(void)fprintf(stderr, msg, p1, p2, p3, p4);

	/*  Printing the line number doesn't always make sense  */
	if (linenum != 0)
		(void)fprintf(stderr, " at line %d\n", linenum);
	else
		(void)fprintf(stderr, "\n");

	exit(1);
}

/*
 * return TRUE if str is a prefix of buf
 */
prefix(buf, str)
char *buf, *str;
{
    return strncmp(buf, str, (int)strlen(str))? FALSE : TRUE;
}

/*
 * make a byte from the first two hex characters in s
 */
unsigned char
hexbyte(s)
char *s;
{
    int i;

    unsigned char b = 0;
    register char c;

    for (i = 2; i; i--) {
		c = *s++;

		if ((c >= '0') && (c <= '9'))
			b = (b<<4) + (c - '0');
		else if ((c >= 'A') && (c <= 'F'))
			b = (b<<4) + 10 + (c - 'A');
		else if ((c >= 'a') && (c <= 'f'))
			b = (b<<4) + 10 + (c - 'a');
		else
			fatal("bad hex char '%c'", c);
    } 
    return b;
}


/* L000 begin
 *
 * void
 * load_plane(char *font_file, gsd_font_t *ioc_arg)
 *
 * Calling / Exit state.
 * 	Called from pcfont.c/main with the name of the BDF file and a pointer
 *	to the ioc_arg structure. load_plane is responsible for parsing the
 *	file and if valid mallocing memory for the font bitmaps and the active
 *	row bitmap.
 *
 * Description:
 *	Validate the BDF file, then malloc memory for the active row bitmap
 *	and the font bitmaps themselves. We zero the font memory upfront, which
 *	is conveniently the empty bitmap, then drop in the bitmaps as we read
 *	them from the BDF file.
 *
 * Note:
 *	This is ripped off from the load_font code above, individual changes
 *	unmarked.
 * 	Also note that the active row bitmap uses one byte per bit becuase it
 *	is very small and this makes the kernels parsing job easier.
 */
void
load_plane(char *font_file, gsd_font_t *ioc_arg)
{
int dummy_x;
int dummy_y;
int fbox_x, fbox_y;			/* Font bounding box params */
int scrw;				/* Output width for this font */
int bm_size;				/* Size in bytes of a glyph */
size_t max_fsize;			/* Max size of this font in bytes */
size_t fsize;				/* Actual size of font data in bytes */
unsigned int row, column, maxrow = 0;
unsigned int minrow = NUM_COLS;
unsigned int row_size;			/* Size in bytes of a row */
unsigned long bm_data;			/* Value of each BITMAP line */
int num_chars = 0;
int nchars;
int nprops;
int p_size;				/* Point size for font */
int i;

unsigned char *font_data;		/* Ptr to actual font data */
unsigned char *codeset;			/* Ptr to codeset to upload */
unsigned char *glyph;			/* Ptr to glyph data buffer */

char linebuf[BUFSIZ];
char namebuf[100];
char font_name[100];
unchar *bmap_ptr;
unchar *bmap_ptr2;
   
FILE *fp;

	/*
	 * Set file_name for the error routine
	 */
	file_name = font_file;

	/*
	 * Open the font file for reading
	 */
	if ((fp = fopen(font_file, "r")) == NULL)
		fatal("could not open BDF font file");

	/*  Format of BDF font file must start with
	 *      STARTFONT <version>
	 *      FONT <font_size>
	 *      SIZE <x> <y> <z>
	 *      FONTBOUNDINGBOX <a> <b> <c> <d>
	 *      STARTPROPERTIES <n>
	 *         .
	 *         .
	 *      ENDPROPERTIES
	 *      CHARS 256 (usually)
	 *
	 *  Most of this information is ignored, as it is not required for the
	 *  downloaded bitmaps.
	 */
	(void)getline(fp, linebuf);

	if ((sscanf(linebuf, "STARTFONT %s", namebuf) != 1) ||
						strcmp(namebuf, "2.1") != 0)
		fatal("bad 'STARTFONT' in font file");

	(void)getline(fp, linebuf);

	if (sscanf(linebuf, "FONT %[^\n]", font_name) != 1)
		fatal("Invalid FONT keyword");

	printf("pcfont: loading %s\n", font_name);

	(void)getline(fp, linebuf);

	if (!prefix(linebuf, "SIZE"))
		fatal("missing 'SIZE' in font file");

	/*
	 * Extract the point size from the SIZE parameters
	 */
	if (sscanf(linebuf, "SIZE %d %d %d", &p_size, &dummy_x, &dummy_y) != 3)
		fatal("SIZE syntax not valid");

	/*
	 * Only 8, 14 and 16 are valid sizes
	 */
/* RGH - temp remove this for now
	if (p_size != 8 && p_size != 14 && p_size != 16)
		fatal("Invalid pointsize");
*/

	(void)getline(fp, linebuf);

	if (!prefix(linebuf, "FONTBOUNDINGBOX"))
		fatal("missing 'FONTBOUNDINGBOX' in font file");

	/*
	 * Extract the fount bounding box params from linebuf.
	 */
	if(sscanf(linebuf, "FONTBOUNDINGBOX %d %d %d %d",
				&fbox_x, &fbox_y, &dummy_x, &dummy_y) != 4)
		fatal("FONTBOUNDINGBOX syntax not valid");

	/*
	 * We have 8x16 bit bitmaps, and we can only handle upto 2 bitmaps
	 * per glyph.
	 */
	if(fbox_y != 16 || (fbox_x != 8 && fbox_x != 16))
		fatal("Invalid FONTBOUNDINGBOX");

	scrw = fbox_x / 8;			/* Output width of this font */
	bm_size = scrw * BITMAP_SIZE;		/* Number of bytes in a glyph */
	row_size = bm_size * NUM_COLS;		/* Num bytes in a row */

	output_width = scrw;

#ifdef DEBUG_TOOLS

	printf("debug: scrw = %d, bm_size = %d\n", scrw, bm_size);

#endif

	(void)getline(fp, linebuf);

	if (!prefix(linebuf, "STARTPROPERTIES"))
		fatal("missing 'STARTPROPERTIES' in font file");

	if (sscanf(linebuf, "STARTPROPERTIES %d", &nprops) != 1)
	   fatal("bad 'STARTPROPERTIES' in font file");

	(void)getline(fp, linebuf);

	/*
	 * Read in the properties (no processing to be done)
	 */
	while((nprops-- > 0) && !prefix(linebuf, "ENDPROPERTIES"))
		(void)getline(fp, linebuf);

	if (!prefix(linebuf, "ENDPROPERTIES"))
		fatal("missing 'ENDPROPERTIES' in font file");

	/*
	 * Check that we got as many properties as we were told to expect
	 */
	if (nprops != -1)
		fatal("%d too few properties in font file", nprops + 1);

	(void)getline(fp, linebuf);

	/*
	 * First really useful bit of information
	 */
	if (sscanf(linebuf, "CHARS %d", &nchars) != 1)
		fatal("bad 'CHARS'");

	/*  Must be at least one character, otherwise what's the point of 
	 *  this file?
	 */
	if (nchars < 1)
		fatal("invalid number of CHARS");

	/*
	 * Here's where we start to differ significantly from load_font()
	 */

	(void)getline(fp, linebuf);

	/*
	 * Malloc the memory we need here for maximum size font, ie all rows
	 * active. Note that we are assuming that the planes are square, which
	 * is true for EUC.
	 */
	max_fsize = bm_size * NUM_COLS * NUM_COLS;

	if((font_data = (unsigned char *)malloc(max_fsize)) == NULL)
		fatal("Can't malloc %d bytes for font data", max_fsize);

	/*
	 * Zero the memory so we can just drop the defined glyph bitmaps into
	 * place.
	 */
	(void)memset((void *)font_data, 0, max_fsize);

#ifdef DEBUG_TOOLS

	/*
	 * Set to 0xAA for debug so we can tell the difference between an
	 * undefined glyph and the empty bitmap defined in the font server.
	 */
	(void)memset((void *)font_data, 0xAA, max_fsize);

#endif
	/*
	 * The loop that does the real work of loading in the bit maps
	 *
	 * For each glyph in the file, simply drop the bitmap into the correct
	 * place in the font_data array and set the coresponding active row
	 * bitmap byte non-zero. Track the highest used row number so we can
	 * determine the actual size of the font_data to send to the kernel.
	 *
	 * Note that encoding values span the non-printable range which we
	 * don't want to deal with so subtract 0x20 from each row, column value
	 * to save some kernel memory.
	 */
	while ((nchars-- > 0) &&
		prefix(linebuf, "STARTCHAR") && !prefix(linebuf, "ENDFONT")){

		unsigned int enc;

		(void)getline(fp, linebuf);	/* Should be encodeing */

		if(sscanf(linebuf, "ENCODING %d", &enc) != 1)
			fatal("bad ENCONDING");

#ifdef DEBUG_TOOLS

		printf("\n%4.4X", enc);
#endif

		/*
		 * Pull the row and column from the ENCODING field.
		 */
		column = (enc & 0xFF) - 0x20;
		row = ((enc >> 8) & 0xFF) - 0x20;

		/* L001 begin
		 *
		 * If the row is not specified in the ENCODING we error out
		 * unless the euc input width is one, in this case stamp the
		 * row to be zero and carry on. This happens when we are
		 * loading height one planes, ie lines.
		 */
		if(ioc_arg->eucw == 1 && row > NUM_COLS)
			row = 0;				/* L001 end */

		if(row > NUM_COLS || column > NUM_COLS){

			if(row > NUM_COLS)
				fatal("bad row = %d", row);
			else
				fatal("bad column = %d", column);  /* L000 */
		}

		if(row > maxrow)
			maxrow = row;

		if(row < minrow)
			minrow = row;

		active_map[row] += 1;		/* Mark this row active */

		/*
		 * Skip SWIDTH, DWIDTH and and BBX lines
		 */
		(void)getline(fp, linebuf);
		(void)getline(fp, linebuf);
		(void)getline(fp, linebuf);

		/* 
		 * If the next line is ATTRIBUTES we ignore else it should be
		 * BITMAP
		 */
		(void)getline(fp, linebuf);

		if(prefix(linebuf, "ATTRIBUTES"))
			(void)getline(fp, linebuf);
		
		if(!prefix(linebuf, "BITMAP"))
			fatal("missing BITMAP");

		/*
		 * Build a pointer to the appropriate place in the font_data
		 * buffer for this glyph.
		 */
		glyph = font_data + (row * row_size);
		glyph += column * bm_size;

		/*
		 * Now suck the bitmap data from the BDF file and stick at
		 * *glyph
		 * We only go through one interation off BITMAP_SIZE because
		 * of we have scrw != we handle it as we go.
		 */
		for(i=0; i<BITMAP_SIZE; i++){

			(void)getline(fp, linebuf);

			if(strlen(linebuf) > 2*scrw)
				fatal("Overflow");

			bm_data = strtoul(linebuf, (char **)NULL, 16);
#ifdef DEBUG_TOOLS

			printf(">%4.4X<", bm_data);
#endif

			if(scrw == 1){				/* L002 begin */
				/*
				 * There is no high byte.
				 */
				*glyph++ = bm_data & 0xFF;
			}
			else{					/* L002 end */
				/*
				 * High byte goes into bitmap first
				 */
				*glyph++ = (bm_data >> 8) & 0xFF;
				*glyph++ = bm_data & 0xFF;

			}					/* L002 */

		}	/* End for glyph data */

		(void)getline(fp, linebuf);

		if(!prefix(linebuf, "ENDCHAR"))
			fatal("missing ENDCHAR");

		(void)getline(fp, linebuf);

	}	/* End while data in font file */

	if(nchars != -1)
		fatal("%d glyphs missing", nchars +1);

	if(prefix(linebuf, "STARTCHAR") || !prefix(linebuf, "ENDFONT"))
		fprintf(stderr, "pcfont: warning more glyphs than expected"
				"in file %s\n", file_name);

	/*
	 * The kernel expects the uploaded font data to be a plane of sparse
	 * rows, where each active row is fully populated so now we have to
	 * crunch our font data into the compressed format.
	 */
	fsize = ((maxrow - minrow) + 1) * row_size;

	if((codeset = (unsigned char *)malloc(fsize)) == NULL)
		fatal("can't malloc %d bytes for codeset", (int)fsize);

	glyph = codeset;		/* Next row position */

	for(i=0; i<NUM_COLS; i++){

		if(active_map[i]){
			/*
			 * This row has glyphs so we need to copy it.
			 */
			memcpy(glyph, font_data + (i * row_size), row_size);

			glyph += row_size;	/* Next free glyph address */
		}
	}

	ioc_arg->font_addr = (caddr_t)codeset;
	ioc_arg->font_len = fsize;
	ioc_arg->bitmap_addr = (caddr_t)active_map;
	ioc_arg->bitmap_len = BITMAP_LEN;

	return;

}	/* End function load_plane,	L000 end */



