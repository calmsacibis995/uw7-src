#ident	"@(#)cvtomf:fltused.c	1.2"

/* Enhanced Application Compatibility Support */

long	_fltused = 0;

asm(".section	.note");
asm(".string	\"Converted OMF object(s), use XENIX semantics!\"");

/* End Enhanced Application Compatibility Support */
