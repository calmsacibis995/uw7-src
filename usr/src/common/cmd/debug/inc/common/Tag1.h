#ident	"@(#)debugger:inc/common/Tag1.h	1.9"
//
// List of all of the tag values known to the debugger.
//

#ifndef DEFTAG
#define DEFTAG(X, Y)
#endif

DEFTAG(		t_none,			"t_none"		)

DEFTAG(		pt_startvars,		"pt_startvars"		)
// variable tags must go after here

DEFTAG(		t_argument,		"t_argument"		)
DEFTAG(		t_variable,		"t_variable"	)
DEFTAG(		t_unionmem,		"t_unionmem"		)
DEFTAG(		t_structuremem,		"t_structuremem"	)

// variable tags must go before here
DEFTAG(		pt_endvars,		"t_endvars"		)

DEFTAG(		t_entry,		"t_entry"		)
DEFTAG(		t_subroutine,		"t_subroutine"		)
DEFTAG(		t_extlabel,		"t_extlabel"		)
DEFTAG(		t_inlined_sub,		"t_inlined_sub"		)

DEFTAG(		pt_starttypes,		"t_starttypes"		)
// type tags must go after here

DEFTAG(		t_basetype,		"t_basetype"		)
DEFTAG(		t_arraytype,		"t_arraytype"		)
DEFTAG(		t_pointertype,		"t_pointertype"		)
DEFTAG(		t_reftype,		"t_reftype"		)
DEFTAG(		t_functiontype,		"t_functiontype"	)
DEFTAG(		t_baseclass_type,	"t_baseclass_type"	)
DEFTAG(		t_classtype,		"t_classtype"		)
DEFTAG(		t_structuretype,	"t_structuretype"	)
DEFTAG(		t_uniontype,		"t_uniontype"		)
DEFTAG(		t_enumtype,		"t_enumtype"		)
DEFTAG(		t_enumlittype,		"t_enumlittype"		)
DEFTAG(		t_functargtype,		"t_functargtype"	)
DEFTAG(		t_discsubrtype,		"t_discsubrtype"	)
DEFTAG(		t_stringtype,		"t_stringtype"		)
DEFTAG(		t_typedef,		"t_typedef"		)
DEFTAG(		t_unspecargs,		"t_unspecargs"		)
DEFTAG(		t_consttype,		"t_consttype"		)
DEFTAG(		t_volatiletype,		"t_volatiletype"	)
DEFTAG(		t_ptr_to_mem_type,	"t_ptr_to_mem_type"	)
DEFTAG(		t_throwntype,		"t_throwntype"		)

// type tags must go before here
DEFTAG(		pt_endtypes,		"t_endtypes"		)

DEFTAG(		t_sourcefile,		"t_sourcefile"		)
DEFTAG(		t_block,		"t_block"		)
DEFTAG(		t_label,		"t_label"		)
DEFTAG(		t_try_block,		"t_try_block"		)
DEFTAG(		t_catch_block,		"t_catch_block"		)
DEFTAG(		t_namespace,		"t_namespace"		)
DEFTAG(		t_namespace_alias,	"t_namespace_alias"	)
DEFTAG(		t_using_directive,	"t_using_directive"	)
DEFTAG(		t_using_declaration,	"t_using_declaration"	)
DEFTAG(		t_template_param,	"t_template_param"	)
