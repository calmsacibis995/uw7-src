/*		copyright	"%c%"	*/

#ident	"@(#)csh:common/cmd/csh/sh.msg.h	1.1.1.3"

/*
	file name : sh.msg.h
*/
void set_flag(),erwrite();
char *sys_err(),*err_no(),*sh_gettxt();
char *gettxt();
int  scrlen();

#define F_REASON	21	/* screen length of reason	*/

#define SIGNAL_M	1
#define EXIT_M		2
#define BUG_M		3

#define CSH_ERROR	1	/* MM_ERROR		*/
#define CSH_NOSTD	2	/* MM_NOSTD		*/
#define CSH_WARNING	3	/* MM_WARNING		*/
#define CSH_ACTION	4	/* MM_ACTION		*/
#define CSH_INFO	5	/* MM_INFO		*/
#define CSH_CLEAR_N	6	/* clear & MM_NOSTD	*/
#define CSH_UNDEF	-1	/* undefine value	*/

#define SFLAG_E		set_flag(CSH_ERROR)
#define SFLAG_N		set_flag(CSH_NOSTD)
#define SFLAG_W		set_flag(CSH_WARNING)
#define SFLAG_A		set_flag(CSH_ACTION)
#define SFLAG_I		set_flag(CSH_INFO)
#define SFLAG_CN	set_flag(CSH_CLEAR_N)

/*----------------------*
 *  message definition  *
 *----------------------*/
