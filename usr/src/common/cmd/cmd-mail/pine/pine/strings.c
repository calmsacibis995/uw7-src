#if !defined(lint) && !defined(DOS)
static char rcsid[] = "$Id$";
#endif
/*----------------------------------------------------------------------

            T H E    P I N E    M A I L   S Y S T E M

   Laurence Lundblade and Mike Seibel
   Networks and Distributed Computing
   Computing and Communications
   University of Washington
   Administration Builiding, AG-44
   Seattle, Washington, 98195, USA
   Internet: lgl@CAC.Washington.EDU
             mikes@CAC.Washington.EDU

   Please address all bugs and comments to "pine-bugs@cac.washington.edu"


   Pine and Pico are registered trademarks of the University of Washington.
   No commercial use of these trademarks may be made without prior written
   permission of the University of Washington.

   Pine, Pico, and Pilot software and its included text are Copyright
   1989-1996 by the University of Washington.

   The full text of our legal notices is contained in the file called
   CPYRIGHT, included with this distribution.


   Pine is in part based on The Elm Mail System:
    ***********************************************************************
    *  The Elm Mail System  -  Revision: 2.13                             *
    *                                                                     *
    * 			Copyright (c) 1986, 1987 Dave Taylor              *
    * 			Copyright (c) 1988, 1989 USENET Community Trust   *
    ***********************************************************************
 

  ----------------------------------------------------------------------*/

/*======================================================================
    strings.c
    Misc extra and useful string functions
      - rplstr         replace a substring with another string
      - sqzspaces      Squeeze out the extra blanks in a string
      - sqzsnewlines   Squeeze out \n and \r.
      - remove_trailing_white_space 
      - remove_leading_white_space 
      - strclean       Remove leading and trailing white space and convert
                       to lower case
      - strucmp        A case insensitive strcmp
      - struncmp       A case insensitve strncmp
      - srchstr        Search a string for a sub string
      - strindex       Replacement for strchr/index
      - strrindex      Replacement for strrchr/rindex
      - sstrcpy        Copy one string onto another, advancing dest'n pointer
      - istrncpy       Copy n chars between bufs, making ctrl chars harmless
      - month_abbrev   Return three letter abbreviations for months
      - month_num      Calculate month number from month/year string
      - cannon_date    Formalize format of a some what formatted date
      - pretty_command Return nice string describing character
      - blanks         Returns a string n blanks long
      - comatose       Format number with nice commas
      - byte_string    Format number of bytes with Kb, Mb, Gb or bytes
      - enth-string    Format number i.e. 1: 1st, 983: 983rd....
      - read_hex       Convert 1 or 2-digit hex string to integer
      - string_to_cstring  Convert string to C-style constant string with \'s
      - cstring_to_hexstring  Convert cstring to hex string

 ====*/

#include "headers.h"

void char_to_hex_pair PROTO((int, char *));
void char_to_octal_triple PROTO((int, char *));
int  read_octal PROTO((char *));


/*----------------------------------------------------------------------
       Replace n characters in one string with another given string

   args: os -- the output string
         dl -- the number of character to delete from start of os
         is -- The string to insert
  
 Result: returns pointer in originl string to end of string just inserted
         First 
  ---*/
char *
rplstr(os,dl,is)
char *os,*is;
int dl;
{   
    register char *x1,*x2,*x3;
    int           diff;

    if(os == NULL)
        return(NULL);
       
    for(x1 = os; *x1; x1++);
    if(dl > x1 - os)
        dl = x1 - os;
        
    x2 = is;      
    if(is != NULL){
        while(*x2++);
        x2--;
    }

    if((diff = (x2 - is) - dl) < 0){
        x3 = os; /* String shrinks */
        if(is != NULL)
            for(x2 = is; *x2; *x3++ = *x2++); /* copy new string in */
        for(x2 = x3 - diff; *x2; *x3++ = *x2++); /* shift for delete */
        *x3 = *x2;
    } else {                
        /* String grows */
        for(x3 = x1 + diff; x3 >= os + (x2 - is); *x3-- = *x1--); /* shift*/
        for(x1 = os, x2 = is; *x2 ; *x1++ = *x2++);
        while(*x3) x3++;                 
    }
    return(x3);
}



/*----------------------------------------------------------------------
     Squeeze out blanks 
  ----------------------------------------------------------------------*/
void
sqzspaces(string)
     char *string;
{
    char *p = string;

    while(*string = *p++)		   /* while something to copy       */
      if(!isspace((unsigned char)*string)) /* only really copy if non-blank */
	string++;
}



/*----------------------------------------------------------------------
     Squeeze out CR's and LF's 
  ----------------------------------------------------------------------*/
void
sqznewlines(string)
    char *string;
{
    char *p = string;

    while(*string = *p++)		      /* while something to copy  */
      if(*string != '\r' && *string != '\n')  /* only copy if non-newline */
	string++;
}



/*----------------------------------------------------------------------  
       Remove leading white space from a string in place
  
  Args: string -- string to remove space from
  ----*/
void
removing_leading_white_space(string)
     char *string;
{
    char *p;

    /* ignore the null/non-blank string */
    if(*(p = string) && isspace((unsigned char)*p)){
	/* find the first non-blank  */
	while(*p && isspace((unsigned char)*p))
	  p++;

	while(*string++ = *p++)		/* copy from there... */
	  ;
    }
}



/*----------------------------------------------------------------------  
       Remove trailing white space from a string in place
  
  Args: string -- string to remove space from
  ----*/
void
removing_trailing_white_space(string)
     char *string;
{
    char *p = NULL;

    for(; *string; string++)		/* remember start of whitespace */
      p = (!isspace((unsigned char)*string)) ? NULL : (!p) ? string : p;

    if(p)				/* if whitespace, blast it */
      *p = '\0';
}



/*----------------------------------------------------------------------  
       Remove quotes from a string in place
  
  Args: string -- string to remove quotes from
  Rreturns: string passed us, but with quotes gone
  ----*/
char *
removing_quotes(string)
    char *string;
{
    register char *p, *q;

    if(*(p = q = string) == '\"'){
	do
	  if(*q == '\"' || *q == '\\')
	    q++;
	while(*p++ = *q++);
    }

    return(string);
}



/*--------------------------------------------------
     A case insensitive strcmp()     
  
   Args: o, r -- The two strings to compare

 Result: integer indicating which is greater
  ---*/
strucmp(o, r)
    register char *o, *r;
{
    if(o == NULL){
	if(r == NULL)
	  return 0;
	else
	  return -1;
    }
    else if(r == NULL)
      return 1;

    while(*o && *r
	  && ((isupper((unsigned char)(*o))
				  ? (unsigned char)tolower((unsigned char)(*o))
				  : (unsigned char)(*o))
	     == (isupper((unsigned char)(*r))
				  ? (unsigned char)tolower((unsigned char)(*r))
				  : (unsigned char)(*r)))){
	o++;
	r++;
    }

    return((isupper((unsigned char)(*o))
				? tolower((unsigned char)(*o))
				: (int)(unsigned char)(*o))
	   - (isupper((unsigned char)(*r))
			        ? tolower((unsigned char)(*r))
				: (int)(unsigned char)(*r)));
}


