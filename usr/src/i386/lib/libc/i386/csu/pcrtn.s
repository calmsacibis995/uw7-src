	.ident	"@(#)libc-i386:csu/pcrtn.s	1.2"
	.file	"pcrtn.s"

/ This code provides the end to the _init and _fini functions which are 
/ used C++ static constructors and desctuctors.  This file is
/ included by cc as the last component of the ld command line

	.section	.init
	ret

	.section	.fini
	ret
