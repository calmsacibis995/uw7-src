/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)olmisc:OlEucToCt.c	1.6"
#endif

/************************************************************************
 *									*
 *  This file contains functions for convering strings from EUC format  *
 *  to the CT (compound text) format, and back.  For the CT to EUC	*
 *  conversion it utilizes the Xct parser from the Xmu library, so any	*
 *  clients using these routines must be linked with libXmu.a		*
 *									*
 ************************************************************************/

#include <stdio.h>

#ifdef I18N

#ifndef sun	/* or other porting that does care I18N */
#include <sys/euc.h>
#else
#define SS3	0x8e	/* copied from sys/euc.h */
#define SS2	0x8f
#endif

#endif /* I18N */

#include <X11/IntrinsicP.h>
#include <X11/Xmu/Xct.h>
#include <Xol/OpenLookP.h>

#define	CharSetNo	16	/* No of standard character set entries */

typedef struct _CharSetNameEncoding {
	char	*csname;
	char	*control;
} CharSetNameEncoding;

static CharSetNameEncoding CTstandards[] = {
        { "ISO8859-1", "-A", },
        { "ISO8859-2", "-B", },
        { "ISO8859-3", "-C", },
        { "ISO8859-4", "-D", },
        { "ISO8859-5", "-L", },
        { "ISO8859-6", "-G", },
        { "ISO8859-7", "-F", },
        { "ISO8859-8", "-H", },
        { "ISO8859-9", "-I", },
/* MORE: do we ever use the left side of the JISX0201 in EUC?   */
/* If yes, we would have to modify the Xct parser		*/
	/* { "JISX0201.1976-0", "(J",},/* Left side of 94 Katakana into GL */
	{ "JISX0201.1976-0", ")I", },	 /* Right side of 94 Katakana into GR */
	{ "GB2312.1980-0",   "$(A", }, /* 94**2 Hanzi into GL */
	{ "GB2312.1980-1",   "$)A", }, /* 94**2 Hanzi into GR */
	{ "JISX0208.1983-0", "$(B", }, /* 94**2 Kanji into GL */
	{ "JISX0208.1983-1", "$)B", }, /* 94**2 Kanji into GR */
	{ "KSC5601.1987-0",  "$(C", }, /* 94**2 Korean into GL */
	{ "KSC5601.1987-1",  "$)C", }, /* 94**2 Korean into GR */
};

#ifdef __STDC__
OlEucToCt(XctString eucstr, XctString ctstr, int ct_len, OlFontList *fontl)
#else
OlEucToCt(eucstr, ctstr, ct_len, fontl)
XctString 	eucstr;
XctString	ctstr;
int		ct_len;
OlFontList	*fontl;
#endif
{
	static XctString  cntrlseq[4];
	int		  i, j, cswidth, total_len = 0;
	char		  euc_codeset, codeset_flag = 5;

	/* If NULL fontl specified, assume Latin-1 which is a legal	*/
	/* Compound Text subset 				    	*/

	if (!fontl) {
	    total_len = strlen((char *)eucstr) + 1;
	    if (total_len > ct_len)
		return (-1);
	    strcpy((char *)ctstr, (char *)eucstr);
	    return (total_len);
	}

	/* if necessary, allocate storage for control sequences		*/

	if (cntrlseq[1] == (XctString) NULL) {
	    cntrlseq[1] = (XctString) malloc (18 * sizeof (unsigned char));
	    cntrlseq[2] = cntrlseq[1] + 6;
	    cntrlseq[3] = cntrlseq[1] + 12;
	}

	/* for each EUC code set save CT control sequence 		*/
	/* MORE: it would be nice if we would have to do this only if 	*/
	/*	 the EUC has changed since the last call to this 	*/
	/*	 function.  But how can we find out if it has changed?	*/

	for (j=1; j<fontl->num; j++) {
	     if (fontl->fontl[j])
	         for (i=0; i<CharSetNo; i++) 
		      if (!strcmp(CTstandards[i].csname, fontl->csname[j])) {
			  strcpy((char *)cntrlseq[j], CTstandards[i].control);
			  break;
		      }

	}

	/* Parse Euc string:						*/
	/* - identify bytes as belonging to one of code sets 0-3.	*/
	/* - write control sequence and data for equivalent CT string.	*/

	while (*eucstr) {
		if (!(*eucstr & 0x80)) {   	/* Code Set 0 -> Ascii */
			if (codeset_flag != 0) {
			    codeset_flag = 0;
			}
			*ctstr++ = *eucstr++;
			total_len++;
			if (total_len == ct_len)
			    return (-1);
		}
		else {				/* Supplementary Code Set */
			switch (*eucstr) {
			  case SS2:	euc_codeset = 2;
					break;
			  case SS3:	euc_codeset = 3;
					break;
			  default:	euc_codeset = 1;
					break;
			}

			/* if codeset changed, include control seq */

			if (codeset_flag != euc_codeset) {
			    i = strlen((char *) cntrlseq[euc_codeset]);
			    total_len += i;
			    if (total_len >= ct_len)
				return (-1);
			    strcpy((char *) ctstr, (char *) cntrlseq[euc_codeset]);
			    ctstr += i;
			    cswidth = fontl->cswidth[euc_codeset];
			    codeset_flag = euc_codeset;
			}
			if (euc_codeset > 1)
			    eucstr++;
			total_len += cswidth;
			if (total_len >= ct_len)
			    return (-1);
			for (i=0; i<cswidth; i++, ctstr++, eucstr++)
			     *ctstr = *eucstr;
		}
		*ctstr = '\0';
	}
	return (total_len);
}


