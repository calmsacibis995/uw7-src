/*		copyright	"%c%" 	*/


#ident	"@(#)dispatch.h	1.3"
#ident	"$Header$"

#include <time.h>
#include "lpsched.h"

#ifdef	__STDC__

int	s_accept_dest ( char * , MESG * );
int	s_alloc_files ( char * , MESG * );
int	s_cancel ( char * , MESG * );
int	s_cancel_request ( char * , MESG * );
int	s_disable_dest ( char * , MESG * );
int	s_enable_dest ( char * , MESG * );
int	s_end_change_request ( char * , MESG * );
int	s_inquire_class ( char * , MESG * );
int	s_inquire_printer_status ( char * , MESG * );
int	s_inquire_remote_printer ( char * , MESG * );
int	s_inquire_request ( char * , MESG * );
int	s_inquire_request_rank ( char * , MESG * );
int	s_load_class ( char * , MESG * );
int	s_load_filter_table ( char * , MESG * );
int	s_load_form ( char * , MESG * );
int	s_load_printer ( char * , MESG * );
int	s_load_printwheel ( char * , MESG * );
int	s_load_system ( char * , MESG * );
int	s_load_user_file ( char * , MESG * );
int	s_mount ( char * , MESG * );
int	s_move_remote  ( char * , MESG * );
int	s_move_dest  ( char * , MESG * );
int	s_move_request ( char * , MESG * );
int	s_print_request ( char * , MESG * );
int	s_quiet_alert ( char * , MESG * );
int	s_reject_dest ( char * , MESG * );
int	s_send_fault ( char * , MESG * );
int	s_shutdown ( char * , MESG * );
int	s_start_change_request ( char * , MESG * );
int	s_unload_class ( char * , MESG * );
int	s_unload_filter_table ( char * , MESG * );
int	s_unload_form ( char * , MESG * );
int	s_unload_printer ( char * , MESG * );
int	s_unload_printwheel ( char * , MESG * );
int	s_unload_system ( char * , MESG * );
int	s_unload_user_file ( char * , MESG * );
int	s_unmount ( char * , MESG * );
int	r_new_child ( char * , MESG * );
int	r_send_job ( char * , MESG * );
int	s_job_completed ( char * , MESG * );
int	s_child_done ( char * , MESG * );

#else

int	s_accept_dest(),
	s_alloc_files(),
	s_cancel(),
	s_cancel_request(),
	s_disable_dest(),
	s_enable_dest(),
	s_end_change_request(),
	s_inquire_class(),
	s_inquire_printer_status(),
	s_inquire_remote_printer(),
	s_inquire_request(),
	s_inquire_request_rank(),
	s_load_class(),
	s_load_filter_table(),
	s_load_form(),
	s_load_printer(),
	s_load_printwheel(),
	s_load_system(),
	s_load_user_file(),
	s_mount(),
	s_move_remote (),
	s_move_dest (),
	s_move_request(),
	s_print_request(),
	s_quiet_alert(),
	s_reject_dest(),
	s_send_fault(),
	s_shutdown(),
	s_start_change_request(),
	s_unload_class(),
	s_unload_filter_table(),
	s_unload_form(),
	s_unload_printer(),
	s_unload_printwheel(),
	s_unload_system(),
	s_unload_user_file(),
	s_unmount(),
	r_new_child(),
	r_send_job(),
	s_job_completed(),
	s_child_done();

#endif
	
/**
 ** dispatch_table[]
 **/

/*
 * The dispatch table is used to decide if we should handle
 * a message and which function should be used to handle it.
 *
 * D_ADMIN is set for messages that should be handled
 * only if it came from an administrator. These entries should
 * have a corresponding entry for the R_... message case, that
 * provides a routine for sending back a MNOPERM message to those
 * that aren't administrators. This is needed because the response
 * message varies in size with the message type.
 */

typedef struct
{

	char *	namep;
	uint	flags;
	int	(*fncp)();

}
DISPATCH;

#define	D_ADMIN		0x01	/* Only "lp" or "root" can use msg. */
#define D_BADMSG	0x02	/* We should never get this message */
#define	D_SYSTEM	0x04	/* Only siblings may use this message */
