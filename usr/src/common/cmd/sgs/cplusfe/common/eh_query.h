#ident	"@(#)cplusfe:common/eh_query.h	1.1"

/*
    routines used to analyze exception usage; useful to both front and back ends
*/
 
#ifndef EH_QUERY_H
#define EH_QUERY_H

#include "host_envir.h"
#include "il.h"

a_boolean routine_has_null_exception_specification(a_routine_ptr routine);

a_boolean routine_may_throw_exception(a_routine_ptr routine, a_boolean hosted);

a_boolean expr_may_throw_exception(an_expr_node_ptr expr, a_boolean hosted);

#endif
