#ident	"@(#)ksh93:src/cmd/ksh93/data/builtins.c	1.3.1.3"
#pragma prototyped

#define mount	_AST_mount
#include	<shell.h>
#include	<signal.h>
#include	"shtable.h"
#include	"name.h"
#ifdef KSHELL
#   include	"builtins.h"
#   include	"jobs.h"
#   include	"FEATURE/cmds"
#   define	bltin(x)	(b_##x)
#else
#   define bltin(x)	0
#endif

/*
 * The order up through "[" is significant
 */
const struct shtable3 shtab_builtins[] =
{
	"login",	NV_BLTIN|BLT_ENV|BLT_SPC,	bltin(login),
	"exec",		NV_BLTIN|BLT_ENV|BLT_SPC,	bltin(exec),
	"set",		NV_BLTIN|BLT_ENV|BLT_SPC,	bltin(set),	
	":",		NV_BLTIN|BLT_ENV|BLT_SPC,	bltin(true),
	"true",		NV_BLTIN|BLT_ENV,		bltin(true),
	"command",	NV_BLTIN|BLT_ENV|BLT_EXIT,	bltin(command),
	"cd",		NV_BLTIN|BLT_ENV,		bltin(cd),
	"break",	NV_BLTIN|BLT_ENV|BLT_SPC,	bltin(brk_cont),
	"continue",	NV_BLTIN|BLT_ENV|BLT_SPC,	bltin(brk_cont),
	"typeset",	NV_BLTIN|BLT_ENV|BLT_SPC|BLT_DCL,bltin(typeset),
	"test",		NV_BLTIN|BLT_ENV|NV_NOFREE,	bltin(test),
	"[",		NV_BLTIN|BLT_ENV,		bltin(test),
#ifdef _bin_newgrp
	"newgrp",	NV_BLTIN|BLT_ENV|BLT_SPC,	bltin(login),
#endif	/* _bin_newgrp */
	".",		NV_BLTIN|BLT_ENV|BLT_SPC,	bltin(dot_cmd),
	"alias",	NV_BLTIN|BLT_SPC|BLT_DCL,	bltin(alias),
	"hash",		NV_BLTIN|BLT_SPC|BLT_DCL,	bltin(alias),
	"exit",		NV_BLTIN|BLT_ENV|BLT_SPC,	bltin(ret_exit),
	"export",	NV_BLTIN|BLT_SPC|BLT_DCL,	bltin(read_export),
	"eval",		NV_BLTIN|BLT_ENV|BLT_SPC|BLT_EXIT,bltin(eval),
	"fc",		NV_BLTIN|BLT_ENV|BLT_EXIT,	bltin(hist),
	"hist",		NV_BLTIN|BLT_ENV|BLT_EXIT,	bltin(hist),
	"readonly",	NV_BLTIN|BLT_ENV|BLT_SPC|BLT_DCL,bltin(read_export),
	"return",	NV_BLTIN|BLT_ENV|BLT_SPC,	bltin(ret_exit),
	"shift",	NV_BLTIN|BLT_ENV|BLT_SPC,	bltin(shift),
	"trap",		NV_BLTIN|BLT_ENV|BLT_SPC,	bltin(trap),
	"unalias",	NV_BLTIN|BLT_ENV|BLT_SPC,	bltin(unalias),
	"unset",	NV_BLTIN|BLT_ENV|BLT_SPC,	bltin(unset),
	"builtin",	NV_BLTIN,			bltin(builtin),
#ifdef SHOPT_ECHOPRINT
	"echo",		NV_BLTIN|BLT_ENV,		bltin(print),
#else
	"echo",		NV_BLTIN|BLT_ENV,		bltin(echo),
#endif /* SHOPT_ECHOPRINT */
#ifdef JOBS
#   ifdef SIGTSTP
	"bg",		NV_BLTIN|BLT_ENV,		bltin(bg_fg),
	"fg",		NV_BLTIN|BLT_ENV|BLT_EXIT,	bltin(bg_fg),
	"disown",	NV_BLTIN|BLT_ENV,		bltin(bg_fg),
	"kill",		NV_BLTIN|BLT_ENV|NV_NOFREE,	bltin(kill),
#   else
	"/bin/kill",	NV_BLTIN|BLT_ENV|NV_NOFREE,	bltin(kill),
#   endif	/* SIGTSTP */
	"jobs",		NV_BLTIN|BLT_ENV,		bltin(jobs),
#endif	/* JOBS */
	"false",	NV_BLTIN|BLT_ENV,		bltin(false),
	"getconf",	NV_BLTIN|BLT_ENV,		bltin(getconf),
	"getopts",	NV_BLTIN|BLT_ENV,		bltin(getopts),
	"let",		NV_BLTIN|BLT_ENV,		bltin(let),
	"print",	NV_BLTIN|BLT_ENV,		bltin(print),
	"printf",	NV_BLTIN|NV_NOFREE,		bltin(printf),
	"pwd",		NV_BLTIN|NV_NOFREE,		bltin(pwd),
	"read",		NV_BLTIN,			bltin(read),
	"sleep",	NV_BLTIN|NV_NOFREE,		bltin(sleep),
	"alarm",	NV_BLTIN,			bltin(alarm),
	"ulimit",	NV_BLTIN|BLT_ENV,		bltin(ulimit),
	"umask",	NV_BLTIN|BLT_ENV,		bltin(umask),
#ifdef _cmd_universe
	"universe",	NV_BLTIN|BLT_ENV,		bltin(universe),
#endif /* _cmd_universe */
#ifdef SHOPT_FS_3D
	"vpath",	NV_BLTIN|BLT_ENV,		bltin(vpath_map),
	"vmap",		NV_BLTIN|BLT_ENV,		bltin(vpath_map),
#endif /* SHOPT_FS_3D */
	"wait",		NV_BLTIN|BLT_ENV|BLT_EXIT,	bltin(wait),
	"type",		NV_BLTIN|BLT_ENV,		bltin(whence),
	"whence",	NV_BLTIN|BLT_ENV,		bltin(whence),
#ifdef apollo
	"inlib",	NV_BLTIN|BLT_ENV,		bltin(inlib),
	"rootnode",	NV_BLTIN,			bltin(rootnode),
	"ver",		NV_BLTIN,			bltin(ver),
#endif	/* apollo */
	"/bin/dirname",	NV_BLTIN|NV_NOFREE,		bltin(dirname),
	"/bin/head",	NV_BLTIN|NV_NOFREE,		bltin(head),
#if defined(_usr_bin_logname)  && !defined(_bin_logname)
	"/usr/bin/logname",	NV_BLTIN|NV_NOFREE,	bltin(logname),
#else
	"/bin/logname",	NV_BLTIN|NV_NOFREE,		bltin(logname),
#endif
#if defined(_usr_bin_cut)  && !defined(_bin_cut)
	"/usr/bin/cut",	NV_BLTIN|NV_NOFREE,		bltin(cut),
#else
	"/bin/cut",	NV_BLTIN|NV_NOFREE,		bltin(cut),
#endif
	"",		0, 0 
};