/*---------------------------------------------------
     Remove leading whitespace, trailing whitespace and convert 
     to lowercase

   Args: s, -- The string to clean

 Result: the cleaned string
  ----*/
char *
strclean(string)
     char *string;
{
    char *s = string, *sc = NULL, *p = NULL;

    for(; *s; s++){				/* single pass */
	if(!isspace((unsigned char)*s)){
	    p = NULL;				/* not start of blanks   */
	    if(!sc)				/* first non-blank? */
	      sc = string;			/* start copying */
	}
	else if(!p)				/* it's OK if sc == NULL */
	  p = sc;				/* start of blanks? */

	if(sc)					/* if copying, copy */
	  *sc++ = isupper((unsigned char)(*s))
			  ? (unsigned char)tolower((unsigned char)(*s))
			  : (unsigned char)(*s);
    }

    if(p)					/* if ending blanks  */
      *p = '\0';				/* tie off beginning */
    else if(!sc)				/* never saw a non-blank */
      *string = '\0';				/* so tie whole thing off */

    return(string);
}



/*----------------------------------------------------------------------
     A case insensitive strncmp()     
  
   Args: o, r -- The two strings to compare
         n    -- length to stop comparing strings at

 Result: integer indicating which is greater
   
  ----*/
struncmp(o, r, n)
    register char *o,
		  *r;
    register int   n;
{
    if(o == NULL){
	if(r == NULL)
	  return 0;
	else
	  return -1;
    }
    else if(r == NULL)
      return 1;

    n--;
    while(n && *o && *r
	  && ((isupper((unsigned char)(*o))
				  ? (unsigned char)tolower((unsigned char)(*o))
				  : (unsigned char)(*o))
	     == (isupper((unsigned char)(*r))
				  ? (unsigned char)tolower((unsigned char)(*r))
				  : (unsigned char)(*r)))){
	o++;
	r++;
	n--;
    }

    return((isupper((unsigned char)(*o))
				? tolower((unsigned char)(*o))
				: (int)(unsigned char)(*o))
	   - (isupper((unsigned char)(*r))
			        ? tolower((unsigned char)(*r))
				: (int)(unsigned char)(*r)));
}



/*----------------------------------------------------------------------
        Search one string for another

   Args:  is -- The string to search in, the larger string
          ss -- The string to search for, the smaller string

   Search for any occurance of ss in the is, and return a pointer
   into the string is when it is found. The search is case indepedent.
  ----*/

char *	    
srchstr(is, ss)
register char *is, *ss;
{                    
    register char *sx, *sy;
    char          *ss_store, *rv;
    char          temp[251];
    
    if(is == NULL || ss == NULL)
      return(NULL);

    if(strlen(ss) > sizeof(temp) - 2)
      ss_store = (char *)fs_get(strlen(ss) + 1);
    else
      ss_store = temp;

    for(sx = ss, sy = ss_store; *sx != '\0' ; sx++, sy++)
      *sy = isupper((unsigned char)(*sx))
		      ? (unsigned char)tolower((unsigned char)(*sx))
		      : (unsigned char)(*sx);
    *sy = *sx;

    rv = NULL;
    while(*is != '\0'){
        for(sx = is, sy = ss_store;
	    ((*sx == *sy)
	      || ((isupper((unsigned char)(*sx))
		     ? (unsigned char)tolower((unsigned char)(*sx))
		     : (unsigned char)(*sx)) == (unsigned char)(*sy))) && *sy;
	    sx++, sy++)
	   ;

        if(!*sy){
            rv = is;
            break;
        }

        is++;
    }

    if(ss_store != temp)
      fs_give((void **)&ss_store);

    return(rv);
}



/*----------------------------------------------------------------------
    A replacement for strchr or index ...

    Returns a pointer to the first occurance of the character
    'ch' in the specified string or NULL if it doesn't occur

 ....so we don't have to worry if it's there or not. We bring our own.
If we really care about efficiency and think the local one is more
efficient the local one can be used, but most of the things that take
a long time are in the c-client and not in pine.
 ----*/
char *
strindex(buffer, ch)
    char *buffer;
    int ch;
{
    do
      if(*buffer == ch)
	return(buffer);
    while (*buffer++ != '\0');

    return(NULL);
}


/* Returns a pointer to the last occurance of the character
 * 'ch' in the specified string or NULL if it doesn't occur
 */
char *
strrindex(buffer, ch)
    char *buffer;
    int   ch;
{
    char *address = NULL;

    do
      if(*buffer == ch)
	address = buffer;
    while (*buffer++ != '\0');
    return(address);
}



/*----------------------------------------------------------------------
  copy the source string onto the destination string returning with
  the destination string pointer at the end of the destination text

  motivation for this is to avoid twice passing over a string that's
  being appended to twice (i.e., strcpy(t, x); t += strlen(t))
 ----*/
void
sstrcpy(d, s)
    char **d;
    char *s;
{
    while((**d = *s++) != '\0')
      (*d)++;
}


/*----------------------------------------------------------------------
  copy at most n chars of the source string onto the destination string
  returning pointer to start of destination and converting any undisplayable
  characters to harmless character equivalents.
 ----*/
char *
istrncpy(d, s, n)
    char *d, *s;
    int n;
{
    char *rv = d;

    do
      if(F_OFF(F_PASS_CONTROL_CHARS, ps_global) && *s && CAN_DISPLAY(*s))
	if(n-- >= 0){
	    *d++ = '^';

	    if(n-- >= 0)
	      *d++ = *s++ + '@';
	}
    while(n-- >= 0 && (*d++ = *s++));

    return(rv);
}



char *xdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", NULL};

