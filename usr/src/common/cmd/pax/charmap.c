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

#ident	"@(#)pax:charmap.c	1.1"

/*
 * OSF/1 1.2
 */
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile$ $Revision$ (OSF) $Date$";
#endif
/*
 * charmap.c - functions to parse and store charmap files.
 *
 * DESCRIPTION
 *
 *	Routines to read and store a charmap file as specified
 *	in POSIX 1003.2 Draft 11.  Also contains the routines
 * 	to access the charmap and convert filenames.
 *
 * AUTHOR
 *
 *     Tom Jordahl - The Open Software Foundation
 *
 *
 */

/* Headers */
#include "config.h"
#include "pax.h"
#include "charmap.h"
#include "func.h"
#include "pax_msgids.h"



/*
 * File wide globals
 */

static char	 line[LMAX];		/* buffer to hold each line */
static char	*lineptr;		/* location in line buffer */
char	 	escape_char='\\'; 	/* escape charater (default '\') */
int		mbmaxlen = 1;		/* max bytes in multi-byte */
int		mbminlen = 1;		/* min bytes in multi-byte */
Charentry	*char_head=NULL;	/* head of Charentry list */
Charentry	*char_tail=NULL;	/* tail of Charentry list */
int		entry_count=0;		/* number of entries in charmap */
Charentry	*charmap;		/* The final sorted charmap table */
Charentry	search_key;		/* Seach key for bsearch */



static char	*get_name(void);
static int	make_entry(char *, Value *);
static int	do_range(char *, char *, Value *);
static int	get_encoding(Value *);
static int	lookup(char *);
static void	charmap_error(char *, int);
static void	eatspace(void);
static void	sort_table(void);
static char	*mblookup(char *);
static char 	*symlookup(char *, int *);
int		sym_compare(const Charentry *, const Charentry *);
int		bval_compare(const Charentry *, const Charentry *);
int 		byte_match(const Charentry *k1, const Charentry *k2);






/* read_charmap - main routine which reads a charmap file
 *
 * DESCRIPTION
 *
 *	Parses the charmap file, storing everything we need to
 *	support the -e option of pax.
 *	
 * PARAMETERS
 *
 *	char 	*file 	- Name of the charmap file.
 *
 * RETURNS
 *
 * 	0 	- if all went well
 * 	-1	- the charmap file was (in our eyes) bogus. 
 *
 */


int read_charmap(char *file)

