#ident	"@(#)ksh93:src/cmd/ksh93/include/path.h	1.2"
#pragma prototyped
#ifndef PATH_OFFSET

/*
 *	UNIX shell path handling interface
 *	Written by David Korn
 *	These are the definitions for the lexical analyzer
 */

#include	"FEATURE/options"
#include	<nval.h>

#define PATH_OFFSET	2		/* path offset for path_join */
#define MAXDEPTH	(sizeof(char*)==2?64:4096) /* maximum recursion depth*/

struct argnod;

/* pathname handling routines */
extern void 		path_alias(Namval_t*,char*);
extern char 		*path_absolute(const char*,const char*);
extern char 		*path_basename(const char*);
extern int 		path_expand(const char*, struct argnod**);
extern void 		path_exec(const char*,char*[],struct argnod*);
extern int		path_open(const char*,char*);
extern char 		*path_get(const char*);
extern char		*path_join(char*,const char*);
extern char 		*path_pwd(int);
extern int		path_search(const char*,const char*,int);
extern char		*path_relative(const char*);
extern int		path_complete(const char*, const char*,struct argnod**);
#ifdef SHOPT_BRACEPAT
    extern int 		path_generate(struct argnod*,struct argnod**);
#endif /* SHOPT_BRACEPAT */

/* constant strings needed for whence */
extern const char e_real[];
extern const char e_user[];
extern const char e_sys[];
extern const char e_dot[];
extern const char e_pwd[];
extern const char e_pwd_id[];
extern const char e_logout[];
extern const char e_logout_id[];
extern const char e_alphanum[];
extern const char e_mailmsg[];
extern const char e_mailmsg_id[];
extern const char e_suidprofile[];
extern const char e_sysprofile[];
extern const char e_traceprompt[];
extern const char e_crondir[];
#ifdef SHOPT_SUID_EXEC
    extern const char	e_suidexec[];
#endif /* SHOPT_SUID_EXEC */
#ifdef SHOPT_VPIX
    extern const char	e_vpix[];
    extern const char	e_vpixdir[];
#endif /* SHOPT_VPIX */
extern const char is_[];
extern const char is_alias[];
extern const char is_alias_id[];
extern const char is_builtin[];
extern const char is_builtin_id[];
extern const char is_builtver[];
extern const char is_builtver_id[];
extern const char is_reserved[];
extern const char is_reserved_id[];
extern const char is_talias[];
extern const char is_talias_id[];
extern const char is_xalias[];
extern const char is_xalias_id[];
extern const char is_function[];
extern const char is_function_id[];
extern const char is_xfunction[];
extern const char is_xfunction_id[];
extern const char is_ufunction[];
extern const char is_ufunction_id[];
#ifdef SHELLMAGIC
    extern const char e_prohibited[];
    extern const char e_prohibited_id[];
#endif /* SHELLMAGIC */

#ifdef SHOPT_ACCT
#   include	"FEATURE/acct"
#   ifdef	_sys_acct
	extern void sh_accinit(void);
	extern void sh_accbegin(const char*);
	extern void sh_accend(void);
	extern void sh_accsusp(void);
#   else
#	undef	SHOPT_ACCT
#   endif	/* _sys_acct */
#endif /* SHOPT_ACCT */

#endif /*! PATH_OFFSET */
