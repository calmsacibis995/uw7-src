#ident	"@(#)ksh93:src/cmd/ksh93/data/msg.c	1.4"
#pragma prototyped
/*
 *	UNIX shell
 *	S. R. Bourne
 *	Rewritten by David Korn
 *
 *	AT&T Bell Laboratories
 *
 */

#include	<ast.h>
#include	<errno.h>
#include	"defs.h"
#include	"path.h"
#include	"io.h"
#include	"shlex.h"
#include	"timeout.h"
#include	"history.h"
#include	"builtins.h"
#include	"jobs.h"
#include	"edit.h"
#include	"national.h"

#ifdef SHOPT_MULTIBYTE
    const char e_version[]	= "\n@(#)Version M-12/28/93e-SCO\0\n";
#else
    const char e_version[]	= "\n@(#)Version 12/28/93e-SCO\0\n";
#endif /* SHOPT_MULTIBYTE */

/* error messages */
const char e_timewarn[]		= "\r\n\ashell will timeout in 60 seconds due to inactivity";
const char e_timewarn_id[]	= ":1";
const char e_runvi[]		= "\\hist -e \"${VISUAL:-${EDITOR:-vi}}\" ";
const char e_timeout[]		= "timed out waiting for input";
const char e_timeout_id[]	= ":2";
const char e_mailmsg[]		= "you have mail in $_";
const char e_mailmsg_id[]	= ":3";
const char e_query[]		= "no query process";
const char e_query_id[]		= ":4";
const char e_history[]		= "no history file";
const char e_history_id[]	= ":5";
const char e_histopen[]		= "history file cannot open";
const char e_histopen_id[]	= ":6";
const char e_option[]		= "%s: bad option(s)";
const char e_option_id[]	= "ksh93:7";
const char e_toomany[]		= "open file limit exceeded";
const char e_toomany_id[]	= ":8";
const char e_argexp[]		= "argument expected"; /* ingrid: NOT USED */
const char e_argexp_id[]		= ":9";
const char e_argtype[]		= "invalid argument of type %c";
const char e_argtype_id[]	= ":10";
const char e_formspec[]		= "%c: unknown format specifier";
const char e_formspec_id[]	= ":11";
const char e_badregexp[]	= "%s: invalid regular expression";
const char e_badregexp_id[]	= ":12";
const char e_number[]		= "%s: bad number";
const char e_number_id[]	= ":13";
const char e_badlocale[]	= "%s: unknown locale";
const char e_badlocale_id[]	= ":14";
const char e_nullset[]		= "%s: parameter null or not set";
const char e_nullset_id[]	= ":15";
const char e_notset[]		= "%s: parameter not set";
const char e_notset_id[]	= ":16";
const char e_notset2[]		= "parameter not set";
const char e_notset2_id[]	= ":17";
const char e_noparent[]		= "%s: no parent";
const char e_noparent_id[]	= ":18";
const char e_subst[]		= "%s: bad substitution";
const char e_subst_id[]		= ":19";
const char e_create[]		= "%s: cannot create";
const char e_create_id[]	= "ksh93:20";
const char e_tmpcreate[]	= "cannot create tempory file";
const char e_tmpcreate_id[]	= "ksh93:21";
const char e_restricted[]	= "%s: restricted";
const char e_restricted_id[]	= ":22";
const char e_pexists[]		= "process already exists";
const char e_pexists_id[]	= ":23";
const char e_exists[]		= "%s: file already exists";
const char e_exists_id[]	= ":24";
const char e_pipe[]		= "cannot create pipe";
const char e_pipe_id[]		= ":25";
const char e_alarm[]		= "cannot set alarm";
const char e_alarm_id[]		= ":26";
const char e_open[]		= "%s: cannot open";
const char e_open_id[]		= ":27";
const char e_logout[]		= "Use 'exit' to terminate this shell";
const char e_logout_id[]	= ":28";
const char e_exec[]		= "%s: cannot execute";
const char e_exec_id[]		= ":29";
const char e_pwd[]		= "cannot access parent directories";
const char e_pwd_id[]		= ":30";
const char e_found[]		= "%s: not found";
const char e_found_id[]		= ":31";
const char e_found2[]		= "function not found";
const char e_found2_id[]	= ":32";
const char e_subscript[]	= "%s: subscript out of range";
const char e_subscript_id[]	= ":33";
const char e_toodeep[]		= "%s: recursion too deep";
const char e_toodeep_id[]	= ":34";
const char e_access[]		= "permission denied";
const char e_access_id[]	= ":35";
#ifdef _cmd_universe
    const char e_nouniverse[]	= "universe not accessible";
    const char e_nouniverse_id[]	= ":36";
