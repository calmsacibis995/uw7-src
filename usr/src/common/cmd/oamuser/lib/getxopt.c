/******************************************************************************
 *	getxopt.c
 *-----------------------------------------------------------------------------
 * Comments:
 * POSIX conforming getxopt() routine for Systems Management utilities
 *
 *-----------------------------------------------------------------------------
 *      @(#) getxopt.c 1.2 97/03/25 
 *
 *-----------------------------------------------------------------------------
 * Revision History:
 *
 *	19 Oct 1994	Keith Carlsen
 *		Original version handed off to the POSIX group.
 *	5 May 1996	Louis Imershein
 *		Updated to support the agreed upon P1387 extended option 
 *		syntax.
 *	1 Feb 1996	Louis Imershein
 *		Bugfix.  Now handles multiple attribute value pairs in
 *		one extended option string.
 *
 *============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "getxopt.h"

/* Valid white space characters */
static char WHITE_SPACE[] = " \t\f\n";

/* Valid operations */
static char *op_str[] = 
{
  "==", /* set default */
  "+=", /* add op */
  "-=", /* delete op */
  "="   /* replace op */
};

/*===================================================================== INT ===
 * CheckOperator
 *
 *   Read in an operator and return a corresponding numeric representation.
 *
 * Parameters:
 *	s	- operator character
 * Returns:
 *	integer representation of the operation.
 *----------------------------------------------------------------------------*/
static int 
CheckOperator(
    const char *s
){
   /* error */ 
   if ( s == NULL )
      return -1;

   /* default */ 
   if ( !strncmp(s, op_str[0], 2) )
       return 0;
   /* add */ 
   else if ( !strncmp(s, op_str[1], 2) )
       return 1;
   /* delete */ 
   else if ( !strncmp(s, op_str[2], 2) )
       return 2;
   /* replace */ 
   else if ( !strncmp(s, op_str[3], 1) )
       return 3;

   /* error */ 
   return -1;  /* not found */
}

/*===================================================================== INT ===
 * FindOperator
 *
 *   Locate the next extended options operator in an extended options string
 *   and move beyond it.  Valid operators include:
 *
 *	"=="	set to default
 *	"+="	add to multi-valued attribute
 *	"-="	remove from multi-valued attribute
 *	"="	set to value
 *
 *   For example, in the string:
 *	foo+=bar
 *
 *   this routine would find the += and then point to the 'b' in bar.
 *
 * Parameters:
 *	s       - pointer to the extended option string
 *	p_attr  - the full extended option string
 *	p_op    - the type of operator found.
 *	p_value - point to the value contained in the string 
 *                AFTER the operator.
 * Returns:
 *	0 on success, else -1
 *----------------------------------------------------------------------------*/
static int 
FindOperator(
    char *s,
    char **p_attr,
    DpaOperator *p_op,
    char **p_value
) {
   int op_num;
   *p_attr = s;
   if ( *s == '-' || *s == '=' )  /* attribute quallifier */
      ++s;
   while ( *s && (op_num = CheckOperator(s)) == -1 )
      ++s;
   if ( *s == 0 )
      return -1;
   *p_op = (DpaOperator)op_num;
   if ( *p_op == ReplaceOp )
   {
      *s = 0;
      *p_value = s+1;
   }
   else
   {
      *s = 0;
      *(s+1) = 0;
      *p_value = s+2;
   }
   return 0;
}

/*===================================================================== INT ===
 * FindEndBrace
 *
 *   Complex getxopt() values can be contained within curly braces '{}'.
 *   This routine is used to seek through a string to the closing brace.
 *
 * Parameters:
 *	s	String of data.
 * Returns:
 *	A pointer to the closing curly brace character or NULL if
 *	an error.
 * Algorithm:
 *	Deal with braces embedded in braces by keeping a count of 
 *	all the open braces found and making sure there is a corresponding
 *	close brace.
 *----------------------------------------------------------------------------*/
static char *
FindEndBrace(
    char *s
) {
   int count;
   
   count = 1;  /* should not have the beginning brace */
   while ( *s )
   {
      if ( *s == CLOSE_BRACE )
      {
        
         if ( --count == 0 )
            return s;
      }
      else if ( *s == OPEN_BRACE )
        ++count;
      s++;
   }
   return NULL;
}

/*===================================================================== INT ===
 * FindValue
 *
 *   Strip starting and ending whitespace and the initial set of open and 
 *   close braces from the value.
 *
 * Parameters:
 *	s	    pointer to the begining of a value in an extended option
 *		    string.
 *
 *      end_str     pointer to the begining of the next attribute-value pair
 *                  in the extended option string.
 *
 *	rest_s      the rest of the string, after the value.
 *
 * Returns:
 *	the processed value
 *----------------------------------------------------------------------------*/
