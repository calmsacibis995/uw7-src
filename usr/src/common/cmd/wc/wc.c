/* copyright    "%c%"   */

#ident	"@(#)wc:wc.c	1.5.4.5"
/*
 * COPYRIGHT NOTICE
 * 
 * This source code is designated as Restricted Confidential Information
 * and is subject to special restrictions in a confidential disclosure
 * agreement between HP, IBM, SUN, NOVELL and OSF.  Do not distribute
 * this source code outside your company without OSF's specific written
 * approval.  This source code, and all copies and derivative works
 * thereof, must be returned or destroyed at request. You must retain
 * this notice on any copies which you make.
 * 
 * (c) Copyright 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED 
 */
/*
 * OSF/1 1.2
 */
/*
 * COMPONENT_NAME: (CMDFILES) commands that manipulate files
 *
 * FUNCTIONS: wc
 *
 *
 * (C) COPYRIGHT International Business Machines Corp. 1985, 1989, 1991
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 * 1.11  com/cmd/files/wc.c, cmdfiles, bos320, 9130320 7/2/91 16:44:57
 */

#include        <stdio.h>
#include        <ctype.h>
#include        <locale.h>
#include        <pfmt.h>
#include        <errno.h>
#include        <string.h>
#include        <stdlib.h>
#include	<limits.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<wctype.h>

static int kflg;		/* 'k'/'m' parameter */
static int lflg;		/* 'l' parameter */
static int wflg;		/* 'w' parameter */
static int cflg;		/* 'c' parameter */
static int OSRflg;		/* If OSRCMDS env var set */

static void wcp(long int, long int, long int, long int);

/*
 * NAME: wc [-kmlwc] names
 *                                                                    
 * FUNCTION: Counts the number of lines, words and characters in a file.
 *           -k|-m    counts actual characters, not bytes.
 *           -l       counts lines only.
 *           -w       counts words only.
 *           -c       counts bytes only. 
 */
