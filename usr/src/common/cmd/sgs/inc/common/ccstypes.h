#ident	"@(#)sgs-inc:common/ccstypes.h	1.3"

#ifndef _SYS_TYPES_H
typedef	unsigned short uid_t;
typedef unsigned short gid_t;
typedef short pid_t;
typedef unsigned short mode_t;
typedef short nlink_t;
#endif
#ifdef uts
#	ifndef _SIZE_T
#		define _SIZE_T
#		ifndef size_t
#			define size_t	unsigned int
#		endif
#	endif
#endif
