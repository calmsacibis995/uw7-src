	.ident	"@(#)kern-i386at:svc/bki.s	1.2.1.1"
	.ident	"$Header$"
	.file	"svc/bki.s"

ifdef(`CCNUMA',`
define(`PMAPLIMIT', 3221225472)
',`
define(`PMAPLIMIT', 67108864)
')

	.section	.bki, "", "note"
	.ascii		"BKI=30\n"
	.ascii		"ALIGN=4096\n"
	.ascii		"`PMAPLIMIT'=PMAPLIMIT\n"

	.data
	.align		4
pmaplimit:
	.long		3221225472
	.globl		pmaplimit
