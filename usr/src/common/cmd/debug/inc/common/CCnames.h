#ifndef CCNAMES_H
#define CCNAMES_H
#ident	"@(#)debugger:inc/common/CCnames.h	1.2"

enum CC_name_type
{
	NT_none,
	NT_qualifier,	// class or namespace name
	NT_simple,	// simple function or object name
	NT_function,
	NT_template_params,	// place-holder arguments (e.g., <T1>
	NT_template_types,	// the complete strings -- [with ...]
	NT_function_params,
};

class Symbol;
struct CC_sub_name;

// CC_parse_name takes a C++ function name and breaks it down into the component pieces
class CC_parse_name
{
	char		*name;
	CC_sub_name	*data;
	CC_sub_name	*current;
	CC_name_type	parse_sub_string(char *&);
public:
	CC_parse_name(const char *n);
	CC_parse_name(Symbol &);
	~CC_parse_name();

	int		parse();
	const char	*first(CC_name_type &);
	const char	*next(CC_name_type &);
	const char	*get_function_name();
	const char	*full_name();
};

#endif // CCNAMES_H