char *
month_abbrev(month_num)
     int month_num;
{
    static char *xmonths[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec", NULL};
    if(month_num < 1 || month_num > 12)
      return("xxx");
    return(xmonths[month_num - 1]);
}

char *
week_abbrev(week_day)
     int week_day;
{
    return(xdays[week_day]);
}


days_in_month(month, year)
     int month, year;
{
    static int d_i_m[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};

    if(month == 2) 
      return(((year%4) == 0 && (year%100) != 0) ? 29 : 28);
    else
      return(d_i_m[month]);
}
    


/*----------------------------------------------------------------------
      Return month number of month named in string
  
   Args: s -- string with 3 letter month abbreviation of form mmm-yyyy
 
 Result: Returns month number with January, year 1900, 2000... being 0;
         -1 if no month/year is matched
 ----*/
int
month_num(s)
     char *s;
{
    int month, year;
    int i;

    for(i = 0; i < 12; i++){
        if(struncmp(month_abbrev(i+1), s, 3) == 0)
          break;
    }
    if(i == 12)
      return(-1);

    year = atoi(s + 4);
    if(year == 0)
      return(-1);

    month = (year < 100 ? year + 1900 : year)  * 12 + i;
    return(month);
}


/*
 * Structure containing all knowledge of symbolic time zones.
 * To add support for a given time zone, add it here, but make sure
 * the zone name is in upper case.
 */
static struct {
    char  *zone;
    short  len,
    	   hour_offset,
	   min_offset;
} known_zones[] = {
    {"PST", 3, -8, 0},			/* Pacific Standard */
    {"PDT", 3, -7, 0},			/* Pacific Daylight */
    {"MST", 3, -7, 0},			/* Mountain Standard */
    {"MDT", 3, -6, 0},			/* Mountain Daylight */
    {"CST", 3, -6, 0},			/* Central Standard */
    {"CDT", 3, -5, 0},			/* Central Daylight */
    {"EST", 3, -5, 0},			/* Eastern Standard */
    {"EDT", 3, -4, 0},			/* Eastern Daylight */
    {"JST", 3,  9, 0},			/* Japan Standard */
    {"GMT", 3,  0, 0},			/* Universal Time */
    {"UT",  2,  0, 0},			/* Universal Time */
#ifdef	IST_MEANS_ISREAL
    {"IST", 3,  2, 0},			/* Israel Standard */
#else
#ifdef	IST_MEANS_INDIA
    {"IST", 3,  5, 30},			/* India Standard */
#endif
#endif
    {NULL, 0, 0},
};

/*----------------------------------------------------------------------
  Parse date in or near RFC-822 format into the date structure

Args: given_date -- The input string to parse
      d          -- Pointer to a struct date to place the result in
 
Returns nothing

The following date fomrats are accepted:
  WKDAY DD MM YY HH:MM:SS ZZ
  DD MM YY HH:MM:SS ZZ
  WKDAY DD MM HH:MM:SS YY ZZ
  DD MM HH:MM:SS YY ZZ
  DD MM WKDAY HH:MM:SS YY ZZ
  DD MM WKDAY YY MM HH:MM:SS ZZ

All leading, intervening and trailing spaces tabs and commas are ignored.
The prefered formats are the first or second ones.  If a field is unparsable
it's value is left as -1. 

  ----*/
void
parse_date(given_date, d)
     char        *given_date;
     struct date *d;
{
    char *p, **i, *q, n;
    int   month;

    d->sec   = -1;
    d->minute= -1;
    d->hour  = -1;
    d->day   = -1;
    d->month = -1;
    d->year  = -1;
    d->wkday = -1;
    d->hours_off_gmt = -1;
    d->min_off_gmt   = -1;

    if(given_date == NULL)
      return;

    p = given_date;
    while(*p && isspace((unsigned char)*p))
      p++;

    /* Start with month, weekday or day ? */
    for(i = xdays; *i != NULL; i++) 
      if(struncmp(p, *i, 3) == 0) /* Match first 3 letters */
        break;
    if(*i != NULL) {
        /* Started with week day */
        d->wkday = i - xdays;
        while(*p && !isspace((unsigned char)*p) && *p != ',')
          p++;
        while(*p && (isspace((unsigned char)*p) || *p == ','))
          p++;
    }
    if(isdigit((unsigned char)*p)) {
        d->day = atoi(p);
        while(*p && isdigit((unsigned char)*p))
          p++;
        while(*p && (*p == '-' || *p == ',' || isspace((unsigned char)*p)))
          p++;
    }
    for(month = 1; month <= 12; month++)
      if(struncmp(p, month_abbrev(month), 3) == 0)
        break;
    if(month < 13) {
        d->month = month;

    } 
    /* Move over month, (or whatever is there) */
    while(*p && !isspace((unsigned char)*p) && *p != ',' && *p != '-')
       p++;
    while(*p && (isspace((unsigned char)*p) || *p == ',' || *p == '-'))
       p++;

    /* Check again for day */
    if(isdigit((unsigned char)*p) && d->day == -1) {
        d->day = atoi(p);
        while(*p && isdigit((unsigned char)*p))
          p++;
        while(*p && (*p == '-' || *p == ',' || isspace((unsigned char)*p)))
          p++;
    }

    /*-- Check for time --*/
    for(q = p; *q && isdigit((unsigned char)*q); q++);
    if(*q == ':') {
        /* It's the time (out of place) */
        d->hour = atoi(p);
        while(*p && *p != ':' && !isspace((unsigned char)*p))
          p++;
        if(*p == ':') {
            p++;
            d->minute = atoi(p);
            while(*p && *p != ':' && !isspace((unsigned char)*p))
              p++;
            if(*p == ':') {
                d->sec = atoi(p);
                while(*p && !isspace((unsigned char)*p))
                  p++;
            }
        }
        while(*p && isspace((unsigned char)*p))
          p++;
    }
    

    /* Get the year 0-49 is 2000-2049; 50-100 is 1950-1999 and
                                           101-9999 is 101-9999 */
    if(isdigit((unsigned char)*p)) {
        d->year = atoi(p);
        if(d->year < 50)   
          d->year += 2000;
        else if(d->year < 100)
          d->year += 1900;
        while(*p && isdigit((unsigned char)*p))
          p++;
        while(*p && (*p == '-' || *p == ',' || isspace((unsigned char)*p)))
          p++;
    } else {
        /* Something wierd, skip it and try to resynch */
        while(*p && !isspace((unsigned char)*p) && *p != ',' && *p != '-')
          p++;
        while(*p && (isspace((unsigned char)*p) || *p == ',' || *p == '-'))
          p++;
    }

    /*-- Now get hours minutes, seconds and ignore tenths --*/
    for(q = p; *q && isdigit((unsigned char)*q); q++);
    if(*q == ':' && d->hour == -1) {
        d->hour = atoi(p);
        while(*p && *p != ':' && !isspace((unsigned char)*p))
          p++;
        if(*p == ':') {
            p++;
            d->minute = atoi(p);
            while(*p && *p != ':' && !isspace((unsigned char)*p))
              p++;
            if(*p == ':') {
                p++;
                d->sec = atoi(p);
                while(*p && !isspace((unsigned char)*p))
                  p++;
            }
        }
    }
    while(*p && isspace((unsigned char)*p))
      p++;


    /*-- The time zone --*/
    d->hours_off_gmt = 0;
    d->min_off_gmt = 0;
    if(*p) {
        if((*p == '+' || *p == '-')
	   && isdigit((unsigned char)p[1])
	   && isdigit((unsigned char)p[2])
	   && isdigit((unsigned char)p[3])
	   && isdigit((unsigned char)p[4])
	   && !isdigit((unsigned char)p[5])) {
            char tmp[3];
            d->min_off_gmt = d->hours_off_gmt = (*p == '+' ? 1 : -1);
            p++;
            tmp[0] = *p++;
            tmp[1] = *p++;
            tmp[2] = '\0';
            d->hours_off_gmt *= atoi(tmp);
            tmp[0] = *p++;
            tmp[1] = *p++;
            tmp[2] = '\0';
            d->min_off_gmt *= atoi(tmp);
        } else {
	    for(n = 0; known_zones[n].zone; n++)
	      if(struncmp(p, known_zones[n].zone, known_zones[n].len) == 0){
		  d->hours_off_gmt = (int) known_zones[n].hour_offset;
		  d->min_off_gmt   = (int) known_zones[n].min_offset;
		  break;
	      }
        }
    }
    dprint(9, (debugfile,
	 "Parse date: \"%s\" to..  hours_off_gmt:%d  min_off_gmt:%d\n",
               given_date, d->hours_off_gmt, d->min_off_gmt));
    dprint(9, (debugfile,
	       "Parse date: wkday:%d  month:%d  year:%d  day:%d  hour:%d  min:%d  sec:%d\n",
            d->wkday, d->month, d->year, d->day, d->hour, d->minute, d->sec));
}



/*----------------------------------------------------------------------
    Convert the given date to GMT

  Args -- d:  The date to be converted in place
 ----*/
void
convert_to_gmt(d)
    MESSAGECACHE *d;
{
    int minutes = d->minutes,			/* 0-59 */
        hours	= d->hours,			/* 0-23 */
	day	= d->day,			/* 1-31 */
	month	= d->month,			/* 1-12 */
	year	= d->year;			/* since 1969 */

    if(d->zhours == 0 && d->zminutes == 0)	/* no offset! */
      return;

    if(d->zoccident){				/* adjust for distance */
	minutes += d->zminutes;			/* ... west of GMT */
	hours	+= d->zhours;
    }
    else{
	minutes -= d->zminutes;
	hours	-= d->zhours;
    }

    if(minutes < 0){				/* go to previous hour */
	hours--;
	minutes += 60;
    }
    else if(minutes > 59){			/* next hour */
	hours++;
	minutes -= 60;
    }

    if(hours < 0){				/* previous day */
	day--;
	hours += 24;
    }
    else if(hours > 23){			/* next day */
	day++;
	hours -= 24;
    }

    if(day < 0){				/* previous month */
	if(--month <= 0){
	    month = 12;
	    year--;
	}
	day = days_in_month(month);
    }
    else if(day > days_in_month(month)){  /* next month */
	if(++month > 12){
	    month = 0;
	    year++;
	}
	day = 1;
    }

    d->zoccident = 0;
    d->zhours    = 0;
    d->zminutes  = 0;
    d->year      = year;
    d->month     = month;
    d->day       = day;
    d->hours     = hours;
    d->minutes   = minutes;
}



/*----------------------------------------------------------------------
  The arguments are pointers to c-client MESSAGECACHE elts which have
  had dates placed into them.
  ----*/
compare_dates(d1, d2)
    MESSAGECACHE *d1, *d2;
{
    /*
     * Check to see if the two dates are in different timezones, and
     * convert to gmt if they are.
     */
    if(!(d1->zoccident == d2->zoccident &&
         d1->zhours    == d2->zhours    &&
         d1->zminutes  == d2->zminutes)){
	convert_to_gmt(d1);
	convert_to_gmt(d2);
    }

    /* Now do the compare */
    if(d1->year == d2->year)
      if(d1->month == d2->month)
        if(d1->day == d2->day)
          if(d1->hours == d2->hours)
            if(d1->minutes == d2->minutes)
              if(d1->seconds == d2->seconds)
		return 0;
	      else
                return((int)(d1->seconds - d2->seconds));
	    else
              return((int)(d1->minutes - d2->minutes));
	  else
            return((int)(d1->hours - d2->hours));
	else
          return((int)(d1->day - d2->day));
      else
        return((int)(d1->month - d2->month));
    else
      return((int)(d1->year - d2->year));
}
    

/*----------------------------------------------------------------------
     Map some of the special characters into sensible strings for human
   consumption.
  ----*/
char *
pretty_command(c)
     int c;
{
    static char  buf[10];
    char	*s;

    switch(c){
      case '\033'    : s = "ESC";		break;
      case '\177'    : s = "DEL";		break;
      case ctrl('I') : s = "TAB";		break;
      case ctrl('J') : s = "LINEFEED";		break;
      case ctrl('M') : s = "RETURN";		break;
      case ctrl('Q') : s = "XON";		break;
      case ctrl('S') : s = "XOFF";		break;
      case KEY_UP    : s = "Up Arrow";		break;
      case KEY_DOWN  : s = "Down Arrow";	break;
      case KEY_RIGHT : s = "Right Arrow";	break;
      case KEY_LEFT  : s = "Left Arrow";	break;
      case KEY_PGUP  : s = "Prev Page";		break;
      case KEY_PGDN  : s = "Next Page";		break;
      case KEY_HOME  : s = "Home";		break;
      case KEY_END   : s = "End";		break;
      case KEY_DEL   : s = "Delete";		break; /* Not necessary DEL! */
      case PF1	     :
      case PF2	     :
      case PF3	     :
      case PF4	     :
      case PF5	     :
      case PF6	     :
      case PF7	     :
      case PF8	     :
      case PF9	     :
      case PF10	     :
      case PF11	     :
      case PF12	     :
        sprintf(s = buf, "F%d", c - PF1 + 1);
	break;

      default:
	if(c < ' ')
	  sprintf(s = buf, "^%c", c + 'A' - 1);
	else
	  sprintf(s = buf, "%c", c);

	break;
    }

    return(s);
}
        
    

/*----------------------------------------------------------------------
     Create a little string of blanks of the specified length.
   Max n is 511.
  ----*/
char *
repeat_char(n, c)
     int  n;
     int  c;
{
    static char bb[512];
    if(n > sizeof(bb))
       n = sizeof(bb) - 1;
    bb[n--] = '\0';
    while(n >= 0)
      bb[n--] = c;
    return(bb);
}


/*----------------------------------------------------------------------
        Turn a number into a string with comma's

   Args: number -- The long to be turned into a string. 

  Result: pointer to static string representing number with commas
  ---*/
char *
comatose(number) 
    long number;
{
#ifdef	DOS
    static char buf[16];		/* no numbers > 1 trillion! */
    char *b;
    short i;

    if(!number)
	return("0");

    if(number < 0x7FFFFFFFL){		/* largest DOS signed long */
        buf[15] = '\0';
        b = &buf[14];
        i = 2;
	while(number){
 	    *b-- = (number%10) + '0';
	    if((number /= 10) && i-- == 0 ){
		*b-- = ',';
		i = 2;
	    }
	}
    }
    else
      return("Number too big!");		/* just fits! */

    return(++b);
#else
    long        i, x, done_one;
    static char buf[100];
    char       *b;

    dprint(9, (debugfile, "comatose(%ld) returns:", number));
    if(number == 0){
        strcpy(buf, "0");
        return(buf);
    }
    
    done_one = 0;
    b = buf;
    for(i = 1000000000; i >= 1; i /= 1000) {
	x = number / i;
	number = number % i;
	if(x != 0 || done_one) {
	    if(b != buf)
	      *b++ = ',';
	    sprintf(b, done_one ? "%03ld" : "%d", x);
	    b += strlen(b);
	    done_one = 1;
	}
    }
    *b = '\0';

    dprint(9, (debugfile, "\"%s\"\n", buf));

    return(buf);
#endif	/* DOS */
}



/*----------------------------------------------------------------------
   Format number as amount of bytes, appending Kb, Mb, Gb, bytes

  Args: bytes -- number of bytes to format

 Returns pointer to static string. The numbers are divided to produce a 
nice string with precision of about 2-4 digits
    ----*/
char *
byte_string(bytes)
     long bytes;
{
    char       *a, aa[5];
    char       *abbrevs = "GMK";
    long        i, ones, tenths;
    static char string[10];

    ones   = 0L;
    tenths = 0L;

    if(bytes == 0L){
        strcpy(string, "0 bytes");
    } else {
        for(a = abbrevs, i = 1000000000; i >= 1; i /= 1000, a++) {
            if(bytes > i) {
                ones = bytes/i;
                if(ones < 10L && i > 10L)
                  tenths = (bytes - (ones * i)) / (i / 10L);
                break;
            }
        }
    
        aa[0] = *a;  aa[1] = '\0'; 
    
        if(tenths == 0)
          sprintf(string, "%ld%s%s", ones, aa, *a ? "B" : "bytes");
        else
          sprintf(string, "%ld.%ld%s%s", ones, tenths, aa, *a ? "B" : "bytes");
    }

    return(string);
}



/*----------------------------------------------------------------------
    Print a string corresponding to the number given:
      1st, 2nd, 3rd, 105th, 92342nd....
 ----*/

char *
enth_string(i)
     int i;
{
    static char enth[10];

    switch (i % 10) {
        
      case 1:
        if( (i % 100 ) == 11)
          sprintf(enth,"%dth", i);
        else
          sprintf(enth,"%dst", i);
        break;

      case 2:
        if ((i % 100) == 12)
          sprintf(enth, "%dth",i);
        else
          sprintf(enth, "%dnd",i);
        break;

      case 3:
        if(( i % 100) == 13)
          sprintf(enth, "%dth",i);
        else
          sprintf(enth, "%drd",i);
        break;

      default:
        sprintf(enth,"%dth",i);
        break;
    }
    return(enth);
}


char *
long2string(l)
     long l;
{
    static char string[20];
    sprintf(string, "%ld", l);
    return(string);
}

char *
int2string(i)
     int i;
{
    static char string[20];
    sprintf(string, "%d", i);
    return(string);
}


/*
 * strtoval - convert the given string to a positive integer.
 */
char *
strtoval(s, val, minmum, maxmum, errbuf, varname)
    char *s;
    int  *val;
    int   minmum, maxmum;
    char *errbuf, *varname;
{
    int   i = 0;
    char *p = s, *errstr = NULL;

    removing_leading_white_space(p);
    removing_trailing_white_space(p);
    for(; *p; p++)
      if(isdigit((unsigned char)*p)){
	  i = (i * 10) + (*p - '0');
      }
      else{
	  sprintf(errstr = errbuf,
		  "Non-numeric value ('%c' in \"%.8s\") in %s. Using \"%d\"",
		  *p, s, varname, *val);
	  return(errbuf);
      }

    /* range describes acceptable values */
    if(maxmum > minmum && (i < minmum || i > maxmum))
      sprintf(errstr = errbuf,
	      "%s of %d not supported (M%s %d). Using \"%d\"",
	      varname, i, (i > maxmum) ? "ax" : "in",
	      (i > maxmum) ? maxmum : minmum, *val);
    /* range describes unacceptable values */
    else if(minmum > maxmum && !(i < maxmum || i > minmum))
      sprintf(errstr = errbuf, "%s of %d not supported. Using \"%d\"",
	      varname, i, *val);
    else
      *val = i;

    return(errstr);
}


/*
 *  Function to parse the given string into two space-delimited fields
 */
void
get_pair(string, label, value, firstws)
    char *string, **label, **value;
    int   firstws;
{
    char *p, *token = NULL;
    int	  quoted = 0;

    *label = *value = NULL;
    for(p = string; *p;){
	if(*p == '"')				/* quoted label? */
	  quoted = (quoted) ? 0 : 1;

	if(*p == '\\' && *(p+1) == '"')		/* escaped quote? */
	  p++;					/* skip it... */

	if(isspace((unsigned char)*p) && !quoted){	/* if space,  */
	    while(*++p && isspace((unsigned char)*p))	/* move past it */
	      ;

	    if(!firstws || !token)
	      token = p;			/* remember start of text */
	}
	else
	  p++;
    }

    if(token){
	*label = p = (char *)fs_get(((token - string) + 1) * sizeof(char));
	for(; string < token ; string++){
	    if(*string == '\\' && *(string+1) == '"')
	      *p++ = *++string;
	    else if(*string != '"')
	      *p++ = *string;
	}

	*p = '\0';				/* tie off nickname */
	removing_trailing_white_space(*label);
	*value = cpystr(token);
    }
    else
      *value = cpystr(string);
}


/*
 * Convert a 1 or 2-digit hex string into an 8-bit character.
 * The input string should be checked for hex'ness before calling this.
 * Only the first two characters of s will be used, and it is ok not
 * to null-terminate it.
 */
int
read_hex(s)
    char *s;
{
    register int i, c, j;

    i = 0;
    for(j = 0; j < 2 && *s && isxdigit((unsigned char)*s); s++,j++){
	c = (unsigned char)(*s);
	if(isupper((unsigned char)c))
	  c = tolower((unsigned char)c);

	i = i*16 + (isdigit((unsigned char)c)  ? c - '0'
					       : c - 'a' + 10);
    }

    return(i);
}


/*
 * Convert a 1, 2, or 3-digit octal string into an 8-bit character.
 * Only the first three characters of s will be used, and it is ok not
 * to null-terminate it.
 */
int
read_octal(s)
    char *s;
{
    register int i, c, j;

    i = 0;
    for(j = 0; j < 3 && *s && isdigit((unsigned char)(*s)); s++,j++)
      i = i*8 + (int)(unsigned char)(*s) - '0';

    return(i);
}


/*
 * Given a character c, put the 3-digit ascii octal value of that char
 * in the 2nd argument, which must be at least 3 in length.
 */
void
char_to_octal_triple(c, octal)
    int   c;
    char *octal;
{
    c &= 0xff;

    octal[2] = (c % 8) + '0';
    c /= 8;
    octal[1] = (c % 8) + '0';
    c /= 8;
    octal[0] = c + '0';
}


/*
 * Given a character c, put the 2-digit ascii hex value of that char
 * in the 2nd argument, which must be at least 2 in length.
 */
void
char_to_hex_pair(c, hex)
    int   c;
    char *hex;
{
    int d;

    c &= 0xff;
    d = c % 16;
    hex[1] = (d < 10) ? ('0' + d) : ('a' + d - 10);
    c /= 16;
    hex[0] = (c < 10) ? ('0' + c) : ('a' + c - 10);
}


/*
 * Convert in memory string s to a C-style string, with backslash escapes
 * like they're used in C character constants.
 *
 * Returns allocated C string version of s.
 */
char *
string_to_cstring(s)
    char *s;
{
    char *b, *p;
    int   n, i;

    if(!s)
      return(cpystr(""));

    n = 20;
    b = (char *)fs_get((n+1) * sizeof(char));
    p  = b;
    *p = '\0';
    i  = 0;

    while(*s){
	if(i + 4 > n){
	    /*
	     * The output string may overflow the output buffer.
	     * Make more room.
	     */
	    n += 20;
	    fs_resize((void **)&b, (n+1) * sizeof(char));
	    p = &b[i];
	}
	else{
	    switch(*s){
	      case '\n':
		*p++ = '\\';
		*p++ = 'n';
		i += 2;
		break;

	      case '\r':
		*p++ = '\\';
		*p++ = 'r';
		i += 2;
		break;

	      case '\t':
		*p++ = '\\';
		*p++ = 't';
		i += 2;
		break;

	      case '\b':
		*p++ = '\\';
		*p++ = 'b';
		i += 2;
		break;

	      case '\f':
		*p++ = '\\';
		*p++ = 'f';
		i += 2;
		break;

	      case '\\':
		*p++ = '\\';
		*p++ = '\\';
		i += 2;
		break;

	      default:
		if(*s >= SPACE && *s <= '~'){
		    *p++ = *s;
		    i++;
		}
		else{  /* use octal output */
		    *p++ = '\\';
		    char_to_octal_triple(*s, p);
		    p += 3;
		    i += 4;
		}

		break;
	    }

	    s++;
	}
    }

    *p = '\0';
    return(b);
}


/*
 * Convert C-style string, with backslash escapes, into a hex string, two
 * hex digits per character.
 *
 * Returns allocated hexstring version of s.
 */
char *
cstring_to_hexstring(s)
    char *s;
{
    char *b, *p;
    int   n, i, c;

    if(!s)
      return(cpystr(""));

    n = 20;
    b = (char *)fs_get((n+1) * sizeof(char));
    p  = b;
    *p = '\0';
    i  = 0;

    while(*s){
	if(i + 2 > n){
	    /*
	     * The output string may overflow the output buffer.
	     * Make more room.
	     */
	    n += 20;
	    fs_resize((void **)&b, (n+1) * sizeof(char));
	    p = &b[i];
	}
	else{
	    if(*s == '\\'){
		s++;
		switch(*s){
		  case 'n':
		    c = '\n';
		    char_to_hex_pair(c, p);
		    i += 2;
		    p += 2;
		    s++;
		    break;

		  case 'r':
		    c = '\r';
		    char_to_hex_pair(c, p);
		    i += 2;
		    p += 2;
		    s++;
		    break;

		  case 't':
		    c = '\t';
		    char_to_hex_pair(c, p);
		    i += 2;
		    p += 2;
		    s++;
		    break;

		  case 'v':
		    c = '\v';
		    char_to_hex_pair(c, p);
		    i += 2;
		    p += 2;
		    s++;
		    break;

		  case 'b':
		    c = '\b';
		    char_to_hex_pair(c, p);
		    i += 2;
		    p += 2;
		    s++;
		    break;

		  case 'f':
		    c = '\f';
		    char_to_hex_pair(c, p);
		    i += 2;
		    p += 2;
		    s++;
		    break;

		  case 'a':
		    c = '\007';
		    char_to_hex_pair(c, p);
		    i += 2;
		    p += 2;
		    s++;
		    break;

		  case '\\':
		    c = '\\';
		    char_to_hex_pair(c, p);
		    i += 2;
		    p += 2;
		    s++;
		    break;

		  case '?':
		    c = '?';
		    char_to_hex_pair(c, p);
		    i += 2;
		    p += 2;
		    s++;
		    break;

		  case '\'':
		    c = '\'';
		    char_to_hex_pair(c, p);
		    i += 2;
		    p += 2;
		    s++;
		    break;

		  case '\"':
		    c = '\"';
		    char_to_hex_pair(c, p);
		    i += 2;
		    p += 2;
		    s++;
		    break;

		  case 0: /* reached end of s too early */
		    c = 0;
		    char_to_hex_pair(c, p);
		    i += 2;
		    p += 2;
		    s++;
		    break;

		  /* hex number */
		  case 'x':
		    s++;
		    if(isxdigit((unsigned char)*s)){
			c = read_hex(s);
			s++;
			if(isxdigit((unsigned char)*s))
			  s++;
		    }
		    else
		      c = 0;

		    char_to_hex_pair(c, p);
		    i += 2;
		    p += 2;

		    break;

		  /* octal number */
		  default:
		    if(isdigit((unsigned char)*s)){
			c = read_octal(s);
			s++;
			if(isdigit((unsigned char)*s)){
			    s++;
			    if(isdigit((unsigned char)*s))
			      s++;
			}
		    }
		    else
		      c = 0;

		    char_to_hex_pair(c, p);
		    i += 2;
		    p += 2;

		    break;
		}
	    }
	    else{
		char_to_hex_pair(*s, p);
		i += 2;
		p += 2;
		s++;
	    }
	}
    }

    *p = '\0';
    return(b);
}


/*
 * Returns non-zero if dir is a prefix of path.
 *         zero     if dir is not a prefix of path, or if dir is empty.
 */
int
in_dir(dir, path)
    char *dir;
    char *path;
{
    return(*dir ? !strncmp(dir, path, strlen(dir)) : 0);
}



/*
 * Return pointer to given string after turning off all hi-order bits
 */
char *
bitstrip(s)
    char *s;
{
    register char *p = s;

    while(*p &= 0x7f)
      p++;

    return(s);
}



/*
 *  * * * * * * * *      RFC 1522 support routines      * * * * * * * *
 *
 *   RFC 1522 support is *very* loosely based on code contributed
 *   by Lars-Erik Johansson <lej@cdg.chalmers.se>.  Thanks to Lars-Erik,
 *   and appologies for taking such liberties with his code.
 */


#define	RFC1522_INIT	"=?"
#define	RFC1522_INIT_L	2
#define RFC1522_TERM	"?="
#define	RFC1522_TERM_L	2
#define	RFC1522_DLIM	"?"
#define	RFC1522_DLIM_L	1
#define	RFC1522_MAXW	75
#define	ESPECIALS	"()<>@,;:\"/[]?.="
#define	RFC1522_OVERHEAD(S)	(RFC1522_INIT_L + RFC1522_TERM_L +	\
				 (2 * RFC1522_DLIM_L) + strlen(S) + 1);