static char *
FindValue(
    char *s, 
    const char* end_str, 
    char **rest_s
){
   char *p;
   char *p_rc;

   if ( s >= end_str )
     return NULL;

   /* remove white space in the front */
   while ( s < end_str && strchr(WHITE_SPACE, *s) ) {

      *s = 0;
      s++;
   }

   if ( s == end_str )
      return NULL;

   if ( *s == OPEN_BRACE ) {

      /* set open brace to NULL */
      *s++ = 0;

      /* try to find an end brace */
      p = FindEndBrace(s);

      /* check for errors */
      if ( p == NULL || p >= end_str )
         return NULL;

      /* set close brace to NULL */
      *p++ = 0;
      p_rc = s;
      *rest_s = p;
 
      return p_rc;
   }
      
   p = s + strlen(s);

   if ( p > end_str )
     return NULL;

   p_rc = s;
   *rest_s = p + 1;

   /* remove the white space in the end */
   while ( p > p_rc && strchr(WHITE_SPACE, *--p) )
      *p = 0;

   return p_rc;
 
}

/*===================================================================== EXT ===
 * getxopt()
 *
 *   Parses POSIX 1387 extended options command line syntax intended to
 *   ease the implementation of printing utilities which use the ISO DPA 
 *   model (as well as POSIX user and software admin.  This routine
 *   is intended to be embedded in a routine which calls getopt(3), so 
 *   that for the command:
 *	
 *	useradd -x option_string user_name
 *
 *   the  "-x option_string" is interpeted  by getopt(3) and then
 *   option_string is passed on to getxopt().
 *
 *   For the command:
 *	
 *	useradd -X options_file user_name
 *
 *   the "-X options_file" is interpreted by getopt(3), then the options
 *   file is opened and its contents is read in and passed to getxopt().
 *
 * Parameters:
 *	extended_str		- the option_string passed as an argument
 *	offset			- offset into the extended options string.
 *	token			- extended options data structure - allocated
 *			          by this function.
 *			          
 * Returns:
 *  0: OK                                                   
 * -1: Token is not Found, i.e., end of parsing              
 * -2: Cannot find the operator                               
 * -3: Memory allocation error                                 
 * -4: Cannot find end of brace for structure value             
 * -5: Internal error(Unknown system error, re-compile and       
 *                    link the program again)                      
 * -6: Unmatched single quote (')                                 
 * -7: Unmatched double quote (")                                 
 *
 * Algorithm:
 * 	Get the token in Dpa extended string one by one.
 *----------------------------------------------------------------------------*/
