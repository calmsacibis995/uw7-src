#ident	"@(#)r5fonts:clients/bdftosnf/bdftosnf.c	1.1"
/*copyright	"%c%"*/

/* $XConsortium: bdftosnf.c,v 1.25 88/10/10 19:37:37 rws Exp $ */

#include <stdio.h>
#include <X11/Xos.h>
#include <errno.h> 
#include <ctype.h> 
/* #include <malloc.h> */	extern char *malloc(), *realloc();

#include "misc.h"
#include <X11/X.h>
#include <X11/Xproto.h>
#include "fontstruct.h"
#include "snfstruct.h"
#include "font.h"

#include "bdftosnf.h"	/* used by converters only */

int byteswap = 0;

#define INDICES 256
#define MAXENCODING 0xFFFF

extern char *gets(), *index();
extern void bitorderinvert(), twobyteinvert(), fourbyteinvert();

static char *myname;	/* initialized from argv[0] */
static char *currentFile = NULL;

int linenum = 0;	/* for error messages */
int badbitswarn = 0;	/* warn about bits outside bbox */
int ignoredcharwarn = 0;	/* warn about ignored characters */
int makeTEfonts = 0;	/* pad glyphs to bounding box for TE capable fonts */
int inhibitInk = 0;	/* don't compute ink metrics for TE fonts */

int glyphPad = DEFAULTGLPAD;
int bitorder = DEFAULTBITORDER;
int scanunit = DEFAULTSCANUNIT;
int byteorder = DEFAULTBYTEORDER;

/*
/*
 *	Invert byte order within each WORD of an array.
 *	"In" and "out" may point to the same array.
 *
 *	Current implementation requires that glyphPad be 4, though this
 *	is probably not strictly required with some work.
 */
void
byteorderinvert( in, out, nwords)
    register BITS32	*in;
    register BITS32	*out;
    register int	nwords;
{
	if (glyphPad != 4) {
	fprintf(stderr, "Error: Can't byte-swap if GLYPHPADBYTES != 4\n");
	return;
    }

    while ( nwords--) {
	*out++ = lswapl( *in);	/* NOTE: lswapl() is a macro found in misc.h */
	in++;
    }
}


/*
 * read the next line and keep a count for error messages
 */
char *
getline(s)
    char *s;
{
    s = gets(s);
    linenum++;
    while (s) {
	int len = strlen(s);
	if (len && s[len-1] == '\015')
	    s[--len] = '\0';
	if ((len==0) || prefix(s, "COMMENT")) {
	    s = gets(s);
	    linenum++;
	} else break;
    }
    return(s);
}

static char **strings = NULL;
static unsigned str_index = 0;
static unsigned strings_size = 0;

unsigned
intern(s)
    char *s;
{
    unsigned i;

    if ((i = str_index) >= strings_size) {
	if (strings_size) {
	    strings_size <<= 1;
	    strings = (char **)realloc((char *)strings,
				       strings_size * sizeof(char *));
	} else {
	    strings_size = 100;
	    strings = (char **)malloc(strings_size * sizeof(char *));
	}
    }
    strings[i] = s;
    str_index++;
    return i;
}

char *
pname(i)
    unsigned i;
{
    return strings[i];
}


/*
 * malloc and copy a string value, and intern it. Handle quoted strings.
 */
unsigned
remember(s)
char *s;
{
    char *p, *pp;

    /* strip leading white space */
    while (*s && (*s == ' ' || *s == '\t'))
	s++;
    if (*s == 0)
    	return intern("");
    if (*s != '"') {
	pp = s;
	/* no white space in value */
	for (pp=s; *pp; pp++)
	    if (*pp == ' ' || *pp == '\t' || *pp == '\015' || *pp == '\n') {
	        *pp = 0;
		break;
	    }
	p = malloc((unsigned)strlen(s)+1);
	strcpy(p, s);
	return intern(p);
    }
    /* quoted string: strip outer quotes and undouble inner quotes */
    s++;
    pp = p = malloc((unsigned)strlen(s)+1);
    while (*s) {
	if (*s == '"') {
	    if (*(s+1) != '"') {
	    	*p++ = 0;
		return intern(pp);
	    } else {
		s++;
	    }
	}
	*p++ = *s++;
    }
    fatal("Property value missing final right quote");
    /*NOTREACHED*/
}

/*
 * return TRUE if str is a prefix of buf
 */
prefix(buf, str)
    char *buf, *str;
{
    return strncmp(buf, str, strlen(str))? FALSE : TRUE;
}