#define	RFC1522_ENC_CHAR(C)	(((C) & 0x80) || !rfc1522_valtok(C)	\
				 || (C) == '_' )


int	       rfc1522_token PROTO((char *, int (*) PROTO((int)), char *,
				    char **));
int	       rfc1522_valtok PROTO((int));
int	       rfc1522_valenc PROTO((int));
int	       rfc1522_valid PROTO((char *, char **, char **, char **,
				    char **));
char	      *rfc1522_8bit PROTO((void *, int));
char	      *rfc1522_binary PROTO((void *, int));
unsigned char *rfc1522_encoded_word PROTO((unsigned char *, int, char *));


/*
 * rfc1522_decode - decode the given source string ala RFC 1522,
 *		    IF NECESSARY, into the given destination buffer.
 *		    Don't bother copying if it turns out decoding
 *		    isn't necessary.
 *
 * Returns: pointer to either the destination buffer containing the
 *	    decoded text, or a pointer to the source buffer if there was
 *	    no reason to decode it.
 */
unsigned char *
rfc1522_decode(d, s, charset)
    unsigned char  *d;
    char	   *s;
    char	  **charset;
{
    unsigned char *rv = NULL, *p;
    char	  *start = s, *sw, *cset, *enc, *txt, *ew, **q;
    unsigned long  l;
    int		   i;

    *d = '\0';					/* init destination */
    if(charset)
      *charset = NULL;

    while(s && (sw = strstr(s, RFC1522_INIT))){
	if(!rv)
	  rv = d;				/* remember start of dest */

	/* validate the rest of the encoded-word */
	if(rfc1522_valid(sw, &cset, &enc, &txt, &ew)){
	    /* copy everything between s and sw to destination */
	    for(i = 0; &s[i] < sw; i++)
	      if(!isspace((unsigned char)s[i])){ /* if some non-whitespace */
		  while(s < sw)
		    *d++ = (unsigned char) *s++;

		  break;
	      }

	    enc[-1] = txt[-1] = ew[0] = '\0';	/* tie off token strings */

	    /* Insert text explaining charset if we don't know what it is */
	    if((!ps_global->VAR_CHAR_SET
		|| strucmp((char *) cset, ps_global->VAR_CHAR_SET))
	       && strucmp((char *) cset, "US-ASCII")){
		dprint(5, (debugfile, "RFC1522_decode: charset mismatch: %s\n",
			   cset));
		if(charset){
		    if(!*charset)		/* only write first charset */
		      *charset = cpystr(cset);
		}
		else{
		    *d++ = '[';
		    sstrcpy((char **) &d, cset);
		    *d++ = ']';
		    *d++ = SPACE;
		}
	    }

	    /* based on encoding, write the encoded text to output buffer */
	    switch(*enc){
	      case 'Q' :			/* 'Q' encoding */
	      case 'q' :
		/* special hocus-pocus to deal with '_' exception, too bad */
		for(l = 0L, i = 0; txt[l]; l++)
		  if(txt[l] == '_')
		    i++;

		if(i){
		    q = (char **) fs_get((i + 1) * sizeof(char *));
		    for(l = 0L, i = 0; txt[l]; l++)
		      if(txt[l] == '_'){
			  q[i++] = &txt[l];
			  txt[l] = SPACE;
		      }

		    q[i] = NULL;
		}
		else
		  q = NULL;

		if(p = rfc822_qprint((unsigned char *)txt, strlen(txt), &l)){
		    strcpy((char *) d, (char *) p);
		    fs_give((void **)&p);	/* free encoded buf */
		    d += l;			/* advance dest ptr to EOL */
		}
		else
		  goto bogus;

		if(q){				/* restore underscores */
		    for(i = 0; q[i]; i++)
		      *(q[i]) = '_';

		    fs_give((void **)&q);
		}

		break;

	      case 'B' :			/* 'B' encoding */
	      case 'b' :
		if(p = rfc822_base64((unsigned char *) txt, strlen(txt), &l)){
		    strcpy((char *) d, (char *) p);
		    fs_give((void **)&p);	/* free encoded buf */
		    d += l;			/* advance dest ptr to EOL */
		}
		else
		  goto bogus;

		break;

	      default:
		sstrcpy((char **) &d, txt);
		dprint(1, (debugfile, "RFC1522_decode: Unknown ENCODING: %s\n",
			   enc));
		break;
	    }

	    /* restore trompled source string */
	    enc[-1] = txt[-1] = '?';
	    ew[0]   = RFC1522_TERM[0];

	    /* advance s to start of text after encoded-word */
	    s = ew + RFC1522_TERM_L;
	}
	else{
	    /* found intro, but bogus data followed */
	    strncpy((char *) d, s, (int) (l = (sw - s) + RFC1522_INIT_L));
	    *(d += l) = '\0';			/* advance d, tie off text */
	    s += l;				/* advance s beyond intro */
	}
    }

    if(rv && *s)				/* copy remaining text */
      strcat((char *) rv, s);

/* BUG: MUST do code page mapping under DOS after decoding */

    return(rv ? rv : (unsigned char *) start);

  bogus:
    dprint(1, (debugfile, "RFC1522_decode: BOGUS INPUT: -->%s<--\n", start));
    return((unsigned char *) start);
}


