#ifndef _GETXOPT_
#define _GETXOPT_

#define OPEN_BRACE '{'
#define CLOSE_BRACE '}'

typedef enum
{
  Compulsory,
  NonCompulsory
} DpaQualifier;

typedef enum
{
  DefaultOp,
  AddOp,
  DeleteOp,
  ReplaceOp
} DpaOperator;

typedef struct
{
  DpaQualifier qualifier; /* Compulsory or NonCompulsory */
  char *cmd_qualifier;  /* Command qualifier           */
  char *attr_name;      /* attriubte name(type) */
  DpaOperator op;         /* DPA operator */
  int num_values;         /* number of attribute values */
  char **attr_values;     /* array of attribute values */
} DpaExtStrToken;

/*******************************************************************/
/* Function: getxopt()                                             */
/* Description: get the token in Dpa extended string one by one.   */
/* Return Values:                                                  */
/*  0: OK                                                          */
/* -1: Token is not Found, i.e., end of parsing                    */
/* -2: Cannot find the operator                                    */
/* -3: Memory allocation error                                     */
/* -4: Cannot find end of brace for structure value                */
/* -5: Internal error(Unknown system error, re-compile and         */
/*                    link the program again)                      */
/* -6: Unmatched single quote (')                                  */
/* -7: Unmatched double quote (")                                  */
/*******************************************************************/
extern int getxopt(
        char *extended_str, /* Input/Output: the extended string  */
        int *offset,        /* Input/Output: the offset of string */
                            /*               for next call        */
        DpaExtStrToken **token /* Output: an allocatged block   */
                               /*         which contains the    */
                               /*         DpaExtendedToken      */
                   );

#endif
