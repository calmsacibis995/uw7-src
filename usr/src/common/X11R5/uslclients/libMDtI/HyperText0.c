/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)libMDtI:HyperText0.c	1.3.1.3"
#endif

/***************************************************************
**
**      Copyright (c) 1990
**      AT&T Bell Laboratories (Department 51241)
**      All Rights Reserved
**
**      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
**      AT&T BELL LABORATORIES
**
**      The copyright notice above does not evidence any
**      actual or intended publication of source code.
**
**      Author: Hai-Chen Tu
**      File:   HyperText0.c
**      Date:   08/06/90
**
****************************************************************/

/*
 *************************************************************************
 *
 * Description:
 *   This file contains the source code for the hypertext widget.
 *
 ******************************file*header********************************
 */

                        /* #includes go here    */

#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "Xm/Xm.h"
#include "HyperTextP.h"

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

/* defines for list formatting */
#define TAB			"\t"
#define DEFAULT_LIST_SYMBOL 	"o "
#define MAX_EMBEDDED_LISTS	10
#define LIST_ITEM_IDENTIFIER	''

#define EATSPACE(P, C)	while (*(P) == C) (P)++

#define ALLOC_MAYBE(sz, local_buf, local_buf_sz) \
	( ((sz) <= local_buf_sz) ? (XtPointer)(local_buf) : \
				   (XtPointer)(XtMalloc(sz)) )

#define FREE_MAYBE(actual, local_buf) \
	if ((actual) != (local_buf)) XtFree((XtPointer)(actual)); else

#define BUF_SIZE		1024
typedef struct {
	ExmHyperSegment *	seg;
	ExmHyperLine *		first_line;
	ExmHyperLine *		line;
	Cardinal		buf_i;
	Cardinal		tabs;
	char			buf[BUF_SIZE];
} WorkingArea;

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *      1. Private Procedures
 *      2. Public  Procedures
 *
 **************************forward*declarations***************************
 */
                    /* private procedures       */

static ExmHyperLine *HyperLineNew(void);
static ExmHyperSegment *HyperSegmentNew(String, int, String, String, int);
static void h_start(WorkingArea *);
static void h_new_segment(WorkingArea *, String, int, String, String);
static void h_flush(WorkingArea *);
static void h_push(WorkingArea *, int);
static void h_tab(WorkingArea *);
static void h_newline(WorkingArea *);
static void h_scan(WorkingArea *, register String);
static void do_command(WorkingArea *, register String, register String);
static void do_key(WorkingArea *, String, Boolean, Boolean);
static String h_parse(WorkingArea *, String);
static String strn_copy(register String, register int, String, int);

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 ****************************procedure*header*****************************
 * ExmHyperLineFromString - Reads input string into first_line and parses
 * the string.  Also constructs lists, if any.  Called if XmNstring resource
 * is set and XmNdiskSource is False.
 *************************************************************************
*/
ExmHyperLine *
ExmHyperLineFromString(Widget widget, String str)
{
	WorkingArea data;
	int len;
	int idx;
	int list = 0; /* list > 1 indicates embedded lists */
	char *symbol[MAX_EMBEDDED_LISTS];
	char delimiter[MAX_EMBEDDED_LISTS];
	char *sp = str;
	char *line;
	char *lp = NULL;
	char *p;

	h_start(&data);

	if (strstr(str, "\\l") == 0) {
		h_scan(&data, str);
		return(data.first_line);
	}

	/* construct lists, if any */
	p = strchr(sp, '\n');
	while (p) {
		++p;
		len = p - sp;
		lp = line = (char *)CALLOC(sizeof(char),len+1);
		strncpy(line, sp, len);
		sp = p;
		if (strncmp(line, "\\l", 2) == 0) {
			/* a new list; if list is already > 0, we have embedded
			 * lists.
			 */
			idx = list;
			++list;
			switch(line[2]) {
				case '(': delimiter[idx] = ')'; break;
				case '<': delimiter[idx] = '>'; break;
				case '{': delimiter[idx] = '}'; break;
				default: delimiter[idx] = line[2]; break;
			}
			line += 3;
			if (strlen(line) == 1)
				symbol[idx] = STRDUP(DEFAULT_LIST_SYMBOL);
			else {
				p = strchr(line, '\n');
				if (p) {
					*p = '\0';
				}
				symbol[idx] = STRDUP(line);
			}
		} else if (list) {
			if (line[0] != delimiter[idx]) {
				int i;
				char item[BUF_SIZE];
				char tabs[MAX_EMBEDDED_LISTS];

				strcpy(tabs, TAB);
				for (i = 1; i < list; i++)
					strcat(tabs, TAB);

				if (line[0] == LIST_ITEM_IDENTIFIER) {
					p = strchr(line, '\n');
					if (p)
						*p = '\0';
					sprintf(item, "%s%s %s\n", tabs,
						symbol[idx], ++line);
				} else {
					sprintf(item, "%s    %s", tabs, line);
				}
				h_scan(&data, item);
			} else {
				/* end of a list */
				--list;
				FREE(symbol[list]);
			}
		} else {
			h_scan(&data, line);
		}
		if (lp) {
			FREE(lp);
			lp = NULL;
		}
		p = strchr(sp, '\n');
	}
	return(data.first_line);

} /* end of HyperLineFromString */