/*
 * rfc1522_token - scan the given source line up to the end_str making
 *		   sure all subsequent chars are "valid" leaving endp
 *		   a the start of the end_str.
 * Returns: TRUE if we got a valid token, FALSE otherwise
 */
int
rfc1522_token(s, valid, end_str, endp)
    char  *s;
    int	 (*valid) PROTO((int));
    char  *end_str;
    char **endp;
{
    while(*s){
	if((char) *s == *end_str		/* test for matching end_str */
	   && ((end_str[1])
	        ? !strncmp((char *)s + 1, end_str + 1, strlen(end_str + 1))
	        : 1)){
	    *endp = s;
	    return(TRUE);
	}

	if(!(*valid)(*s++))			/* test for valid char */
	  break;
    }

    return(FALSE);
}


/*
 * rfc1522_valtok - test for valid character in the RFC 1522 encoded
 *		    word's charset and encoding fields.
 */
int
rfc1522_valtok(c)
    int c;
{
    return(!(c == SPACE || iscntrl(c & 0x7f) || strindex(ESPECIALS, c)));
}


/*
 * rfc1522_valenc - test for valid character in the RFC 1522 encoded
 *		    word's encoded-text field.
 */
int
rfc1522_valenc(c)
    int c;
{
    return(!(c == '?' || c == SPACE) && isprint((unsigned char)c));
}


