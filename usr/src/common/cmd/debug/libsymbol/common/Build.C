#ident	"@(#)debugger:libsymbol/common/Build.C	1.9"

#include "Build.h"
#include "Protorec.h"
#include "str.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
#if OLD_DEMANGLER
#define demangle elf_demangle
extern char *demangle(const char *);
#else
#include "dem.h"
#endif
#ifdef __cplusplus
}
#endif

char *
demangle_name(const char *name)
{
	offset_t	needed;

#if OLD_DEMANGLER
	char *demangled_name =  demangle(name);
	if (demangled_name == (char *)-1 // actually an internal error
		|| demangled_name == name)
	{
		return 0;
	}
	else
		needed = strlen(demangled_name) + 1;
#else
	static	char	*demangled_name;
	static	offset_t	name_len = 100;

	if (!demangled_name)
		demangled_name = new char[name_len];

	// demangler passes back size needed for output string
	// if there is not enough space allocated, reallocate and
	// call the demangler again to get the full string
	needed = (offset_t)demangle(name, demangled_name, name_len);
	if (needed != (offset_t)-1 && needed > name_len)
	{
		delete demangled_name;
		// needed includes the null byte
		demangled_name = new char[needed];
		name_len = needed;
		needed = demangle(name, demangled_name, name_len);
	}
	if (needed == (size_t)-1
		|| strcmp(name, demangled_name) == 0)
	{
		return 0;
	}
#endif
	// have a mangled name
	return str(demangled_name);
}

// COFF, DWARF, ELF common routine to handle names that might
// be mangled C++ names.
void
Build::buildNameAttrs(Protorec& protorec, const char* name)
{
	char *demangledNm;

	if ((demangledNm = demangle_name(name)) == 0)
	{
		protorec.add_attr(an_name, af_stringndx, (void *)name);
	}
	else // have a mangled name
	{
		protorec.add_attr(an_name, af_stringndx, demangledNm);
		protorec.add_attr(an_mangledname, af_stringndx, (void *)name);
	}
}

Attribute *
Build::find_record(offset_t)
{
	return 0;
}
