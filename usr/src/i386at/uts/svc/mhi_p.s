/	Copyright (c) 1996-1997 Santa Cruz Operation, Inc. All Rights Reserved.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386at:svc/mhi_p.s	1.1"
	.ident	"$Header$"
	.file	"svc/mhi_p.s"

/
/ This module provides the interface from the base kernel to Merge, so
/ called "hook".
/	
/ There is an entry point in this module for every _A_MHI_. The routine
/ in this file loads the hook entry address from the "mhi_table" table and
/ jumps to the hook routine. No stack frame is needed or created.
/

include(KBASE/svc/asm.m4)
include(assym_include)
include(KBASE/svc/intr.m4)
include(KBASE/util/debug.m4)

FILE(`mhi_p.s')

ENTRY(mhi_switch_to)
	jmp	*mhi_table+_A_SWITCH_TO
	SIZE(mhi_switch_to)

ENTRY(mhi_switch_away)
	jmp	*mhi_table+_A_SWITCH_AWAY
		SIZE(mhi_switch_away)	

ENTRY(mhi_thread_exit)
	jmp	*mhi_table+_A_THREAD_EXIT
		SIZE(mhi_thread_exit)	

ENTRY(mhi_ret_user)
	jmp	*mhi_table+_A_RET_USER
		SIZE(mhi_ret_user)	

ENTRY(mhi_signal)
	jmp	*mhi_table+_A_SIGNAL
		SIZE(mhi_signal)	

ENTRY(mddi_query)
	jmp	*mhi_table+_A_QUERY	
	SIZE(mddi_query)	

ENTRY(mhi_port_alloc)
	jmp	*mhi_table+_A_UW_PORT_ALLOC
		SIZE(mhi_port_alloc)	

ENTRY(mhi_port_free)
	jmp	*mhi_table+_A_UW_PORT_FREE
		SIZE(mhi_port_free)	

ENTRY(mhi_com_ppi_ioctl)
	jmp	*mhi_table+_A_UW_COM_PPI_IOCTL
		SIZE(mhi_com_ppi_ioctl)	

ENTRY(mhi_kd_ppi_ioctl)
	jmp	*mhi_table+_A_UW_KD_PPI_IOCTL
		SIZE(mhi_kd_ppi_ioctl)	
	
