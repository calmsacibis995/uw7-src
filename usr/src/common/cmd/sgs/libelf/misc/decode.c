#ident	"@(#)libelf:misc/decode.c	1.3"
/******************************************************************************
*                                                             \  ___  /       *
*                                                               /   \         *
* Edison Design Group C++/C Front End                        - | \^/ | -      *
*                                                               \   /         *
*                                                             /  | |  \       *
* Copyright 1996 Edison Design Group Inc.                        [_]          *
*                                                                             *
******************************************************************************/
/*
Copyright (c) 1996, Edison Design Group, Inc.

Redistribution and use in source and binary forms are permitted
provided that the above copyright notice and this paragraph are
duplicated in all source code forms.  The name of Edison Design
Group, Inc. may not be used to endorse or promote products derived
from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
Any use of this software is at the user's own risk.
*/
/*

decode.c -- Name demangler for C++.

The demangling is intended to work only on names of external entities.
There is some name mangling done for internal entities, or by the
C-generating back end, that this program does not try to decode.
*/

#define SCO_DEMANGLER 1
#if SCO_DEMANGLER
#include <stddef.h>
typedef size_t sizeof_t;
typedef int a_boolean;
#define TRUE	1
#define FALSE	0
#include <dem.h>
#else
#include "basics.h"
#include "host_envir.h"
#include "decode.h"
#endif /* SCO_DEMANGLER */

/*
Block used to hold state variables.  A block is used so that these routines
will be reentrant.
*/
typedef struct a_decode_control_block *a_decode_control_block_ptr;
typedef struct a_decode_control_block {
  unsigned long
		input_id_len;
			/* Length of the input identifier, not counting the
			   final null. */
  char		*output_id;
			/* Pointer to buffer for demangled version of
			   the current identifier. */
  sizeof_t	output_id_len;
			/* Length of output_id, not counting the final
			   null. */
  sizeof_t	output_id_size;
			/* Allocated size of output_id. */
  a_boolean	err_in_id;
			/* TRUE if any error was encountered in the current
			   identifier. */
  a_boolean	output_overflow_err;
			/* TRUE if the demangled output overflowed the
			   output buffer. */
  unsigned long	suppress_id_output;
			/* If > 0, demangled id output is suppressed.  This
			   might be because of an error or just as a way
			   of avoiding output during some processing. */
} a_decode_control_block;


/*
Block that contains information used to control the output of template
parameter lists.
*/
typedef struct a_template_param_block *a_template_param_block_ptr;
typedef struct a_template_param_block {
  unsigned long	nesting_level;
			/* Number of levels of template nesting at this
			   point (1 == top level). */
  const char	*final_specialization;
			/* Set to point to the mangled encoding for the final
			   specialization encountered while working from
			   outermost template to innermost.  NULL if
			   no specialization has been found yet. */
  a_boolean	set_final_specialization;
			/* TRUE if final_specialization should be set while
			   scanning. */
  a_boolean	actual_template_args_until_final_specialization;
			/* TRUE if template parameter names should not be
			   put out.  Reset when the final_specialization
			   position is reached. */
  a_boolean	output_only_correspondences;
			/* TRUE if doing a post-pass to output only template
			   parameter/argument correspondences and not
			   anything else.  suppress_id_output will have been
			   incremented to suppress everything else, and
			   gets decremented temporarily when correspondences
			   are output. */
  a_boolean	first_correspondence;
			/* TRUE until the first template parameter/argument
			   correspondence is put out. */
  a_boolean	use_old_form_for_template_output;
			/* TRUE if templates should be output in the old
			   form that always puts actual argument values
			   in template argument lists. */
} a_template_param_block;


/*
Declarations needed because of forward references:
*/
static const char *full_demangle_name(const char           *ptr,
                                unsigned long              nchars,
                                const char                 *mclass,
                                a_template_param_block_ptr temp_par_info,
                                a_decode_control_block_ptr dctl);
/*
Interface to full_demangle_name for the simple case.
*/
#define demangle_name(ptr, dctl)                                      \
  full_demangle_name((ptr), (unsigned long)0, (char *)NULL,           \
                     (a_template_param_block_ptr)NULL, (dctl))
static const char *demangle_name_with_preceding_length(
                                const char                 *ptr,
                                a_template_param_block_ptr temp_par_info,
                                a_decode_control_block_ptr dctl);
static const char *demangle_operation(const char           *ptr,
                                a_decode_control_block_ptr dctl);
static const char *demangle_operator(const char  *ptr,
                               int       *mangled_length,
                               a_boolean *takes_type);
static const char *demangle_type(const char           *ptr,
                           a_decode_control_block_ptr dctl);
static const char *full_demangle_type_name(const char           *ptr,
                                     a_boolean                  base_name_only,
                                     a_template_param_block_ptr temp_par_info,
                                     a_decode_control_block_ptr dctl);
/*
Interface to full_demangle_type_name for the simple case.
*/
#define demangle_type_name(ptr, dctl)                                 \
  full_demangle_type_name((ptr), /*base_name_only=*/FALSE,            \
                          /*temp_par_info=*/(a_template_param_block_ptr)NULL, \
                          (dctl))


static void write_id_ch(char                       ch,
                        a_decode_control_block_ptr dctl)
/*
Add the indicated character to the demangled version of the current identifier.
*/
{
  if (!dctl->suppress_id_output) {
    if (!dctl->output_overflow_err) {
      /* Test for buffer overflow, leaving room for a terminating null. */
      if (dctl->output_id_len >= dctl->output_id_size-1) {
        /* There's no room for the character in the buffer. */
        dctl->output_overflow_err = TRUE;
        /* Make sure the (truncated) output is null-terminated. */
        dctl->output_id[dctl->output_id_size-1] = '\0';
      } else {
        /* No overflow; put the character in the buffer. */
        dctl->output_id[dctl->output_id_len] = ch;
      }  /* if */
    }  /* if */
    /* Keep track of the number of characters (even if output has overflowed
       the buffer). */
    dctl->output_id_len++;
  }  /* if */
}  /* write_id_ch */


static void write_id_str(const char               *str,
                        a_decode_control_block_ptr dctl)
/*
Add the indicated string to the demangled version of the current identifier.
*/
{
  const char *p = str;

  if (!dctl->suppress_id_output) {
    for (; *p != '\0'; p++) write_id_ch(*p, dctl);
  }  /* if */
}  /* write_id_str */


static void bad_mangled_name(a_decode_control_block_ptr dctl)
/*
A bad name mangling has been encountered.  Record an error.
*/
{
  if (!dctl->err_in_id) {
    dctl->err_in_id = TRUE;
    dctl->suppress_id_output++;
  }  /* if */
}  /* bad_mangled_name */


static a_boolean start_of_id_is(const char *str, const char *id)
/*
Return TRUE if the identifier (at id) begins with the string str.
*/
{
  a_boolean is_start = FALSE;

  for (;;) {
    char chs = *str++;
    if (chs == '\0') {
      is_start = TRUE;
      break;
    }  /* if */
    if (chs != *id++) break;
  }  /* for */
  return is_start;
}  /* start_of_id_is */


static const char *advance_past_underscore(const char           *p,
                                     a_decode_control_block_ptr dctl)
/*
An underscore is expected at *p.  If it's there, advance past it.  If
not, call bad_mangled_name.  In either case, return the updated value of p.
*/
{
  if (*p == '_') {
    p++;
  } else {
    bad_mangled_name(dctl);
  }  /* if */
  return p;
}  /* advance_past_underscore */


static const char *get_number(const char           *p,
                        unsigned long              *num,
                        a_decode_control_block_ptr dctl)
/*
Accumulate a number starting at position p and return its value in *num.
Return a pointer to the character position following the number.
*/
{
  unsigned long n = 0;

  if (!isdigit((unsigned char)*p)) {
    bad_mangled_name(dctl);
    goto end_of_routine;
  }  /* if */
  do {
    n = n*10 + (*p - '0');
    if (n > dctl->input_id_len) {
      /* Bad number. */
      bad_mangled_name(dctl);
      n = dctl->input_id_len;
      goto end_of_routine;
    }  /* if */
    p++;
  } while (isdigit((unsigned char)*p));
end_of_routine:
  *num = n;
  return p;
}  /* get_number */


static const char *get_single_digit_number(const char           *p,
                                     unsigned long              *num,
                                     a_decode_control_block_ptr dctl)
/*
Accumulate a number starting at position p and return its value in *num.
The number is a single digit.  Return a pointer to the character position
following the number.
*/
{
  *num = 0;
  if (!isdigit((unsigned char)*p)) {
    bad_mangled_name(dctl);
    goto end_of_routine;
  }  /* if */
  *num = (*p - '0');
  p++;
end_of_routine:
  return p;
}  /* get_single_digit_number */


static const char *get_number_with_optional_underscore(
                                               const char                 *p,
                                               unsigned long              *num,
                                               a_decode_control_block_ptr dctl)
/*
Accumulate a number starting at position p and return its value in *num.
If the number has more than one digit, it is followed by an underscore.
(Or, in a newer representation, surrounded by underscores.)
Return a pointer to the character position following the number.
*/
{
  if (*p == '_') {
    /* New encoding (not from cfront) -- the length is surrounded by
       underscores whether it's a single digit or several digits,
       e.g., "L_10_1234567890". */
    p++;
    /* Multi-digit number followed by underscore. */
    p = get_number(p, num, dctl);
    p = advance_past_underscore(p, dctl);
  } else if (isdigit((unsigned char)p[0]) && isdigit((unsigned char)p[1]) &&
             p[2] == '_') {
    /* The cfront version -- a multi-digit length is followed by an
       underscore, e.g., "L10_1234567890".  This doesn't work well because
       something like "L11", intended to have a one-digit length, can
       be made ambiguous by following it by a "_" for some other reason.
       So this form is not used in new cases where that can come up, e.g.,
       nontype template arguments for functions.  In any case, interpret
       "multi-digit" as "2-digit" and don't look further for the underscore. */
    /* Multi-digit number followed by underscore. */
    p = get_number(p, num, dctl);
    p = advance_past_underscore(p, dctl);
  } else {
    /* Single-digit number not followed by underscore. */
    p = get_single_digit_number(p, num, dctl);
  }  /* if */
  return p;
}  /* get_number_with_optional_underscore */


