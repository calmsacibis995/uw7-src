#ident	"@(#)r5fonts:clients/snftobdf/snftobdf.c	1.1"
/*

Copyright 1991 by Mark Leisher (mleisher@nmsu.edu)

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  I make no representations about the
suitability of this software for any purpose.  It is provided "as is"
without express or implied warranty.

*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include "snftobdf.h"

char *program;

void
usage()
{
    fprintf (stderr, "usage:  %s [-options ...] snffile ...\n\n", program);
    fprintf (stderr, "where options include:\n");
    fprintf (stderr, "\t-m\t\tset bit order to Most Significant Bit First\n");
    fprintf (stderr, "\t-l\t\tset bit order to Least Significant Bit First\n");
    fprintf (stderr, "\t-M\t\tset byte order to Most Significant Byte First\n");
    fprintf (stderr, "\t-L\t\tset byte order to Least Significant Byte First\n");
    fprintf (stderr, "\t-p#\t\tset glyph padding to #\n");
    fprintf (stderr, "\t-u#\t\tset scanline unit to #\n\n");
    exit (1);
}

main(argc, argv)
int argc;
char **argv;
{
    char *name;
    TempFont *tf;
    int glyphPad  = DEFAULTGLPAD,
        bitOrder  = DEFAULTBITORDER,
        byteOrder = DEFAULTBYTEORDER,
        scanUnit  = DEFAULTSCANUNIT;

    program = argv[0];

    argc--;
    *argv++;
    while(argc) {
        if (argv[0][0] == '-') {
            switch(argv[0][1]) {
              case 'm': bitOrder = MSBFirst; break;
              case 'l': bitOrder = LSBFirst; break;
              case 'M': byteOrder = MSBFirst; break;
              case 'L': byteOrder = LSBFirst; break;
              case 'p': glyphPad = atoi(argv[0] + 2); break;
              case 'u': scanUnit = atoi(argv[0] + 2); break;
              default: usage(); break;
            }
        } else if (!argv[0][0])
          usage();
        else {
            tf = GetSNFInfo(argv[0], bitOrder, byteOrder, scanUnit);
            BDFHeader(tf, argv[0]);
            BDFBitmaps(tf, glyphPad);
            BDFTrailer(tf);
        }
        argc--;
        *argv++;
    }
}
