#ifndef _Severity_h
#define _Severity_h
#ident	"@(#)debugger:inc/common/Severity.h	1.1"

// Error message severities

enum Severity
{
	E_NONE = 0,	// not an error message
	E_WARNING,	// warn, but continue command
	E_ERROR,	// error, abort command
	E_FATAL,	// fatal error, exit
};

#endif	// _Severity_h
