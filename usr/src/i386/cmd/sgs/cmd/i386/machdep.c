#ident	"@(#)sgs-cmd:i386/machdep.c	1.38"

#include	"cc.h"
#include        <unistd.h>

static char	*Acomp_Mach = NULL; /* will hold the -2Tn option, which is
				       passed to acomp	*/

static char	*Mach = NULL;	/* will hold the -i386, -i486, -pentium, or -p6 
				   option, which is passed to optim	*/

static char	*As_Mach = NULL; /* will hold the -t386, -t486 or -tpentium
                                   option, which is passed to as.  Note that
				   as does not need to be told about P6  */

static char	*Acomp_Frame = NULL; /* the frame option to pass to acomp:
					-2F1 for fixed frame, else nothing */
#define ACOMP_FIXED_FRAME_OPTION "-2F1"	/* used more than once */

static char	*Frame = NULL;  /* the frame option to pass to optim:
					-_e for regular frame, else nothing */

static int	Frame_Requested = 0;   /* boolean, was any frame mode explicitly
					  requested by the user?  */

static int	Ilevel = 0;	/* will hold the level of inlining requested */
static int	Inline_Requested = 0;  /* boolean, was inlining explicitly 
				 	  requested by the user?  */

static char     *LoopUnroll = NULL;  /* will be used to control the loop
                                        unrolling optimization. The default
                                        is to turn on loop unrolling in amigo */

static char	*Args_in_Regs = NULL; /* if argument passing in registers is
					 requested, will hold -1R1 for acomp; 
					 otherwise, nothing */

static char	*Alloca = NULL;	  /* alloca built-in option to pass to acomp:
					-2A if wanted, nothing if not */

#if !defined(GEMINI_ON_UW) && !defined(GEMINI_ON_OSR5)
static unsigned char	assert_udk_binary = 0;
#else
static unsigned char	assert_udk_binary = 1;
#endif

#undef CPLUSPLUS_INLINING_ONLY_DEFAULTS_WITH_OPTIMIZATION
	/* Should inlining be done if optimization is not?

	   Depends on how well acomp can generated inlined code without full
	   optimization and also how well debugger deals with inlining; 
	   decision has gone back and forth several times, so define or undef
	   here accordingly.

	   In any case, inlining is done if explicitly asked for.
	*/

void initvars()
{

	if (CCflag) {
#ifdef CPLUSPLUS_INLINING_ONLY_DEFAULTS_WITH_OPTIMIZATION
		Ilevel = -1;	/* C++ inline default depends on other options,
				   indicate no default value for now */
#else
		Ilevel = 2;	/* default to c++_inline for C++ */
#endif
		Inline_Requested = 0;
		Frame_Requested = 0;
	}
	return;
}