{
	FILE	*cfile;			/* charmap input file */
	int	 linenum=0;		/* line number in the charmap file */
	char	*name1;			/* first symbolic name in line */
	char	*name2;			/* second sybbolic name (if range) */
	char	 comment_char='#'; 	/* comment charater (default '#') */
	short	 range=FALSE;		/* true if we are parsing a range */
	short	 incharmap=FALSE;	/* true if we have parsed CHARMAP */
	Value	 value;			/* struct that hold both mb and wc */
	char	*charmap="CHARMAP";
	char	*endcharmap="END CHARMAP";
	int		 charmap_len=strlen(charmap);
	int		 endcharmap_len=strlen(endcharmap);


    if ((cfile = fopen(file, "r")) == NULL) {
	warn(file, strerror(errno));
	return(-1);
    }

    while (fgets(line, LMAX, cfile) != NULL) {

	linenum++;
	lineptr = line;
	range = FALSE;

	if ((line[0] == comment_char) || (line[0] == '\n'))
	    continue;		/* comment */

	if (!strncmp(line, charmap, charmap_len)) {
	    incharmap = TRUE;
	    continue;
	}

	if (!strncmp(line, endcharmap, endcharmap_len)) {
	    incharmap = FALSE;
	    break;	/* we are done with the file */
	}

	if ((line[0] != '<') || ((name1 = get_name()) == NULL)) {
	    charmap_error(gettxt(CM_SYNTAX,"Syntax error - skipping"), linenum);
	    continue;
	}

	if (!incharmap) {
	    eatspace();
	    switch (lookup(name1)) {
		case CODE_SET_NAME:
		    break;
		case MB_MAX:
		    mbmaxlen = atoi(lineptr);
		    break;
		case MB_MIN:
		    mbminlen = atoi(lineptr);
		    break;
		case ESCAPE_CHAR:
		    escape_char = *lineptr;
		    break;
		case COMMENT_CHAR:
		    comment_char = *lineptr;
		    break;
		default:
		    charmap_error(gettxt(CM_DECL, "Error in declarations - skipping"), linenum);
		    free(name1);
		    break;
	    }
	    continue;
	}
		    
	if (*lineptr == '.' && *++lineptr == '.' && *++lineptr == '.') {
	    lineptr++;
	    range = TRUE; 
	    if ((name2 = get_name()) == NULL) {
		charmap_error(gettxt(CM_RANGE, "Error in range - skipping"), linenum);
		free(name1);
		continue;
	    }
	}

	if (get_encoding(&value) < 0) {
	    charmap_error(gettxt(CM_ENCODE, "Error in encoding - skipping"), linenum);
	    free(name1);
	    continue;
	}

	if (range) {
	    if (do_range(name1, name2, &value) < 0) {
		charmap_error(gettxt(CM_ADDRANGE, "Error adding range"), linenum);
	        free(name1);
	        free(name2);
		continue;
	    }
	    free(name2);
	} else if (make_entry(name1, &value) < 0) {
	    charmap_error(gettxt(CM_ADDENTRY, "Error adding entry"), linenum);
	    free(name1);
	    continue;
	}
	free(name1);

    }
    sort_table();
    return(0);
}


/* charmap_convert - run a filename through the charmap conversion.
 *
 * DESCRIPTION
 *
 * 	Converts a filename to or from the POSIX portable charater set
 *
 *  -w  Examines the filename string for any characters which are
 * 	not in the POSIX portable character set.  If we find one
 * 	we look it up in our charmap table and replace it with
 * 	its symbol name.  ie '/' would be replaced with "<slash>".
 *	
 * [-r] Examines the filename for any symbol names and replaces
 *	them with their encoding from the charmap.  If we can't 
 *	find the symbol, we will just leave it alone.
 *	
 *
 * PARAMETERS
 *
 *	char	*name	- File name to scan.
 * 			  Must be a pointer to at least PATH_MAX + 1 chars.
 * 
 * RETURNS
 *
 *	0  - if succesfully converted the filename using the charmap.
 * 	-1 - if failure.
 *
 */


int charmap_convert(char *name)

