#ident	"@(#)debugger:debug_paths.C	1.3"

// gather all of the path-specific information 
// in one place

#if defined(GEMINI_ON_OSR5) || defined(GEMINI_ON_UW)
const char	*debug_ui_path = ALT_PREFIX"/usr/ccs/lib/";
const char	*debug_config_path = ALT_PREFIX"/usr/ccs/lib/debug_config";
const char	*debug_alias_path = ALT_PREFIX"/usr/ccs/lib/debug_alias";
const char	*debug_follow_path = ALT_PREFIX"/usr/ccs/lib/follow";
#else
const char	*debug_ui_path = "/usr/ccs/lib/";
const char	*debug_config_path = "/usr/ccs/lib/debug_config";
const char	*debug_alias_path = "/usr/ccs/lib/debug_alias";
const char	*debug_follow_path = "/usr/ccs/lib/follow";
#endif