int Kelse(s)
char *s;
{
	if (strcmp(s, "i386")==0){
		Acomp_Mach = "-2T3";
		Mach = "-386";
		As_Mach = "-t386";
		}
	else if (strcmp(s, "i486")==0) {
		Acomp_Mach = "-2T4";
		Mach = "-486";
		As_Mach = "-t486";
		}
	else if (strcmp(s, "pentium")==0){
		Acomp_Mach = "-2T5";
		Mach = "-pentium";
		As_Mach = "-tpentium";
		}
	else if ((strcmp(s, "pentium_pro")==0) || (strcmp(s, "pentiumpro")==0)
		|| (strcmp(s, "p6")==0)) {	/* consider this deprecated */
		Acomp_Mach = "-2T6";
		Mach = "-p6";
		As_Mach = "-tpentium";	/* sic */
		}
	else if (strcmp(s, "blended")==0){
		Acomp_Mach = NULL;
		Mach = NULL;  /* This should be changed in the future when
				 optim is changed to accept the -blended mode
				 of compilation as an argument.  Currently,
				 no 386/486/pentium/p6 option indicates blended
				 mode in optim */
		As_Mach = NULL;
		}
	else if (strcmp(s, "frame")==0) {
		Acomp_Frame = NULL;
		Frame = "-_e";
		Frame_Requested = 1;
		}
	else if (strcmp(s, "fixed_frame")==0) {
		Acomp_Frame = ACOMP_FIXED_FRAME_OPTION;
		Frame = NULL;
		Frame_Requested = 1;
		}
	else if ((strcmp(s, "no_frame")==0) || (strcmp(s, "noframe")==0)) {
		Acomp_Frame = Frame = NULL;
		Frame_Requested = 1;
		}
	else if (strcmp(s, "inline")==0) {
		Ilevel = CCflag ? 3 : 1;
		Inline_Requested = 1;
		}
	else if (CCflag && (strcmp(s, "c++inline")==0) || (strcmp(s, "c++_inline")==0)) {
		Ilevel = 2;
		Inline_Requested = 1;
		}
	else if (CCflag && (strcmp(s, "acompinline")==0) || (strcmp(s, "acomp_inline")==0)) {
		Ilevel = 1;
		Inline_Requested = 1;
		}
	else if (CCflag && (strcmp(s, "allinline")==0) || (strcmp(s, "all_inline")==0)) {
		Ilevel = 4;
		Inline_Requested = 1;
		}
	else if ((strcmp(s, "no_inline")==0) || (strcmp(s, "noinline")==0)) {
		Ilevel = 0;
		Inline_Requested = 0;
		}
	else if (strcmp(s, "ieee")==0) {
		addopt(Xc0, "-2i1");
		addopt(Xc2, "-Kieee");
		}
	else if ((strcmp(s, "no_ieee")==0) || (strcmp(s, "noieee")==0) ){
		addopt(Xc0, "-2i0");
		addopt(Xc2, "-Knoieee");
		}
	else if ((strcmp(s, "loop_unroll") == 0) || (strcmp(s, "lu") == 0))
                        LoopUnroll = "-Glu";
        else if ((strcmp(s,"no_loop_unroll") == 0) || (strcmp(s,"no_lu") == 0)
                || (strcmp(s,"noloop_unroll") == 0) || (strcmp(s,"nolu") == 0))
                        LoopUnroll = "-G~lu";
	else if (strcmp(s, "args_in_regs")==0)
		Args_in_Regs = "-1R1";
	else if ((strcmp(s, "no_args_in_regs")==0) || (strcmp(s, "noargs_in_regs")==0))
		Args_in_Regs = NULL;
        else if (strcmp(s,"host") == 0)
                addopt(Xc0, "-2h1"); /* host is the default in acomp */
        else if ((strcmp(s, "no_host") == 0) || (strcmp(s, "nohost") == 0)) {
		if (CCflag)
			/* for c++be this is also a FE and BE pass 1 flag */
			addopt(Xfe, "--no_host");
			addopt(Xc0, "-1h0");
                addopt(Xc0, "-2h0");
		}
	else if (!CCflag && strcmp(s, "alloca") == 0) 
		Alloca = "-2A";
	else if (!CCflag && ((strcmp(s, "no_alloca") == 0) || (strcmp(s, "noalloca") == 0)))
		Alloca = NULL;
	else if (strcmp(s, "narrow_floats") == 0)
		; /* accept and ignore for OSR5 compatibility */
	else if (strcmp(s, "rodata") == 0)
		; /* accept and ignore for OSR5 compatibility */
	else if ((strcmp(s, "no_space") == 0) || (strcmp(s, "nospace") == 0) || (strcmp(s, "space") == 0))
		; /* accept and ignore for OSR5 compatibility */
	else if (strcmp(s, "udk") == 0)
		assert_udk_binary = 1;
	else if ((strcmp(s, "no_udk") == 0) || (strcmp(s, "noudk") == 0))
		assert_udk_binary = 0;
	else if (strcmp(s, "osrcrt") == 0)
		crt = OSRCRT1;		
        else
                return 0;

        return 1;
}

int Yelse(c, np)
int c;
char *np;
{
	return (0);
}

int
Welse(c)
char c;
{
	return (-1);
}

int optelse(c, s)
int c;
char *s;
{
	switch(c) {
	case 'Z':	/* acomp: pack structures for 386 */
		if (!s) {
			error('e', gettxt(":46","Option -Z requires an argument\n"));
			exit(1);
		} else if (*s++ != 'p') {
			error('e', gettxt(":47","Illegal argument to -Z flag\n"));
			exit(1);
		}
		switch ( *s ) {
		case '1':
			if (CCflag) addopt(Xfe, "-Z1");
			if (!CCflag || cplusplus_mode == via_c) addopt(Xc0, "-Zp1");
			break;
		case '2':
			if (CCflag) addopt(Xfe, "-Z2");
			if (!CCflag || cplusplus_mode == via_c) addopt(Xc0, "-Zp2");
			break;
		case '4':
			if (CCflag) addopt(Xfe, "-Z4");
			if (!CCflag || cplusplus_mode == via_c) addopt(Xc0, "-Zp4");
			break;
		case '\0':
			if (CCflag) addopt(Xfe, "-Z1");
			if (!CCflag || cplusplus_mode == via_c) addopt(Xc0, "-Zp1");
			break;
		default:
			error('e', gettxt(":48","Illegal argument to -Zp flag\n"));
			exit(1);
		}
		return 1;
	}
	return 0;
}

