	.ident	"@(#)libc-i386:csu/crtn.s	1.6"
	.file	"crtn.s"

/ This code provides the end to the _init and _fini functions which are 
/ used C++ static constructors and desctuctors.  This file is
/ included by cc as the last component of the ld command line

	.section	.init
	ret

	.section	.fini
	ret
