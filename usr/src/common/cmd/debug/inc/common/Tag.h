#ifndef Tag_h
#define Tag_h
#ident	"@(#)debugger:inc/common/Tag.h	1.2"

#undef DEFTAG
#define DEFTAG(VAL, NAME) VAL,

enum Tag {
#include "Tag1.h"
};

#define	IS_ENTRY(x) ((x) == t_subroutine||(x) == t_entry)
#define IS_VARIABLE(x)	((x) == t_variable || (x) == t_argument)

#endif