{
    char	buf[PATH_MAX+1];	/* scratch buffer */
    char	*bindex;		/* index in to buf */
    char	*nindex;		/* index in to name */
    char	c;			/* scratch character */
    char	*sym;			/* symbol returned from mblookup */
    char	*symbol;		/* symbol scrounged out of name */
    char	*symptr;		/* index in to symbol */
    short	esc;			/* flag for escape char */
    char	*orig_name;		/* original name for error */
    char	*val;			/* multi-byte value from symlookup */
    int		bytes;			/* numbers of bytes in 'val' */


    orig_name = mem_str(name);
    bindex = buf;
    nindex = name;
    if (f_create) {		/* -w */
	/*
	 *  insert escape chars before <,>, and escape_chars 
	 */
	for (nindex = name; *nindex != '\0'; nindex++) {
	    c = *nindex;
	    if ((c == '<') || (c == '>') || (c == escape_char)) {
		*bindex++ = escape_char;
	    }
	    *bindex++ = c;
	    if ((bindex - buf) > PATH_MAX+1) {
		warn(orig_name, gettxt(CM_LONG, "Filename becomes too long"));
		free(orig_name);
		return(-1);
	    }
	}
	*bindex = '\0';

	/*
	 * filename now in buf
	 *
	 * Scan filename for any character not in PCS and replace with name.
	 * The name we get back from mblookup will have <, >, and 
	 * escape_chars escaped.
	 */
	nindex = name;		/* name is now our "scratch" buffer */
	for (bindex = buf; *bindex != '\0'; bindex++) {
	    if (!isportable(*bindex)) {
		sym = mblookup(bindex);
		bindex += (mblen(bindex, MB_LEN_MAX) - 1); 
		if (sym == NULL) {
		    warn(orig_name, gettxt(CM_NOTFOUND, "Could not find charmap entry"));
		    free(orig_name);
		    return(-1);
		}
		if ((nindex-name) + strlen(sym) > (size_t)(PATH_MAX+1)) {
		    warn(orig_name, gettxt(CM_LONG, "Filename becomes too long"));
		    free(orig_name);
		    return(-1);
		}
		strcpy(nindex, sym);
		nindex += strlen(sym);
	    }
	    else
		*nindex++ = *bindex;
	}
	*nindex = '\0';

    } else {			/* -r or index */
	/* symbols can't be longer than the name itself */
	symbol = mem_get(strlen(name)+1);		

	/* scan for a string "<string>" in name */
	for (nindex = name; *nindex != '\0'; nindex++) {
	    c = *nindex;
	    if (c == escape_char) {
		*bindex++ = c;
		esc = (esc + 1) % 2;
	    } else if (c == '<' && !esc) {	/* start of a new symbol */
		nindex++;			/* skip '<' */
		for (symptr = symbol; *nindex != '>' || esc; nindex++) {
		    c = *nindex;
		    if (c == escape_char) {
			if (esc) {
			    *symptr++ = escape_char;
			    esc = 0;
			} else
			    esc++;
		    } else {
			if (esc && (c != '<') && (c != '>')) {
			    *symptr++ = escape_char;
			    esc = 0;
			}
			*symptr++ = c;
		    }
		}
		symptr = '\0';

		/* lookup the symbol in charmap and replace with encoding */
		val = symlookup(symbol, &bytes);
		if (val) {
		    strncpy(bindex, val, bytes); /* insert mb sequence */
		    bindex += bytes;
		} else {
		    strcpy(bindex, symbol);	/* put back the symbol */
		    bindex += strlen(symbol);
		}
	    } else {
		esc = 0;
		*bindex++ = c;
	    }
	}
	*bindex = '\0';

	/*
	 * filename is now in buf - copy it back to name
	 * 
	 * remove escape chars in front of <, >, and escape_chars 
	 */
	for (esc = 0,bindex = buf,nindex = name; *bindex != '\0'; bindex++) {
	    c = *bindex;
	    if (c == escape_char) {
		if (esc) {
		    *nindex++ = escape_char;
		    esc = 0;
		} else
		    esc++;
	    } else {
		if (esc && (c != '<') && (c != '>')) {
		    *nindex++ = escape_char;
		}
		esc = 0;
		*nindex++ = c;
	    }
	}
	if (esc) 
	    *nindex++ = escape_char;
	*nindex = '\0';
	free(symbol);
    }
    free(orig_name);
    return(0);
}

/* get_name - read a symbolic name of a character or declaration
 *
 * DESCRIPTION
 *
 *	Examines the line buffer (pointed at by lineptr) and returns
 * 	the string in between the first '<' and '>'.
 *	
 *
 * PARAMETERS
 *
 * RETURNS
 *
 *	pointer to a symbolic name or NULL if name isn't found.
 *
 */


static char *get_name(void)

{
    char	*ret;		/* pointer to returned buffer */
    char	*end;		/* pointer to the end of symbol */
    int		 length=0;	/* length of symbol */
    int		 escaped=0;	/* flag for escaped chars */
    int		 i;		/* loop counter */

    while ((*lineptr != '<') && (*lineptr != '\0')) {
	lineptr++;	/* find bracket */
    }

    if (*lineptr == '\0')
	return(NULL);

    lineptr++;

    end = lineptr;
    while (*end && *end != '>') {
	length++;
	if (*end == escape_char)
	    end += 2;
	else
	    end++;
    }
    if (*end == '\0')
	return(NULL);

    if ((ret = mem_get(length+1)) == NULL) {
	fatal(gettxt(NOMEM, "Out of memory"));
    }

    for (i=0; lineptr < end; lineptr++) {
	if ((*lineptr == escape_char) && !escaped) {
	    escaped++;
	    continue;
	} else {
	    if (escaped) escaped = 0;
	    ret[i++] = *lineptr;
	}
    }
    ret[length] = '\0';
    lineptr = end + 1;		/* just after '>' */
    return(ret);
}