const char sh_optalarm[]	= "r [varname seconds]";
const char sh_optalarm_id[]	= ":339";
const char sh_optalias[]	= "ptx [name=[value]...]";
const char sh_optalias_id[]	= ":340";
const char sh_optbuiltin[]	= "dsf:[library] [name...]";
const char sh_optbuiltin_id[]	= ":341";
const char sh_optcd[]		= "LP [dir] [change]";
const char sh_optcd_id[]		= ":342";
const char sh_optcflow[]	= " [n]";
const char sh_optcommand[]	= "pvV name [arg]...";
const char sh_optcommand_id[]	= ":343";
const char sh_optdot[]		= " name [arg...]";
const char sh_optdot_id[]		= ":344";
#ifndef ECHOPRINT
    const char sh_optecho[]	= " [-n] [arg...]";
    const char sh_optecho_id[]	= ":345";
#endif /* !ECHOPRINT */
const char sh_opteval[]		= " [arg...]";
const char sh_opteval_id[]		= ":346";
const char sh_optexec[]		= "a:[name]c [command [args...] ]";
const char sh_optexec_id[]		= ":347";
const char sh_optexport[]	= "p [name[=value]...]";
const char sh_optexport_id[]	= ":348";
const char sh_optgetopts[]	= ":a:[name] optstring name [args...]";
const char sh_optgetopts_id[]	= ":349";
const char sh_optgetconf[]	= " [name [pathname] ]";
const char sh_optgetconf_id[]	= ":350";
const char sh_optjoblist[]	= " [job...]";
const char sh_optjoblist_id[]	= ":351";
const char sh_opthist[]		= "e:[editor]lnrsN# [first] [last]";
const char sh_opthist_id[]		= ":352";
const char sh_optjobs[]		= "nlp [job...]";
const char sh_optjobs_id[]		= ":353";
const char sh_optkill[]		= "ln#[signum]s:[signame] job...";
const char sh_optkill_id[]		= ":354";
const char sh_optlet[]		= " expr...";
const char sh_optlet_id[]		= ":355";
const char sh_optprint[]	= "f:[format]enprsu:[filenum] [arg...]";
const char sh_optprint_id[]	= ":356";
const char sh_optprintf[]	= " format [arg...]";
const char sh_optprintf_id[]	= ":357";
const char sh_optpwd[]		= "LP";
const char sh_optread[]		= "Ad:[delim]prsu#[filenum]t#[timeout]n#[nbytes] [name...]";
const char sh_optread_id[]		= ":358";
#ifdef SHOPT_KIA
    const char sh_optksh[]		= "+DircabefhkmnpstuvxCR:[file]o:?[option] [arg...]";
    const char sh_optksh_id[]		= "ksh93:359";
    const char sh_optset[]		= "+abefhkmnpstuvxCR:[file]o:?[option]A:[name] [arg...]";
    const char sh_optset_id[]		= ":360";