/* This function converts character string encoded in the CT format	*/
/* into EUC format.  The function returns the length, in bytes (not 	*/
/* including the null character) of the EUC string on success, or -1	*/
/* on failure								*/

#ifdef __STDC__
OlCtToEuc (XctString ctstr, XctString eucstr, int euc_len, OlFontList *fontl)
#else
OlCtToEuc (ctstr, eucstr, euc_len, fontl)
XctString	ctstr;
XctString 	eucstr;
int		euc_len;
OlFontList	*fontl;
#endif
{
	static XctData	data;
	XctResult	result;
	int		euc_codeset;
	int		req_len, total_len = 0;
	char		*encoding;
	static void	_ct_to_euc();

	/* create XctData structure from the ctstr.  if it already	*/
	/* exists, the re-initialize it.  This structure will be used	*/
	/* the Xct parser.						*/

	if (!data)
	    data = XctCreate (ctstr, strlen((char *)ctstr), XctSingleSetSegments);
	else {
	    data->total_string = ctstr;
	    data->total_length = strlen((char *)ctstr);
	    XctReset(data);
	}

	/* parse one segment at the time and encode them into EUC format */

	while ((result = XctNextItem(data)) != XctEndOfText) {
	   switch (result) {

	   case XctGLSegment:
	   case XctGRSegment:

		/* determine if the returned segment belongs 	*/
		/* to one of the EUC code sets			*/

		encoding = (result == XctGLSegment ? data->GL_encoding :
                                                     data->GR_encoding);
		for (euc_codeset=0; euc_codeset < fontl->num; euc_codeset++)
	     	     if (strcmp (encoding, fontl->csname[euc_codeset]) == 0)
		 	 break;

		if (euc_codeset == fontl->num)
		    return(-1);

		/* determine if there is enough place left in the eucstr */
		/* to store another segment				 */

		if (euc_codeset < 2)		/* ASCII or Suppl. Code Set 1 */
		    req_len = data->item_length;
		else				/* Suppl. Code Sets 2 or 3    */
		    req_len = data->item_length +
			      data->item_length / data->char_size;
		total_len += req_len;
		if (total_len >= euc_len)
		    return (-1);

		/* convert CT string segment into EUC format	*/

		_ct_to_euc (data->item, data->item_length,
				   data->char_size,
		    	   	   eucstr, euc_codeset);

		/* increment EUC string pointer to point to the	end of	*/
		/*  the string						*/

		eucstr += req_len;
		break;

		/* ignore control segments, and segments containing	*/
		/* directional information				*/

	   case XctC0Segment:
	   case XctC1Segment:
	   case XctHorizontal:
	   case XctExtendedSegment: /* MORE: we should probably handle	*/
				    /*       extended segments		*/
		break;

	   default:
		return (-1);
	   }
	}
	*eucstr = '\0';
	return (total_len);
}


/* This function convert segments from the CT string into EUC format.	*/

#ifdef __STDC__
static void
_ct_to_euc (XctString item,	/* pointer to the CT string segment	*/
	   int       item_len,  /* length of the CT string in bytes	*/
	   int       char_size, /* bytes per character			*/
	   XctString eucstr,    /* pointer to the EUC string		*/
	   int 	     euc_codeset)	/* EUC codeset (0-3)		*/
#else
static void _ct_to_euc (item, item_len, char_size, eucstr, euc_codeset)
XctString	item;
int		item_len, char_size;
XctString	eucstr;
int		euc_codeset;
#endif
{
	switch (euc_codeset) {
	   case 0:				/* ASCII */
	   case 1:				/* Supplementary Code Set 1 */
		strncpy ((char *)eucstr, (char *)item, item_len);
		eucstr += item_len;
		break;
	   case 2:				/* Supplementary Code Set 2 */
	   case 3:				/* Supplementary Code Set 3 */
		{
		    register int i, j;
		    unsigned char c = (euc_codeset == 2 ? SS2 : SS3);

		    for (i=0, j=char_size; i<item_len; i++, j++) {
		         if (j == char_size) {
			     j = 0;
			     *eucstr = c;
			     eucstr++;
		         }
		         *eucstr++ = *item++;
		    }
		}    
		break;
	}
}