/* make_entry - Make an entry in the charmap table.
 *
 * DESCRIPTION
 *
 *	Makes an entry in the charmap for the given symbol name
 * 	with the given value.
 *	
 *
 * PARAMETERS
 *
 *	char	 *symbol 	- symbol to make the entry for.
 *	Value  	 *value 	- encoding value of this symbol.
 *
 * RETURNS
 *
 *	0	- OK
 *	-1 	- if error.
 *
 */


static int make_entry(char *symbol, Value *value)

{
    Charentry	*entry;

    if ((entry = (Charentry *)mem_get(sizeof(Charentry))) == NULL)
	return(-1);

    entry->symbol = mem_str(symbol);
    strcpy((char *)&entry->value, (char *)&value->mbvalue);
    entry->nbytes = value->nbytes;
    entry->next = NULL;

    if (char_head == NULL) {
	char_head = entry;
	char_tail = entry;
    } else {
	char_tail->next = entry;
	char_tail = entry;
    }
    
    entry_count++;
    return(0);
}



/* do_range - make the entries for a range of symbolic names.
 *
 * DESCRIPTION
 *
 *	
 * 	Makes entries into the charmap for a range of characters
 * 	which have sequential values.
 *	
 *	The symbol must names consist of zero or more nonnumeric characters
 *	followed by an interget formed by one or more decimal digits.
 *
 * PARAMETERS
 *
 *	char *name1 	- fist symbolic name
 *	char *name2 	- last symbolic name
 *	Value *value 	- starting value.
 *
 * RETURNS
 *
 *	0 	- is successfull.
 *	-1 	- if an error occurred.
 *
 */


static int  do_range(char *name1, char *name2, Value *value)

{
    int		start;		/* starting symbol number */
    int		end;		/* ending symbol number */
    int		numlen1;	/* length of number in name1 */
    int		numlen2;	/* length of number in name2 */
    char	*buf;		/* area of memory for constructed name */
    char	*prefix;	/* nondigit part of name */
    char	*p;
    int		i;
    int		byte;


    p = name1;
    while (*p && !isdigit(*p))
	p++;
    if (!*p)
	return(-1);
    start = atoi(p);
    numlen1 = strlen(p);

    p = name2;
    while (*p && !isdigit(*p))
	p++;
    if (!*p)
	return(-1);
    end = atoi(p);
    numlen2 = strlen(p);

    *p='\0';
    prefix = name2;
    if ((start > end) || (numlen1 != numlen2))
	return(-1);

    buf = mem_get(strlen(name1));

    for (i=start; i <= end; i++) {
	sprintf(buf, "%s%0*d", prefix, numlen1, i);

	if (make_entry(buf, value) < 0)
	    return(-1);

	byte = value->nbytes - 1;
	value->mbvalue[byte]++;
	for (;byte > 0; byte--) {
	   if ((value->mbvalue[byte]) == 0)
		value->mbvalue[byte-1]++;
	}
    }
    free(buf);

    return(0);
}


/* get_encoding - Decodes the character encoding into multi-bye.
 *
 * DESCRIPTION
 *
 *	Examines the line buffer (via lineptr)  and returns the 
 * 	multi-byte encoding of the one or 
 * 	more concatinated decimal, hex, or octal numbers.
 *	
 *
 * PARAMETERS
 *
 *	Value	*value	- where to put the encoding.
 *
 * RETURNS
 *
 *	0 	- OK
 * 	-1 	- on error.
 *
 */


static int get_encoding(Value *value)

