	.file	"bki.s"

	.ident	"@(#)stand:i386/boot/test/bki.s	1.1"
	.ident	"$Header$"

	.text
	.align		0x1000

	.data
	.align		0x1000

	.section	.bki, "", "note"
	.ascii		"BKI=30\n"
	.ascii		"ALIGN=4096\n"
