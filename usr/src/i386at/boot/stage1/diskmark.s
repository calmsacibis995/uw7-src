/ident	"@(#)stand:i386at/boot/stage1/diskmark.s	1.1"
/ident	"$Header$"
/
/ The loader of this bootstrap (ROM BIOS or "fdisk" boot) expects a
/ 0x55,0xAA pattern to indicate a valid bootstrap.  For the hard disk,
/ this pattern is expected right at the end of the sector (offset 510).
/
	.section disk_mark
disk_mark:
	.byte	0x55
	.byte	0xAA