/*
 ****************************procedure*header*****************************
 * HyperLineFromFile - Sets first_line to first line in input file. Called
 * when XtNfile is set. 
 *************************************************************************
*/
ExmHyperLine *
ExmHyperLineFromFile(Widget widget, String file_name)
{
	WorkingArea	data;
	FILE *		fp;
	char		buf[BUF_SIZE];
	char *		str;
	char *		s;
	int		idx;
	int list = 0; /* list > 1 indicates embedded lists */
	char *symbol[MAX_EMBEDDED_LISTS];
	char delimiter[MAX_EMBEDDED_LISTS];

	if ((fp = fopen(file_name, "r")) == NULL) {
		fprintf(stderr, "Cannot open hypertext file %s.\n", file_name);
		return(NULL);
	}

	h_start(&data);
	for (str = fgets(buf, sizeof(buf)-1, fp); str;
		str = fgets(buf, sizeof(buf)-1, fp))
	{
		if (strncmp(str, "\\l", 2) == 0) {
			int len;
			char *p;

			/* a new list; if list is already > 0, we have embedded
			 * lists.
			 */
			idx = list;
			++list;
			switch(str[2]) {
				case '(': delimiter[idx] = ')'; break;
				case '<': delimiter[idx] = '>'; break;
				case '{': delimiter[idx] = '}'; break;
				default: delimiter[idx] = str[2]; break;
			}
			str += 3;
			p = strchr(str, '\n');
			if ((len = p - str) == 0)
				symbol[idx] = strdup(DEFAULT_LIST_SYMBOL);
			else {
				symbol[idx] = (char *)calloc(sizeof(char),len+1);
				strncpy(symbol[idx], str, len);
			}
			continue;
		} else if (list) {
			if (str[0] == LIST_ITEM_IDENTIFIER) {
				int i;
				char item[BUF_SIZE];
				char tabs[MAX_EMBEDDED_LISTS];

				strcpy(tabs, TAB);
				for (i = list; i; i--)
					strcat(tabs, TAB);

				sprintf(item, "%s %s%s", tabs, symbol[idx],
					++str);
				h_scan(&data, item);
				free(tabs);
				continue;
			} else if (str[0] == delimiter[idx]) {
				/* end of a list */
				--list;
				free(symbol[list]);
			}
		} else {
			/* append \0 */
			s = strchr(str, '\n');
			if (s)
				*++s = '\0';
			h_scan(&data, str);
		}
	}
	fclose(fp);
	return(data.first_line);

} /* end of HyperLineFromFile */

/*--------------------- HyperSegment * Functions --------------------*/

/*
 ****************************procedure*header*****************************
 * HyperSegmentNew - 
 *************************************************************************
*/
static ExmHyperSegment *
HyperSegmentNew(String text, int len, String key, String script, int tabs)
{
	register ExmHyperSegment *hs;

	hs = (ExmHyperSegment *)XtMalloc(sizeof(ExmHyperSegment));

#define STR_DUP(str)		((str) ? strdup(str) : NULL)

	hs->next = NULL;
	hs->prev = NULL;
	hs->text = STR_DUP(text);
	hs->len = len;
	hs->tabs = tabs;
	hs->key = STR_DUP(key);
	hs->script = STR_DUP(script);
	hs->x = hs->y = 0;
	hs->w = hs->h = 0;
	hs->reverse_video = False;
	hs->shell_cmd = False;

#undef STR_DUP

	return(hs);

} /* end of HyperSegmentNew */

/*
 *************************************************************************
 *
 * Private Procedures - HyperLine functions.
 *
 ***************************private*procedures****************************
 */

/*
 ****************************procedure*header*****************************
 * HyperLineNew -
 *************************************************************************
*/
static ExmHyperLine *
HyperLineNew(void)
{
    register ExmHyperLine *hl;

    hl = (ExmHyperLine *)XtMalloc(sizeof(ExmHyperLine));
    hl->next = NULL;
    hl->first_segment = NULL;
    hl->last_segment  = NULL;

    return(hl);

} /* end of HyperLineNew */