static a_boolean is_immediate_type_qualifier(const char *p)
/*
Return TRUE if the encoding pointed to is one that indicates type
qualification.
*/
{
  a_boolean is_type_qual = FALSE;

  if (*p == 'C' || *p == 'V') {
    /* This is a type qualifier. */
    is_type_qual = TRUE;
  }  /* if */
  return is_type_qual;
}  /* is_immediate_type_qualifier */


static void write_template_parameter_name(unsigned long              depth,
                                          unsigned long              position,
                                          a_boolean                  nontype,
                                          a_decode_control_block_ptr dctl)
/*
Output a representation of a template parameter with depth and position
as indicated.  It's a nontype parameter if nontype is TRUE.
*/
{
  char buffer[100];
  char letter = '\0';

  if (nontype) {
    /* Nontype parameter. */
    /* Use a code letter for the first few levels, then the depth number. */
    if (depth == 1) {
      letter = 'N';
    } else if (depth == 2) {
      letter = 'O';
    } else if (depth == 3) {
      letter = 'P';
    }  /* if */
    if (letter != '\0') {
      (void)sprintf(buffer, "%c%lu", letter, position);
    } else {
      (void)sprintf(buffer, "N_%lu_%lu", depth, position);
    }  /* if */
  } else {
    /* Normal type parameter. */
    /* Use a code letter for the first few levels, then the depth number. */
    if (depth == 1) {
      letter = 'T';
    } else if (depth == 2) {
      letter = 'U';
    } else if (depth == 3) {
      letter = 'V';
    }  /* if */
    if (letter != '\0') {
      (void)sprintf(buffer, "%c%lu", letter, position);
    } else {
      (void)sprintf(buffer, "T_%lu_%lu", depth, position);
    }  /* if */
  }  /* if */
  write_id_str(buffer, dctl);
}  /* write_template_parameter_name */


static const char *demangle_template_parameter_name(
                                            const char                 *ptr,
                                            a_boolean                  nontype,
                                            a_decode_control_block_ptr dctl)
/*
Demangle a template parameter name at the indicated location.  The parameter
is a nontype parameter if nontype is TRUE.  Return a pointer to the character
position following what was demangled.
*/
{
  const char    *p = ptr;
  unsigned long position, depth = 1;

  /* This comes up with the modern mangling for template functions.
     Form is "ZnZ" or "Zn_mZ", where n is the parameter number and m
     is the depth number (1 if not specified). */
  p++;  /* Advance past the "Z". */
  /* Get the position number. */
  p = get_number(p, &position, dctl);
  if (*p == '_') {
    /* Form including depth ("Zn_mZ"). */
    p++;
    p = get_number(p, &depth, dctl);
  }  /* if */
  /* Check for the final "Z".  This appears in the mangling to avoid
     ambiguities when the template parameter is followed by something whose
     encoding begins with a digit, e.g., a class name. */
  if (*p != 'Z') {
    bad_mangled_name(dctl);
  } else {
    p++;
  }  /* if */
  /* Output the template parameter name. */
  write_template_parameter_name(depth, position, nontype, dctl);
  return p;
}  /* demangle_template_parameter_name */


static const char *demangle_constant(const char           *ptr,
                               a_decode_control_block_ptr dctl)
/*
Demangle a constant (e.g., a nontype template class argument) beginning at
ptr, and output the demangled form.  Return a pointer to the character
position following what was demangled.
*/
{
  const char    *p = ptr, *type = NULL, *index;
  unsigned long nchars;

  /* A constant has a form like
       CiL15   <-- integer constant 5
           ^-- Literal constant representation.
          ^--- Length of literal constant.
         ^---- L indicates literal constant; c indicates address
               of variable, etc.
       ^^----- Type of template argument, with "const" added.
     A template parameter constant or a constant expression does not have
     the initial "C" and type.
  */
  if (*p == 'C') {
    /* Advance past the type. */
    type = p;
    dctl->suppress_id_output++;
    p = demangle_type(p, dctl);
    dctl->suppress_id_output--;
  }  /* if */
  /* The next thing has one of the following forms:
       3abc        Address of "abc".
       L211        Literal constant; length ("2") followed by the characters of
                   the constant ("11").
       LM0_L2n1_1j Pointer-to-member-function constant; the three parts
                   correspond to the triplet of values in the __mptr
                   data structure.
       Z1Z         Template parameter.
       Opl2Z1ZZ2ZO Expression.
  */
  if (isdigit((unsigned char)*p)) {
    /* A name preceded by its length, e.g., "3abc".  Put out "&name". */
    write_id_ch('&', dctl);
    /* Process the length and name. */
    p = demangle_name_with_preceding_length(p,
                                            (a_template_param_block_ptr)NULL,
                                            dctl);
  } else if (*p == 'L') {
    if (p[1] != 'M') {
      /* Normal literal constant.  Form is something like
           L3n12     encoding for -12
             ^^^---- Characters of constant.  Some characters get remapped:
                       n --> -
                       p --> +
                       d --> .
            ^------- Length of constant.
         Output is
           (type)constant
         That is, the literal constant preceded by a cast to the right type.
      */
      /* See if the type is bool. */
      a_boolean is_bool = (type+2 == p && *(type+1) == 'b'), is_nonzero;
      /* If the type is bool, don't put out the cast. */
      if (!is_bool) {
        write_id_ch('(', dctl);
#if SCO_DEMANGLER
	write_id_str("const ", dctl);
#endif /* SCO_DEMANGLER */
        /* Start at type+1 to avoid the "C" for const. */
        (void)demangle_type(type+1, dctl);
        write_id_ch(')', dctl);
      }  /* if */
      p++;  /* Advance past the "L". */
      /* Get the length of the constant. */
      p = get_number_with_optional_underscore(p, &nchars, dctl);
      /* Process the characters of the literal constant. */
      is_nonzero = FALSE;
      for (; nchars > 0; nchars--, p++) {
        /* Remap characters where necessary. */
        char ch = *p;
        switch (ch) {
          case '\0':
          case '_':
            /* Ran off end of string. */
            bad_mangled_name(dctl);
            goto end_of_routine;
          case 'p':
            ch = '+';
            break;
          case 'n':
            ch = '-';
            break;
          case 'd':
            ch = '.';
            break;
        }  /* switch */
        if (is_bool) {
          /* For the bool case, just keep track of whether the constant is
             non-zero; true or false will be output later. */
          if (ch != '0') is_nonzero = TRUE;
        } else {
          /* Normal (non-bool) case.  Output the character of the constant. */
          write_id_ch(ch, dctl);
        }  /* if */
      }  /* for */
      if (is_bool) {
        /* For bool, output true or false. */
        write_id_str(is_nonzero ? "true" : "false", dctl);
      }  /* if */
    } else {
      /* Pointer-to-member-function.  The form of the constant is
           LM0_L2n1_1j  Non-virtual function
           LM0_L11_0    Virtual function
           LM0_L10_0    Null pointer
         The three parts match the three components of the __mptr structure:
         (delta, index, function or offset).  The index is -1 for a non-virtual
         function, 0 for a null pointer, and greater than 0 for a virtual
         function.  The index is represented like an integer constant (see
         above).  For virtual functions, the last component is always "0"
         even if the offset is not zero. */
#ifdef SCO_DEMANGLER
      /* For some cfront mangling, the second 'L' is optional. */
#endif  /* SCO_DEMANGLING */
      /* Advance past the "LM". */
      p += 2;
      /* Advance over the first component, ignoring it. */
      while (isdigit((unsigned char)*p)) p++;
      p = advance_past_underscore(p, dctl);
      /* The index component should be next. */
#ifdef SCO_DEMANGLER
      if (isdigit((unsigned)*p)) {
        /* This looks like an old format. */
        nchars = 1;		/* force */
      } else {
#endif  /* SCO_DEMANGLER */
        if (*p != 'L') {
          bad_mangled_name(dctl);
          goto end_of_routine;
        }  /* if */
        p++;
        /* Get the index length. */
        /* Note that get_number_with_optional_underscore is not used because
           this is an ambiguous situation: an underscore follows the index
           value, and there's no way to tell if it's the multi-digit
           indicator for the length or the separator between fields. */
      if (*p == '_') {
        /* New-form encoding, no ambiguity. */
        p = get_number_with_optional_underscore(p, &nchars, dctl);
      } else {
        p = get_single_digit_number(p, &nchars, dctl);
      }  /* if */
#ifdef SCO_DEMANGLER
      }  /* if */
#endif  /* SCO_DEMANGLER */
      /* Remember the start of the index. */
      index = p;
      /* Skip the rest of the index. */
      while (isdigit((unsigned char)*p) || (*p == 'n')) p++;
      p = advance_past_underscore(p, dctl);
      /* If the index number starts with 'n', this is a non-virtual
         function. */
      if (*index == 'n') {
        /* Non-virtual function. */
        /* The third component is a name preceded by its length, e.g.,
           "1f".  Put out "&A::f", where "A" is the class type retrieved
           from the type. */
        write_id_ch('&', dctl);
        /* Start at type+2 to skip the "C" for const and the "M" for
           pointer-to-member. */
        (void)demangle_type_name(type+2, dctl);
        write_id_str("::", dctl);
        /* Demangle the length and name. */
        p = demangle_name_with_preceding_length(p,
                                              (a_template_param_block_ptr)NULL,
                                                dctl);
      } else {
        /* Not a non-virtual function.  The encoding for the third component
           should be simply "0". */
        if (*p != '0') {
          bad_mangled_name(dctl);
          goto end_of_routine;
        }  /* if */
        p++;
        if (nchars == 1 && *index == '0') {
          /* Null pointer constant.  Output "(type)0", that is, a zero cast
             to the pointer-to-member type. */
          write_id_ch('(', dctl);
          (void)demangle_type(type, dctl);
          write_id_str(")0", dctl);
        } else {
          /* Virtual function.  This case can't really be demangled properly,
             because the mangled name doesn't have enough information.
             Output "&A::virtual-function-n". */
          write_id_ch('&', dctl);
          /* Start at type+2 to skip the "C" for const and the "M" for
             pointer-to-member. */
          (void)demangle_type_name((const char *)type+2, dctl);
          write_id_str("::", dctl);
          write_id_str("virtual-function-", dctl);
          /* Write the index number. */
          for (; nchars > 0; nchars--, index++) write_id_ch(*index, dctl);
        }  /* if */
      }  /* if */
    }  /* if */
  } else if (*p == 'Z') {
    /* A template parameter. */
    p = demangle_template_parameter_name(p, /*nontype=*/TRUE, dctl);
  } else if (*p == 'O') {
    /* An operation. */
    p = demangle_operation(p, dctl);
  } else {
    /* The constant starts with something unexpected. */
    bad_mangled_name(dctl);
  }  /* if */
end_of_routine:
  return p;
}  /* demangle_constant */