#else
    const char sh_optksh[]		= "+DircabefhkmnpstuvxCo:?[option] [arg...]";
    const char sh_optksh_id[]		= ":359";
    const char sh_optset[]		= "+abefhkmnpstuvxCo:?[option]A:[name] [arg...]";
    const char sh_optset_id[]		= ":360";
#endif /* SHOPT_KIA */
const char sh_optsleep[]	= " seconds";
const char sh_optsleep_id[]	= ":361";
const char sh_opttrap[]		= "p [action condition...]";
const char sh_opttrap_id[]		= ":362";
#ifdef SHOPT_OO
    const char sh_opttypeset[]	= "+AC:[name]E#?F#?HL#?R#?Z#?fi#?[base]lnprtux [name=[value]...]";
    const char sh_opttypeset_id[]	= ":363";
#else
    const char sh_opttypeset[]	= "+AE#?F#?HL#?R#?Z#?fi#?[base]lnprtux [name=[value]...]";
    const char sh_opttypeset_id[]	= ":363";
#endif /* SHOPT_OO */
const char sh_optulimit[]	= "HSacdfmnstv [limit]";
const char sh_optulimit_id[]	= ":364";
const char sh_optumask[]	= "S [mask]";
const char sh_optumask_id[]	= ":365";
const char sh_optuniverse[]	= " [name]";
const char sh_optuniverse_id[]	= ":366";
const char sh_optunset[]	= "fnv name...";
const char sh_optunset_id[]	= ":367";
const char sh_optunalias[]	= "a name...";
const char sh_optunalias_id[]	= ":368";
#ifdef SHOPT_FS_3D
    const char sh_optvpath[]	= " [top] [base]";
    const char sh_optvpath_id[]	= ":369";
    const char sh_optvmap[]	= " [dir] [list]";
    const char sh_optvmap_id[]	= ":370";
#endif /* SHOPT_FS_3D */
const char sh_optwhence[]	= "afpv name...";
const char sh_optwhence_id[]	= ":371";



const char e_alrm1[]		= "alarm -r %s +%.3g\n";
const char e_alrm1_id[]		= ":372";
const char e_alrm2[]		= "alarm %s %.3f\n";
const char e_alrm2_id[]		= ":373";
const char e_badfun[]		= "%s: illegal function name";
const char e_badfun_id[]	= ":110";
const char e_baddisc[]		= "%s: invalid discipline function";
const char e_baddisc_id[]	= ":111";
const char e_nospace[]		= "out of memory";
const char e_nospace_id[]	= ":112";
const char e_nofork[]		= "cannot fork";
const char e_nofork_id[]	= ":113";
const char e_nosignal[]		= "%s: unknown signal name";
const char e_nosignal_id[]	= ":114";
const char e_numeric[]		= "*([0-9])?(.)*([0-9])";
const char e_condition[]	= "condition(s) required";
const char e_condition_id[]	= ":115";
const char e_cneedsarg[]	= "-c requires argument";
const char e_cneedsarg_id[]	= "ksh93:116";