/*
 ****************************procedure*header*****************************
 * h_start -
 *************************************************************************
*/
static void
h_start(WorkingArea *	data)
{
	data->seg	 = NULL;
	data->first_line =
	data->line	 = NULL;
	data->buf_i	 = 0;
	data->tabs	 = 0;

} /* end of h_start */

/*
 ****************************procedure*header*****************************
 * h_new_segment -
 *************************************************************************
*/
static void
h_new_segment(WorkingArea * data,String str, int len, String key, String script)
{
	ExmHyperSegment *hs1;

	hs1 = HyperSegmentNew(str, len, key, script, data->tabs);

	/* have to update data->seg, data->line, and also reset data->tabs */
	if (data->seg == NULL) {
		ExmHyperLine *hl1;

		hl1 = HyperLineNew();
		if (data->line != NULL)
			data->line->next = hl1;
		else
			data->first_line = hl1;

		data->line = hl1;
		data->line->first_segment = hs1;
	} else {
		hs1->prev = data->seg;
		data->seg ->next = hs1;
	}
	data->seg = hs1;
	data->tabs = 0;

	/* keep updating last segment in hyper line for each new
	 * segment that's added so that it eventually contains the
	 * ptr to the last segment in the line.
	 */
	data->line->last_segment = hs1;

} /* end of h_new_segment */

/*
 ****************************procedure*header*****************************
 * h_flush -
 *************************************************************************
*/
static void
h_flush(WorkingArea * data)
{
	if (data->buf_i == 0)
		return;

	data->buf[data->buf_i] = '\0';
	h_new_segment(data, data->buf, data->buf_i, NULL, NULL);
	data->buf_i = 0;
} /* end of h_flush */

/*
 ****************************procedure*header*****************************
 * h_push -
 *************************************************************************
*/
static void
h_push(WorkingArea * data, int ch)
{
	/* flush it first if buf is full */
	if (data->buf_i >= sizeof(data->buf) - 1)
		h_flush(data);
	data->buf[data->buf_i++] = ch;
} /* end of h_push */

/*
 ****************************procedure*header*****************************
 * h_tab - Since h_flush() may not call h_new_segment(), which will set
 * h_tabs to 0, tabs may grow larger than 1 under such condition.
 *************************************************************************
*/
static void
h_tab(WorkingArea * data)
{
	h_flush(data);
	data->tabs++;
} /* end of h_tab */

/*
 ****************************procedure*header*****************************
 * h_newline -
 *************************************************************************
*/
static void
h_newline(WorkingArea * data)
{
	h_flush(data);
	data->seg = NULL;
	data->tabs = 0;

	/* need a place holder so empty lines can be properly handled */
	h_new_segment(data, NULL, 0, NULL, NULL);
} /* end of h_newline */

/*
 ****************************procedure*header*****************************
 * h_scan -
 * escape commands can be
 *	\n  : new line
 *	\t  : tab
 *	\   : line continuation
 *	\k(...)  : keyword
 */
static void
h_scan(WorkingArea * data, register String s)
{
	register char c;

	if (s == NULL) {
		h_push(data, ' ');
		h_flush(data);
		return;
	}

	while ((c = *s++)) {
	switch (c) {
	case '\\':                  /* escpace character */
		switch (c = *s++) {
		case '\0':              /* end of string */
			goto done;
		case '\\':              /* normal \ character */
			h_push(data, c);
			break;
		case '\n':              /* ignore new line character after \ */
			break;
		case 'n':               /* new line */
			h_newline(data);
			break;
		case 't':               /* tab */
			h_tab(data);
			break;
		default:
		{    /* \cmd(arg) */
			char *s1;
			s1 = h_parse(data, s-1);
			if (s1 == s-1) {
				h_push(data, '\\');
				h_push(data, c);
				s1++;
			}
			s = s1;
		}
			break;
		}
		break;

	case '\n':                  /* newline */
		h_newline(data);
		break;
	case '\t':                  /* tab */
		h_tab(data);
		break;
	default:                    /* normal character */
		h_push(data, c);
		break;
	}
    }

done:
	h_flush(data);
} /* end of h_scan */

