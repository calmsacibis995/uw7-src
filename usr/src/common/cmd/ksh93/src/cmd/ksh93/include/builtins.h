#ident	"@(#)ksh93:src/cmd/ksh93/include/builtins.h	1.1.1.3"
#pragma prototyped

#ifndef SYSDECLARE

#include	<option.h>
#include	"FEATURE/options"
#include	"FEATURE/dynamic"
#include	"shtable.h"

#define	SYSLOGIN	(sh.bltin_cmds)
#define SYSEXEC		(sh.bltin_cmds+1)
#define SYSSET		(sh.bltin_cmds+2)
#define SYSTRUE		(sh.bltin_cmds+4)
#define SYSCOMMAND	(sh.bltin_cmds+5)
#define SYSCD		(sh.bltin_cmds+6)
#define SYSBREAK	(sh.bltin_cmds+7)
#define SYSCONT		(sh.bltin_cmds+8)
#define SYSTYPESET	(sh.bltin_cmds+9)
#define SYSTEST		(sh.bltin_cmds+10)
#define SYSBRACKET	(sh.bltin_cmds+11)

/* entry point for shell special builtins */
extern int b_alias(int, char*[],void*);
extern int b_brk_cont(int, char*[],void*);
extern int b_dot_cmd(int, char*[],void*);
extern int b_exec(int, char*[],void*);
extern int b_eval(int, char*[],void*);
extern int b_ret_exit(int, char*[],void*);
extern int b_login(int, char*[],void*);
extern int b_true(int, char*[],void*);
extern int b_false(int, char*[],void*);
extern int b_read_export(int, char*[],void*);
extern int b_set(int, char*[],void*);
extern int b_shift(int, char*[],void*);
extern int b_trap(int, char*[],void*);
extern int b_typeset(int, char*[],void*);
extern int b_unset(int, char*[],void*);
extern int b_unalias(int, char*[],void*);

/* The following are for job control */
#if defined(SIGCLD) || defined(SIGCHLD)
    extern int b_jobs(int, char*[],void*);
    extern int b_kill(int, char*[],void*);
#   ifdef SIGTSTP
	extern int b_bg_fg(int, char*[],void*);
#   endif	/* SIGTSTP */
#endif

/* The following utilities are built-in because of side-effects */
extern int b_builtin(int, char*[],void*);
extern int b_cd(int, char*[],void*);
extern int b_command(int, char*[],void*);
extern int b_getopts(int, char*[],void*);
extern int b_hist(int, char*[],void*);
extern int b_let(int, char*[],void*);
extern int b_read(int, char*[],void*);
extern int b_ulimit(int, char*[],void*);
extern int b_umask(int, char*[],void*);
#ifdef _cmd_universe
    extern int b_universe(int, char*[],void*);
#endif /* _cmd_universe */
#ifdef SHOPT_FS_3D
    extern int b_vpath_map(int, char*[],void*);
#endif /* SHOPT_FS_3D */
extern int b_wait(int, char*[],void*);
extern int b_whence(int, char*[],void*);

extern int b_alarm(int, char*[],void*);
extern int b_print(int, char*[],void*);
extern int b_printf(int, char*[],void*);
extern int b_pwd(int, char*[],void*);
extern int b_sleep(int, char*[],void*);
extern int b_test(int, char*[],void*);
#ifndef SHOPT_ECHOPRINT
    extern int b_echo(int, char*[],void*);
#endif /* SHOPT_ECHOPRINT */
/* The following utilities need not be built-ins, but improve performance */
extern int b_cut(int, char*[],void*);
extern int b_dirname(int, char*[],void*);
extern int b_getconf(int, char*[],void*);
extern int b_head(int, char*[],void*);
extern int b_id(int, char*[],void*);
extern int b_logname(int, char*[],void*);
extern int b_tail(int, char*[],void*);
extern int b_tty(int, char*[],void*);

/* The following are extensions for apollo computers */
#ifdef	 apollo
    extern int b_rootnode(int, char*[],void*);
    extern int b_inlib(int, char*[],void*);
    extern int b_ver(int, char*[],void*);
#endif	/* apollo */

extern const char	e_alrm1[];
extern const char	e_alrm1_id[];
extern const char	e_alrm2[];
extern const char	e_alrm2_id[];
extern const char	e_badfun[];
extern const char	e_badfun_id[];
extern const char	e_baddisc[];
extern const char	e_baddisc_id[];
extern const char	e_nofork[];
extern const char	e_nofork_id[];
extern const char	e_nosignal[];
extern const char	e_nosignal_id[];
extern const char	e_nolabels[];
extern const char	e_nolabels_id[];
extern const char	e_notimp[];
extern const char	e_notimp_id[];
extern const char	e_nosupport[];
extern const char	e_nosupport_id[];
extern const char	e_badbase[];
extern const char	e_badbase_id[];
extern const char	e_overlimit[];
extern const char	e_overlimit_id[];

