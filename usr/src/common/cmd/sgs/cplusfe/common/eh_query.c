#ident	"@(#)cplusfe:common/eh_query.c	1.2"

#include "fe_common.h"
#include "eh_query.h"

#include <search.h>

/*	This is a list of every function in the C standard library,
	as of ISO/IEC 9899:1990(E), except for qsort and bsearch 
	(calls to both of which might result in an exception thrown 
	via their comparsion functions; note signal and atexit are ok,
	since the functions passed to them are not invoked at the time
	of the call).  Some of these functions are normally implemented 
	as macros, in which case no harm done.
*/

static char* c_stdlib_funcs[] = {
	"isalnum",
	"isalpha",
	"iscntrl",
	"isdigit",
	"isgraph",
	"islower",
	"isprint",
	"ispunct",
	"isspace",
	"isupper",
	"isxdigit",
	"tolower",
	"toupper",
	"setlocale",
	"localeconv",
	"acos",
	"asin",
	"atan",
	"atan2",
	"cos",
	"sin",
	"tan",
	"cosh",
	"sinh",
	"tanh",
	"exp",
	"frexp",
	"ldexp",
	"log",
	"log10",
	"modf",
	"pow",
	"sqrt",
	"ceil",
	"fabs",
	"floor",
	"fmod",
	"setjmp",
	"longjmp",
	"signal",
	"raise",
	"remove",
	"rename",
	"tmpfile",
	"tmpnam",
	"fclose",
	"fflush",
	"fopen",
	"freopen",
	"setbuf",
	"setvbuf",
	"fprintf",
	"fscanf",
	"printf",
	"scanf",
	"sprintf",
	"sscanf",
	"vfprintf",
	"vprintf",
	"vsprintf",
	"fgetc",
	"fgets",
	"fputc",
	"fputs",
	"getc",
	"getchar",
	"gets",
	"putc",
	"putchar",
	"puts",
	"ungetc",
	"fread",
	"fwrite",
	"fgetpos",
	"fseek",
	"fsetpos",
	"ftell",
	"rewind",
	"clearerr",
	"feof",
	"ferror",
	"perror",
	"atof",
	"atoi",
	"atol",
	"strtod",
	"strtol",
	"strtoul",
	"rand",
	"srand",
	"calloc",
	"free",
	"malloc",
	"realloc",
	"abort",
	"atexit",
	"exit",
	"getenv",
	"system",
	/* "bsearch", */
	/* "qsort", */
	"abs",
	"div",
	"labs",
	"ldiv",
	"mblen",
	"mbtowc",
	"wctomb",
	"mbstowcs",
	"wcstombs",
	"memcpy",
	"memmove",
	"strcpy",
	"strncpy",
	"strcat",
	"strncat",
	"memcmp",
	"strcmp",
	"strcoll",
	"strncmp",
	"strxfrm",
	"memchr",
	"strchr",
	"strcspn",
	"strpbrk",
	"strrchr",
	"strspn",
	"strstr",
	"strtok",
	"memset",
	"strerror",
	"strlen",
	"clock",
	"difftime",
	"mktime",
	"time",
	"asctime",
	"ctime",
	"gmtime",
	"localtime",
	"strftime"
	};

static a_boolean name_is_standard(char* name)
{
	static a_boolean first_time = TRUE;

	ENTRY name_e; name_e.key = name; name_e.data = NULL;

	if (first_time) {
		/* create and fill the hash table */
		const int nbr_funcs = sizeof(c_stdlib_funcs)/sizeof(char*);
		int i;
		hcreate(nbr_funcs);
		for (i = 0; i < nbr_funcs; i++) {
			ENTRY e; e.key = c_stdlib_funcs[i]; e.data = NULL;
			hsearch(e, ENTER);
		}
		first_time = FALSE;
	}

	return hsearch(name_e, FIND) != NULL;

	/* not putting in call to hdestroy for now; table is created just
	   twice per translation unit and takes relatively minimal storage */
}
 
a_boolean routine_has_null_exception_specification(a_routine_ptr routine)
{
	/* does this routine have a null exception specification? */
	an_exception_specification_ptr exc_spec =
		skip_typerefs(routine->type)->
		   variant.routine.extra_info->exception_specification;
	return exc_spec != NULL && exc_spec->exception_specification_type_list == NULL;
}
	
static a_boolean routine_is_normal_call_to_C_standard_library(a_routine_ptr routine)
{
	/* Is this a call to the C standard library?  
	   Exclude calls to qsort et al that might have C++ callbacks.
	   We've already made sure this is a hosted implementation. */


	if (routine->source_corresp.name_linkage == nlk_external && strcmp(name_linkage_kind_names[(int)routine->source_corresp.name_linkage], "C") == 0)
		return name_is_standard(routine->source_corresp.name);
	else
		return FALSE;
}

static a_boolean routine_is_nonthrowing_compiler_generated(a_routine_ptr routine)
{
	/* Is this a call to a compiler-generated function that we know
	   can't possibly throw an exception? */
	if (routine->compiler_generated &&
	    routine->source_corresp.name_linkage == nlk_external && 
            strcmp(name_linkage_kind_names[(int)routine->source_corresp.name_linkage], "C") == 0)
		if ((strcmp(routine->source_corresp.name,"__record_needed_destruction") == 0) ||
		    (strcmp(routine->source_corresp.name,"__dynamic_cast") == 0) || 
		    (strcmp(routine->source_corresp.name,"__throw_setup") == 0) || 
		    (strcmp(routine->source_corresp.name,"__exception_started") == 0) || 
		    (strcmp(routine->source_corresp.name,"__exception_caught") == 0) || 
		    (strcmp(routine->source_corresp.name,"__pure_virtual_called") == 0))
			return TRUE;
	return FALSE;
}

a_boolean routine_may_throw_exception(a_routine_ptr routine, a_boolean hosted)
{
	/* might a function call to this routine throw an exception? */

	if (routine_has_null_exception_specification(routine))
		/* it has null exception specification, can't throw */
		return FALSE;
	else if (hosted && routine_is_normal_call_to_C_standard_library(routine))
		/* is a call to C standard library that can't throw exception */
		return FALSE;
	else if (routine_is_nonthrowing_compiler_generated(routine))
		/* is call to compiler-generated function that doesn't throw */
		return FALSE;
	else if (! routine->defined)
		/* an extern or function not scanned yet, assume it throws */
		return TRUE;
	else 
		/* we've seen the function and it's been marked */
		return routine->may_throw_exception;
}

a_boolean function_variable_may_throw_exception(a_variable_ptr variable)
{
	/* might a function call through this variable throw an exception? */
	if (variable->type->kind == tk_pointer) {
		const a_type_ptr type_pointed_to = variable->type->variant.pointer.type;
		if (type_pointed_to->kind == tk_routine) {
			const an_exception_specification_ptr exc_spec =
			  skip_typerefs(type_pointed_to)->
			  variant.routine.extra_info->exception_specification;
			const a_boolean has_null_exc_spec = exc_spec != NULL && 
				 exc_spec->exception_specification_type_list == NULL;
			return !has_null_exc_spec;
		} else
			return TRUE;
	} else
		return TRUE;
}
	

a_boolean expr_may_throw_exception(an_expr_node_ptr expr, a_boolean hosted)
{
	if (expr->kind == enk_routine_address)
		return routine_may_throw_exception(expr->variant.routine, hosted);
	else if (expr->kind == enk_variable)
		return function_variable_may_throw_exception(expr->variant.variable);
	else
		/* otherwise, don't bother yet trying to figure out */
		return TRUE;
}