/* message file of libc */
#define ER_EVNTBASE	4	/* base of signal_msg in message list*/
/* message file of uxue */
#define MSG_OUTSTR_F	 ":177:%s"
#define MSG_MALLOC_ST	 ":178","Memory allocation statistics %s\nfree"
#define MSG_USED	 ":179","\nused"
#define MSG_TTLUSE	 ":180","Total in use %d, total free %d\n"
#define ER_GETWD_NSTS	 ":181","getwd: cannot stat /"
#define ER_GETWD_NSTD	 ":182","getwd: cannot stat ."
#define ER_GETWD_OPN	 ":183","getwd: cannot open .."
#define ER_GETWD_RED	 ":184","getwd: read error in .."
#define ER_NJOB_CNTL	 ":185","Warning no access to tty; thus no job control in this shell...\n"
#define MSG_LOGOUT	 ":186","logout\n"
#define ER_INTERRUPT	 ":187","Interrupted"
#define ER_FEW_ARG	 ":188","Too few arguments"
#define MSG_HAVE_MAIL	 ":189","You have %smail.\n"
#define MSG_NEW		 ":190","new"
#define MSG_NEW_MAIL	 ":191","New mail"
#define MSG_MAIL	 ":192","Mail"
#define MSG_ARIV_MAIL	 ":193","%s in %t.\n"
#define USG_DIRS	 ":194","Usage dirs [ -l ]"
#define ER_NO_HOME	 ":195","No home directory"
#define ER_NCHG_HOME	 ":196","Cannot change to home directory"
#define ER_NO_DIR	 ":197","No other directory"
#define ER_NDEEP_DSTK	 ":198","Directory stack not that deep"
#define ER_INV_ARG	 ":199","Invalid argument"
#define ER_EMP_DSTK	 ":200","Directory stack empty"
#define ER_AMBIGOUS	 ":201","Ambiguous"
#define ER_UNMATCH_C	 ":202","Unmatched %c"
#define ER_LONG_WORD	 ":203","Word too long"
#define ER_LONG_LINE	 ":204","$< line too long"
#define ER_NO_FILE	 ":205","No file for $0"
#define ER_VAR_SYNTAX	 ":206","Variable syntax"
#define ER_BAD_MODF	 ":207","Bad: mod in $"
#define ER_NO_TERM	 ":208","<< terminator not found"
#define ER_OVF_LINE	 ":209","Line overflow"
#define ER_NO_MATCH	 ":210","No match"
#define ER_NO_EXEC	 ":211","%s cannot execute"
#define ER_NFND_CMD	 ":212","Command not found"
#define ER_NEXE_BIN	 ":213","Cannot execute binary file.\n"
#define MSG_HASH_STATIC	 ":214","%d hits, %d misses, %d%%\n"
#define ER_DIV_0	 ":215","Divide by 0"
#define ER_MOD_0	 ":216","Mod by 0"
#define ER_EXP_SYNTAX	 ":217","Expression syntax"
#define ER_MIS_MPARN	 ":218","Missing }"
#define ER_MALFORM_FILE	 ":219","Malformed file inquiry"
#define ER_MIS_FNAME	 ":220","Missing file name"
#define ER_TOO_MANY	 ":221","Yikes!! Too many %s!!\n"
#define ER_NM_PASSWD	 ":222","names in password file"
#define ER_NM_FILE	 ":223","files"
#define ER_MANY_ARGS	 ":224","Too many arguments"
#define ER_NUSE_TERM	 ":225","Cannot from terminal"
#define ER_DANG_ALIAS	 ":226","Too dangerous to alias that"
#define ER_NLOGIN_SHELL	 ":227","Not login shell"
#define ER_EMP_IF	 ":228","Empty if"
#define ER_IMP_THEN	 ":229","Improper then"
#define ER_NIN_WHILE	 ":230","Not in while/foreach"
#define ER_INV_VAR	 ":231","Invalid variable"
#define ER_NPARN_WORD	 ":232","Words not ()'ed"
#define ER_NFND_THEN	 ":233","then/endif not found"
#define ER_NFND_ENDIF	 ":234","endif not found"
#define ER_NFND_ENDSW	 ":235","endsw not found"
#define ER_NFND_END	 ":236","end not found"
#define ER_NFND_LABEL	 ":237","label not found"
#define ER_INV_LOCALE	 ":238","%t Invalid locale --- ignored"
#define ER_IMP_MASK	 ":239","Improper mask"
#define ER_NO_LIMIT	 ":240","No such limit"
#define ER_IMP_SCALE	 ":241","Improper or unknown scale factor"
#define ER_BAD_SCALE	 ":242","Bad scaling; did you mean ``%t''?"
#define MSG_UNLIMIT	 ":243","unlimited"
#define ER_NFND_LIMIT	 ":244","%t %t Cannot %s%s limit\n"
#define ER_REMV		 ":245","remove"
#define ER_SET		 ":246","set"
#define ER_HARD		 ":247"," hard"
#define ER_NSUP_SHELL	 ":248","Cannot suspend a login shell (yet)"
#define MSG_SWTCH_NEW	 ":249","Switching to new tty driver...\n"
#define ER_UNKNOWN_U	 ":250","Unknown user %t"
#define ER_MIS_LPARN	 ":251","Missing ]"
#define ER_LONG_ARGS	 ":252","Arguments too long"
#define ER_LONG_PATHNM	 ":253","Pathname too long"
#define ER_UNMATCH_Q	 ":254","Unmatched `"
#define ER_MANY_WORD_Q	 ":255","Too many words from ``"
#define ER_UNKNOWN_F	 ":256","Unknown flag -%c\n"
#define USG_HISTORY	 ":257","Usage history [-rh] [# number of events]"
#define ER_UNMATCH	 ":258","Unmatched "
#define ER_OVF_EXPBUF	 ":259","Expansion buf ovflo"
#define ER_BAD_FORM	 ":260","Bad ! form"
#define ER_NPRV_SUB	 ":261","No prev sub"
#define ER_BAD_SUB	 ":262","Bad substitute"
#define ER_NPRV_LHS	 ":263","No prev lhs"
#define ER_LONG_RHS	 ":264","Rhs too long"
#define ER_BAD_MOD	 ":265","Bad ! modifier "
#define ER_FAIL_MOD	 ":266","Modifier failed"
#define ER_OVF_SBBUF	 ":267","Subst buf ovflo"
#define ER_BAD_SEL	 ":268","Bad ! arg selector"
#define ER_NPRV_SRCH	 ":269","No prev search"
#define ER_NFND_EVENT	 ":270",": Event not found"
#define MSG_RSET_PGRP	 ":271","Reset tty pgrp from %d to %d\n"
#define USG_LOGOUT	 ":272","Use \"logout\" to logout.\n"
#define USG_EXIT	 ":273","Use \"exit\" to leave csh.\n"
#define ER_OUT_MEM	 ":274","Out of memory"
#define ER_UNDEF_VAR	 ":275","Undefined variable"
#define ER_LOOP_ALIAS	 ":276","Alias loop"
#define ER_MANY_RPARN	 ":277","Too many )'s"
#define ER_MANY_LPARN	 ":278","Too many ('s"
#define ER_BAD_LPARN	 ":279","Badly placed ("
#define ER_MIS_RNAME	 ":280","Missing name for redirect"
#define ER_AMB_ROUT	 ":281","Ambiguous output redirect"
#define ER_NUSE_RDIRCT	 ":282","Cannot << within ()'s"
#define ER_AMB_RIN	 ":283","Ambiguous input redirect"
#define ER_BAD_LRPARN	 ":284","Badly placed ()'s"
#define ER_INV_NCMD	 ":285","Invalid null command"
#define ER_BUG_WJOB	 ":286","BUG waiting for background job!\n"
#define ER_BUG_FLUSH	 ":287","BUG process flushed twice"
#define MSG_RUN		 ":288","Running "
#define MSG_EXIT	 ":289","Exit %d"
#define MSG_DONE	 ":290","Done"
#define ER_BUG_STTS	 ":291","BUG status=%o"
#define ER_DMP_CORE	 ":292"," (core dumped)"
#define MSG_WORK_DIR	 ":293"," (wd: "
#define MSG_NWORK_DIR	 ":294","(wd now: "
#define USG_JOBS	 ":295","Usage jobs [ -l ]"
#define ER_BAD_SIGNO	 ":296","Bad signal number"
#define ER_UNKNOWN_S	 ":297","Unknown signal; kill -l lists signals"
#define ER_ALD_STOP	 ":298","%t Already stopped\n"
#define ER_ARG_ID	 ":299","Arguments should be jobs or process id's"
#define ER_STOP_JOB	 ":300","There are stopped jobs"
#define ER_NCUR_JOB	 ":301","No current job"
#define ER_NPRV_JOB	 ":302","No previous job"
#define ER_NMAT_JOB	 ":303","No job matches pattern"
#define ER_NSUCH_JOB	 ":304","No such job"
#define ER_FAIL_FORK	 ":305","Fork failed"
#define ER_NCNTL_JOB	 ":306","No job control in this shell"
#define ER_NCNTL_JOBS	 ":307","No job control in subshells"
#define ER_FAIL_VFORK	 ":308","Vfork failed"
#define ER_NMAKE_PIPE	 ":309","Cannot make pipe"
#define ER_EXI_FILE	 ":310","%t File exists"
#define ER_SYNTAX	 ":311","Syntax error"
#define ER_MIS_RPARN	 ":312","Missing )"
#define ER_SUBSCRIPT	 ":313","Subscript error"
#define ER_OUT_SUBS	 ":314","Subscript out of range"
#define ER_BAD_NUMBER	 ":315","Badly formed number"
#define ER_NO_WORD	 ":316","No more words"
#define ER_TRUNC_PATH	 ":317","Warning ridiculously long PATH truncated\n"