extern const char	e_needspath[];
extern const char	e_needspath_id[];
extern const char	e_eneedsarg[];
extern const char	e_eneedsarg_id[];
extern const char	e_toodeep[];
extern const char	e_toodeep_id[];
extern const char	e_badconf[];
extern const char	e_badconf_id[];
extern const char	e_badname[];
extern const char	e_badname_id[];
extern const char	e_badwrite[];
extern const char	e_badwrite_id[];
extern const char	e_badsyntax[];
extern const char	e_badsyntax_id[];
#ifdef _cmd_universe
    extern const char	e_nouniverse[];
    extern const char	e_nouniverse_id[];
#endif /* _cmd_universe */
extern const char	e_histopen[];
extern const char	e_histopen_id[];
extern const char	e_condition[];
extern const char	e_condition_id[];
extern const char	e_badrange[];
extern const char	e_badrange_id[];
extern const char	e_numeric[];
extern const char	e_trap[];
extern const char	e_trap_id[];
extern const char	e_direct[];
extern const char	e_direct_id[];
extern const char	e_defedit[];
extern const char	e_cneedsarg[];
extern const char	e_cneedsarg_id[];
#ifdef SHOPT_FS_3D
    extern const char	e_cantsetmap[];
    extern const char	e_cantsetmap_id[];
    extern const char	e_cantsetver[];
    extern const char	e_cantsetver_id[];
    extern const char	e_cantgetmap[];
    extern const char	e_cantgetmap_id[];
    extern const char	e_cantgetver[];
    extern const char	e_cantgetver_id[];
#endif /* SHOPT_FS_3D */
#ifdef apollo
    extern const char	e_rootnode[];
    extern const char	e_rootnode_id[];
    extern const char	e_nover[];
    extern const char	e_nover_id[];
    extern const char	e_badver[];
    extern const char	e_badver_id[];
    extern const char	e_badinlib[];
    extern const char	e_badinlib_id[];
#endif /* apollo */

/* for option parsing */
extern const char sh_optalarm[];
extern const char sh_optalarm_id[];
extern const char sh_optalias[];
extern const char sh_optalias_id[];
extern const char sh_optbuiltin[];
extern const char sh_optbuiltin_id[];
extern const char sh_optcd[];
extern const char sh_optcd_id[];
extern const char sh_optcflow[];
extern const char sh_optcommand[];
extern const char sh_optcommand_id[];
extern const char sh_optdot[];
extern const char sh_optdot_id[];
#ifndef ECHOPRINT
    extern const char sh_optecho[];
    extern const char sh_optecho_id[];
#endif /* !ECHOPRINT */
extern const char sh_opteval[];
extern const char sh_opteval_id[];
extern const char sh_optexec[];
extern const char sh_optexec_id[];
extern const char sh_optexport[];
extern const char sh_optexport_id[];
extern const char sh_optgetopts[];
extern const char sh_optgetopts_id[];
extern const char sh_optgetconf[];
extern const char sh_optgetconf_id[];
extern const char sh_optjoblist[];
extern const char sh_optjoblist_id[];
extern const char sh_opthist[];
extern const char sh_opthist_id[];
extern const char sh_optjobs[];
extern const char sh_optjobs_id[];
extern const char sh_optkill[];
extern const char sh_optkill_id[];
extern const char sh_optksh[];
extern const char sh_optksh_id[];
extern const char sh_optlet[];
extern const char sh_optlet_id[];
extern const char sh_optprint[];
extern const char sh_optprint_id[];
extern const char sh_optprintf[];
extern const char sh_optprintf_id[];
extern const char sh_optpwd[];
extern const char sh_optread[];
extern const char sh_optread_id[];
extern const char sh_optset[];
extern const char sh_optset_id[];
extern const char sh_optsleep[];
extern const char sh_optsleep_id[];
extern const char sh_opttrap[];
extern const char sh_opttrap_id[];
extern const char sh_opttypeset[];
extern const char sh_opttypeset_id[];
extern const char sh_optulimit[];
extern const char sh_optulimit_id[];
extern const char sh_optumask[];
extern const char sh_optumask_id[];
extern const char sh_optunalias[];
extern const char sh_optunalias_id[];
#ifdef _cmd_universe
    extern const char sh_optuniverse[];
    extern const char sh_optuniverse_id[];
#endif /* _cmd_universe */
extern const char sh_optunset[];
extern const char sh_optunset_id[];
#ifdef SHOPT_FS_3D
    extern const char sh_optvpath[];
    extern const char sh_optvpath_id[];
    extern const char sh_optvmap[];
    extern const char sh_optvmap_id[];
#endif /* SHOPT_FS_3D */
extern const char sh_optwhence[];
extern const char sh_optwhence_id[];
#endif /* SYSDECLARE */