static const char *demangle_operation(const char           *ptr,
                                a_decode_control_block_ptr dctl)
/*
Demangle an operation in a constant expression (these come up in template
arguments and array sizes, in template function parameter lists) beginning
at ptr, and output the demangled form.  Return a pointer to the character
position following what was demangled.
*/
{
  const char    *p = ptr, *operator_str;
  int           op_length;
  unsigned long num_operands;
  a_boolean     takes_type;

  /* An operation has the form
       Opl2Z1ZZ2ZO <-- "Z1 + Z2", Z1/Z2 indicating nontype template parameters.
                 ^---- "O" to end the operation encoding.
              ^^^----- Second operand.
           ^^^-------- First operand.
          ^----------- Count of operands.
        ^^------------ Operation, using same encoding as for operator
                       function names.
       ^-------------- "O" for operation.
  */
  p++;  /* Advance past the "O". */
  /* Decode the operator name, e.g., "pl" is "+". */
  operator_str = demangle_operator(p, &op_length, &takes_type);
  if (operator_str == NULL) {
    bad_mangled_name(dctl);
  } else {
    p += op_length;
    /* Put parentheses around the operation. */
    write_id_ch('(', dctl);
    /* For a cast, sizeof, or __ALIGNOF__, get the type. */
    if (takes_type) {
      if (strcmp(operator_str, "cast") == 0) {
        write_id_ch('(', dctl);
        operator_str = "";
      } else {
        write_id_str(operator_str, dctl);
      }  /* if */
      p = demangle_type(p, dctl);
      write_id_ch(')', dctl);
    }  /* if */
    /* Get the count of operands. */
    p = get_single_digit_number(p, &num_operands, dctl);
    /* sizeof and __ALIGNOF__ take zero operands. */
    if (num_operands != 0) {
      if (num_operands == 1) {
        /* Unary operator -- operator comes first. */
        write_id_str(operator_str, dctl);
      }  /* if */
      /* Process the first operand. */
      p = demangle_constant(p, dctl);
      if (num_operands > 1) {
        /* Binary and ternary operators -- operator comes after first
           operand. */
        write_id_str(operator_str, dctl);
        /* Process the second operand. */
        p = demangle_constant(p, dctl);
        if (num_operands > 2) {
          /* Ternary operand -- "?". */
          write_id_ch(':', dctl);
          /* Process the third operand. */
          p = demangle_constant(p, dctl);
        }  /* if */
      }  /* if */
    }  /* if */
    write_id_ch(')', dctl);
    /* Check for the final "O". */
    if (*p != 'O') {
      bad_mangled_name(dctl);
    } else {
      p++;
    }  /* if */
  }  /* if */
  return p;
}  /* demangle_operation */


static void clear_template_param_block(a_template_param_block_ptr tpbp)
/*
Clear the fields of the indicated template parameter block.
*/
{
  tpbp->nesting_level = 0;
  tpbp->final_specialization = NULL;
  tpbp->set_final_specialization = FALSE;
  tpbp->actual_template_args_until_final_specialization = FALSE;
  tpbp->output_only_correspondences = FALSE;
  tpbp->first_correspondence = FALSE;
  tpbp->use_old_form_for_template_output = FALSE;
}  /* clear_template_param_block */


static const char *demangle_template_arguments(
                                      const char                 *ptr,
                                      a_boolean                  partial_spec,
                                      a_template_param_block_ptr temp_par_info,
                                      a_decode_control_block_ptr dctl)
/*
Demangle the template class arguments beginning at ptr and output the
demangled form.  Return a pointer to the character position following what was
demangled.  ptr points to just past the "__tm__", "__ps__", or "__pt__"
string.  partial_spec is TRUE if this is a partial-specialization
parameter list ("__ps__").  When temp_par_info != NULL, it points to a
block that controls output of extra information on template parameters.
*/
{
  const char    *p = ptr, *arg_base;
  unsigned long nchars, position;
  a_boolean     nontype, skipped, unskipped;

  if (temp_par_info != NULL && !partial_spec) temp_par_info->nesting_level++;
  /* A template argument list looks like
       __tm__3_ii
               ^^---- Argument types.
             ^------- Size of argument types, including the underscore.
             ^------- ptr points here.
     For the first argument list of a partial specialization, "__tm__" is
     replaced by "__ps__".  For old-form mangling of templates, "__tm__"
     is replaced by "__pt__".
  */
  write_id_ch('<', dctl);
  /* Scan the size. */
  p = get_number(p, &nchars, dctl);
  arg_base = p;
  p = advance_past_underscore(p, dctl);
  /* Loop to process the arguments. */
  for (position = 1;; position++) {
    if (dctl->err_in_id) break;  /* Avoid infinite loops on errors. */
    if (*p == '\0' || *p == '_') {
      /* We ran off the end of the string. */
      bad_mangled_name(dctl);
      break;
    }  /* if */
    /* "X" identifies the beginning of a nontype argument. */
    nontype = (*p == 'X');
    skipped = unskipped = FALSE;
    if (!partial_spec && temp_par_info != NULL &&
        !temp_par_info->use_old_form_for_template_output &&
        !temp_par_info->actual_template_args_until_final_specialization) {
      /* Doing something special: writing out the template parameter name. */
      if (temp_par_info->output_only_correspondences) {
        /* This is the second pass, which writes out parameter/argument
           correspondences, e.g., "T1=int".  Output has been suppressed
           in general, and is turned on briefly here. */
        dctl->suppress_id_output--;
        unskipped = TRUE;
        /* Put out a comma between entries and a left bracket preceding the
           first entry. */
        if (temp_par_info->first_correspondence) {
          write_id_str(" [with ", dctl);
          temp_par_info->first_correspondence = FALSE;
        } else {
          write_id_str(", ", dctl);
        }  /* if */
      }  /* if */
      /* Write the template parameter name. */
      write_template_parameter_name(temp_par_info->nesting_level, position,
                                    nontype, dctl);
      if (temp_par_info->output_only_correspondences) {
        /* This is the second pass, to write out correspondences, so put the
           argument value out after the parameter name. */
        write_id_ch('=', dctl);
      } else {
        /* This is the first pass.  The argument value is skipped.  In
           the second pass, its value will be written out. */
        /* We still have to scan over the argument value, but suppress
           output. */
        dctl->suppress_id_output++;
        skipped = TRUE;
      }  /* if */
    }  /* if */
    /* Write the argument value. */
    if (nontype) {
      /* Nontype argument. */
      p++;  /* Advance past the "X". */
      p = demangle_constant(p, dctl);
    } else {
      /* Type argument. */
      p = demangle_type(p, dctl);
    }  /* if */
    if (skipped) dctl->suppress_id_output--;
    if (unskipped) dctl->suppress_id_output++;
    /* Stop after the last argument. */
    if ((p - arg_base) >= nchars) break;
    write_id_str(", ", dctl);
  }  /* for */
  write_id_ch('>', dctl);
  return p;
}  /* demangle_template_arguments */


static const char *demangle_operator(const char   *ptr,
                               int       *mangled_length,
                               a_boolean *takes_type)
/*
Examine the first few characters at ptr to see if they are an encoding for
an operator (e.g., "pl" for plus).  If so, return a pointer to a string for
the operator (e.g., "+"), set *mangled_length to the number of characters
in the encoding, and *takes_type to TRUE if the operator takes a type
modifier (e.g., cast).  If the first few characters are not an operator
encoding, return NULL.
*/
{
  char *s;
  int  len = 2;

  *takes_type = FALSE;
  /* The length-3 codes are tested first to avoid taking their first two
     letters as one of the length-2 codes. */
  if (start_of_id_is("apl", ptr)) {
    s = "+=";
    len = 3;
  } else if (start_of_id_is("ami", ptr)) {
    s = "-=";
    len = 3;
  } else if (start_of_id_is("amu", ptr)) {
    s = "*=";
    len = 3;
  } else if (start_of_id_is("adv", ptr)) {
    s = "/=";
    len = 3;
  } else if (start_of_id_is("amd", ptr)) {
    s = "%=";
    len = 3;
  } else if (start_of_id_is("aer", ptr)) {
    s = "^=";
    len = 3;
  } else if (start_of_id_is("aad", ptr)) {
    s = "&=";
    len = 3;
  } else if (start_of_id_is("aor", ptr)) {
    s = "|=";
    len = 3;
  } else if (start_of_id_is("ars", ptr)) {
    s = ">>=";
    len = 3;
  } else if (start_of_id_is("als", ptr)) {
    s = "<<=";
    len = 3;
  } else if (start_of_id_is("nwa", ptr)) {
    s = "new[]";
    len = 3;
  } else if (start_of_id_is("dla", ptr)) {
    s = "delete[]";
    len = 3;
  } else if (start_of_id_is("nw", ptr)) {
    s = "new";
  } else if (start_of_id_is("dl", ptr)) {
    s = "delete";
  } else if (start_of_id_is("pl", ptr)) {
    s = "+";
  } else if (start_of_id_is("mi", ptr)) {
    s = "-";
  } else if (start_of_id_is("ml", ptr)) {
    s = "*";
  } else if (start_of_id_is("dv", ptr)) {
    s = "/";
  } else if (start_of_id_is("md", ptr)) {
    s = "%";
  } else if (start_of_id_is("er", ptr)) {
    s = "^";
  } else if (start_of_id_is("ad", ptr)) {
    s = "&";
  } else if (start_of_id_is("or", ptr)) {
    s = "|";
  } else if (start_of_id_is("co", ptr)) {
    s = "~";
  } else if (start_of_id_is("nt", ptr)) {
    s = "!";
  } else if (start_of_id_is("as", ptr)) {
    s = "=";
  } else if (start_of_id_is("lt", ptr)) {
    s = "<";
  } else if (start_of_id_is("gt", ptr)) {
    s = ">";
  } else if (start_of_id_is("ls", ptr)) {
    s = "<<";
  } else if (start_of_id_is("rs", ptr)) {
    s = ">>";
  } else if (start_of_id_is("eq", ptr)) {
    s = "==";
  } else if (start_of_id_is("ne", ptr)) {
    s = "!=";
  } else if (start_of_id_is("le", ptr)) {
    s = "<=";
  } else if (start_of_id_is("ge", ptr)) {
    s = ">=";
  } else if (start_of_id_is("aa", ptr)) {
    s = "&&";
  } else if (start_of_id_is("oo", ptr)) {
    s = "||";
  } else if (start_of_id_is("pp", ptr)) {
    s = "++";
  } else if (start_of_id_is("mm", ptr)) {
    s = "--";
  } else if (start_of_id_is("cm", ptr)) {
    s = ",";
  } else if (start_of_id_is("rm", ptr)) {
    s = "->*";
  } else if (start_of_id_is("rf", ptr)) {
    s = "->";
  } else if (start_of_id_is("cl", ptr)) {
    s = "()";
  } else if (start_of_id_is("vc", ptr)) {
    s = "[]";
  } else if (start_of_id_is("cs", ptr)) {
    s = "cast";
    *takes_type = TRUE;
  } else if (start_of_id_is("sz", ptr)) {
    s = "sizeof(";
    *takes_type = TRUE;
  } else if (start_of_id_is("af", ptr)) {
    s = "__ALIGNOF__(";
    *takes_type = TRUE;
  } else {
    s = NULL;
  }  /* if */
  *mangled_length = len;
  return s;
}  /* demangle_operator */