#endif /* _cmd_universe */
const char e_direct[]		= "bad directory";
const char e_direct_id[]	= ":37";
const char e_file[]		= "%s: bad file unit number";
const char e_file_id[]		= ":38";
const char e_file2[]		= "bad file unit number";
const char e_file2_id[]		= ":39";
const char e_trap[]		= "%s: bad trap";
const char e_trap_id[]		= ":40";
const char e_readonly[]		= "%s: is read only";
const char e_readonly_id[]	= ":41";
const char e_badfield[]		= "%d: negative field size";
const char e_badfield_id[]	= ":42";
const char e_ident[]		= "%s: is not an identifier";
const char e_ident_id[]		= ":43";
const char e_badname[]		= "%s: invalid name";
const char e_badname_id[]	= ":44";
const char e_varname[]		= "%s: invalid variable name";
const char e_varname_id[]	= ":45";
const char e_funname[]		= "%s: invalid function name";
const char e_funname_id[]	= ":46";
const char e_aliname[]		= "%s: invalid alias name";
const char e_aliname_id[]	= ":47";
const char e_badexport[]	= "%s: invalid export name";
const char e_badexport_id[]	= ":48";
const char e_badref[]		= "%s: reference variable cannot be an array";
const char e_badref_id[]	= ":49";
const char e_noref[]		= "%s: no reference name";
const char e_noref_id[]		= ":50";
const char e_selfref[]		= "%s: invalid self reference";
const char e_selfref_id[]	= ":51";
const char e_noalias[]		= "%s: alias not found\n";
const char e_noalias_id[]	= ":52";
const char e_format[]		= "%s: bad format";
const char e_format_id[]	= ":53";
const char e_nolabels[]		= "%s: label not implemented";
const char e_nolabels_id[]	= ":54";
const char e_notimp[]		= "%s: not implemented";
const char e_notimp_id[]	= ":55";
const char e_nosupport[]	= "not supported";
const char e_nosupport_id[]	= ":56";
const char e_badrange[]		= "%d-%d: invalid range";
const char e_badrange_id[]	= ":57";
const char e_needspath[]	= "%s: requires pathname argument"; /* ingrid: NOT USED */
const char e_needspath_id[]	= ":58";
const char e_eneedsarg[]	= "-e - requires single argument";
const char e_eneedsarg_id[]	= ":59";
const char e_badconf[]		= "%s: fails %s"; /* ingrid: NOT USED */
const char e_badconf_id[]	= ":60";
const char e_badbase[]		= "%s unknown base";
const char e_badbase_id[]	= ":61";
const char e_loop[]		= "%s: would cause loop";
const char e_loop_id[]		= ":62";
const char e_overlimit[]	= "%s: limit exceeded";
const char e_overlimit_id[]	= ":63";
const char e_badsyntax[]	= "incorrect syntax";
const char e_badsyntax_id[]	= ":64";
const char e_badwrite[]		= "write to %d failed";
const char e_badwrite_id[]	= ":65";
const char e_on	[]		= "on";
const char e_on_id[]		= ":66";
const char e_off[]		= "off";
const char e_off_id[]		= ":67";
const char is_reserved[]	= "%s is a keyword\n";
const char is_reserved_id[]	= ":68";
const char is_builtin[]		= "%s is a shell builtin\n";
const char is_builtin_id[]	= ":69";
const char is_builtver[]	= "%s is a shell builtin version of %s\n";
const char is_builtver_id[]	= ":70";
const char is_alias[]		= "%s is an alias for %s\n";
const char is_alias_id[]	= ":71";
const char is_xalias[]		= "%s is an exported alias for %s\n";
const char is_xalias_id[]	= ":72";
const char is_talias[]		= "%s is a tracked alias for %s\n";
const char is_talias_id[]	= ":73";
const char is_function[]	= "%s is a function\n";
const char is_function_id[]	= ":74";
const char is_xfunction[]	= "%s is an exported function\n";
const char is_xfunction_id[]	= ":75";
const char is_ufunction[]	= "%s is an undefined function\n";
const char is_ufunction_id[]	= ":76";
#ifdef JOBS
#   ifdef SIGTSTP
	const char e_oldtty[]	= "Reverting to old tty driver...";
	const char e_oldtty_id[]	= ":77";
	const char e_newtty[]	= "Switching to new tty driver...";
	const char e_newtty_id[]	= ":78";
	const char e_no_start[]	= "Cannot start job control";
	const char e_no_start_id[]	= ":79";
