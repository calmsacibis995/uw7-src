#ident	"@(#)truss:i386/cmd/truss/registers.h	1.1"
#ident	"$Header$"

CONST char * CONST regname[NGREG] = {
	"gs", "fs", "es", "ds", "edi", "ebp", "esp", "ebx",
	"edx", "ecx", "eax", "trapno", "err", "eip", "cs", "efl"
	"uesp", "ss"
};
