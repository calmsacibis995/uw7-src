	.ident	"@(#)kern-i386at:svc/syms_p.s	1.7.5.3"
	.ident	"$Header$"

/ Platform-specific symbol definitions; concatenated onto syms.s

	SYM_DEF(nmi_handler, _A_KVPLOCAL + _A_L_NMI_HANDLER)
	SYM_DEF(fpu_external, _A_KVPLOCAL + _A_L_FPU_EXTERNAL)
	SYM_DEF(noprmpt, _A_KVPLOCAL + _A_L_NOPRMPT);
	SYM_DEF(ipl, _A_KVPLOCAL + _A_L_IPL)
	SYM_DEF(prmpt_count, _A_KVPLOCAL + _A_L_PRMPT_STATE);
	SYM_DEF(picipl, _A_KVPLOCAL + _A_L_PICIPL)
	SYM_DEF(kvpage0, _A_KVPAGE0)
	SYM_DEF(os_this_cpu, _A_KVPLOCAL+_A_L_ENG_NUM)