#   endif /*SIGTSTP */
    const char e_no_jctl[]	= "No job control";
    const char e_no_jctl_id[]	= ":80";
    const char e_terminate[]	= "You have stopped jobs";
    const char e_terminate_id[]	= ":81";
    const char e_done[]		= " Done";
    const char e_done_id[]	= ":82";
    const char e_nlspace[]	= "\n      ";
    const char e_running[]	= " Running";
    const char e_running_id[]	= ":83";
    const char e_ambiguous[]	= "%s: Ambiguous";
    const char e_ambiguous_id[]	= ":84";
    const char e_jobsrunning[]	= "You have running jobs";
    const char e_jobsrunning_id[]	= ":85";
    const char e_no_job[]	= "no such job";
    const char e_no_job_id[]	= ":86";
    const char e_no_proc[]	= "no such process";
    const char e_no_proc_id[]	= ":87";
    const char e_jobusage[]	= "%s: Arguments must be %%job or process ids";
    const char e_jobusage_id[]	= ":88";
    const char e_kill[]		= "kill"; /* ingrid: NOT USED */
    const char e_kill_id[]	= ":89";
#endif /* JOBS */
const char e_coredump[]		= "(coredump)";
const char e_coredump_id[]	= ":90";
const char e_alphanum[]		= "[_[:alpha:]]*([_[:alnum:]])";
#ifdef SHOPT_VPIX
    const char e_vpix[]		= "/vpix";
    const char e_vpixdir[]	= "/usr/bin";
#endif /* SHOPT_VPIX */
const char e_devfdNN[]		= "/dev/fd/+([0-9])";
#ifdef apollo
    const char e_rootnode[]	= "Bad root node specification";
    const char e_rootnode_id[]	= ":91";
    const char e_nover[]	= "Version not defined";
    const char e_nover_id[]	= ":92";
    const char e_badver[]	= "Unrecognized version";
    const char e_badver_id[]	= ":93";
    const char e_badinlib[]	= "???" /* ingrid: missing definition */
    const char e_badinlib_id[]	= ":94";
#endif /* apollo */
#ifdef SHOPT_FS_3D
    const char e_cantgetmap[]	= "cannot get mapping";
    const char e_cantgetmap_id[]	= ":95";
    const char e_cantgetver[]	= "cannot get versions";
    const char e_cantgetver_id[]	= ":96";
    const char e_cantsetmap[]	= "cannot set mapping";
    const char e_cantsetmap_id[]	= ":97";
    const char e_cantsetver[]	= "cannot set versions";
    const char e_cantsetver_id[]	= ":98";
#endif /* SHOPT_FS_3D */

/* string constants */
const char e_heading[]		= "Current option settings\n";
const char e_heading_id[]	= ":99";
const char e_sptbnl[]		= " \t\n";
const char e_defpath[]		= "/bin:/usr/bin:";
const char e_defedit[]		= "/bin/ed";
const char e_unknown[]		= "<command unknown>"; /* ingrid: NOT USED */
const char e_unknown_id[]	= ":100";
const char e_devnull[]		= "/dev/null";
const char e_traceprompt[]	= "+ ";
const char e_supprompt[]	= "# ";
const char e_stdprompt[]	= "$ ";
const char e_profile[]		= "${HOME:-.}/.profile";
const char e_sysprofile[]	= "/etc/profile";
const char e_suidprofile[]	= "/etc/suid_profile";
const char e_crondir[]		= "/usr/spool/cron/atjobs";
const char e_prohibited[]	= "login setuid/setgid shells prohibited";
const char e_prohibited_id[]	= ":101";
#ifdef SHOPT_SUID_EXEC
   const char e_suidexec[]	= "/etc/suid_exec";
#endif /* SHOPT_SUID_EXEC */
const char hist_fname[]		= "/.sh_history";
const char e_dot[]		= ".";
const char e_envmarker[]	= "A__z";
const char e_real[]		= "\nreal";
const char e_user[]		= "user";
const char e_sys[]		= "sys";

#ifdef SHOPT_VPIX
   const char *suffix_list[] = { ".com", ".exe", ".bat", "" };
#endif	/* SHOPT_VPIX */

