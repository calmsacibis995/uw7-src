#ident	"@(#)r5fonts:clients/bdftosnf/showsnf.c	1.1"
/*copyright	"%c%"*/


#include <stdio.h>
#include <X11/Xos.h>
#include <sys/stat.h> 
#include <errno.h> 
/* #include <malloc.h> */	extern char *malloc(), *realloc();

#include "misc.h"
#include <X11/Xmd.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include "fontstruct.h"
#include "snfstruct.h"
#include "font.h"

#include "bdftosnf.h"	/* used by converters only */
char *program;

int byteswap = 0 ;		/* bo byte-swapping */
extern void bitorderinvert(), twobyteinvert(), fourbyteinvert();

int glyphPad = DEFAULTGLPAD;
int bitorder = DEFAULTBITORDER;
int byteorder = DEFAULTBYTEORDER;
int scanunit = DEFAULTSCANUNIT;

main(argc, argv)
	int	argc;
	char	**argv;
{
	int	verbose = 0,
		gverbose = 0,
 		fontcnt = 0;

	program = *argv;
	for (argc--, argv++; argc; argc--, argv++) {
		if (argv[0][0] == '-')
			switch(argv[0][1]) {
			case 'g':	gverbose++; break;
			case 'v':	verbose++; break;
			case 'm':	bitorder = MSBFirst; break;
			case 'l':	bitorder = LSBFirst; break;
			case 's':	byteswap = 1; break;
			case 'M':	byteorder = MSBFirst; break;
			case 'L':	byteorder = LSBFirst; break;
			case 'p':	glyphPad = atoi (argv[0] + 2);break;
			case 'u':	scanunit = atoi (argv[0] + 2);
					break;
			default:	usage(); break;
			}
		else {
			showfont(argv[0], verbose, gverbose);
			fontcnt++;
		}
	}
	if (!fontcnt)
		usage();
	exit(0);
}

usage()
{
    fprintf (stderr, "usage:  %s [-options ...] snffile ...\n\n", program);
    fprintf (stderr, "where options include:\n");
    fprintf (stderr,
	"    -v            print character bearings and sizes\n");
    fprintf (stderr,
	"    -g            print character glyph bitmaps\n");
    fprintf (stderr,
	"    -m            set bit order to Most Significant Bit First\n");
    fprintf (stderr,
	"    -l            set bit order to Least Significant Bit First\n");
    fprintf (stderr,
	"    -s		   do byte swapping \n");
    fprintf (stderr,
	"    -M            set byte order to Most Significant Byte First\n");
    fprintf (stderr,
	"    -L            set byte order to Least Significant Byte First\n");
    fprintf (stderr,
	"    -p#           set glyph padding to #\n");
    fprintf (stderr,
	"    -u#           set scanline unit to #\n");
    fprintf (stderr, "\n");
    exit (1);
}

/*
** Function:	showfont
** Description:
**
** Read in a .snf file and dump font information to standard output.
** 
** Modified by:	Karl Larson
** Date:	8/23/89
** Modification:
**
** Swap font info, character info, properties, and ink metrics before
** dumping font information if running on a 3B2.  This converts the 6386/630
** format .snf file to a format readable by the 3B2.
*/
showfont(file, verbose,gverbose)
	char	*file;
	int	verbose;
	int 	gverbose;
{
	char	*buf;
	struct stat	st;
	TempFont	tf;
	FontInfoRec f;
	int	fd, i, strings;
	int bytesGlUsed;
	int total;
#ifdef u3b2
	void SwapFontInfo(), SwapCharInfo(), SwapPropInfo(), SwapInkInfo();
#endif

	if (stat(file, &st) < 0) {
		fprintf(stderr, "can't stat %s\n", file);
		return;
	}

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "can't open %s\n", file);
		return;
	}
	buf = malloc(st.st_size);
	read(fd, buf, st.st_size);
	close(fd);

	total = st.st_size;
	tf.pFI = (FontInfoPtr)buf;
#ifdef u3b2
	SwapFontInfo(&tf);
#endif
	i = BYTESOFFONTINFO(tf.pFI);
	total -= i;
	if (total < 0) goto badSNF;
	if (tf.pFI->version1 != FONT_FILE_VERSION ||
	    tf.pFI->version2 != FONT_FILE_VERSION)
	    goto badSNF;
	tf.pCI = (CharInfoPtr)(buf + i);
#ifdef u3b2
	SwapCharInfo(&tf, n2dChars(tf.pFI));
#endif
	i = BYTESOFCHARINFO(tf.pFI);
	total -= i;
	if (total < 0) goto badSNF;
	tf.pGlyphs = ((unsigned char *)tf.pCI) + i;
	bytesGlUsed = BYTESOFGLYPHINFO(tf.pFI);
	total -= bytesGlUsed;
	if (total < 0) goto badSNF;
	tf.pFP = (FontPropPtr)(tf.pGlyphs + bytesGlUsed);
#ifdef u3b2
	SwapPropInfo(&tf, tf.pFI->nProps);
#endif
	i = BYTESOFPROPINFO(tf.pFI);
	total -= i;
	if (total < 0) goto badSNF;
	strings = (int)tf.pFP + i;
	if (tf.pFI->inkMetrics) {
	    i = BYTESOFSTRINGINFO(tf.pFI);
	    total -= i;
	    if (total < 0) goto badSNF;
	    tf.pInkMin = (CharInfoPtr)(strings + i);
	    tf.pInkMax = tf.pInkMin + 1;
	    tf.pInkCI = tf.pInkMax + 1;
#ifdef u3b2
	    SwapInkInfo(&tf, n2dChars(tf.pFI));
#endif
	}

	for (i=0; i<tf.pFI->nProps; i++) {
		tf.pFP[i].name += strings;
		if (tf.pFP[i].indirect)
			tf.pFP[i].value += strings;
	}
	if (bitorder == LSBFirst)
	    bitorderinvert(tf.pGlyphs, bytesGlUsed);
	if (bitorder != byteorder) {
	    if (scanunit == 2)
		twobyteinvert(tf.pGlyphs, bytesGlUsed);
	    else if (scanunit == 4)
		fourbyteinvert(tf.pGlyphs, bytesGlUsed);
	}
	printf("\n\n-------------  %s  ---------------\n\n", file);
	DumpFont(&tf, glyphPad, verbose,gverbose);
	free(buf);
	return;

badSNF:
	fprintf(stderr, "bad format SNF file: %s\n", file);
	return;
}
