/*		copyright	"%c%" 	*/

#ident	"@(#)lp.tell.c	1.2"
#ident	"$Header$"

#include "signal.h"
#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "string.h"

#include "lp.h"
#include "msgs.h"

void			startup(),
			cleanup(),
			done();

extern char		*getenv();

void			*malloc(),
			*realloc();

extern long		atol();

extern int		atoi();

static void		wakeup();

/**
 ** main()
 **/

int			main (argc, argv)
	int			argc;
	char			*argv[];
{
	char			*alert_text,
				buf[BUFSIZ],
				msgbuf[MSGMAX],
				*printer,
				*s_key;

	int			mtype,
				oldalarm;

	short			status;

	long			key;

	void			(*oldsignal)();


	/*
	 * Run immune from typical interruptions, so that
	 * we stand a chance to get the fault message.
	 * EOF (or startup error) is the only way out.
	 */
	(void) signal (SIGHUP, SIG_IGN);
	(void) signal (SIGINT, SIG_IGN);
	(void) signal (SIGQUIT, SIG_IGN);
	(void) signal (SIGTERM, SIG_IGN);

	/*
	 * Which printer is this? Do we have a key?
	 */
	if (
		argc != 2
	     || !(printer = argv[1])
	     || !*printer
	     || !(s_key = getenv("SPOOLER_KEY"))
	     || !*s_key
	     || (key = atol(s_key)) <= 0
	)
		exit (90);

	/*
	 * Wait for a message on the standard input. When a single line
	 * comes in, take a couple of more seconds to get any other lines
	 * that may be ready, then send them to the Spooler.
	 */
	while (fgets(buf, BUFSIZ, stdin)) {

		oldsignal = signal(SIGALRM, wakeup);
		oldalarm = (int) alarm((unsigned int) 2);

		alert_text = 0;
		do {
			if (alert_text)
				alert_text = realloc(
					alert_text,
					(unsigned int) (strlen(alert_text)+
						strlen(buf)+1)
				);
			else {
				alert_text = malloc((unsigned int)(strlen(buf) + 1));
				alert_text[0] = 0;
			}
			(void) strcat (alert_text, buf);

		} while (fgets(buf, BUFSIZ, stdin));

		(void) alarm ((unsigned int) oldalarm);
		(void) signal (SIGALRM, oldsignal);

		if (alert_text) {

			startup ();

			(void)putmessage (
				msgbuf,
				S_SEND_FAULT,
				printer,
				key,
				alert_text
			);

			if (msend(msgbuf) == -1)
				done (91);
			if (mrecv(msgbuf, sizeof(msgbuf)) == -1)
				done (92);

			mtype = getmessage(msgbuf, R_SEND_FAULT, &status);
			if (mtype != R_SEND_FAULT)
				done (93);

			if (status != MOK)
				done (94);
		}
	}
	done (0);
	/* NOTREACHED */
}

/**
 ** startup() - OPEN MESSAGE QUEUE TO SPOOLER
 ** cleanup() - CLOSE THE MESSAGE QUEUE TO THE SPOOLER
 **/

static int		have_contacted_spooler	= 0;

void			startup ()
{

	/*
	 * Open a message queue to the Spooler.
	 * An error is deadly.
	 */
	if (!have_contacted_spooler) {
		if (mopen() == -1) {
	
			switch (errno) {
			case ENOMEM:
			case ENOSPC:
				break;
			default:
				break;
			}

			exit (1);
		}
		have_contacted_spooler = 1;
	}
	return;
}

void			cleanup ()
{
	if (have_contacted_spooler)
		(void) mclose ();
	return;
}

/**
 ** wakeup() - TRAP ALARM
 **/

static void		wakeup ()
{
	return;
}

/**
 ** done() - CLEANUP AND EXIT
 **/

void			done (ec)
	int			ec;
{
	cleanup ();
	exit (ec);
}
