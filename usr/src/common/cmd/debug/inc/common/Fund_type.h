#ifndef Fund_type_h
#define Fund_type_h
#ident	"@(#)debugger:inc/common/Fund_type.h	1.3"

// These must be ordered to match the fundamental type
// codes from DWARF Version 1

enum Fund_type {
	ft_none,	// not a type representation
	ft_char,	// generic character
	ft_schar,	// signed character
	ft_uchar,	// unsigned character
	ft_short,	// generic short
	ft_sshort,	// signed short
	ft_ushort,	// unsigned short
	ft_int,		// generic integer
	ft_sint,	// signed integer
	ft_uint,	// unsigned integer
	ft_long,	// generic long
	ft_slong,	// signed long
	ft_ulong,	// unsigned long
	ft_pointer,	// untyped pointer, void *
	ft_sfloat,	// short float
	ft_lfloat,	// long float (double)
	ft_xfloat,	// extra long float (long double)
	ft_scomplex,	// Fortran complex
	ft_lcomplex,	// Fortran double precision complex
	ft_unused,	// historical leftover
	ft_void,	// C void
	ft_boolean,
	ft_xcomplex, // Fortran extended precision complex
	ft_label, 
#if LONG_LONG
	ft_long_long,	// generic long long
	ft_slong_long,	// signed long long
	ft_ulong_long,	// unsigned long long
#endif
	ft_string,	// string constant (internal)
};

#endif /* Fund_type_h */