static a_boolean is_operator_function_name(const char *ptr,
                                           const char **demangled_name,
                                           int  *mangled_length)
/*
Examine the string beginning at ptr to see if it is the mangled name for
an operator function.  If so, return TRUE and set *demangled_name to
the demangled form, and *mangled_length to the length of the mangled form.
*/
{
  const char      *s, *end_ptr;
  int       len;
  a_boolean takes_type;

  /* Get the operator name.*/
  s = demangle_operator(ptr, &len, &takes_type);
  if (s != NULL) {
    /* Make sure we took the whole name and nothing more. */
    end_ptr = ptr + len;
    if (*end_ptr == '\0' || (end_ptr[0] == '_' && end_ptr[1] == '_')) {
      /* Okay. */
    } else {
      s = NULL;
    }  /* if */
  }  /* if */
  *demangled_name = s;
  *mangled_length = len;
  return (s != NULL);
}  /* demangle_operator_function_name */


static void note_specialization(const char                 *ptr,
                                a_template_param_block_ptr temp_par_info)
/*
Note the fact that a specialization indication has been encountered at ptr
while scanning a mangled name.  temp_par_info, if non-NULL, points to
a block of information related to template parameter processing.
*/
{
  if (temp_par_info != NULL) {
    if (temp_par_info->set_final_specialization) {
      /* Remember the location of the last specialization seen. */
      temp_par_info->final_specialization = ptr;
    } else if (temp_par_info->actual_template_args_until_final_specialization&&
               ptr == temp_par_info->final_specialization) {
      /* Stop doing the special processing for specializations when the
         final specialization is reached. */
      temp_par_info->actual_template_args_until_final_specialization = FALSE;
    }  /* if */
  }  /* if */
}  /* note_specialization */


static char get_char(const char    *ptr,
                     const char    *base_ptr,
                     unsigned long nchars)
/*
Get and return the character pointed to by ptr.  However, if nchars is
non-zero, the string from which the character is to be extracted starts
at base_ptr and has length nchars.  An attempt to get a character past
the end of the string returns a null character.
*/
{
  char ch;

  if (nchars > 0 && ptr >= base_ptr+nchars) {
    ch = '\0';
  } else {
    ch = *ptr;
  }  /* if */
  return ch;
}  /* get_char */


static const char *full_demangle_name(const char           *ptr,
                                unsigned long              nchars,
                                const char                 *mclass,
                                a_template_param_block_ptr temp_par_info,
                                a_decode_control_block_ptr dctl)
/*
Demangle the name at ptr and output the demangled form.  Return a pointer
to the character position following what was demangled.  A "name" is
usually just a string of alphanumeric characters.  However, names of
constructors, destructors, and operator functions require special
handling, as do template entity names.  nchars indicates the number
of characters in the name, or is zero if the name is open-ended
(it's ended by a null or double underscore).  mclass, when non-NULL,
points to the mangled form of the class of which this name is a
member.  When it's non-NULL, constructor and destructor names will
be put out in the proper form (otherwise, they are left in their
original forms).  When temp_par_info != NULL, it points to a
block that controls output of extra information on template parameters.
See the macro demangle_name for an interface to this routine for the
simple case.
*/
{
  const char *p, *end_ptr = NULL;
  a_boolean is_special_name = FALSE, is_pt, is_partial_spec = FALSE;
  a_boolean partial_spec_output_suppressed = FALSE;
  const char      *demangled_name;
  int       mangled_length;

  /* See if the name is special in some way. */
  if ((nchars == 0 || nchars >= 4) && ptr[0] == '_' && ptr[1] == '_') {
    /* Name beginning with two underscores. */
    p = ptr + 2;
    if (start_of_id_is("ct", p)) {
      /* Constructor. */
      end_ptr = p + 2;
      if (mclass == NULL) {
        /* The mangled name for the class is not provided, so handle this as
           a normal name. */
      } else {
        /* Output the class name for the constructor name. */
        is_special_name = TRUE;
        (void)full_demangle_type_name(mclass, /*base_name_only=*/TRUE,
                                      /*temp_par_info=*/
                                              (a_template_param_block_ptr)NULL,
                                      dctl);
      }  /* if */
#if SCO_DEMANGLER
    /* special case check for __dtors__Fv - needed for debug to
       recognize cfront output */
    } else if (start_of_id_is("dt", p) && !start_of_id_is("dtors", p)) {
#else
    } else if (start_of_id_is("dt", p)) {
#endif /* SCO_DEMANGLER */
      /* Destructor. */
      end_ptr = p + 2;
      if (mclass == NULL) {
        /* The mangled name for the class is not provided, so handle this as
           a normal name. */
      } else {
        /* Output ~class-name for the destructor name. */
        is_special_name = TRUE;
        write_id_ch('~', dctl);
        (void)full_demangle_type_name(mclass, /*base_name_only=*/TRUE,
                                      /*temp_par_info=*/
                                              (a_template_param_block_ptr)NULL,
                                      dctl);
      }  /* if */
    } else if (start_of_id_is("op", p)) {
      /* Conversion function.  Name looks like __opi__... where the part
         after "op" encodes the type (e.g., "opi" is "operator int"). */
      is_special_name = TRUE;
      write_id_str("operator ", dctl);
      end_ptr = demangle_type(p+2, dctl);
    } else if (is_operator_function_name(p, &demangled_name,
                                         &mangled_length)) {
      /* Operator function. */
      is_special_name = TRUE;
      write_id_str("operator ", dctl);
      write_id_str(demangled_name, dctl);
      end_ptr = p + mangled_length;
    } else {
      /* Something unrecognized. */
    }  /* if */
  }  /* if */
  /* Here, end_ptr non-null means the end of the string has been found
     already (because the name is special in some way). */
  if (end_ptr == NULL) {
    /* Not a special name. Find the end of the string and set end_ptr.
       Also look for template-related things that terminate the name
       earlier. */
    for (p = ptr; ; p++) {
      char ch = get_char(p, ptr, nchars);
      /* Stop at the end of the string (real, or as indicated by nchars). */
      if (ch == '\0') break;
      /* Stop on a double underscore, but not one at the start of the string.
         More than 2 underscores in a row does not terminate the string,
         so that something like the name for "void f_()" (i.e., "f___Fv")
         can be demangled successfully. */
      if (ch == '_' && p != ptr &&
          get_char(p+1, ptr, nchars) == '_' &&
          get_char(p+2, ptr, nchars) != '_' &&
          /* When the length is known, stop only on "__tm", "__ps", "__pt",
             or "__S".  Double underscores can appear in the middle of some
             names, e.g., member names used as template arguments. */
          (nchars == 0 ||
           (get_char(p+2, ptr, nchars) == 't' &&
            get_char(p+3, ptr, nchars) == 'm') ||
           (get_char(p+2, ptr, nchars) == 'p' &&
            get_char(p+3, ptr, nchars) == 's') ||
           (get_char(p+2, ptr, nchars) == 'p' &&
            get_char(p+3, ptr, nchars) == 't') ||
           get_char(p+2, ptr, nchars) == 'S')) {
        break;
      }  /* if */
    }  /* for */
    end_ptr = p;
  }  /* if */
  /* Here, end_ptr indicates the character after the end of the initial
     part of the name. */
  if (!is_special_name) {
    /* Output the characters of the base name. */
    for (p = ptr; p < end_ptr; p++) write_id_ch(*p, dctl);
  }  /* if */
  /* If there's a template argument list for a partial specialization
     (beginning with "__ps__"), process it. */
  if ((nchars == 0 || (end_ptr-ptr+6) < nchars) &&
      start_of_id_is("__ps__", end_ptr)) {
    /* Write the arguments.  This first argument list gives the arguments
       that appear in the partial specialization declaration:
         template <class T, class U> struct A { ... };
         template <class T> struct A<T *, int> { ... };
                                     ^^^^^^^^this argument list
       This first argument list will be followed by another argument list
       that gives the arguments according to the partial specialization.
       For A<int *, int> according to the example above, the second
       argument list is <int>.  The second argument list is scanned but
       not put out, except when argument correspondences are output. */
    end_ptr = demangle_template_arguments(end_ptr+6, /*partial_spec=*/TRUE,
                                          temp_par_info, dctl);
    note_specialization(end_ptr, temp_par_info);
    is_partial_spec = TRUE;
  }  /* if */
  /* If there's a specialization indication ("__S"), ignore it. */
  if (get_char(end_ptr, ptr, nchars)   == '_' &&
      get_char(end_ptr+1, ptr, nchars) == '_' &&
      get_char(end_ptr+2, ptr, nchars) == 'S') {
    note_specialization(end_ptr, temp_par_info);
    end_ptr += 3;
  }  /* if */
  /* If there's a template argument list (beginning with "__pt__" or "__tm__"),
     process it. */
  if ((nchars == 0 || (end_ptr-ptr+6) < nchars) &&
      ((is_pt = start_of_id_is("__pt__", end_ptr)) ||
       start_of_id_is("__tm__", end_ptr))) {
    /* The "__pt__ form indicates an old-style mangled template name. */
    if (is_pt && temp_par_info != NULL ) {
      temp_par_info->use_old_form_for_template_output = TRUE;
    }  /* if */
    /* For the second argument list of a partial specialization,
       process the argument list but suppress output. */
    if (is_partial_spec && temp_par_info != NULL &&
        !temp_par_info->output_only_correspondences) {
      dctl->suppress_id_output++;
      partial_spec_output_suppressed = TRUE;
    }  /* if */
    /* Write the arguments. */
    end_ptr = demangle_template_arguments(end_ptr+6, /*partial_spec=*/FALSE,
                                          temp_par_info, dctl);
    if (partial_spec_output_suppressed) dctl->suppress_id_output--;
    /* If there's a(nother) specialization indication ("__S"), ignore it. */
    if (get_char(end_ptr, ptr, nchars)   == '_' &&
        get_char(end_ptr+1, ptr, nchars) == '_' &&
        get_char(end_ptr+2, ptr, nchars) == 'S') {
      note_specialization(end_ptr, temp_par_info);
      end_ptr += 3;
    }  /* if */
  }  /* if */
  /* Check that we took exactly the characters we should have. */
  if ((nchars > 0) ?
          ((end_ptr - ptr) == nchars) :
          (*end_ptr == '\0' || (end_ptr[0] == '_' && end_ptr[1] == '_'))) {
    /* Okay. */
  } else {
    bad_mangled_name(dctl);
  }  /* if */
  return end_ptr;
}  /* full_demangle_name */


