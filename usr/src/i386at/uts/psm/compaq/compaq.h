/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996 Santa Cruz Operation, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/compaq/compaq.h	1.1.1.3"
#ident	"$Header$"


#include <svc/psm.h>
#include <psm/toolkits/psm_i8259/psm_i8259.h>

/* General defines */
#define	NULL			0
#define	CPQ_MAXNUMCPU		XL_MAXNUMCPU	/* max CPU # */
#define CPQ_RESET_VECT		0x467		/* reset vector location */
#define CPQ_FPU_BUSY_LATCH	0xf0


/* cpq_mptype defines */
#define	CPQ_NONE	0x00
#define	CPQ_SYSTEMPRO	0x01
#define	CPQ_SYSTEMPROXL	0x02
#define	CPQ_PROLIANT	0x04
#define	CPQ_SYSTEMPRO_COMPATIBLE	0x08

/* Time specific defines */
#define	CPQ_SECS_IN_HOUR	3600
#define	CPQ_USECS_IN_SEC	1000000
#define	CPQ_NSECS_IN_USEC	1000
#define	CPQ_SECS_IN_1HIRT	4294
#define	CPQ_USECS_IN_1HIRT	967296

/* EISA Id defines */
#define CPQ_ID_PORT	0x0C80		/* Where to get EISA ID of IO/Board */

#define CPQ_ID_PORT0	0x0C80		/* Where to get EISA ID of IO/Board */
#define CPQ_ID_PORT1	0x0C81		/* Where to get EISA ID of IO/Board */
#define CPQ_ID_PORT2	0x0C82		/* Where to get EISA ID of IO/Board */
#define CPQ_ID_PORT3	0x0C83		/* Where to get EISA ID of IO/Board */

#define	ISCPQ		(inb(CPQ_ID_PORT0) == 0xE && inb(CPQ_ID_PORT1) == 0x11)
#define	ISPOWERPRO	(inb(CPQ_ID_PORT0) == 0x5 && inb(CPQ_ID_PORT1) == 0x92)
#define	ISSYSTEMPROXL	(inb(CPQ_ID_PORT2) == 0x15)
#define	ISSYSTEMPRO	(inb(CPQ_ID_PORT2) == 0x01)

#define SP_XL_ID	0x0115110E	/* EISA ID of Systempro_xl i/o board */
#define SP_NOTSPRO	0x11327204	/* EISA ID of non SystemPro clone */
#define SP_NOTSPRO1	0x11327304	/* EISA ID of non SystemPro clone */
#define SP_EBBETS_ID	0x0915110E	/* EISA ID of Ebbets i/o board */


/* Interrupt defines */
#define CPQ_MAX_SLOT   	((I8259_MAX_ICS*I8259_NIRQ)-1)
#define CPQ_SP_SLOT    	(I8259_NIRQ-1)
#define CPQ_FIRST_SLOT	0
#define CPQ_TIMER_SLOT 	CPQ_FIRST_SLOT+0
#define CPQ_CASC_SLOT 	CPQ_FIRST_SLOT+2
#define CPQ_XINT_SLOT	CPQ_FIRST_SLOT+13

#define CPQ_EDGE_TRIGGER	0
#define CPQ_LEVEL_TRIGGER	1

#define CPQ_INIT_MASK		0xffffdffa	/* Mask on TIMER, CASC, XINT */

/* Internal common types */
struct cpq_psmops {
	ms_cpu_t	(*cpq_ps_initpsm)(void);
	void		(*cpq_ps_init_cpu)(void);
	ms_cpu_t 	(*cpq_ps_assignvec)(ms_islot_t);
	void		(*cpq_ps_set_eltr)(ms_cpu_t, ms_islot_t,							   ms_intr_dist_t *);
	void		(*cpq_ps_online_engine)(ms_cpu_t);
	void		(*cpq_ps_send_xintr)(ms_cpu_t);
	void		(*cpq_ps_clear_xintr)(ms_cpu_t);
	ms_bool_t 	(*cpq_ps_isxcall)(void);
	ms_bool_t 	(*cpq_ps_fpu_intr)(void);
	void		(*cpq_ps_offline_prep)(void);
	void		(*cpq_ps_reboot)(ms_cpu_t);
	ms_rawtime_t 	(*cpq_ps_usec_time)(void);
};

struct cpq_slot_info {
	ms_intr_dist_t	*idtp;
	ms_cpu_t	attached_to;
	ms_bool_t	event2_pending;
	ms_bool_t	masked;
};