/*
 * return TRUE if strings are equal
 */
streq(a, b)
    char *a, *b;
{
    return strcmp(a, b)? FALSE : TRUE;
}

name_eq(pfp, s)
    FontPropPtr pfp;
    char *s;
{
    return streq(pname((unsigned)pfp->name), s);
}

/*
 * return TRUE if string is a valid integer
 */
Bool
isinteger(str)
    char *str;
{
    char c;

    c = *str++;
    if( !(isdigit(c) || c=='-' || c=='+') )
	return(FALSE);

    while(c = *str++)
	if( !isdigit(c) )
	    return(FALSE);

    return(TRUE);
}

/*
 * make a byte from the first two hex characters in s
 */
unsigned char
hexbyte(s)
    char *s;
{
    unsigned char b = 0;
    register char c;
    int i;

    for (i=2; i; i--) {
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

/*VARARGS*/
warning(msg, p1, p2, p3, p4)
    char *msg, *p1;
{
    fprintf(stderr, "%s: %s: ", myname, currentFile);
    fprintf(stderr, msg, p1, p2, p3, p4);
    if (linenum)
	fprintf(stderr, " at line %d\n", linenum);
    else
	fprintf(stderr, "\n");
}

/*
 * fatal error. never returns.
 */
/*VARARGS*/
fatal(msg, p1, p2, p3, p4)
    char *msg, *p1;
{
    warning(msg, p1, p2, p3, p4);
    exit(1);
}

/*
 * these properties will be generated if not already present.
 */
#define NULLPROP (FontPropPtr)0

FontPropPtr fontProp = NULLPROP;
FontPropPtr pointSizeProp = NULLPROP;
FontPropPtr resolutionProp = NULLPROP;
FontPropPtr xHeightProp = NULLPROP;
FontPropPtr weightProp = NULLPROP;
FontPropPtr quadWidthProp = NULLPROP;
#define GENPROPS 6

BOOL haveFontAscent = FALSE;
BOOL haveFontDescent = FALSE;

/*
 * check for known property values
 */

int
specialproperty(pfp, pfi)
    FontPropPtr pfp;
    FontInfoPtr pfi;
{
    if (name_eq(pfp, "FONT_ASCENT") && !pfp->indirect)
    {
	pfi->fontAscent = pfp->value;
	haveFontAscent = TRUE;
	return 0;
    }
    else if (name_eq(pfp, "FONT_DESCENT") && !pfp->indirect)
    {
	pfi->fontDescent = pfp->value;
	haveFontDescent = TRUE;
	return 0;
    }
    else if (name_eq(pfp, "DEFAULT_CHAR") && !pfp->indirect)
    {
	pfi->chDefault = pfp->value;
	return 0;
    }
    else if (name_eq(pfp, "POINT_SIZE"))
	pointSizeProp = pfp;
    else if (name_eq(pfp, "RESOLUTION"))
	resolutionProp = pfp;
    else if (name_eq(pfp, "X_HEIGHT"))
	xHeightProp = pfp;
    else if (name_eq(pfp, "WEIGHT"))
	weightProp = pfp;
    else if (name_eq(pfp, "QUAD_WIDTH"))
	quadWidthProp = pfp;
    else if (name_eq(pfp, "FONT"))
	fontProp = pfp;
    return 1;
}

computeweight(font)
    TempFont *font;
{
    unsigned int i;
    int width = 0, area, bits = 0;
    register b;
    register unsigned char *p;

    for (i=0; i<n1dChars(font->pFI); i++)
	width += font->pCI[i].metrics.characterWidth;
    area = width*(font->pFI->fontAscent+font->pFI->fontDescent);
    for (i=0,p=font->pGlyphs; i<font->pFI->maxbounds.byteOffset; i++,p++)
    	for (b=(*p); b; b >>= 1)
	    bits += b & 1;
    if (area == 0) return 0;
    return (int)((bits*1000.0)/area);
}

main(argc, argv)
    int		argc;
    char *	argv[];
{
    TempFont	font;
    FontInfoRec	fi;
    CharInfoPtr	cinfos[INDICES];	/* rows waiting to be allocated */
    int		bytesGlAlloced = 1024;	/* amount now allocated for glyphs
					   (bytes) */
    unsigned char *pGl = (unsigned char *)malloc((unsigned)bytesGlAlloced);
    int		bytesGlUsed = 0;
    int		nGl = 0;
    int		nchars;
    float	pointSize;
    int		xRes, yRes;
    char	linebuf[BUFSIZ];
    char	namebuf[100];
    char	secondbuf[BUFSIZ];
    char	thirdbuf[BUFSIZ];
    char	fontName[100];
    unsigned int attributes;
    int		digitWidths = 0, digitCount = 0, ex = 0;
    int		char_row, char_col;
    int		i;
    CharInfoRec	emptyCharInfo;

    myname = argv[0];
    argc--, argv++;
    while (argc--) {
	if (argv[0][0] == '-') {
	    switch (argv[0][1]) {
	    case 'p':
		switch (argv[0][2]) {
		case '1':
		case '2':
		case '4':
		case '8':
		    if (argv[0][3] != '\0')
			goto usage;
		    glyphPad = argv[0][2] - '0';
		    break;
		default:
		    goto usage;
		}
		break;


	    case 's':
		byteswap = 1;
		break;
		
	    case 'u':
		switch (argv[0][2]) {
		case '1':
		case '2':
		case '4':
		    if (argv[0][3] != '\0')
			goto usage;
		    scanunit = argv[0][2] - '0';
		    break;
		default:
		    goto usage;
		}
		break;

	    case 'm':
		if (argv[0][2] != '\0')
		    goto usage;
		bitorder = MSBFirst;
		break;

	    case 'l':
		if (argv[0][2] != '\0')
		    goto usage;
		bitorder = LSBFirst;
		break;

	    case 'M':
		if (argv[0][2] != '\0')
		    goto usage;
		byteorder = MSBFirst;
		break;

	    case 'L':
		if (argv[0][2] != '\0')
		    goto usage;
		byteorder = LSBFirst;
		break;

	    case 'w':
		if (argv[0][2] != '\0')
		    goto usage;
		badbitswarn = 1;
		break;

	    case 'W':
		if (argv[0][2] != '\0')
		    goto usage;
		ignoredcharwarn = 1;
		break;

	    case 't':	/* attempt to make terminal fonts if possible */
		if (argv[0][2] != '\0')
		    goto usage;
		makeTEfonts = 1;
		break;

	    case 'i':	/* inhibit ink metric computation */
		if (argv[0][2] != '\0')
		    goto usage;
		inhibitInk = 1;
		break;

	    default:
		goto usage;
		break;
	    }
	} else {
	    if (currentFile)
	    {
	usage:
		fprintf(stderr,
	"usage: %s [-s] [-p#] [-u#] [-m] [-l] [-M] [-L] [-w] [-W] [-t] [-i] [bdf file]\n",
			myname);
		fprintf(stderr,
			"       where # for -p is 1, 2, 4, or 8\n");
		fprintf(stderr,
			"       and   # for -u is 1, 2, or 4\n");
		exit(1);
	    }
	    currentFile = argv[0];
	}
	argv++;
    }
    if (currentFile) {
	if (freopen(currentFile, "r", stdin) == NULL)
	    fatal("could not open file\n");
    } else {
	currentFile = "(stdin)";
    }

    emptyCharInfo.metrics.leftSideBearing = 0;
    emptyCharInfo.metrics.rightSideBearing = 0;
    emptyCharInfo.metrics.ascent = 0;
    emptyCharInfo.metrics.descent = 0;
    emptyCharInfo.metrics.characterWidth = 0;
    emptyCharInfo.byteOffset = 0;
    emptyCharInfo.exists = FALSE;
    emptyCharInfo.pad = 0;
    emptyCharInfo.metrics.attributes = 0;

    for (i = 0; i < INDICES; i++)
	cinfos[i] = (CharInfoPtr)NULL;

    font.pFI = &fi;
    bzero((char*)&fi, sizeof(fi));
    fi.firstRow = INDICES;
    fi.lastRow = 0;
    fi.chFirst = INDICES;
    fi.chLast = 0;
    fi.pixDepth = 1;
    fi.glyphSets = 1;
    fi.chDefault = 0;	/* may be overridden by a property */

    getline(linebuf);

    if ((sscanf(linebuf, "STARTFONT %s", namebuf) != 1) ||
	!streq(namebuf, "2.1"))
	fatal("bad 'STARTFONT'");
    getline(linebuf);

    if (sscanf(linebuf, "FONT %[^\n]", fontName) != 1)
	fatal("bad 'FONT'");
    getline(linebuf);

    if (!prefix(linebuf, "SIZE"))
	fatal("missing 'SIZE'");
    if ((sscanf(linebuf, "SIZE %f%d%d", &pointSize, &xRes, &yRes) != 3))
	fatal("bad 'SIZE'");
    if ((pointSize < 1) || (xRes < 1) || (yRes < 1))
	fatal("SIZE values must be > 0");
    if (xRes != yRes)
        fatal("x and y resolution must be equal");
    getline(linebuf);

    if (!prefix(linebuf, "FONTBOUNDINGBOX"))
	fatal("missing 'FONTBOUNDINGBOX'");
    getline(linebuf);

    if (prefix(linebuf, "STARTPROPERTIES")) {
	int nprops;
	FontPropPtr pfp;

	if (sscanf(linebuf, "STARTPROPERTIES %d", &nprops) != 1)
	   fatal("bad 'STARTPROPERTIES'");
	fi.nProps = nprops;
	pfp = (FontPropPtr)malloc((unsigned)(nprops+GENPROPS) *
				  sizeof(FontPropRec));
	font.pFP = pfp;
	getline(linebuf);
	while((nprops-- > 0) && !prefix(linebuf, "ENDPROPERTIES")) {

	    switch (sscanf(linebuf, "%s%s%s", namebuf, secondbuf, thirdbuf) ) {

	    case 1: /* missing required parameter value */
		fatal("missing '%s' parameter value",namebuf);
		break;

	    case 2:
		/*
		 * Possibilites include:
		 * valid quoted string with no white space
	         * valid integer value 
		 * invalid value
		 */
		if( secondbuf[0] == '"'){
		    pfp->indirect = TRUE;
		    pfp->value = (INT32)remember(linebuf+strlen(namebuf));
		    break;
		} else if( isinteger(secondbuf) ){
		    pfp->indirect = FALSE;
		    pfp->value = atoi(secondbuf);
		    break;
		} else {
		    fatal("invalid '%s' parameter value",namebuf);
		    break;
		}

	    case 3:
		/* 
		 * Possibilites include:
		 * valid quoted string with some white space
		 * invalid value (reject even if second string is integer)
		 */
		if( secondbuf[0] == '"'){
		    pfp->indirect = TRUE;
		    pfp->value = (INT32)remember(linebuf+strlen(namebuf));
		    break;
		} else {
		    fatal("invalid '%s' parameter value",namebuf);
		    break;
		}
	    }
	    pfp->name = (CARD32)remember(namebuf);
	    if (specialproperty(pfp, &fi))
	        pfp++;
	    else
		fi.nProps--;
	    getline(linebuf);
	}
	if (!prefix(linebuf, "ENDPROPERTIES"))
	    fatal("missing 'ENDPROPERTIES'");
	if (!haveFontAscent || !haveFontDescent)
	    fatal("must have 'FONT_ASCENT' and 'FONT_DESCENT' properties");
	if (nprops != -1)
	    fatal("%d too few properties", nprops+1);
	if (!fontProp) {
	    fi.nProps++;
	    pfp->name = (CARD32)intern("FONT");
	    pfp->value = (INT32)intern(fontName);
	    pfp->indirect = TRUE;
	    fontProp = pfp++;
	}
	if (!pointSizeProp) {
	    fi.nProps++;
	    pfp->name = (CARD32)intern("POINT_SIZE");
	    pfp->value = (INT32)(pointSize*10.0);
	    pfp->indirect = FALSE;
	    pointSizeProp = pfp++;
	}
	if (!weightProp) {
	    fi.nProps++;
	    pfp->name = (CARD32)intern("WEIGHT");
	    pfp->value = -1;	/* computed later */
	    pfp->indirect = FALSE;
	    weightProp = pfp++;
	}
	if (!resolutionProp) {
	    fi.nProps++;
	    pfp->name = (CARD32)intern("RESOLUTION");
	    pfp->value = (INT32)((xRes*100.0)/72.27);
	    pfp->indirect = FALSE;
	    resolutionProp = pfp++;
	}
	if (!xHeightProp) {
	    fi.nProps++;
	    pfp->name = (CARD32)intern("X_HEIGHT");
	    pfp->value = -1;	/* computed later */
	    pfp->indirect = FALSE;
	    xHeightProp = pfp++;
	}
	if (!quadWidthProp) {
	    fi.nProps++;
	    pfp->name = (CARD32)intern("QUAD_WIDTH");
	    pfp->value = -1;	/* computed later */
	    pfp->indirect = FALSE;
	    quadWidthProp = pfp++;
	}
    } else { /* no properties */
	fatal("missing 'STARTPROPERTIES'");
    }
    getline(linebuf);

    if (sscanf(linebuf, "CHARS %d", &nchars) != 1)
	fatal("bad 'CHARS'");
    if (nchars < 1)
	fatal("invalid number of CHARS");
    getline(linebuf);

    while ((nchars-- > 0) && prefix(linebuf, "STARTCHAR"))  {
        int	t;
	int	ix;	/* counts bytes in a glyph */
	int	wx;	/* x component of width */
	int	wy;	/* y component of width */
	int	bw;	/* bounding-box width */
	int	bh;	/* bounding-box height */
	int	bl;	/* bounding-box left */
	int	bb;	/* bounding-box bottom */
	int	enc, enc2;	/* encoding */
	char	*p;	/* temp pointer into linebuf */
	int	bytesperrow, row, hexperrow, perrow, badbits;
	char	charName[100];

	if (sscanf(linebuf, "STARTCHAR %s", charName) != 1)
	    fatal("bad character name");

	getline( linebuf);
	if ((t=sscanf(linebuf, "ENCODING %d %d", &enc, &enc2)) < 1)
	    fatal("bad 'ENCODING'");
	if ((enc < -1) || ((t == 2) && (enc2 < -1)))
	    fatal("bad ENCODING value");
	if (t == 2 && enc == -1)
	    enc = enc2;
	if (enc == -1) {
	    if (ignoredcharwarn) warning("character '%s' ignored\n", charName);
	    do {
	    	char *s = getline(linebuf);
		if (!s)
		    fatal("Unexpected EOF");
	    } while (!prefix(linebuf, "ENDCHAR"));
	    getline(linebuf);
	    continue;
	}
	if (enc > MAXENCODING)
	    fatal("character '%s' has encoding(=%d) too large", charName, enc);
	char_row = (enc >> 8) & 0xFF;
	char_col = enc & 0xFF;
	fi.firstRow = MIN(fi.firstRow, char_row);
	fi.lastRow = MAX(fi.lastRow, char_row);
	fi.chFirst = MIN(fi.chFirst, char_col);
	fi.chLast = MAX(fi.chLast, char_col);
	if (!cinfos[char_row])
	{
	    cinfos[char_row] =
		(CharInfoPtr)malloc(sizeof(CharInfoRec)*INDICES);
	    bzero((char *)cinfos[char_row], sizeof(CharInfoRec)*INDICES);
	}

	getline( linebuf);
	if (sscanf( linebuf, "SWIDTH %d %d", &wx, &wy) != 2)
	    fatal("bad 'SWIDTH'");
	if (wy != 0)
	    fatal("SWIDTH y value must be zero");

	getline( linebuf);
	if (sscanf( linebuf, "DWIDTH %d %d", &wx, &wy) != 2)
	    fatal("bad 'DWIDTH'");
	if (wy != 0)
	    fatal("DWIDTH y value must be zero");

	getline( linebuf);
	if (sscanf( linebuf, "BBX %d %d %d %d", &bw, &bh, &bl, &bb) != 4)
	    fatal("bad 'BBX'");
	if ((bh < 0) || (bw < 0))
	    fatal("character '%s' has a negative sized bitmap, %dx%d", 
	    	  charName, bw, bh);

	getline( linebuf);
	if (prefix(linebuf, "ATTRIBUTES"))
	{
	    for (p = linebuf + strlen("ATTRIBUTES ");
		(*p == ' ') || (*p == '\t');
		p ++)
		/* empty for loop */ ;
	    attributes = hexbyte(p)<< 8 + hexbyte(p+2);
	    getline( linebuf);	/* set up for BITMAP which follows */
	}
	else
	    attributes = 0;
	if (!prefix(linebuf, "BITMAP"))
	    fatal("missing 'BITMAP'");

	/* collect data for generated properties */
	if ((strlen(charName) == 1)){
	    if ((charName[0] >='0') && (charName[0] <= '9')) {
		digitWidths += wx;
		digitCount++;
	    } else if (charName[0] == 'x') {
	        ex = (bh+bb)<=0? bh : bh+bb ;
	    }
	}

	cinfos[char_row][char_col].metrics.leftSideBearing = bl;
	cinfos[char_row][char_col].metrics.rightSideBearing = bl+bw;
	cinfos[char_row][char_col].metrics.ascent = bh+bb;
	cinfos[char_row][char_col].metrics.descent = -bb;
	cinfos[char_row][char_col].metrics.characterWidth = wx;
	cinfos[char_row][char_col].byteOffset = bytesGlUsed;
	cinfos[char_row][char_col].exists = FALSE;  /* overwritten later */
        cinfos[char_row][char_col].pad = 0;
	cinfos[char_row][char_col].metrics.attributes = attributes;

	badbits = 0;
	bytesperrow = GLWIDTHBYTESPADDED(bw,glyphPad);
	hexperrow = (bw + 7) >> 3;
	if (hexperrow == 0) hexperrow = 1;
	for (row=0; row < bh; row++) {
	    getline(linebuf);
	    p = linebuf;
	    t = strlen(p);
	    if (t & 1)
		fatal("odd number of characters in hex encoding");
	    t >>= 1;
	    if ((bytesGlUsed + bytesperrow) >= bytesGlAlloced) {
		bytesGlAlloced = (bytesGlUsed + bytesperrow) * 2;
		pGl = (unsigned char *)realloc((char *)pGl,
					       (unsigned)bytesGlAlloced);
	    }
	    perrow = MIN(hexperrow, t);
	    for ( ix=0; ix < perrow; ix++, p+=2, bytesGlUsed++)
	    {
	        pGl[bytesGlUsed] = hexbyte(p);
	    }
	    if (perrow && (hexperrow <= t) && (bw & 7) &&
		(ix = (pGl[bytesGlUsed-1] & (0xff >> (bw & 7))))) {
		pGl[bytesGlUsed-1] &= ~ix;
		if (badbitswarn)
		    badbits = 1;
	    }
	    if (badbitswarn) {
		for ( ix = perrow; ix < t; ix++, p+= 2) {
		    if (hexbyte(p) != 0) {
			badbits = 1;
			break;
		    }
		}
	    }
	    for ( ix=perrow; ix < bytesperrow; ix++, bytesGlUsed++)
	    {
	        pGl[bytesGlUsed] = 0;
	    }
	}
	if (badbits)
	    warning("character '%s' has bits outside bounding box ignored",
		    charName);
	getline( linebuf);
	if (!prefix(linebuf, "ENDCHAR"))
            fatal("missing 'ENDCHAR'");
	nGl++;
	getline( linebuf);		/* get STARTCHAR or ENDFONT */
    }

    if (nchars != -1)
        fatal("%d too few characters", nchars+1);
    if (prefix(linebuf, "STARTCHAR"))
	fatal("more characters than specified");
    if (!prefix(linebuf, "ENDFONT"))
        fatal("missing 'ENDFONT'");
    if (nchars != -1)
        fatal("%d too few characters", nchars+1);
    if (nGl == 0)
        fatal("No characters with valid encodings");

    fi.maxbounds.byteOffset = bytesGlUsed;
    font.pGlyphs = pGl;

    font.pCI = (CharInfoPtr)malloc(sizeof(CharInfoRec)*n2dChars(font.pFI));
    i = 0;
    for (char_row = fi.firstRow; char_row <= fi.lastRow; char_row++)
    {
	if (!cinfos[char_row])
	    for (char_col = fi.chFirst; char_col <= fi.chLast; char_col++)
		{
		font.pCI[i] = emptyCharInfo;
		i++;
		}
	else
	    for (char_col = fi.chFirst; char_col <= fi.chLast; char_col++)
		{
		font.pCI[i] = cinfos[char_row][char_col];
		i++;
		}
    }
    computeNaccelerators(&font, makeTEfonts, inhibitInk, glyphPad);

    bytesGlUsed = fi.maxbounds.byteOffset;
    pGl = font.pGlyphs;

    /* generate properties */
    if (xHeightProp && (xHeightProp->value == -1))
        xHeightProp->value = ex? ex : fi.minbounds.metrics.ascent;
    if (quadWidthProp && (quadWidthProp->value == -1))
        quadWidthProp->value = digitCount?
	    (INT32)((float)digitWidths/(float)digitCount) :
	    (fi.minbounds.metrics.characterWidth+fi.maxbounds.metrics.characterWidth)/2;
    if (weightProp && (weightProp->value == -1))
        weightProp->value = computeweight(&font);

    if (bitorder == LSBFirst)
	bitorderinvert(pGl, bytesGlUsed);
    if (bitorder != byteorder) {
	if (scanunit == 2)
	    twobyteinvert(pGl, bytesGlUsed);
	else if (scanunit == 4)
	    fourbyteinvert(pGl, bytesGlUsed);
    }
    if (byteswap)
	byteorderinvert((BITS32 *) pGl, (BITS32 *) pGl,bytesGlUsed >>2);
    WriteNFont(stdout, &font, pname);
    exit(0);
}