static const char *demangle_name_with_preceding_length(
                                   char const                 *ptr,
                                   a_template_param_block_ptr temp_par_info,
                                   a_decode_control_block_ptr dctl)
/*
Demangle a name that is preceded by a length, e.g., "3abc" for the type
name "abc".  Return a pointer to the character position following what
was demangled.  When temp_par_info != NULL, it points to a block that
controls output of extra information on template parameters.
*/
{
  const char    *p = ptr;
  unsigned long nchars;

  /* Get the length. */
  p = get_number(p, &nchars, dctl);
  /* Demangle the name. */
  p = full_demangle_name(p, nchars, (char *)NULL, temp_par_info, dctl);
  return p;
}  /* demangle_name_with_preceding_length */


static const char *demangle_simple_type_name(
                                   const char                 *ptr,
                                   a_template_param_block_ptr temp_par_info,
                                   a_decode_control_block_ptr dctl)
/*
Demangle a type name (or namespace name) consisting of a length followed
by the name.  Return a pointer to the character position following what
was demangled.  The name is not a nested name, but it can have template
arguments.  When temp_par_info != NULL, it points to a block that
controls output of extra information on template parameters.
*/
{
  const char    *p = ptr;

  if (*p == 'Z') {
    /* A template parameter name. */
    p = demangle_template_parameter_name(p, /*nontype=*/FALSE, dctl);
  } else {
    /* A simple mangled type name consists of digits indicating the length of
       the name followed by the name itself, e.g., "3abc". */
    p = demangle_name_with_preceding_length(p, temp_par_info, dctl);
  }  /* if */
  return p;
}  /* demangle_simple_type_name */


static const char *full_demangle_type_name(const char           *ptr,
                                     a_boolean                  base_name_only,
                                     a_template_param_block_ptr temp_par_info,
                                     a_decode_control_block_ptr dctl)
/*
Demangle the type name at ptr and output the demangled form.  Return a pointer
to the character position following what was demangled.  The name can be
a simple type name or a nested type name, or the name of a namespace.
If base_name_only is TRUE, do not put out any nested type qualifiers,
e.g., put out "A::x" as simply "x".  When temp_par_info != NULL, it
points to a block that controls output of extra information on template
parameters.  Note that this routine is called for namespaces too
(the mangling is the same as for class names; you can't actually tell
the difference in a mangled name).  See demangle_type_name for an
interface to this routine for the simple case.
*/
{
  const char    *p = ptr;
  unsigned long nquals;

#ifdef SCO_DEMANGLER
  /* fix for ambiguity in encoding, either "Q" or "<len>Q" */
  if (isdigit((unsigned char)*p)) {
    const char	*s = p + 1;
    while (isdigit((unsigned char)*s)) s++;
    if (*s == 'Q') {
      p = s;
    }  /* if */
  }  /* if */
#endif /* SCO_DEMANGLER */

  if (*p == 'Q') {
    /* A nested type name has the form
         Q2_5outer5inner   (outer::inner)
            ^-----^--------Names from outermost to innermost
          ^----------------Number of levels of qualification.
       Note that the levels in the qualifier can be class names or namespace
       names. */
    p = get_number(p+1, &nquals, dctl);
    p = advance_past_underscore(p, dctl);
    /* Handle each level of qualification. */
    for (; nquals > 0; nquals--) {
      if (dctl->err_in_id) break;  /* Avoid infinite loops on errors. */
      /* Do not put out the nested type qualifiers if base_name_only is
         TRUE. */
      if (base_name_only && nquals != 1) dctl->suppress_id_output++;
      p = demangle_simple_type_name(p, temp_par_info, dctl);
      if (nquals != 1) write_id_str("::", dctl);
      if (base_name_only && nquals != 1) dctl->suppress_id_output--;
    }  /* for */
  } else {
    /* A simple (non-nested) type name. */
    p = demangle_simple_type_name(p, temp_par_info, dctl);
  }  /* if */
  return p;
}  /* full_demangle_type_name */


static const char *demangle_vtbl_class_name(const char           *ptr,
                                      a_decode_control_block_ptr dctl)
/*
Demangle a class or base class name that is one component of a virtual
function table name.  Such names are mangled mostly as types, but with
a few special quirks.
*/
{
  const char      *p = ptr;
  a_boolean nested_name_case = FALSE;

  /* If the name begins with a number, "Q", and another number, assume
     it's a name with a form like "7Q2_1A1B", which is used to encode
     A::B as the complete object class name component of a virtual
     function table name.  This doesn't have any particular sense to
     it; it's just what cfront does (and EDG's front end does the same
     at ABI versions >= 2.30 in cfront compatibility mode).  This could
     fail if the user actually has a class with a name that begins
     like "Q2_", but there's not much we can do about that. */
  if (isdigit((unsigned char)*p)) {
    do { p++; } while (isdigit((unsigned char)*p));
    if (*p == 'Q') {
      const char *save_p = p;
      p++;
      if (isdigit((unsigned char)*p)) {
        do { p++; } while (isdigit((unsigned char)*p));
        if (*p == '_') {
          /* Yes, this is the strange nested name case.  Start the demangling
             at the "Q". */
          nested_name_case = TRUE;
          p = save_p;
        }  /* if */
      }  /* if */
    }  /* if */
  }  /* if */
  if (!nested_name_case) p = ptr;
  /* Now use the normal routine to demangle the class name. */
  p = demangle_type_name(p, dctl);
  return p;
}  /* demangle_vtbl_class_name */


static const char *demangle_type_qualifiers(
                                     const char                 *ptr,
                                     a_boolean                  trailing_space,
                                     a_decode_control_block_ptr dctl)
/*
Demangle any type qualifiers (const/volatile) at the indicated location.
Return a pointer to the character position following what was demangled.
If trailing_space is TRUE, add a space at the end if any qualifiers were
put out.
*/
{
  const char *p = ptr;
  a_boolean any_quals = FALSE;

  for (;; p++) {
    if (*p == 'C') {
      if (any_quals) write_id_ch(' ', dctl);
      write_id_str("const", dctl);
    } else if (*p == 'V') {
      if (any_quals) write_id_ch(' ', dctl);
      write_id_str("volatile", dctl);
    } else {
      break;
    }  /* if */
    any_quals = TRUE;
  }  /* for */
  if (any_quals && trailing_space) write_id_ch(' ', dctl);
  return p;
}  /* demangle_type_qualifiers */


static const char *demangle_type_specifier(const char           *ptr,
                                     a_decode_control_block_ptr dctl)
/*
Demangle the type at ptr and output the specifier part.  Return a pointer
to the character position following what was demangled.
*/
{
  const char *p = ptr, *s;

  /* Process type qualifiers. */
  p = demangle_type_qualifiers(p, /*trailing_space=*/TRUE, dctl);
  if (isdigit((unsigned char)*p) || *p == 'Q' || *p == 'Z') {
    /* Named type, like class or enum, e.g., "3abc". */
    p = demangle_type_name(p, dctl);
  } else {
    /* Builtin type. */
    /* Handle signed and unsigned. */
    if (*p == 'S') {
      write_id_str("signed ", dctl);
      p++;
    } else if (*p == 'U') {
      write_id_str("unsigned ", dctl);
      p++;
    }  /* if */
    switch (*p++) {
      case 'v':
        s = "void";
        break;
      case 'c':
        s = "char";
        break;
      case 'w':
        s = "wchar_t";
        break;
      case 'b':
        s = "bool";
        break;
      case 's':
        s = "short";
        break;
      case 'i':
        s = "int";
        break;
      case 'l':
        s = "long";
        break;
      case 'L':
        s = "long long";
        break;
      case 'f':
        s = "float";
        break;
      case 'd':
        s = "double";
        break;
      case 'r':
        s = "long double";
        break;
      default:
        bad_mangled_name(dctl);
        s = "";
    }  /* switch */
    write_id_str(s, dctl);
  }  /* if */
  return p;
}  /* demangle_type_specifier */


static const char *demangle_function_parameters(const char           *ptr,
                                          a_decode_control_block_ptr dctl)
