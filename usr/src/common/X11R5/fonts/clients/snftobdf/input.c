#ident	"@(#)r5fonts:clients/snftobdf/input.c	1.1"
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

extern char *program;

void
invertbits(buf, count)
unsigned char *buf;
int count;
{
    int i, n, m;
    unsigned char c, oc;

    for ( i = 0; i < count; i++ ) {
        c = buf[i];
        oc = 0;
        for (n=0, m=7; n < 8; n++, m--) {
            oc |= ((c >> n) & 1) << m;
        }
        buf[i] = oc;
    }
}

void
invert2(buf, count)
unsigned char *buf;
int count;
{
    int i;
    unsigned char c;

    for ( i = 0; i < count; i += 2) {
        c = buf[i];
        buf[i] = buf[i+1];
        buf[i+1] = c;
    }
}

void
invert4(buf, count)
unsigned char *buf;
int count;
{
    int i;
    unsigned char c;

    for (i = 0; i < count; i += 4) {
        c = buf[i];
        buf[i] = buf[i+3];
        buf[i+3] = c;
        c = buf[i+1];
        buf[i+1] = buf[i+2];
        buf[i+2] = c;
    }
}

TempFont *
GetSNFInfo(name, bitOrder, byteOrder, scanUnit)
char *name;
int bitOrder, byteOrder, scanUnit;
{
    int fd, tot, strings, i, glyphbytes;
    char *buf;
    struct stat st;
    TempFont *tf;

    if (stat(name, &st) < 0) {
        fprintf(stderr, "%s: problem with file %s\n", program, name);
        exit(1);
    }

    if ((fd = open(name, O_RDONLY)) < 0) {
        fprintf(stderr, "%s: can't open file %s\n", program, name);
        exit(1);
    }

    if (!(tf = (TempFont *)malloc(sizeof(TempFont)))) {
        fprintf(stderr, "BAD MALLOC\n");
        close(fd);
        exit(1);
    }

    if (!(buf = (char *)malloc(st.st_size))) {
        fprintf(stderr, "BAD MALLOC\n");
        close(fd);
        exit(1);
    }

    if ((tot = read(fd, buf, st.st_size)) < st.st_size) {
        fprintf(stderr, "%s: read problem on font %s\n", program, name);
        close(fd);
        exit(1);
    }
    close(fd);

    tf->pFI = (snfFontInfoPtr)buf;
    if (tf->pFI->version1 != FONT_FILE_VERSION ||
        tf->pFI->version2 != FONT_FILE_VERSION)
      fprintf(stderr, "%s: %s is old style BDF\n", program, name);
    i = BYTESOFFONTINFO(tf->pFI);
    tf->pCI = (snfCharInfoPtr)(buf + i);
    i = BYTESOFCHARINFO(tf->pFI);
    tf->pGlyphs = ((unsigned char *)(tf->pCI)) + i;
    glyphbytes = BYTESOFGLYPHINFO(tf->pFI);
    tf->pFP = (snfFontPropPtr)(tf->pGlyphs + glyphbytes);
    strings = (int)tf->pFP + BYTESOFPROPINFO(tf->pFI);
    if (tf->pFI->inkMetrics) {
        tf->pInkMin = (CharInfoPtr)(strings + BYTESOFSTRINGINFO(tf->pFI));
        tf->pInkMax = tf->pInkMin + 1;
        tf->pInkCI  = tf->pInkMax + 1;
    }
    for (i = 0; i < tf->pFI->nProps; i++) {
        tf->pFP[i].name += strings;
        if (tf->pFP[i].indirect)
          tf->pFP[i].value += strings;
    }

    if (bitOrder == LSBFirst)
      invertbits(tf->pGlyphs, glyphbytes);
    if (bitOrder != byteOrder) {
        if (scanUnit == 2)
          invert2(tf->pGlyphs, glyphbytes);
        else if (scanUnit == 4)
          invert4(tf->pGlyphs, glyphbytes);
    }
        
    return(tf);
}

unsigned char *
GetSNFBitmap(tf, charnum, glyphPad)
TempFont *tf;
unsigned int charnum;
int glyphPad;
{
    snfCharInfoPtr pCI = (snfCharInfoPtr)tf->pCI;
    int ht, wd, gbp, wid, bytes, i, j = 0, count = 0;
    unsigned char *bitmaps, *bmap, *ptr;

    ht = pCI[charnum].metrics.descent +
         pCI[charnum].metrics.ascent;
    if (ht == 0)
      ht = 1;

    wd = pCI[charnum].metrics.rightSideBearing -
         pCI[charnum].metrics.leftSideBearing;
    if (wd == 0)
      wd = pCI[charnum].metrics.characterWidth;

    gbp = GLWIDTHBYTESPADDED(wd, glyphPad);
    wid = ((wd / 8) + ((wd % 8) ? 1 : 0));
    bytes = ht * wid;

    ptr = bitmaps = (unsigned char *)tf->pGlyphs + pCI[charnum].byteOffset;
    bmap = (unsigned char *)malloc(sizeof(unsigned char) * bytes);

    if (!pCI[charnum].exists || ReallyNonExistent(pCI[charnum]))
      return(NULL);

    for (i = 0; i < ht; i++) {
        for (j = 0; j < wid; j++)
          bmap[count++] = *ptr++;
        ptr += (gbp - wid);
    }
    return(bmap);
}
