#ident	"@(#)gencat:msg_conv.c	1.1.6.3"
#ident  "$Header$"
#include <stdio.h>
#include <ctype.h>
#include <pfmt.h>
#include "gencat.h"
#define S_INIT		0
#define S_STRING	1
#define S_SLASH		2
#define S_NUMBER	3
#define S_OCT_HEX	4
#define S_NUMBER2	5
#define S_END		6

extern int cat_format;
/*
 * Take a string in its C representation and replace it with its internal
 * representation. Also returns its actual length
 */
msg_conv(fname, set_nr, msg_nr, fd, start, linelen, res, maxlen, quotechar)
  char *fname;
  int set_nr, msg_nr;
  FILE *fd;
  char *start;
  int linelen;
  char *res;
  int maxlen;
  char quotechar;
{
  int status = S_INIT;
  unsigned char curpos;
  unsigned char nmbr;
  int base;
  unsigned char c;
  int len;
  char *inbuf, *begin;
  int found_quote = 0;
  
  len = 0;
  inbuf = start;
  
  /*
   * If no quotechar, start character is the first of the string
   * and end of string is newline
   */
  if (quotechar == 0){
    status = S_STRING;
    if (isspace(*start)) {
      start++;
    }
  }

 begin = start;
  while (status != S_END){
    c = *start++;
    switch(status){
    case S_INIT:
      if (c == '\n'){
	status = S_END;
	continue;
      }
      if (c == quotechar){
	status = S_STRING;
	found_quote = 1;
	continue;
      }
      if (!isspace(c)){
	status = S_STRING;
	start = begin;
	found_quote = 0;
	continue;
      }
      continue;
    case S_STRING:
      if (c == '\n'){
	if (found_quote){
	  pfmt(stderr, MM_ERROR, 
	  	":32:%s: set %d msg %d: Unmatched quote (%c)\n", fname, set_nr, 
	  	msg_nr, quotechar);
	  return -1;
	}
	status = S_END;
	continue;
      }
      if (c == quotechar){
	if (!found_quote){
	  pfmt(stderr, MM_ERROR, 
	  	":33:%s: set %d msg %d: Unexpected quote (%c)\n", fname, set_nr,
	  	msg_nr, quotechar);
	  return -1;
	}
	status = S_END;
	continue;
      }
      switch(c){
      case '\\':
        status = S_SLASH;
        continue;
      default:
        *res++ = c;
        if ((len = inc_len(fname, set_nr, msg_nr, len, maxlen)) == -1)
          return -1;
        continue;
      }
    case S_SLASH:
      if (c != quotechar || c == '\n'){
        switch(c){
        case '\\': if (cat_format && cat_format != SVR_FORMAT)
			c = '\\' ;
		    else {
			/*
			 * mkmsgs cannot
			 * handle \\
			 */
			*res++ = '\\';
			if ((len = inc_len(fname, set_nr, msg_nr, len, maxlen)) == -1)
			  return -1;
			c = '\\';
		    }
		    break;
        case 'n':   if (cat_format && cat_format != SVR_FORMAT)
			c = '\n' ;
		    else {
			/*
			 * mkmsgs cannot
			 * handle new line
			 */
			*res++ = '\\';
			if ((len = inc_len(fname, set_nr, msg_nr, len, maxlen)) == -1)
			  return -1;
			c = 'n';
		    }
		    break;
        case 'r':   c = '\r' ; break;
        case 'b':   c = '\b' ; break;
        case 't':   c = '\t' ; break;
        case 'v':   c = '\v' ; break;
        case 'f':   c = '\f' ; break;
        case '\'':  c = '\'' ; break;
        case '\"':  c = '\"' ; break;
        case '\n':
          /*
           * Back slash at end of line. Read a new line
           */
          if (fgets(inbuf, linelen, fd) == 0){
            status = S_END;
            continue;
          }
          start = inbuf;
          status = S_STRING;
          continue;
	case '0':
            if (!cat_format || cat_format == SVR_FORMAT) {
		/*
		 * mkmsgs cannot handle null character
		 */
		*res++ = '\\';
		if ((len = inc_len(fname, set_nr, msg_nr, len, maxlen)) == -1)
		  return -1;
		c = '0';
		break;
	    }
        default:
          if (isdigit(c)){
            status = S_NUMBER;
            start--;
            continue;
          }
        }
      }
      *res++ = c;
      if ((len = inc_len(fname, set_nr, msg_nr, len, maxlen)) == -1)
        return -1;
      status = S_STRING;
      continue;
    case S_NUMBER:
      nmbr = 0;
      base = 8;
      if (!isdigit(c)){
        status = S_STRING;
        *res++ = c;
        if ((len = inc_len(fname, set_nr, msg_nr, len, maxlen)) == -1)
          return -1;
        continue;
      }
      if (c == '0'){
        status = S_OCT_HEX;
        continue;
      }
      status = S_NUMBER2;
      start--;
      continue;
    case S_OCT_HEX:
      if (c == 'x'|| c == 'X'){
        status = S_NUMBER2;
        base = 16;
        continue;
      }
      status = S_NUMBER2;
      start--;
      continue;
    case S_NUMBER2:
      if ((base == 8 && (c < '0' || c > '7')) ||
          (base == 16 && !isxdigit(c))){
        start--;
	if ((nmbr == 012 || nmbr == 0134) &&
	    (!cat_format || cat_format == SVR_FORMAT)) {
		/*
		 * mkmsgs can't deal with newlines or \\'s,
		 * so don't interpret these sequences here;
		 * instead, send them to mkmsgs directly.
		 */
		*res++ = '\\';
		if ((len = inc_len(fname, set_nr, msg_nr, len, maxlen)) == -1)
		  return -1;
		switch(nmbr) {
		  case 012:
			*res++ = 'n';
			if ((len = inc_len(fname, set_nr, msg_nr, len, maxlen))
			    == -1)
			  return -1;
			break;
		  case 0134:
			*res++ = '\\';
			if ((len = inc_len(fname, set_nr, msg_nr, len, maxlen))
			    == -1)
			  return -1;
			break;
		}
	}
	else {
        	*res++ = nmbr;
        	if ((len = inc_len(fname, set_nr, msg_nr, len, maxlen)) == -1)
        	  return -1;
	}
        status = S_STRING;
      }
      nmbr *= base;
      c = tolower(c);
      nmbr += (c >= 'a') ? (c - 'a' + 10) : (c - '0');
      continue;
    }
  }
  if (c == quotechar){
    while ((c = *start++) != '\n'){
      if (!isspace(c)){
	pfmt(stderr, MM_ERROR, 
		":34:%s: set %d msg %d: Unexpected extra characters\n", fname, 
		set_nr, msg_nr);
	return -1;
      }
    }
  }
  *res = '\0';
  return len;
}  

static
inc_len(fname, set_nr, msg_nr, len, maxlen)
  char *fname;
  int set_nr, msg_nr;
  int len, maxlen;
{
  if (++len > maxlen){
    pfmt(stderr, MM_ERROR, ":35:%s: set %d msg %d: Invalid message length\n", 
    	fname, set_nr, msg_nr);
    return -1;
  }
  return len;
}