/*
Demangle the parameter list beginning at ptr and output the demangled form.
Return a pointer to the character position following what was demangled.
*/
{
  const char *p = ptr;
  const char *param_pos[10];
  unsigned  long curr_param_num, param_num, nreps;
  a_boolean any_params = FALSE;

  write_id_ch('(', dctl);
  if (*p == 'v') {
    /* Void parameter list. */
    p++;
#if SCO_DEMANGLER
    if (*p != 'e') write_id_str("void", dctl);
#endif /* SCO_DEMANGLER */
  } else {
    any_params = TRUE;
    /* Loop for each parameter. */
    curr_param_num = 1;
    for (;;) {
      if (dctl->err_in_id) break;  /* Avoid infinite loops on errors. */
      if (*p == 'T' || *p == 'N') {
        /* Tn means repeat the type of parameter "n". */
        /* Nmn means "m" repetitions of the type of parameter "n".  "m"
           is a one-digit number. */
        /* "n" is also treated as a single-digit number; the front end enforces
           that (in non-cfront object code compatibility mode).  cfront does
           not, which leads to some ambiguities when "n" is followed by
           a class name. */
        if (*p++ == 'N') {
          /* Get the number of repetitions. */
          p = get_single_digit_number(p, &nreps, dctl);
        } else {
          nreps = 1;
        }  /* if */
        /* Get the parameter number. */
        p = get_single_digit_number(p, &param_num, dctl);
        if (param_num < 1 || param_num >= curr_param_num ||
            param_pos[param_num] == NULL) {
          /* Parameter number out of range. */
          bad_mangled_name(dctl);
          goto end_of_routine;
        }  /* if */
        /* Produce "nreps" copies of parameter "param_num". */
        for (; nreps > 0; nreps--) {
          if (dctl->err_in_id) break;  /* Avoid infinite loops on errors. */
          if (curr_param_num < 10) param_pos[curr_param_num] = NULL;
          (void)demangle_type(param_pos[param_num], dctl);
          if (nreps != 1) write_id_str(", ", dctl);
          curr_param_num++;
        }  /* if */
      } else {
        /* A normal parameter. */
        if (curr_param_num < 10) param_pos[curr_param_num] = p;
        p = demangle_type(p, dctl);
        curr_param_num++;
      }  /* if */
      /* Stop after the last parameter. */
      if (*p == '\0' || *p == 'e' || *p == '_') break;
      write_id_str(", ", dctl);
    }  /* for */
  }  /* if */
  if (*p == 'e') {
    /* Ellipsis. */
    if (any_params) write_id_str(", ", dctl);
    write_id_str("...", dctl);
    p++;
  }  /* if */
  write_id_ch(')', dctl);
end_of_routine:
  return p;
}  /* demangle_function_parameters */


static const char *skip_extern_C_indication(const char *ptr)
/*
ptr points to the character after the "F" of a function type.  Skip over
and ignore an indication of extern "C" following the "F", if one is present.
Return a pointer to the character following the extern "C" indication.
There's no syntax for representing the extern "C" in the function type, so
just ignore it.
*/
{
  if (*ptr == 'K') ptr++;
  return ptr;
}  /* skip_extern_C_indication */

#if SCO_DEMANGLER
static const char *return_type_ptr;
#endif

static const char *demangle_type_first_part(
                               const char                 *ptr,
                               a_boolean                  under_lhs_declarator,
                               a_boolean                  need_trailing_space,
                               a_decode_control_block_ptr dctl)
/*
Demangle the type at ptr and output the specifier part and the part of the
declarator that precedes the name.  Return a pointer to the character
position following what was demangled.  If under_lhs_declarator is TRUE,
this type is directly under a type that uses a left-side declarator,
e.g., a pointer type.  (That's used to control use of parentheses around
parts of the declarator.)  If need_trailing_space is TRUE, put a space
at the end of the specifiers part (needed if the declarator part is
not empty, because it contains a name or a derived type).
*/
{
  const char *p = ptr, *qualp = p;
  char kind;

  /* Remove type qualifiers. */
  while (is_immediate_type_qualifier(p)) p++;
  kind = *p;
  if (kind == 'P' || kind == 'R') {
    /* Pointer or reference type, e.g., "Pc" is pointer to char. */
    p = demangle_type_first_part(p+1, /*under_lhs_declarator=*/TRUE,
                                 /*need_trailing_space=*/TRUE, dctl);
    /* Output "*" or "&" for pointer or reference. */
    if (kind == 'R') {
      write_id_ch('&', dctl);
    } else {
      write_id_ch('*', dctl);
    }  /* if */
    /* Output the type qualifiers on the pointer, if any. */
    (void)demangle_type_qualifiers(qualp, /*trailing_space=*/TRUE, dctl);
  } else if (kind == 'M') {
    /* Pointer-to-member type, e.g., "M1Ai" is pointer to member of A of
       type int. */
    const char *classp = p+1;
    /* Skip over the class name. */
    dctl->suppress_id_output++;
    p = demangle_type_name(classp, dctl);
    dctl->suppress_id_output--;
    p = demangle_type_first_part(p, /*under_lhs_declarator=*/TRUE,
                                 /*need_trailing_space=*/TRUE, dctl);
    /* Output Classname::*. */
    (void)demangle_type_name(classp, dctl);
    write_id_str("::*", dctl);
    /* Output the type qualifiers on the pointer, if any. */
    (void)demangle_type_qualifiers(qualp, /*trailing_space=*/TRUE, dctl);
  } else if (kind == 'F') {
    /* Function type, e.g., "Fii_f" is function(int, int) returning float.
       The return type is not present for top-level function types (except
       for template functions). */
    p = skip_extern_C_indication(p+1);
    /* Skip over the parameter types without outputting anything. */
    dctl->suppress_id_output++;
    p = demangle_function_parameters(p, dctl);
    dctl->suppress_id_output--;
    if (*p == '_' && p[1] != '_') {
      /* The return type is present. */
#if SCO_DEMANGLER
      if (dctl->suppress_id_output) {
        const char *ret_ptr = p+1;
        p = demangle_type_first_part(p+1, /*under_lhs_declarator=*/FALSE,
                                   /*need_trailing_space=*/FALSE, dctl);
        return_type_ptr = ret_ptr;
      } else
#endif /* SCO_DEMANGLER */
      p = demangle_type_first_part(p+1, /*under_lhs_declarator=*/FALSE,
                                   /*need_trailing_space=*/TRUE, dctl);
    }  /* if */
    /* This is a right-side declarator, so if it's under a left-side declarator
       parentheses are needed. */
    if (under_lhs_declarator) write_id_ch('(', dctl);
  } else if (kind == 'A') {
    /* Array type, e.g., "A10_i" is array[10] of int. */
    p++;
    if (*p == '_') {
      /* Length is specified by a constant expression based on template
         parameters.  Ignore the expression. */
      p++;
      dctl->suppress_id_output++;
      p = demangle_constant(p, dctl);
      dctl->suppress_id_output--;
    } else {
      /* Normal constant number of elements. */
      /* Skip the array size. */
      while (isdigit((unsigned char)*p)) p++;
    }  /* if */
    p = advance_past_underscore(p, dctl);
    /* Process the element type. */
    p = demangle_type_first_part(p, /*under_lhs_declarator=*/FALSE,
                                 /*need_trailing_space=*/TRUE, dctl);
    /* This is a right-side declarator, so if it's under a left-side declarator
       parentheses are needed. */
    if (under_lhs_declarator) write_id_ch('(', dctl);
  } else {
    /* No declarator part to process.  Handle the specifier type. */
    p = demangle_type_specifier(qualp, dctl);
    if (need_trailing_space) write_id_ch(' ', dctl);
  }  /* if */
  return p;
}  /* demangle_type_first_part */


static const char *demangle_type_second_part(
                               const char                 *ptr,
                               a_boolean                  under_lhs_declarator,
                               a_decode_control_block_ptr dctl)
/*
Demangle the type at ptr and output the part of the declarator that follows the
name.  This routine does not return a pointer to the character position
following what was demangled; it is assumed that the caller will save
that from the call of demangle_type_first_part, and it saves a lot of
time if this routine can avoid scanning the specifiers again.
If under_lhs_declarator is TRUE, this type is directly under a type that
uses a left-side declarator, e.g., a pointer type.  (That's used to control
use of parentheses around parts of the declarator.)
*/
{
  const char *p = ptr, *qualp = p;
  char kind;

  /* Remove type qualifiers. */
  while (is_immediate_type_qualifier(p)) p++;
  kind = *p;
  if (kind == 'P' || kind == 'R') {
    /* Pointer or reference type, e.g., "Pc" is pointer to char. */
    demangle_type_second_part(p+1, /*under_lhs_declarator=*/TRUE, dctl);
  } else if (kind == 'M') {
    /* Pointer-to-member type, e.g., "M1Ai" is pointer to member of A of
       type int. */
    /* Advance over the class name. */
    dctl->suppress_id_output++;
    p = demangle_type_name(p+1, dctl);
    dctl->suppress_id_output--;
    demangle_type_second_part(p, /*under_lhs_declarator=*/TRUE, dctl);
  } else if (kind == 'F') {
    /* Function type, e.g., "Fii_f" is function(int, int) returning float.
       The return type is not present for top-level function types (except
       for template functions). */
    /* This is a right-side declarator, so if it's under a left-side declarator
       parentheses are needed. */
    if (under_lhs_declarator) write_id_ch(')', dctl);
    p = skip_extern_C_indication(p+1);
    /* Put out the parameter types. */
    p = demangle_function_parameters(p, dctl);
    /* Put out any cv-qualifiers (member functions). */
    /* Note that such things could come up on nonmember functions in the
       presence of typedefs.  In such a case what we generate here will not
       be valid C, but it's a reasonable representation of the mangled
       type, and there's no way of getting the typedef name in there,
       so let it be. */
    if (*qualp != 'F') {
      write_id_ch(' ', dctl);
      (void)demangle_type_qualifiers(qualp, /*trailing_space=*/FALSE, dctl);
    }  /* if */
    if (*p == '_' && p[1] != '_') {
      /* Process the return type. */
      demangle_type_second_part(p+1, /*under_lhs_declarator=*/FALSE, dctl);
    }  /* if */
  } else if (kind == 'A') {
    /* Array type, e.g., "A10_i" is array[10] of int. */
    /* This is a right-side declarator, so if it's under a left-side declarator
       parentheses are needed. */
    if (under_lhs_declarator) write_id_ch(')', dctl);
    write_id_ch('[', dctl);
    p++;
    if (*p == '_') {
      /* Length is specified by a constant expression based on template
         parameters. */
      p++;
      p = demangle_constant(p, dctl);
    } else {
      /* Normal constant number of elements. */
      if (*p == '0' && p[1] == '_') {
        /* Size is zero, so do not put out a size (the result is "[]"). */
        p++;
      } else {
        /* Put out the array size. */
        while (isdigit((unsigned char)*p)) write_id_ch(*p++, dctl);
      }  /* if */
    }  /* if */
    p = advance_past_underscore(p, dctl);
    write_id_ch(']', dctl);
    /* Process the element type. */
    demangle_type_second_part(p, /*under_lhs_declarator=*/FALSE, dctl);
  } else {
    /* No declarator part to process.  No need to scan the specifiers type --
       it was done by demangle_type_first_part. */
  }  /* if */
}  /* demangle_type_second_part */