void
main(int argc, char **argv) {
  unsigned char *p1, *p2;
  unsigned char buf[LINE_MAX + 1];	/* for file/stdin reads */
  char *curp, *cure;	/* current and endpointers in the buffer */

  int bytesread;	/* no. of bytes read from disk */
  int mbcnt;		/* no. of bytes in a character */
  int fd = 0;		/* file descripter, default stdin */
  int leftover;		/*
  			 * no. of bytes left over in the buffer
  			 * that need remaining bytes for conversion
  			 * to wide char 
  			 */

  int filect = 0;	/* file count */
  long wordct;		/* word count */
  long twordct = 0;	/* total word count */
  long linect;		/* line count */
  long tlinect = 0;	/* total line count */
  long bytect;		/* byte count */
  long tbytect = 0;	/* total byte count */
  long charct;		/* actual character count */
  long tcharct = 0;	/* total character count */
  int mb_cur_max;	/*
  		 	 * the maximum no. of bytes for multibyte
  		 	 * characters in the current locale 
  		 	 */
  int mbcodeset = 0;	/*
  			 * Boolean flg to indicate if current code
  			 * set is a multibyte code set. 
  			 */
  wchar_t wc;		/* used for wide char processing */
  int token;
  int errflg = 0;
  int status = 0;
  int c;

  (void) setlocale(LC_ALL, "");
  (void) setcat("uxcore");
  (void) setlabel("UX:wc");

  /* Check if OSRCMDS is set to something */
  if (getenv("OSRCMDS") == NULL)
	OSRflg = 0;
  else
	OSRflg = 1;

  while ((c = getopt(argc, argv, "mklcw")) != EOF) {
  	switch (c) {
  	case 'k':
  	case 'm':
		if (cflg)
			errflg++;
		else
			kflg = 1;
  		break;
  	case 'l':
  		lflg = 1;
  		break;
  	case 'w':
  		wflg = 1;
  		break;
  	case 'c':
		if (kflg)
			errflg++;
		else
			cflg = 1;
  		break;
  	default:
  		errflg++;
  	}
  }

  if (errflg) {
  	(void)pfmt(stderr, MM_ACTION,
  	      ":1016:Usage: wc [-c|-m] [-lw] [file ...]\n");
  	exit(2);
  }
  if (!kflg & !lflg & !wflg & !cflg)
	if (!OSRflg)	/* default UW mode "lwc" */
  		lflg = wflg = cflg = 1;
	else		/* default OSR mode "lwm" */
  		lflg = wflg = kflg = 1;

  mb_cur_max = MB_CUR_MAX;	/*
  				 * max no. of bytes in a multibyte
  				 * char for current locale. 
  				 */

  if (mb_cur_max > 1)
  	mbcodeset = 1;

  do {
  	if (optind < argc) {
  		if ((fd = open(argv[optind], O_RDONLY)) < 0) {
  			(void)pfmt(stderr, MM_ERROR, ":92:Cannot open %s: %s\n",
  			           argv[optind], strerror(errno));
  			status = 2;
  			continue;
  		} else
  			filect++;
  	}
  	p1 = p2 = buf;
  	linect = 0;
  	wordct = 0;
  	bytect = 0;
  	token = 0;
  	charct = 0;

  	/*
  	 * count lines, words and characters but check options before 
  	 * printing 
  	 */

  	if (mbcodeset) {	/* I18N support */
  		leftover = 0;
  		for (;;) {
  			bytesread = read(fd, (char *) buf + leftover, LINE_MAX - leftover);
  			if (bytesread <= 0)
  				break;
  			buf[leftover + bytesread] = '\0';	/* protect partial reads */
  			bytect += bytesread;
  			curp = (char *) buf;
  			cure = (char *) (buf + bytesread + leftover);
  			leftover = 0;
  			for (; curp < cure;) {
  				/*
  				 * convert to wide character 
  				 */
  				mbcnt = mbtowc(&wc, curp, mb_cur_max);
  				if (mbcnt <= 0) {
  					if (mbcnt == 0) {
  						/*
  						 * null string,
  						 * handle exception 
  						 */
  						mbcnt = 1;
  					} else if (cure - curp >= mb_cur_max) {
  						/*
  						 * invalid multibyte,
  						 * handle one byte at a time.
  						 */
  						wc = *(unsigned char *) curp;
  						mbcnt = 1;
  					} else {
  						/*
  						 * needs more data
  						 * from next read 
  						 */
  						leftover = cure - curp;
  						(void)strncpy((char *) buf, curp, leftover);
  						break;
  					}
  				}
  				curp += mbcnt;
  				charct++;

  				/*
  				 * count real characters 
  				 */
  				if (!iswspace(wc)) {
  					if (!token) {
  						wordct++;
  						token++;
  					}
  					continue;
  				}
  				token = 0;
  				if (wc == L'\n')
  					linect++;
  			}	/* end wide char conversion */
  		}	/* end read more bytes */
  	} else {	
  		/* 
  		 * single byte support 
  		 */
  		for (;;) {
  			if (p1 >= p2) {
  				p1 = buf;
  				c = read(fd, (char *) p1, LINE_MAX);
  				if (c <= 0)
  					break;
  				bytect += c;
  				charct += c;
  				p2 = p1 + c;
  			}
  			c = *p1++;
			if (!isspace(c)) {
  				if (!token) {
  					wordct++;
  					token++;
  				}
  				continue;
  			}
			token = 0;
  			if (c == '\n')
  				linect++;
  		}	/* end for loop */
  	}	/* end dual path for I18N */

  	/*
  	 * print lines, words, chars/bytes 
  	 */
  	wcp(linect, wordct, charct, bytect);

  	if (filect)
  		(void)printf(" %s\n", argv[optind]);
  	else
  		(void)printf("\n");

  	(void)close(fd);
  	tlinect += linect;
  	twordct += wordct;
  	tbytect += bytect;
  	tcharct += charct;
  } while (++optind < argc);	/* process next file */

  if (filect >= 2) {	/* print totals for multiple files */
  	wcp(tlinect, twordct, tcharct, tbytect);
  	(void)pfmt(stdout, MM_NOSTD, ":551: total\n");
  }
  exit(status);
}

/*
 * NAME: wcp
 *                                                                    
 * FUNCTION: check options then print out the requested numbers.
 */
static void
wcp(long int linect, long int wordct, long int charct, long int bytect) {

  if (lflg)
  	(void) printf("%ld", linect);

  if (wflg) {
  	if (lflg)
  		(void) putchar(' ');
  	(void) printf("%ld" , wordct);
  }

  if (cflg || kflg) {
  	if (lflg || wflg)
  		(void) putchar(' ');
  	(void) printf("%ld", cflg ? bytect : charct);
  }
}