/*
 ****************************procedure*header*****************************
 * h_parse - Input: s0 pointto the character after '\'.
 *************************************************************************
*/
static String
h_parse(WorkingArea * data, String s0)
{
	/* include \0, possible commands are "k", "d", "key", "def" */
	char	cmd_local[4];
	char	arg_local[80];
	char *	s;
	char *	s1;
	char *	cmd;
	char *	arg;
	int	delimiter;
	register int len;

	s = s0;

	/* find the command name first */
	len = 0;
	s1 = s;
	while (isalnum(*s))
		s++;
	len = s - s1;

	/* no name follow the \ character, give up */
	if (!len || !s1)
		return(s0);

	cmd = strn_copy(s1, len, cmd_local, sizeof(cmd_local));
	arg = arg_local;

	/* parse argument, first we get the delimiter */
	if (isprint(*s))
	{  /* but we know is not alnum */
		delimiter = *s;

		/* adjust delimiter */
		switch (delimiter) {
		case '(': delimiter = ')'; break;
		case '<': delimiter = '>'; break;
		case '{': delimiter = '}'; break;
			/* this one is used in setting up links
			 * in the table of contents */
		case '$': delimiter = '$'; break;
		default: break;
		}

		s1 = ++s;
		s = strchr(s1, delimiter);
		if (s == NULL) { /* no matching delimiter, give up */
			fprintf(stderr, "Couldn't find matching delimiter "\
				"'%c' for command '%s'\n", delimiter, cmd);
			FREE_MAYBE(cmd, cmd_local);
			FREE_MAYBE(arg, arg_local);
			return(s1-1);
		}
		len = s - s1; /* len may be 0 */
		FREE_MAYBE(arg, arg_local);
		arg = strn_copy(s1, len, arg_local, sizeof(arg_local));
		++s;
	} else {
		FREE_MAYBE(cmd, cmd_local);
		FREE_MAYBE(arg, arg_local);
		return(s0);
	}

	do_command(data, cmd, arg);

	FREE_MAYBE(cmd, cmd_local);
	FREE_MAYBE(arg, arg_local);

	return(s);

} /* end of h_parse */

/*
 ****************************procedure*header*****************************
 * do_command -
 *************************************************************************
*/
static void
do_command(WorkingArea * data, register String cmd, register String arg)
{
	h_flush(data);

	if (strcmp(cmd, "k") == 0 || strcmp(cmd, "key") == 0)
		do_key(data, arg, False, False);
	else if (strcmp(cmd, "d") == 0 || strcmp(cmd, "def") == 0)
		do_key(data, arg, True, False);
	else if (strcmp(cmd, "s") == 0 || strcmp(cmd, "shell") == 0)
		do_key(data, arg, False, True);
} /* end of do_command */

/*
 ****************************procedure*header*****************************
 * do_key -
 *************************************************************************
*/
static void
do_key(WorkingArea * data, String arg, Boolean reverse_video, Boolean shell_cmd)
{
	char *name;
	char *label;
	char *script;

	label = name = arg;

	/* find the first non-escaped caret */
	script = strchr(name, '^');

	while (script && *(script-1) == '\\') {
		script = strcpy(script-1, script);
		script = strchr(script+1, ',');
	}

	if (script) {
		*script++ = '\0';
		/* eat leading spaces in script */
		EATSPACE(script, ' ');
	}
	h_new_segment(data, label, strlen(label), name, script);
	data->seg->reverse_video = reverse_video;
	data->seg->shell_cmd = shell_cmd;
} /* end of do_key */

/*
 ****************************procedure*header*****************************
 * strn_copy -
 *************************************************************************
*/
static char *
strn_copy(register String s, register int sz, String local_buf, int buf_size)
{
#define str_len(s) ((s) ? (int)strlen((s)) : 0)

	register char *	s1;
	int		s_len = str_len(s);

#undef str_len

		/* adjust the length in case the caller lies */
	if (s_len < sz)
		sz = s_len;

	if (sz < 0)
		sz = 0;

	s1 = (char *)ALLOC_MAYBE(sz+1, local_buf, buf_size);
	if (!sz)
		*s1 = 0;
	else
	{
		strncpy(s1, s, sz);
		s1[sz] = 0;
	}

	return (s1);
} /* end of strn_copy */

/*
 ****************************procedure*header*****************************
 * DumpHyperText - Prints out the text portion of the widget.
 *************************************************************************
*/
void
DumpHyperText(Widget w)
{
	ExmHyperLine    *hl;
	ExmHyperSegment *hs;

#define HPART(w)	(((ExmHyperTextWidget)(w))->hyper_text)

	for (hl = HPART(w).first_line; hl; hl = hl->next) {
		for (hs = hl->first_segment; hs; hs = hs->next) {
			fprintf(stderr,"%s\n",hs->text);
		}
	}
#undef HPART

} /* end of DumpHyperText */