void init_mach_opt()
{
	return;
}

void add_mach_opt()
{
	static char	*Inline= NULL;  /* the inlining argument for acomp */

	if (As_Mach != NULL)
		addopt(Xas, As_Mach);

#ifdef CPLUSPLUS_INLINING_ONLY_DEFAULTS_WITH_OPTIMIZATION
	if (CCflag && (Ilevel == -1))
		/* inlining defaults if optimization on, otherwise doesn't */
		Ilevel = Oflag ? 2 : 0;
#endif

#ifdef CPLUSPLUS_DEBUGGING_INLINED_CODE_DOES_NOT_WORK
	/* This is no longer necessary, due to acomp improvements in this area,
	   but it might become necessary again -- this has been changed back
	   and worth several times already.    */

        /* If C++ and -g has been specified, suppress any inlining.
	 * This is because inlining confuses the debugger.
	 * Also, if inlining was explicitly requested (rather than normal default behaviour), issue warning.
	 */
	if (CCflag && Ilevel && gflag) {
		Ilevel = 0;
		if (Inline_Requested)
			error('w', gettxt(":1553","debugging and inlining mutually exclusive; inlining disabled\n"));
	}
#endif

#define	CPLUSPLUS_BASIC_BLOCK_PROFILING_INLINED_CODE_DOES_NOT_WORK
#ifdef	CPLUSPLUS_BASIC_BLOCK_PROFILING_INLINED_CODE_DOES_NOT_WORK
	/* This still is necessary, but might not be in the future. */

        /* If C++ and -ql has been specified, suppress any inlining.
	 * This is because inlining confuses the basic block profiler.
	 * Also, if inlining was explicitly requested (rather than normal default behaviour), issue warning.
	 */
	if (CCflag && Ilevel && qarg) {
		Ilevel = 0;
		if (Inline_Requested)
			error('w', gettxt(":1544","basic block profiling and inlining mutually exclusive; inlining disabled\n"));
	}
#endif
 
	/* Now we pass the inline level to acomp, but only if the user
	 * has also turned on optimization (since C inlining is only
	 * done with -O), or we are in C++ mode (since C++ inlining is
	 * independent of optimization, for now).
	 */
	if ((CCflag || Oflag) && Ilevel) {
		Inline = stralloc(4);
		(void) sprintf(Inline, "-1I%d", Ilevel);
		addopt(Xc0, Inline);
	}

#ifdef CPLUSPLUS_INLINING_SHOULD_TURN_ON_REGISTER_ALLOCATION
	/* This has not yet been committed to, but is still being considered. */

	/* Since C++ inlining is independent of optimization, we must tell
	 * amigo to do register allocation if the user has not used -O.
	 * Otherwise many benefits of inlining are lost.    */

	if (CCflag && Ilevel && !Oflag)
		addopt(Xc0, "???");
#endif
	/* Alloca used with or without -O. */
	if (Alloca != NULL)
		addopt(Xc0, Alloca);

	if (!Oflag)
		return;

	/* These options only passed on if -O present. */

	/* ... but first check for any clashes */
	if (Args_in_Regs && Ilevel) {
		Args_in_Regs = NULL;
		error('w', gettxt(":1597","arguments in registers and inlining mutually exclusive; arguments in registers disabled\n"));
	}

	/* ... and now check for any changes to defaults */
	if (!Frame_Requested && Mach != NULL && (strcmp(Mach,"-p6") == 0)) {
		/* when -Kp6 is specified, the default becomes -Kfixed_frame */
		Acomp_Frame = ACOMP_FIXED_FRAME_OPTION;
		Frame = NULL;
	}

	if (Acomp_Mach != NULL)
		addopt(Xc0, Acomp_Mach);
	if (Mach != NULL)
		addopt(Xc2, Mach);
	if (Acomp_Frame != NULL)
		addopt(Xc0, Acomp_Frame);
	if (Frame != NULL)
		addopt(Xc2, Frame);
	if (LoopUnroll != NULL)
		addopt(Xc0, LoopUnroll);
	if (Args_in_Regs != NULL)
		addopt(Xc0, Args_in_Regs);
	
	return;

}

void mach_defpath()
{
	return;
}

void AVmore()
{
	if (Oflag)	/* pass -O to acomp */
		addopt(AV, "-O");

	if (Eflag || Pflag)
		return;

	if (pflag)
		addopt(AV,"-p");

	return;
}