{
    char	c;
    int		i;
    int		code;

    eatspace();

    for (i=0; i<mbmaxlen; i++) {
	if ((*lineptr == '\0') || (*lineptr++ != escape_char))
	    break;

	switch ((c = *lineptr++)) {
	    case 'd':			/* decimal */
		code = atoi(lineptr);
		break;
	    case 'x':			/* hex */
		code = strtol(lineptr, (char **)NULL, 16);
		break;
	    default:
		if (isdigit(c) && (c >= '0' && c <= '7')) 	/* octal */
		    code = strtol(lineptr, (char **)NULL, 8);
	        else
		    return(-1);
	}
	value->mbvalue[i] = (char)code;
    }
    if (i < mbminlen)
	return(-1);

    value->nbytes = i;

    return(0);
}


/* lookup - lookup a symbolic name for a declaration
 *
 * DESCRIPTION
 *
 *	Takes a string and maches it with one of the declaration 
 * 	names.
 *	
 *
 * PARAMETERS
 *
 *	char *name 	- name to look at.
 *
 * RETURNS
 *
 *	Returns an integer ordinal value of the name
 *	-1 if name isn't found.
 *
 */


static int  lookup(char *name)

{

    if (!strcmp(name, "code_set_name"))
	return(CODE_SET_NAME);
    if (!strcmp(name, "mb_cur_max"))
	return(MB_MAX);
    if (!strcmp(name, "mb_cur_min"))
	return(MB_MIN);
    if (!strcmp(name, "escape_char"))
	return(ESCAPE_CHAR);
    if (!strcmp(name, "comment_char"))
	return(COMMENT_CHAR);

    return(-1);
}


/* charmap_error - print an error message
 *
 * DESCRIPTION
 *
 *	Prints the line number and a provided message.
 *	
 *
 * PARAMETERS
 *
 *	char *mess 	- message to print
 *	int   line	- line number where error occurred.
 *
 */


static void  charmap_error(char *mess, int l)

{
    char	lbuf[10];

    sprintf(lbuf, gettxt(CM_LINE, "line %d"), l);
    warn(lbuf, mess);
}

/*
 * eatspace - move lineptr past spaces
 *
 */

static void eatspace(void)
{
    while ((*lineptr != '\0') && isspace(*lineptr))
	lineptr++;
}


/* sort_table - sort the character map.
 *
 * DESCRIPTION
 *
 *	Reads through the linked list we created while parsing
 * 	the charmap file and constructs a contiguous array
 * 	of the entryies, which is sorts.
 * 
 * 	Two kinds of sorts are done:
 * 	 - when writing an archive, we sort on the multi-byte encoding
 * 	 - when reading an archive, we sort on the symbol name.
 *
 *
 */


static void sort_table(void)

{
    Charentry	*cptr;		/* list walker */
    int		 i=0;


    charmap = (Charentry *)mem_get(entry_count*sizeof(Charentry));
    if (!charmap)
	fatal(gettxt(CM_NOMEM, "Out of memory constructing charmap"));

    cptr = char_head;
    while (cptr != NULL) {
	memcpy(&charmap[i], cptr, sizeof(Charentry));
	free(cptr);
	cptr = charmap[i].next;
	i++;
    }    

    /* sort them */
    if (f_create) 
	/* sort by byte value */
	qsort(charmap, 
	      (size_t)entry_count, 
	      sizeof(Charentry), 
	      (int (*)(const void *, const void *))bval_compare);
    else /* extract or list */
	/* sort by symbol name */
	qsort(charmap, 
	      (size_t)entry_count, 
	      sizeof(Charentry), 
	      (int (*)(const void *, const void *))sym_compare);


}

/* mblookup - lookup a symbolic name of a multi-byte character.
 *
 * DESCRIPTION
 *
 *	Searches through the charmap for a multi-byte character
 * 	that matches the one pointed to by mbchar.
 * 
 *	before returning the symbol name we place an escape in
 * 	front of '<', '>', or escape_char's.  We also put in
 * 	the surrounding brackets '<>'.
 *
 * PARAMETERS
 *
 *	char	*mbchar	- pointer to the start of a multi-byte char.
 *
 * RETURNS
 *
 *	pointer to a symbolic name or NULL if name isn't found.
 *
 */


