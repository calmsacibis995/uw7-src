#ifndef _PPP_UTIL_H
#define _PPP_UTIL_H

#ident	"@(#)ppp_util.h	1.2"

#define PPPLOG_CMN(fp, level, fmt, ap) \
{ \
	switch(level) { \
	case MSG_WARN: \
		fprintf(fp, "WARNING "); \
		break; \
\
	case MSG_ERROR: \
		fprintf(fp, "ERROR "); \
		break; \
\
	case MSG_ULR:\
		fprintf(fp, "ULR "); \
		break; \
\
	case MSG_FATAL: \
		fprintf(fp, "FATAL ERROR "); \
		break; \
\
	case MSG_AUDIT: \
		fprintf(fp, "AUDIT "); \
		break; \
\
	case MSG_DEBUG: \
		fprintf(fp, "DBG "); \
		break; \
	} \
\
	va_start(ap, fmt); \
	(void)vfprintf(fp, fmt, ap); \
	va_end(ap); \
	fflush(fp); \
\
	switch(level) { \
	case MSG_FATAL: \
		abort(); \
\
	} \
}

#endif /* _PPP_UTIL_H */
