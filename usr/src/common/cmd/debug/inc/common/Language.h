#ifndef Language_h
#define Language_h
#ident	"@(#)debugger:inc/common/Language.h	1.8"

enum Language {
    UnSpec,	// unspecified, use current default
    C,		// debug C exprs.
    CPLUS,	// debug full C++ expressions, C++ lang attribute found in DWARF record
    CPLUS_ASSUMED, // debug C++ subset, C++ assumed from presence of __cpp_version_302
		   // variable generated by cfront
    CPLUS_ASSUMED_V2, // debug C++ subset, assumed from presence of __cpp_unixware_20
		      // variable generated by C-generating back end of ANSI C++ compiler
};

class ProcObj;
class Frame;

Language	current_language(ProcObj *pobj = 0, Frame *f = 0);
		// This routine arbitrates the value between %lang and %db_lang
const char	*language_name(Language);
int	 	set_language(const char *);
		// This routine sets the %lang value.
Language	current_context_language(ProcObj *pobj = 0, Frame *f = 0);
		// This routine gets the %db_lang value.
Language	current_user_language();
		// This routine gets the %lang value.

#endif /*Language_h*/