static const char *demangle_type(const char           *ptr,
                           a_decode_control_block_ptr dctl)
/*
Demangle the type at ptr and output the demangled form.  Return a pointer to
the character position following what was demangled.
*/
{
  const char *p;

  /* Generate the specifier part of the type. */
  p = demangle_type_first_part(ptr, /*under_lhs_declarator=*/FALSE,
                               /*need_trailing_space=*/FALSE, dctl);
  /* Generate the declarator part of the type. */
  demangle_type_second_part(ptr, /*under_lhs_declarator=*/FALSE, dctl);
  return p;
}  /* demangle_type */


static const char *demangle_identifier(const char           *ptr,
                                 a_decode_control_block_ptr dctl)
/*
Demangle the identifier at ptr and output the demangled form.  Return
a pointer to the character position following what was demangled.
*/
{
  const char    *p = ptr, *origname, *pname, *end_ptr;
  const char    *final_specialization, *end_ptr_first_scan;
  a_boolean     member_function = TRUE;
  a_template_param_block
                temp_par_info;

start_of_mangled_name:
  origname = p;
  clear_template_param_block(&temp_par_info);
  /* Scan through the name (the first part of the mangled name) without
     generating output, to see what's beyond it.  Special processing is
     necessary for names of constructors, conversion routines, etc. */
  /* If the name has a specialization indication in it (which can happen for
     function names), note that fact. */
  temp_par_info.set_final_specialization = TRUE;
  dctl->suppress_id_output++;
  p = full_demangle_name(origname, (unsigned long)0, (char *)NULL,
                         &temp_par_info, dctl);
  dctl->suppress_id_output--;
  final_specialization = temp_par_info.final_specialization;
  clear_template_param_block(&temp_par_info);
  temp_par_info.final_specialization = final_specialization;
  if (*p == '\0') {
    /* There is no mangled part of the name.  This happens for strange
       cases like
         extern "C" int operator +(A, A);
       which gets mangled as "__pl".  Just write out the name and stop. */
    end_ptr = demangle_name(origname, dctl);
  } else {
    /* There's more.  There should be a "__" between the name and the
       additional mangled information. */
    if (p[0] != '_' || p[1] != '_') {
      bad_mangled_name(dctl);
      end_ptr = p;
      goto end_of_routine;
    }  /* if */
    end_ptr = p + 2;
    /* Now origname points to the original-name part of the mangled name, and
       end_ptr points to the mangled-name part at the end.
         f__1AFv
            ^---- end_ptr
         ^------- origname
       The mangled-name part is
         (a)  A class name for a static data member.
         (b)  A class name followed by "F" followed by the encoding for the
              parameter types for a member function.
         (c)  "F" followed by the encoding for the parameter types for a
              nonmember function.
         (d)  "L" plus a local block number, followed by the mangled function
              name, for a function-local entity.
       Members of namespaces are encoded similarly. */
    p = end_ptr;
    pname = NULL;
    if (end_ptr[0] == 'L') {
      unsigned long block_number;
      /* The name of an entity within a function, mangled on promotion out
         of the function.  For example, "i__L1__f__Fv" for "i" from block 1
         of function "f(void)".  Note that this is not the same mangling
         used by cfront (in the cfront scheme, the __L1 is at the end, and
         the number is different). */
      /* Put out the entity name (the first part of the mangled name). */
      (void)demangle_name(origname, dctl);
      write_id_str(" in", dctl);
      /* Get the block number and put it out.  Block 0 is the top-level block
         of the function, and need not be identified. */
      p = get_number(end_ptr+1, &block_number, dctl);
      if (block_number != 0) {
        char buffer[30];
        write_id_str(" block ", dctl);
        (void)sprintf(buffer, "%lu", block_number);
        write_id_str(buffer, dctl);
        write_id_str(" of", dctl);
      }  /* if */
      write_id_str(" function ", dctl);
      /* Check for the two underscores following the block number. */
      if (p[0] != '_' || p[1] != '_') {
        bad_mangled_name(dctl);
        end_ptr = p;
        goto end_of_routine;
      }  /* if */
      /* Go back to scan and output the function name. */
      p += 2;
      goto start_of_mangled_name;
    } else if (end_ptr[0] != 'F') {
      /* A class (or namespace) name must be next. */
      /* Remember the location of the parent entity name. */
      pname = end_ptr;
      /* Scan over the class name, producing no output, and remembering the
         position of the final specialization, if any.  If we already
         found a specialization on the function name, it's the final one
         and we shouldn't change it. */
      dctl->suppress_id_output++;
      if (temp_par_info.final_specialization == NULL) {
        temp_par_info.set_final_specialization = TRUE;
      }  /* if */
      end_ptr = full_demangle_type_name(pname, /*base_name_only=*/FALSE,
                                        &temp_par_info, dctl);
      temp_par_info.set_final_specialization = FALSE;
      dctl->suppress_id_output--;
      /* If the name ends here, this is a simple member (e.g., a static
         data member). */
      if (*end_ptr == '\0' ||
          (end_ptr[0] == '_' && end_ptr[1] == '_')) member_function = FALSE;
    }  /* if */
    if (member_function) {
      /* "S" here means a static member function (ignore). */
      if (*end_ptr == 'S') end_ptr++;
      /* Write the specifier part of the type. */
#if SCO_DEMANGLER
      dctl->suppress_id_output++;
      return_type_ptr = 0;
#endif /* SCO_DEMANGLER */
      end_ptr_first_scan =
                  demangle_type_first_part(end_ptr,
                                           /*under_lhs_declarator=*/FALSE,
                                           /*need_trailing_space=*/TRUE, dctl);
#if SCO_DEMANGLER
      dctl->suppress_id_output--;
#endif /* SCO_DEMANGLER */
    }  /* if */
    temp_par_info.nesting_level = 0;
    if (pname != NULL) {
      /* Write the parent class or namespace qualifier. */
      if (temp_par_info.final_specialization != NULL) {
        /* Up to the final specialization, put out actual template arguments
           for specializations. */
        temp_par_info.actual_template_args_until_final_specialization = TRUE;
      }  /* if */
      (void)full_demangle_type_name(pname, /*base_name_only=*/FALSE,
                                    &temp_par_info, dctl);
      /* Force template parameter information out on the function even if
         it is specialized. */
      temp_par_info.actual_template_args_until_final_specialization = FALSE;
      write_id_str("::", dctl);
    }  /* if */
    /* Write the name of the member. */
    (void)full_demangle_name(origname, (unsigned long)0, pname,
                             &temp_par_info, dctl);
    if (member_function) {
      /* Write the declarator part of the type. */
      demangle_type_second_part(end_ptr, /*under_lhs_declarator=*/FALSE,
                                dctl);
      end_ptr = end_ptr_first_scan;
    }  /* if */
    if (!temp_par_info.use_old_form_for_template_output &&
        temp_par_info.nesting_level != 0) {
      /* Put out correspondences for template parameters, e.g, "T=int". */
      temp_par_info.nesting_level = 0;
      temp_par_info.first_correspondence = TRUE;
      temp_par_info.output_only_correspondences = TRUE;
      /* Output is suppressed in general, and turned on only where
         appropriate. */
      dctl->suppress_id_output++;
      if (pname != NULL) {
        /* Write the parent class or namespace qualifier. */
        if (temp_par_info.final_specialization != NULL) {
          /* Up to the final specialization, put out actual template arguments
             for specializations. */
          temp_par_info.actual_template_args_until_final_specialization = TRUE;
        }  /* if */
        (void)full_demangle_type_name(pname, /*base_name_only=*/FALSE,
                                      &temp_par_info, dctl);
      }  /* if */
      /* Force template parameter information out on the function even if
         it is specialized. */
      temp_par_info.actual_template_args_until_final_specialization = FALSE;
      /* Write the name of the member. */
      (void)full_demangle_name(origname, (unsigned long)0, pname,
                               &temp_par_info, dctl);
      dctl->suppress_id_output--;
#if SCO_DEMANGLER
      if (member_function && return_type_ptr && *return_type_ptr) {
        if (!temp_par_info.first_correspondence) {
          write_id_str(", ", dctl);
        } else {
          write_id_str(" [", dctl);
        } /* if */
        write_id_str("return type=", dctl);
        (void)demangle_type_first_part(return_type_ptr,
				/*under_lhs_declarator=*/FALSE,
				/*need_trailing_space=*/FALSE, dctl);
      } /* if */
#endif /* SCO_DEMANGLER */
      if (!temp_par_info.first_correspondence) {
        /* End the list of correspondences. */
        write_id_ch(']', dctl);
      }  /* if */
    }  /* if */
#if SCO_DEMANGLER
    if (dctl->err_in_id && pname != NULL) {
      /* handle old UnixWare 2.1-style hoisted type names
	 id__mangled_function__L#_# */
      a_decode_control_block     ctlb;
      const char *endp;
      ctlb.input_id_len = strlen(pname);
      ctlb.output_id = 0;
      ctlb.output_id_len = 0;
      ctlb.output_id_size = 0;
      ctlb.err_in_id = FALSE;
      ctlb.output_overflow_err = FALSE;
      ctlb.suppress_id_output = 1;
      /* skip over the function name -- pname points to the beginning
         of the mangled function name (past id) */
      p = demangle_identifier(pname, &ctlb);
      /* Check that the remainder looks like __L#_# */
      if (p && *p++ == '_' && *p++ == '_' && *p++ == 'L' && isdigit(*p++)) {
        while (isdigit(*p)) ++p;
        if (*p++ == '_' && isdigit(*p++)) {
          while (isdigit(*p)) ++p;
          if (!*p) {
            /* Put out the entity name (the first part of the mangled name). */
            dctl->err_in_id = FALSE;
	    dctl->suppress_id_output = FALSE;
            (void)demangle_name(origname, dctl);
            return p;
          } /* if */
        } /* if */
      }  /* if */
    } /* if */
#endif /* SCO_DEMANGLER */
  }  /* if */
end_of_routine:
  return end_ptr;
}  /* demangle_identifier */


static const char *demangle_local_name(const char           *ptr,
                                 a_decode_control_block_ptr dctl)