/*
 * rfc1522_valid - validate the given string as to it's rfc1522-ness
 */
int
rfc1522_valid(s, charset, enc, txt, endp)
    char  *s;
    char **charset;
    char **enc;
    char **txt;
    char **endp;
{
    char *c, *e, *t, *p;
    int   rv;

    rv = rfc1522_token(c = s+RFC1522_INIT_L, rfc1522_valtok, RFC1522_DLIM, &e)
	   && rfc1522_token(++e, rfc1522_valtok, RFC1522_DLIM, &t)
	   && rfc1522_token(++t, rfc1522_valenc, RFC1522_TERM, &p)
	   && p - s <= RFC1522_MAXW;

    if(charset)
      *charset = c;

    if(enc)
      *enc = e;

    if(txt)
      *txt = t;

    if(endp)
      *endp = p;

    return(rv);
}


/*
 * rfc1522_encode - encode the given source string ala RFC 1522,
 *		    IF NECESSARY, into the given destination buffer.
 *		    Don't bother copying if it turns out encoding
 *		    isn't necessary.
 *
 * Returns: pointer to either the destination buffer containing the
 *	    encoded text, or a pointer to the source buffer if we didn't
 *          have to encode anything.
 */
char *
rfc1522_encode(d, s, charset)
    char	  *d;
    unsigned char *s;
    char	  *charset;
{
    unsigned char *p, *q;
    char	   enc;
    int		   n, l;

    if(!(s && charset))
      return((char *) s);

    /* look for a reason to encode */
    for(p = s, n = 0; *p; p++)
      if((*p) & 0x80){
	  n++;
      }
      else if(*p == RFC1522_INIT[0]
	      && !strncmp((char *) p, RFC1522_INIT, RFC1522_INIT_L)){
	  if(rfc1522_valid((char *) p, NULL, NULL, NULL, (char **) &q))
	    p = q + RFC1522_TERM_L - 1;		/* advance past encoded gunk */
	  else
	    n++;
      }
      else if(*p == ESCAPE && match_escapes((char *)(p+1))){
	  n++;
      }

    if(n){					/* found, encoding to do */
	char *rv  = d, *t,
	      enc = (n > (2 * (p - s)) / 3) ? 'B' : 'Q';
	int   slen;

	while(*s){
	    sstrcpy(&d, RFC1522_INIT);		/* insert intro header, */
	    sstrcpy(&d, charset);		/* character set tag, */
	    sstrcpy(&d, RFC1522_DLIM);		/* and encoding flavor */
	    *d++ = enc;
	    sstrcpy(&d, RFC1522_DLIM);

	    /*
	     * feed lines to encoder such that they're guaranteed
	     * less than RFC1522_MAXW.
	     */
	    p = rfc1522_encoded_word(s, enc, charset);
	    if(enc == 'B')			/* insert encoded data */
	      sstrcpy(&d, t = rfc1522_binary(s, p - s));
	    else				/* 'Q' encoding */
	      sstrcpy(&d, t = rfc1522_8bit(s, p - s));

	    sstrcpy(&d, RFC1522_TERM);		/* insert terminator */
	    fs_give((void **) &t);
	    if(*p)				/* more src string follows */
	      sstrcpy(&d, "\015\012 ");	/* insert continuation line */

	    s = p;				/* advance s */
	}

	return(rv);
    }
    else
      return((char *) s);			/* no work for us here */
}



