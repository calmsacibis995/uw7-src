/*		copyright	"%c%"	*/

#ident	"@(#)ksh:include/outmsg.h	1.1.1.1"

/*
	file name : outmsg.h
*/
void set_flag(),fclose_();
char *sys_err(),*err_no();
int  scrlen(),erwrite();
char	*sh_gettxt();
#define SIGNAL_M	1
#define SIGNO_M		2
#define BUG_M		3

#define KSH_ERROR	1	/* MM_ERROR		*/
#define KSH_NOSTD	2	/* MM_NOSTD		*/
#define KSH_WARNING	3	/* MM_WARNING		*/
#define KSH_ACTION	4	/* MM_ACTION		*/
#define KSH_INFO	5	/* MM_INFO		*/
#define KSH_CLEAR_N	6	/* clear & MM_NOSTD	*/
#define KSH_UNDEF	-1	/* undefine value	*/

#define SFLAG_E		set_flag(KSH_ERROR)
#define SFLAG_N		set_flag(KSH_NOSTD)
#define SFLAG_W		set_flag(KSH_WARNING)
#define SFLAG_A		set_flag(KSH_ACTION)
#define SFLAG_I		set_flag(KSH_INFO)
#define SFLAG_CN	set_flag(KSH_CLEAR_N)

/*----------------------*
 *  message definition  *
 *----------------------*/
/* message file of libc */
#define ER_EVNTBASE	4	/* base of signal_msg in message list*/
/* message file of uxue */
#define MSG_OUTSTR_F	":67:%s"

#define E_TIMEWARN	":69",e_timewarn	/* from msg.c	*/
#define E_TIMEOUT	":70",e_timeout
#define E_MAILMSG	":71",e_mailmsg
#define E_QUERY		":72",e_query
#define E_HISTORY	":73",e_history
#define E_OPTION	":74",e_option
#define E_SPACE		":75",e_space
#define E_ARGEXP	":76",e_argexp
#define E_BRACKET	":77",e_bracket
#define E_NUMBER	":78",e_number
#define E_NULLSET	":79",e_nullset
#define E_NOTSET	":80",e_notset
#define E_SUBST		":81",e_subst
#define E_CREATE	":82",e_create
#define E_RESTRICTED	":83",e_restricted
#define E_FORK		":84",e_fork
#define E_PEXISTS	":85",e_pexists
#define E_FEXISTS	":86",e_fexists
#define E_SWAP		":87",e_swap
#define E_PIPE		":88",e_pipe
#define E_OPEN		":89",e_open
#define E_LOGOUT	":90",e_logout
#define E_ARGLIST	":91",e_arglist
#define E_TXTBSY	":92",e_txtbsy
#define E_TOOBIG	":93",e_toobig
#define E_EXEC		":94",e_exec
#define E_PWD		":95",e_pwd
#define E_FOUND		":96",e_found
#define E_FLIMIT	":97",e_flimit
#define E_ULIMIT	":98",e_ulimit
#define E_SUBSCRIPT	":99",e_subscript
#define E_NARGS		":100",e_nargs
#define E_LIBACC	":101",e_libacc
#define E_LIBBAD	":102",e_libbad
#define E_LIBSCN	":103",e_libscn
#define E_LIBMAX	":104",e_libmax
#define E_MULTIHOP	":105",e_multihop
#define E_LONGNAME	":106",e_longname
#define E_LINK		":107",e_link
#define E_ACCESS	":108",e_access
#define E_DIRECT	":109",e_direct
#define E_NOTDIR	":110",e_notdir
#define E_FILE		":111",e_file
#define E_TRAP		":112",e_trap
#define E_READONLY	":113",e_readonly
#define E_IDENT		":114",e_ident
#define E_ALINAME	":115",e_aliname
#define E_TESTOP	":116",e_testop
#define E_ALIAS		":117",e_alias
#define E_FUNCTION	":118",e_function
#define E_FORMAT	":119",e_format
#define E_ON		":120",e_on
#define E_OFF		":121",e_off
#define IS_RESERVED	":122",is_reserved
#define IS_BUILTIN	":123",is_builtin
#define IS_ALIAS	":124",is_alias
#define IS_FUNCTION	":125",is_function
#define IS_XALIAS	":126",is_xalias
#define IS_TALIAS	":127",is_talias
#define IS_XFUNCTION	":128",is_xfunction
#define IS_UFUNCTION	":129",is_ufunction
#define IS_		":130",is_
#define E_NEWTTY	":131",e_newtty
#define E_OLDTTY	":132",e_oldtty
#define E_NO_START	":133",e_no_start
#define E_NO_JCTL	":134",e_no_jctl
#define E_TERMINATE	":135",e_terminate
#define S_DONE		":136",e_Done
#define S_RUNNING	":137",e_Running
#define E_AMBIGUOUS	":138",e_ambiguous
#define E_RUNNING	":139",e_running
#define E_NO_JOB	":140",e_no_job
#define E_NO_PROC	":141",e_no_proc
#define E_JOBUSAGE	":142",e_jobusage
#define E_COREDUMP	":143",e_coredump
#define E_ROOTNODE	":144",e_rootnode
#define E_NOVER		":145",e_nover
#define E_BADVER	":146",e_badver
#define E_BADINLIB	":147",e_badinlib
#define E_HEADING	":148",e_heading
#define E_ENDOFFILE	":149",e_endoffile
#define E_UNEXPECTED	":150",e_unexpected
#define E_UNMATCHED	":151",e_unmatched
#define E_UNKNOWN	":152",e_unknown
#define E_ATLINE	":153",e_atline
#define E_PROHIBITED	":154",e_prohibited
#define E_UNLIMITED	":155",e_unlimited
#define E_MORETOKENS	":156",e_moretokens	/* from strdata.c */
#define E_PAREN   	":157",e_paren
#define E_BADCOLON   	":158",e_badcolon
#define E_DIVZERO   	":159",e_divzero
#define E_SYNBAD   	":160",e_synbad
#define E_NOTLVALUE   	":161",e_notlvalue
#define E_RECURSIVE   	":162",e_recursive
#define E_QUESTCOLON   	":163",e_questcolon
#define E_INCOMPATIBLE 	":164",e_incompatible
#define E_UNDEF_SYM	":165","undefined symbols"	/* from builtin.c */
#define E_NGET_MAP	":166","cannot get mapping"
#define E_NGET_VER	":167","cannot get versions"
#define E_NSET_MAP	":168","cannot set mapping"
#define E_NSET_VPH	":169","cannot set vpath"
#define E_INVNAME	":170","invalid name"
#define E_NACCESS	":171","not accessible"
#define E_NEWLINE	":172","newline or ;"
#define E_CURCMD	":173","Current command "	/* from emacs.c */
#define E_RP_LINE	":174"," (line "
#define E_PRVCMD	":175","; Previous command "
#define S_SIGMSG	":176","Signal "	/* from jobs.c */