static char *mblookup(char *mbchar)

{
    Charentry	*entry;			/* entry return by bsearch */
    char	*symbol;		/* symbol from entry */
    char	 escname[PATH_MAX + 1];	/* buffer to hold final symbol */
    char	*eindex;		/* index into escname buffer */
    char	*ret;			/* pointer to returned buffer */
    char	 c;			/* scratch character */

    strncpy((char *)search_key.value, mbchar, 10);
    entry = bsearch(&search_key,
		    charmap, 
		    (size_t)entry_count, 
		    sizeof(Charentry), 
		    (int (*)(const void *, const void *))byte_match);
    if (entry == NULL)
	return(NULL);

    symbol = entry->symbol;
    eindex = escname;
    *eindex++ = '<';
    for (; *symbol !='\0'; symbol++) {
	c = *symbol;
	if ((c == '<') || (c == '>') || (c == escape_char)) {
	    *eindex++ = escape_char;
	}
	    *eindex++ = c;
    }
    *eindex++ = '>';
    *eindex = '\0';

    ret = mem_str(escname);
    return(ret);
}


/* symlookup - lookup the multi-byte encoding of a symbolic name
 *
 * DESCRIPTION
 *
 *	Searches through the charmap for a matching symbol
 * 	that matches the one pointed to by mbchar.  Returns
 * 	that multi-byte encoding of that symbol from the charmap.
 * 
 * PARAMETERS
 *
 *	char	*symbol	- symbol name to search charmap for
 * 	int	*nbytes - number of valid bytes in the returned mb value.
 *
 * RETURNS
 *
 *	pointer to a value array of bytes  or NULL if symbol isn't found.
 *
 */


static char *symlookup(char *symbol, int *nbytes)

{
    Charentry	*entry;

    search_key.symbol = symbol;
    entry = bsearch(&search_key,
		    charmap,
		    (size_t)entry_count,
		    sizeof(Charentry),
		    (int (*)(const void *, const void *))sym_compare);
    if (entry == NULL)
	return(NULL);

    *nbytes = entry->nbytes;
    return((char *)entry->value);

}


/*
 * byte_match - comparison routine for bsearch to find a matching mbvalue
 *
 * DESCRIPTION
 * 
 * 	We need to know which of the two elements is the key since
 * 	we must use the nbytes from the element in the table.
 */

int byte_match(const Charentry *k1, const Charentry *k2)
{
    const Charentry	*item;

    if (k1 == &search_key) 
	item = k2;
    else
	item = k1;

    return(strncmp((char *)k1->value, (char *)k2->value, item->nbytes));
}

/*
 * bval_compare - comparison routine for qsort-ing multi-byte encodings
 *
 * DESCRIPTION
 * 
 * 	Used for qsort to compare two multi-byte encodings
 * 
 * RETURNS
 *	-1 - if x1 < x2
 * 	 0 - if x1 = x2
 * 	 1 - if x1 > x2
 * 
 */
int bval_compare(const Charentry *x1, const Charentry *x2)
{

	if (x1->nbytes > x2->nbytes)
		return(1);
	else if (x1->nbytes < x2->nbytes)
		return(-1);
	else
		return(strncmp((char *)x1->value, (char *)x2->value, x1->nbytes));
}

/*
 * sym_compare - comparison routine for symbols
 *
 * DESCRIPTION
 * 
 * 	Used for qsort and bsearch to compare two symbols
 * 
 * RETURNS
 *	-1 - if x1 < x2
 * 	 0 - if x1 = x2
 * 	 1 - if x1 > x2
 * 
 */
int sym_compare(const Charentry *x1, const Charentry *x2)
{

	return(strcmp(x1->symbol,x2->symbol));
}