/*
Demangle the local name at ptr and output the demangled form.  Return
a pointer to the character position following what was demangled.
This demangles the "__nn_mm_name" form produced by the C-generating
back end.  This is not something visible unless the C-generating back end
is used, and it's a local name, which is ordinarily outside the charter
of these demangling routines, but it's an easy and common case, so...
*/
{
  const char *p = ptr+2;
#if SCO_DEMANGLER
  const char *s;
#endif /* SCO_DEMANGLER */

  /* Check for the initial two numbers and underscores.  The caller checked
     for the two initial underscores and the digit following that. */
  do { p++; } while (isdigit((unsigned char)*p));
#if SCO_DEMANGLER
  if (*p == '\0') {
    bad_mangled_name(dctl);
    goto end_of_routine;
  } /* if */
  s = p;
  if (*s++ == '_') {
    if (isdigit((unsigned char)*s)) {
      do { s++; } while (isdigit((unsigned char)*s));
      if (*s == '_')
        p = s+1;
      else if (*s != '\0')
	p = s;
    } /* if */
  } /* if */  
  if (start_of_id_is("__Q", p) ||
      start_of_id_is("__pt__", p)) {
      p = demangle_type_name(p+2, dctl);
  } else {
    s = demangle_identifier(p, dctl);
    if (dctl->err_in_id) {
      /* reset control block and just write string minus the __nn__ prefix */
      dctl->output_id_len = 0;
      dctl->err_in_id = FALSE;
      dctl->output_overflow_err = FALSE;
      dctl->suppress_id_output = 0;
      write_id_str(p, dctl);
      p += strlen(p);
    } else {
      p = s;
    } /* if */
  }
#else   
  if (*p != '_') {
    bad_mangled_name(dctl);
    goto end_of_routine;
  }  /* if */
  p++;
  if (!isdigit((unsigned char)*p)) {
    bad_mangled_name(dctl);
    goto end_of_routine;
  }  /* if */
  do { p++; } while (isdigit((unsigned char)*p));
  if (*p != '_') {
    bad_mangled_name(dctl);
    goto end_of_routine;
  }  /* if */
  p++;
  /* Copy the rest of the string to output. */
  while (*p != '\0') {
    write_id_ch(*p, dctl);
    p++;
  }  /* while */
#endif /* SCO_DEMANGLER */
end_of_routine:
  return p;
}  /* demangle_local_name */

#if SCO_DEMANGLER
static
#endif /* SCO_DEMANGLER */
void decode_identifier(const char      *id,
                       char      *output_buffer,
                       sizeof_t  output_buffer_size,
                       a_boolean *err,
                       a_boolean *buffer_overflow_err,
                       sizeof_t  *required_buffer_size)
/*
Demangle the identifier id (which is null-terminated), and put the demangled
form (null-terminated) into the output_buffer provided by the caller.
output_buffer_size gives the allocated size of output_buffer.  If there
is some error in the demangling process, *err will be returned TRUE.
In addition, if the error is that the output buffer is too small,
*buffer_overflow_err will (also) be returned TRUE, and *required_buffer_size
is set to the size of buffer required to do the demangling.
*/
{
  const char                 *end_ptr;
  a_decode_control_block     control_block;
  a_decode_control_block_ptr dctl = &control_block;

  /* Set global variables. */
  dctl->input_id_len = strlen(id);
  dctl->output_id = output_buffer;
  dctl->output_id_len = 0;
  dctl->output_id_size = output_buffer_size;
  dctl->err_in_id = FALSE;
  dctl->output_overflow_err = FALSE;
  dctl->suppress_id_output = 0;
  /* Check for special cases. */
#if SCO_DEMANGLER
  if (start_of_id_is("__", id)) {
    if (start_of_id_is("vtbl", id+2)) {
      /* Note that if the first name is a base class name and it's not simple,
         this will produce output containing partially-mangled information.
         It's hard to do better given the cfront encoding form. */
      end_ptr = demangle_vtbl_class_name(id+8, dctl);
      write_id_str("::__vtbl", dctl);
      if (start_of_id_is("__A", end_ptr)) {
        /* "__A" indicates an ambiguous base class. */
        write_id_str(" (ambiguous)", dctl);
        end_ptr += 3;
      }  /* if */
      if (start_of_id_is("__", end_ptr)) {
        /* Virtual function table for base class in derived class. */
        end_ptr += 2;
        write_id_str(" in ", dctl);
        end_ptr = demangle_vtbl_class_name(end_ptr, dctl);
      }  /* if */
    } else if (start_of_id_is("std__", id+2)) {
      end_ptr = demangle_identifier(id+7, dctl);
      write_id_str("::__std", dctl);
    } else if (start_of_id_is("sti__", id+2)) {
      end_ptr = demangle_identifier(id+7, dctl);
      write_id_str("::__sti", dctl);
    } else if (start_of_id_is("ptbl_vec__", id+2)) {
      end_ptr = demangle_identifier(id+12, dctl);
      write_id_str("::__ptbl_vec", dctl);
    } else if (start_of_id_is("CBI__", id+2)) {
      write_id_str("__CBI__::", dctl);
      end_ptr = demangle_identifier(id+7, dctl);
    } else if (start_of_id_is("DNI__", id+2)) {
      write_id_str("__DNI__::", dctl);
      end_ptr = demangle_identifier(id+7, dctl);
    } else if (start_of_id_is("TIR__", id+2)) {
      write_id_str("__TIR__::", dctl);
      end_ptr = demangle_identifier(id+7, dctl);
    } else if (start_of_id_is("LSG__", id+2)) {
      write_id_str("__LSG__::", dctl);
      end_ptr = demangle_identifier(id+7, dctl);
    } else if (start_of_id_is("TID_", id+2)) {
      write_id_str("__TID__::", dctl);
      end_ptr = demangle_type(id+6, dctl);
    } else if (start_of_id_is("VFE__", id+2)) {
      write_id_str("__VFE__::", dctl);
      end_ptr = demangle_type(id+7, dctl);
    } else if (start_of_id_is("T_", id+2)) {
      write_id_str("__T_::", dctl);
      end_ptr = demangle_type(id+4, dctl);
    } else if (start_of_id_is("Q", id+2)) {
      /* Nested class name. */
      end_ptr = demangle_type_name(id+2, dctl);
    } else if (isdigit((unsigned char)id[2])) {
      /* Local variable mangled by the C-generating back end: __nn_mm_name,
         where "nn" and "mm" are decimal integers. */
      end_ptr = demangle_local_name(id, dctl);
    } else {
      /* Normal case: function name, static data member name, or
         name of type or variable promoted out of function. */
      end_ptr = demangle_identifier(id, dctl);
    } /* if */
#else
  if (start_of_id_is("__vtbl__", id)) {
    write_id_str("virtual function table for ", dctl);
    /* Note that if the first name is a base class name and it's not simple,
       this will produce output containing partially-mangled information.
       It's hard to do better given the cfront encoding form. */
    end_ptr = demangle_vtbl_class_name(id+8, dctl);
    if (start_of_id_is("__A", end_ptr)) {
      /* "__A" indicates an ambiguous base class. */
      write_id_str(" (ambiguous)", dctl);
      end_ptr += 3;
    }  /* if */
    if (start_of_id_is("__", end_ptr)) {
      /* Virtual function table for base class in derived class. */
      end_ptr += 2;
      write_id_str(" in ", dctl);
      end_ptr = demangle_vtbl_class_name(end_ptr, dctl);
    }  /* if */
  } else if (start_of_id_is("__CBI__", id)) {
    write_id_str("can-be-instantiated flag for ", dctl);
    end_ptr = demangle_identifier(id+7, dctl);
  } else if (start_of_id_is("__DNI__", id)) {
    write_id_str("do-not-instantiate flag for ", dctl);
    end_ptr = demangle_identifier(id+7, dctl);
  } else if (start_of_id_is("__TIR__", id)) {
    write_id_str("template-instantiation-request flag for ", dctl);
    end_ptr = demangle_identifier(id+7, dctl);
  } else if (start_of_id_is("__LSG__", id)) {
    write_id_str("initialization guard variable for ", dctl);
    end_ptr = demangle_identifier(id+7, dctl);
  } else if (start_of_id_is("__TID_", id)) {
    write_id_str("type identifier for ", dctl);
    end_ptr = demangle_type(id+6, dctl);
  } else if (start_of_id_is("__T_", id)) {
    write_id_str("typeinfo for ", dctl);
    end_ptr = demangle_type(id+4, dctl);
  } else if (start_of_id_is("__VFE__", id)) {
    write_id_str("surrogate in class ", dctl);
    p = demangle_type(id+7, dctl);
    if (p[0] != '_' || p[1] != '_') {
      bad_mangled_name(dctl);
      end_ptr = p;
    } else {
      write_id_str(" for ", dctl);
      end_ptr = demangle_identifier(p+2, dctl);
    }  /* if */
  } else if (start_of_id_is("__Q", id)) {
    /* Nested class name. */
    end_ptr = demangle_type_name(id+2, dctl);
  } else if (start_of_id_is("__", id) && isdigit((unsigned char)id[2])) {
    /* Local variable mangled by the C-generating back end: __nn_mm_name,
       where "nn" and "mm" are decimal integers. */
    end_ptr = demangle_local_name(id, dctl);
#endif /* SCO_DEMANGLER */
  } else {
    /* Normal case: function name, static data member name, or
       name of type or variable promoted out of function. */
    end_ptr = demangle_identifier(id, dctl);
  }  /* if */
  /* Make sure the whole identifier was taken. */
  if (!dctl->err_in_id && *end_ptr != '\0') bad_mangled_name(dctl);
  /* Add a terminating null. */
  if (!dctl->output_overflow_err) dctl->output_id[dctl->output_id_len] = 0;
  *err = dctl->err_in_id;
  *buffer_overflow_err = dctl->output_overflow_err;
  *required_buffer_size = dctl->output_id_len + 1; /* +1 for final null. */
}  /* decode_identifier */


#if SCO_DEMANGLER
int demangle(const char *in, char *out, size_t len)
{
	a_boolean err = FALSE;
	a_boolean overflow = FALSE;
	sizeof_t  size_required;

	decode_identifier(in, out, len, &err, &overflow, &size_required);
	if (err && !overflow)
		return -1;
	return size_required;
} /* demangle */
#endif /* SCO_DEMANGLER */

/******************************************************************************
*                                                             \  ___  /       *
*                                                               /   \         *
* Edison Design Group C++/C Front End                        - | \^/ | -      *
*                                                               \   /         *
*                                                             /  | |  \       *
* Copyright 1996 Edison Design Group Inc.                        [_]          *
*                                                                             *
******************************************************************************/
