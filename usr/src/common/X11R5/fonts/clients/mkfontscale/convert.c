#ident	"@(#)r5fonts:clients/mkfontscale/convert.c	1.1"

/*
 * Module:     dtadmin:convert   Graphical Administration of Fonts
 * File:       convert.c
 */

/*
 * Copyright (C) 1987-1991 by Adobe Systems Incorporated.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notices appear in all copies and that
 * both those copyright notices and this permission notice appear in
 * supporting documentation and that the name of Adobe Systems
 * Incorporated not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  If any portion of this software is changed, it cannot be
 * marketed under Adobe's trademarks and/or copyrights unless Adobe, in
 * its sole discretion, approves by a prior writing the quality of the
 * resulting implementation.
 * 
 * ADOBE MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THE SOFTWARE FOR
 * ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
 * ADOBE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT OF THIRD PARTY RIGHTS.  IN NO EVENT SHALL ADOBE BE LIABLE
 * TO YOU OR ANY OTHER PARTY FOR ANY SPECIAL, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE, STRICT LIABILITY OR ANY OTHER ACTION ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.  ADOBE WILL NOT
 * PROVIDE ANY TRAINING OR OTHER SUPPORT FOR THE SOFTWARE.
 * 
 * PostScript, Display PostScript, and Adobe are trademarks of Adobe Systems
 * Incorporated registered in the U.S.A. and other countries.
 *
 * Author: Adobe Systems Incorporated
 */


/*
 * This program is a UNIX filter that expands an Adobe IBM PC font file into
 * ASCII that can be saved and loaded into the Unix PostScript interpreter.
 * 
 * usage: ibmpsfont filename
 *
 * The IBM file format is a sequence of segments, each of which has a header
 * part and a data part.  The header format, defined in struct Segment below,
 * consists of a one-byte sanity check number (128) then a one byte segment
 * type and finally a four byte length field for the data following data.
 * The length field is stored in the file with the least significant byte
 * first.
 * 
 * The segment types are:
 *	1. The data is a sequence of ASCII character.
 *	2. The data is a sequence of binary characters to be converted to
 *	   a sequence of pairs of hexadecimal digits.
 *	3. The last segment in the file.  This segment has no length or data
 *	   fields.
 *
 * The segment types are defined explicitly rather than as an enumerated type
 * because the values for each type are defined by the file format rather than
 * the compiler manipulating them.
 */

#include <stdio.h>

#define	TRUE	 1
#define	FALSE	 0

#define	CHECK_BYTE	128

#define	ASCII_SEGMENT	1	/* Pure text (newlines converted) */
#define	BINARY_SEGMENT	2	/* Data converted to hex (newlines inserted) */
#define	END_SEGMENT	3	/* Last segment, no length or data */

struct Segment {
  unsigned char Check,		/* Must be CHECK_BYTE */
          Type;
  long    Length;
};

static    FILE *in_file, *out_file;

/*
 * BinaryToHex converts a value in the range [0,15] to a hexadecimal digit
 * in 0-9, a-f.  This operation assumes its argument is in the input range.
 */

static char conversionTable[] = "0123456789abcdef";

#define	BinaryToHex(c)	(conversionTable[(c) & 0xF])

static unsigned char GetByte ()
 /*
  * Reads a single byte from in_file.
  *
  * pre:  Expecting more data on input stream.
  * post: RESULT = next input byte.
  *
  * error:  returns after reporting an error when no more input available.
  */
{
  register int RESULT;

  RESULT = getc( in_file);

  if (RESULT == EOF) {
    fprintf (stderr, "Mkfontscale: Error:  Premature end of input.\n");
    return -1;
  } else
    return (RESULT);
}

BinaryToAscii(in_file_name, out_file_name, header_only)
  char *in_file_name, *out_file_name;
  int header_only;
 /*
  * Process the input stream by repeating the cycle of reading a segment
  * header and then the data, translating the data on output as necessary.
  */
{
    struct Segment Header;
    register unsigned char C;
    register int Count,		/* Count for length field and data field */
          Low_Nibble,		/* Least significant four bits of C */
          High_Nibble;		/* Most significant four bits of C */
    long    Multiplier;		/* Multilier for building length field */
    int success = TRUE;

    if ((in_file = fopen( in_file_name, "r")) == NULL)
	return FALSE;
    if ((out_file = fopen( out_file_name, "w")) == NULL) {
	fclose( in_file);
	return FALSE;
    }

  do {

    /*
     * Read segment header. 
     */

    Header.Check = GetByte ();

    if (Header.Check != CHECK_BYTE) {
      fprintf (stderr, "Mkfontscale: Error: Invalid segment header.  (Check byte 0x%02x)\n", Header.Check);
      fclose( in_file);
      fclose( out_file);
      success = FALSE;
      return success;
    }

    Header.Type = GetByte ();

    Header.Length = 0;

    /*
     * Read the segment length, least significant byte first, by multiplying
     * each byte by increasing powers of 256 and adding them into the total. 
     */

    Multiplier = 1;		/* 256 to the power 0 */

    if (Header.Type != END_SEGMENT) {
      for (Count = 3; Count >= 0; Count--) {
        /* Assert: Multiplier == 256 to the power (3 - Count) */
	Header.Length += GetByte () * Multiplier;
	Multiplier *= 256;
      }
    }

    /*
     * Read and translate data part of segment 
     */

    switch (Header.Type) {
     case ASCII_SEGMENT:
      while (Header.Length-- > 0) {
	C = GetByte ();

	if (C == '\r')		/* Convert newline convention */
	  putc ('\n', out_file);
	else
	  putc (C, out_file);
      }

      break;

     case BINARY_SEGMENT:
      for (Count = 1; Count <= Header.Length; Count++) {
	C = GetByte ();

	High_Nibble = C >> 4;	/* Most significant four bits  */
	Low_Nibble = C & 0xF;	/* Least significant four bits */

	putc (BinaryToHex (High_Nibble), out_file);
	putc (BinaryToHex (Low_Nibble), out_file);

	if ((Count % 32) == 0) {/* Insert newline for readability */
	  C = '\n';		/* Remember for last time through */
	  putc (C, out_file);
	}
      }

      if (C != '\n')		/* Finish last line if necessary. */
	putc ('\n', out_file);

      break;

     case END_SEGMENT:
      break;

     default:
      fprintf (stderr, "Mkfontscale: Error: Invalid segment type %d.\n", Header.Type);
      success = FALSE;      
    }
  } while (Header.Type != END_SEGMENT && success);

    fclose( in_file);
    fclose( out_file);
    return success;
}