int 
getxopt( 
    char *extended_str, 
    int  *offset,
    DpaExtStrToken **token 
){
   char *p_start;
   char *p_attr, *p_value, *p_next_attr;
   DpaQualifier qualifier;
   DpaOperator op;
   char *p_cmd_qualifier;
   char *p_last_space;
   char *p_temp;
   int index;
   int n_space, n_values;
   int extra_size;
   char squote=0, dquote=0, escaped=0;

   *token = NULL;  /* set token to NULL as default */
   if ( extended_str == NULL )
      return -1; /* not found */
   if ( !extended_str[*offset] )  /* NULL char */
      return -1; /* not found */

   p_start = extended_str+(*offset);  /* set the start char */


   /* remove the white space in the front if it is necessary */
   while ( *p_start && strchr(WHITE_SPACE, *p_start) )
         ++p_start;

   if ( *p_start == 0 )
      return -1; /* not found */

   /* find the first operator, and beginning address of attribute */
   /* and values                                                  */
   if ( FindOperator(p_start, &p_attr, &op, &p_value) == -1 )
      return -2;  /* Cannot find the operator */

   /* remove the white space in the end of attribute if it is     */
   /* necessary                                                   */
   index = strlen(p_attr) - 1;
   while ( index >= 0 && strchr(WHITE_SPACE, p_attr[index]) )
      p_attr[index--] = 0;     /* fill NULL char */

   /* Determine qualifier */
   if ( *p_attr == '-' )
   {
      qualifier = Compulsory;
      ++p_attr;
   }
   else if ( *p_attr == '=' )
   {
      qualifier = NonCompulsory;
      ++ p_attr;
   }
   else  /* default */
      qualifier = Compulsory;
 
   /* Determine command qualifier */
   if ( (p_cmd_qualifier = strchr(p_attr, '.')) != NULL )
   {
      *p_cmd_qualifier = 0;
      p_temp = p_cmd_qualifier+1;
      p_cmd_qualifier = p_attr;
      p_attr = p_temp;
   }
 

   /* Default operator requires no values */
   if ( op == DefaultOp ) 
   {
      *token = (DpaExtStrToken *)malloc(sizeof(DpaExtStrToken));
      if ( token == NULL )
         return -3;
      (*token)->qualifier = qualifier;
      (*token)->cmd_qualifier = p_cmd_qualifier;
      (*token)->attr_name = p_attr;
      (*token)->op = op;
      (*token)->num_values = 0;
      (*token)->attr_values = NULL;
      *offset = p_value - extended_str;
      return 0;
   }

   /* Other operators requires values */
   /* First remove white space in the front */
   while ( *p_value && strchr(WHITE_SPACE, *p_value) )
     ++p_value;
 
   /* find space until reach operator to determine the number of */
   /* values                                                     */
   n_space = 0;
   p_temp = p_value;
   while ( *p_temp && CheckOperator(p_temp) < 0 )
   {
     char *p_buf;

     if ( *p_temp == '\\' && !escaped )
     {
         escaped=1;
         p_buf=(char *)strdup(p_temp);
         strcpy(p_temp, p_buf+1);
	 free(p_buf);
     } 
     else if ( *p_temp == '\'' && !escaped) {
	if( squote )
	    squote=0;
	else
	    squote=1;
        p_buf=(char *)strdup(p_temp);
        strcpy(p_temp, p_buf+1);
	free(p_buf);
     }
     else if ( *p_temp == '\"' && !escaped) {
	if( dquote )
	    dquote=0;
	else
	    dquote=1;
        p_buf=(char *)strdup(p_temp);
        strcpy(p_temp, p_buf+1);
	free(p_buf);
     }
     else if ( *p_temp == OPEN_BRACE && !squote && !dquote && !escaped) 
     /* structure */
     {
        p_temp = FindEndBrace(p_temp+1);
        if ( p_temp == NULL )
	    return -4;  /* Cannot find end of brace */
        ++p_temp;
     }
     else if ( (*p_temp == ' ' || *p_temp == '\n' || *p_temp == '\t' 
		|| *p_temp == '\f')
		&& !squote && !dquote && !escaped ) /* space */
     {
        ++n_space;
        p_last_space = p_temp; /* set the space position */
        *p_temp = 0; /* remove space */
        ++p_temp;
     }
     else
     {
	if(escaped)
	    escaped=0;
        ++p_temp;
     }
   }
   if(squote)
	return -6;  /* unmatched single quote (') */
   if(dquote)
	return -7;  /* unmatched double quote (") */
     

   /* find the next attribute from p_last_space */
   if ( *p_temp == 0 ) /* no more tokens */
   {
     n_space++;
     p_next_attr = p_temp;
     /* remove white space before end of string */
     while ( p_next_attr != p_value &&
             strchr(WHITE_SPACE, *(p_next_attr-1)) )
     {
         --p_next_attr;
         *p_next_attr = 0;
     }
     if ( p_next_attr == p_value )
         return -5;  /* internal error */
   }
   else
   {
     if ( n_space == 0 ) /* only one value */
     {
        p_next_attr = p_value;
        /* skip entire structure value */
        if ( *p_next_attr == OPEN_BRACE )
        {
           p_next_attr = FindEndBrace(p_next_attr+1);
           if ( p_next_attr == NULL )
             return -4;
        }
        /* find next attribute by hitting a white space */
        while ( *p_next_attr && 
                strchr(WHITE_SPACE, *p_next_attr) == NULL )
           ++p_next_attr;
        if ( *p_next_attr == 0 )
           return -5;
        /* insert NULL to indicate end of values */
        *p_next_attr = 0;
        ++p_next_attr;
     }
     else /* more than one value */
     {
        p_next_attr = p_last_space + 1;
        /* skip white space after space */
        while ( *p_next_attr &&
                strchr(WHITE_SPACE, *p_next_attr) )
            ++p_next_attr;
        if ( *p_next_attr == 0 )
           return -5;  /* internal error */

        /* search next attribute by hitting a white space */
        /* skip entire structure value */
        if ( *p_next_attr == OPEN_BRACE )
        {
           p_next_attr = FindEndBrace(p_next_attr+1);
           if ( p_next_attr == NULL )
             return -4;
        }
     }
   }
   
   /* allocate and fill DpaExtStrToken */
   n_values = n_space;
   extra_size = n_values * sizeof(char *);
   *token = (DpaExtStrToken *)malloc(sizeof(DpaExtStrToken)+
                                     extra_size);
   if ( token == NULL )
      return -3;
   (*token)->qualifier = qualifier;
   (*token)->cmd_qualifier = p_cmd_qualifier;
   (*token)->attr_name = p_attr;
   (*token)->op = op;
   (*token)->num_values = n_values;
   (*token)->attr_values = 
            (char **)(*token + sizeof(DpaExtStrToken));

   /* assign value */
   for ( index = 0; index < n_values; index++ )
   {
     ((*token)->attr_values)[index] = 
                  FindValue(p_value, p_next_attr, &p_temp);
     if ( ((*token)->attr_values)[index] == NULL )
     {
        free(*token);
        *token = NULL;
        return -5;
     }
     p_value = p_temp;
   }
   *offset = p_next_attr - extended_str;

 
 
   return 0;
}