/*
 * rfc1522_encoded_word -- cut given string into max length encoded word
 *
 * Return: pointer into 's' such that the encoded 's' is no greater
 *	   than RFC1522_MAXW
 *
 *  NOTE: this line break code is NOT cognizant of any SI/SO
 *  charset requirements nor similar strategies using escape
 *  codes.  Hopefully this will matter little and such
 *  representation strategies don't also include 8bit chars.
 */
unsigned char *
rfc1522_encoded_word(s, enc, charset)
    unsigned char *s;
    int		   enc;
    char	  *charset;
{
    int goal = RFC1522_MAXW - RFC1522_OVERHEAD(charset);

    if(enc == 'B')			/* base64 encode */
      for(goal = ((goal / 4) * 3) - 2; goal && *s; goal--, s++)
	;
    else				/* special 'Q' encoding */
      for(; goal && *s; s++)
	if((goal -= RFC1522_ENC_CHAR(*s) ? 3 : 1) < 0)
	  break;

    return(s);
}



/*
 * rfc1522_8bit -- apply RFC 1522 'Q' encoding to the given 8bit buffer
 *
 * Return: alloc'd buffer containing encoded string
 */
char *
rfc1522_8bit(src, slen)
    void *src;
    int   slen;
{
    static char *hex = "0123456789ABCDEF";
    char *ret = (char *) fs_get ((size_t) (3*slen + 2));
    char *d = ret;
    unsigned char c;
    unsigned char *s = (unsigned char *) src;

    while (slen--) {				/* for each character */
	if (((c = *s++) == '\015') && (*s == '\012') && slen) {
	    *d++ = '\015';			/* true line break */
	    *d++ = *s++;
	    slen--;
	}
	else if(c == SPACE){			/* special encoding case */
	    *d++ = '_';
	}
	else if(RFC1522_ENC_CHAR(c)){
	    *d++ = '=';				/* quote character */
	    *d++ = hex[c >> 4];			/* high order 4 bits */
	    *d++ = hex[c & 0xf];		/* low order 4 bits */
	}
	else
	  *d++ = (char) c;			/* ordinary character */
    }

    *d = '\0';					/* tie off destination */
    return(ret);
}


