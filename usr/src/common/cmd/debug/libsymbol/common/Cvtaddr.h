#ident	"@(#)debugger:libsymbol/common/Cvtaddr.h	1.1"

// machine specific routines to convert a COFF symbol's value
// into a location description

void cvt_arg( Locdesc &loc, unsigned long value );
void cvt_reg( Locdesc &loc, unsigned long value );
void cvt_auto( Locdesc &loc, unsigned long value );
void cvt_extern( Locdesc &loc, unsigned long value );