/*===================================================================*/
/*                                                                   */
/*                      OPTIMIZER                                    */
/*                                                                   */
/*===================================================================*/
int optimize (i)
	int i;
{
	int j;
	
	nlist[AV]= 0;
		addopt(AV,passname(prefix, N_OPTIM));

	addopt(AV,"-I");
	addopt(AV,c_out);
	addopt(AV,"-O");
#if 0
	addopt(AV,as_in
		 = (Sflag && !qarg) ? setsuf(list[CF][i], "s") : tmp5);
#endif
	if (Sflag)
		/* if assembly file requested and this is the
		   last phase, put out .s; otherwise, put out temp */
#if ASFILT
		if ((CCflag && cplusplus_mode == via_glue) || qarg)
#else
		if (qarg)
#endif
			/* not the last phase */
			as_in = tmp5;
		else
			/* is the last phase */
			as_in = setsuf(list[CF][i], "s");
	else	/* not Sflag */
		as_in = tmp5;
	addopt(AV,as_in);
	for (j = 0; j < nlist[Xc2]; j++)
		addopt(AV,list[Xc2][j]);

	list[AV][nlist[AV]] = 0;	/* terminate arg list */

	PARGS;


	if (callsys(passc2, list[AV], NO_TMP4_REDIRECTION, NORMAL_STDOUT)) {
		error('e', "Optimizer failed, %s was not compiled\n", list[CF][i]);
		cflag++;
		eflag++;
		cunlink(c_out);
		cunlink(as_in);
		return(0);
	} else {
        	c_out= as_in;
        	cunlink(tmp2);
        }


#ifdef PERF
	STATS("optimizer");
#endif

	return(1);
}

void option_mach()
{
	pfmt(stderr,MM_NOSTD,":1765:\t[-K [blended|pentium_pro|pentium|i486|i386]]:\n");
	pfmt(stderr,MM_NOSTD,":1766:\t\tcontrols target code generation.\n");
	pfmt(stderr,MM_NOSTD,":1605:\t[-K [ieee|no_ieee]]: controls floating point conformance.\n");
	pfmt(stderr,MM_NOSTD,":1606:\t[-K [no_frame|fixed_frame|frame]]: controls stack frame layout.\n");
	pfmt(stderr,MM_NOSTD,":1607:\t[-K [no_args_in_regs|args_in_regs]]: controls argument passing method.\n");
	pfmt(stderr,MM_NOSTD,":1608:\t[-K [host|no_host]]: controls standard library name recognition.\n");
	if (CCflag)
		pfmt(stderr,MM_NOSTD,":1547:\t[-K [c++_inline|no_inline|inline]]: controls the degree of\n");
	else
		pfmt(stderr,MM_NOSTD,":1610:\t[-K [no_inline|inline]]: controls the degree of\n");
	pfmt(stderr,MM_NOSTD,":1548:\t\tfunction inlining.\n");
	pfmt(stderr,MM_NOSTD,":1609:\t[-K [loop_unroll|no_loop_unroll]]: controls loop optimization.\n");
	pfmt(stderr,MM_NOSTD,":1675:\t[-K [no_alloca|alloca]]: controls inlining of alloca() calls.\n");
#if !defined(GEMINI_ON_UW) && !defined(GEMINI_ON_OSR5)
	pfmt(stderr,MM_NOSTD,":1676:\t[-K [no_udk|udk]]: mark executable as a UDK binary.\n");
#else
	pfmt(stderr,MM_NOSTD,":1677:\t[-K [udk|no_udk]]: mark executable as a UDK binary.\n");
#endif
	pfmt(stderr,MM_NOSTD,":1678:\t[-K osrcrt]: use alternate start-up for OSR5 binaries.\n");
	pfmt(stderr,MM_NOSTD,":1679:\t[-K narrow_floats]: accepted and ignored for OSR5 compatibility.\n");
	pfmt(stderr,MM_NOSTD,":1680:\t[-K rodata]: accepted and ignored for OSR5 compatibility.\n");
	pfmt(stderr,MM_NOSTD,":1681:\t[-K [space|no_space]]: accepted and ignored for OSR5 compatibility.\n");
	pfmt(stderr,MM_NOSTD,":109:\t[-Zp[124]]: controls packing of structures.\n");

	return;
}

/* add machine specific link-editor options */
void
LDmore()
{
	/* add mark to ELF file asserting UDK binary */
	if (assert_udk_binary)
		addopt(AV, "-fudk");
}