/*
 * rfc1522_binary -- apply RFC 1522 'B' encoding to the given 8bit buffer
 *
 * Return: alloc'd buffer containing encoded string
 */
char *
rfc1522_binary (src, srcl)
    void *src;
    int   srcl;
{
    static char *v =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned char *s = (unsigned char *) src;
    char *ret, *d;

    d = ret = (char *) fs_get ((size_t) ((((srcl + 2) / 3) * 4) + 1));
    for (; srcl; s += 3) {	/* process tuplets */
				/* byte 1: high 6 bits (1) */
	*d++ = v[s[0] >> 2];
				/* byte 2: low 2 bits (1), high 4 bits (2) */
	*d++ = v[((s[0] << 4) + (--srcl ? (s[1] >> 4) : 0)) & 0x3f];
				/* byte 3: low 4 bits (2), high 2 bits (3) */
	*d++ = srcl ? v[((s[1] << 2) + (--srcl ? (s[2] >> 6) :0)) & 0x3f] :'=';
				/* byte 4: low 6 bits (3) */
	*d++ = srcl ? v[s[2] & 0x3f] : '=';
	if(srcl)
	  srcl--;		/* count third character if processed */
    }

    *d = '\0';			/* tie off string */
    return(ret);		/* return the resulting string */
}
