/*		copyright	"%c%" 	*/

#ident	"@(#)ipc:ipcrm.c	1.7.7.8"
#ident "$Header$"


/***************************************************************************
 * Inheritable Privileges : P_MACWRITE,P_MACREAD,P_OWNER
 *       Fixed Privileges : None
 * Notes:
 *
 ***************************************************************************/

/*
 * ipcrm - IPC remove
 *
 * Remove specified message queues,
 * semaphore sets and shared memory ids.
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <dshm.h>
#include <sys/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <priv.h>

#define NULL_MSG	((struct msqid_ds *)NULL)
#define NULL_SEM	((struct semid_ds *)NULL)
#define NULL_SHM	((struct shmid_ds *)NULL)
#define NULL_DSHM 	((struct dshmid_ds *)NULL)

char opts[] = "q:m:s:d:Q:M:S:D:";/* allowable options for getopt */
extern char	*optarg;	/* arg pointer for getopt */
extern int	optind;		/* option index for getopt */
extern int	errno;		/* error return */

/*
*Procedure:     main
*
* Restrictions:
                 getkey:none
                 getopt:none
                 msgctl(2):none
                 shmctl(2):none
                 semctl(2):none
                 fprintf:none
*/
main(argc, argv)
int	argc;
char	**argv;
{
	register int	o;	/* option flag */
	register int	err;	/* error count */
	register int	ipc_id;	/* id to remove */
	register key_t	ipc_key;/* key to remove */
	int ret, serrno;
	key_t getkey();
	void oops();


	/*
	 * If one or more of the IPC modules is not
	 * included in the kernel, the corresponding
	 * system calls will incur SIGSYS.  Ignoring
	 * that signal makes the system call appear
	 * to fail with errno == EINVAL, which can be
	 * interpreted appropriately in oops().
	 */

	(void) signal(SIGSYS, SIG_IGN);

	/*
	 * Go through the options.
	 */

	err = 0;
	while ((o = getopt(argc, argv, opts)) != EOF)
	{
		switch (o)
		{
		case 'q':	/* message queue */
			ipc_id = atoi(optarg);
			ret=msgctl(ipc_id, IPC_RMID, NULL_MSG);
			serrno=errno;
			if ( ret == -1)
			{
				oops("msqid", optarg, serrno);
				err++;
			}
			break;

		case 'm':	/* shared memory */
			ipc_id = atoi(optarg);
			ret=shmctl(ipc_id, IPC_RMID, NULL_SHM);
			serrno=errno;
			if (ret == -1)
			{
				oops("shmid", optarg, serrno);
				err++;
			}
			break;

		case 's':	/* semaphores */
			ipc_id = atoi(optarg);
			ret=semctl(ipc_id, 0, IPC_RMID, NULL_SEM);
			serrno=errno;
			if (ret == -1)
			{
				oops("semid", optarg, serrno);
				err++;
			}
			break;

		case 'd':	/* DSHM */
			ipc_id = atoi(optarg);
			ret=dshm_control(ipc_id, IPC_RMID, NULL_DSHM);
			serrno=errno;
			if (ret == -1)
			{
				oops("dshmid", optarg, serrno);
				err++;
			}
			break;

		case 'Q':	/* message queue (by key) */
			if ((ipc_key = getkey(optarg)) == 0)
			{
				err++;
				break;
			}
			if ((ipc_id = msgget(ipc_key, 0)) == -1)
			{
				oops("msgkey", optarg, errno);
				err++;
				break;
			}
			ret=msgctl(ipc_id, IPC_RMID, NULL_MSG);
			serrno=errno;
			if (ret == -1)
			{
				oops("msgkey", optarg, serrno);
				err++;
			}
			break;

		case 'M':	/* shared memory (by key) */
			if ((ipc_key = getkey(optarg)) == 0)
			{
				err++;
				break;
			}
			if ((ipc_id = shmget(ipc_key, 0, 0)) == -1)
			{
				oops("shmkey", optarg, errno);
				err++;
				break;
			}
			ret=shmctl(ipc_id, IPC_RMID, NULL_SHM); 
			serrno=errno;
			if (ret == -1)
			{
				oops("shmkey", optarg, serrno);
				err++;
			}
			break;

		case 'S':	/* semaphores (by key) */
			if ((ipc_key = getkey(optarg)) == 0)
			{
				err++;
				break;
			}
			if ((ipc_id = semget(ipc_key, 0, 0)) == -1)
			{
				oops("semkey", optarg, errno);
				err++;
				break;
			}
			ret=semctl(ipc_id, 0, IPC_RMID, NULL_SEM); serrno=errno;
			if (ret == -1)
			{
				oops("semkey", optarg, serrno);
				err++;
			}
			break;

		case 'D':	/* DSHM (by key) */
			if ((ipc_key = getkey(optarg)) == 0) {
				err++;
				break;
			}

			if ((ipc_id = dshm_get(ipc_key, 0, 0, 0, 0, 0, 0))
			    == -1) {
				oops("dshmkey", optarg, errno);
				err++;
				break;
			}
			ret=dshm_control(ipc_id, IPC_RMID, NULL);
			serrno=errno;
			if (ret == -1)
			{
				oops("dshmkey", optarg, serrno);
				err++;
			}
			break;

		case '?':	/* anything else */
		default:
			err++;
			break;
		}
	}
	if (err || (optind < argc))
	{
		(void) fprintf(stderr,
		   "usage: ipcrm [ [-q msqid] [-m shmid] [-s semid]"
			       " [-d dshmid]\n"
		   "	[-Q msgkey] [-M shmkey] [-S semkey] [-D dshmkey]"
			       " ... ]\n"); 
		err++;
	}
	exit(err);
	/*NOTREACHED*/
}

/*
*Procedure:     oops
*
* Restrictions:
                 fprintf:none
*/
void
oops(thing, arg, errno)
char *thing;
char *arg;
int  errno;
{
	char *e;

	switch (errno)
	{
	case ENOENT:	/* key not found */
	case EINVAL:	/* id not found */
		e = "not found";
		break;

	case EPERM:
		e = "permission denied";
		break;
	default:
		e = "unknown error";
	}

	(void) fprintf(stderr, "ipcrm: %s(%s): %s\n", thing, arg, e);
}

/*
*Procedure:     getkey
*
* Restrictions:
                 fprintf:none
*/
key_t
getkey(kp)
register char *kp;
{
	key_t k;
	char *tp;	/* will point to char that terminates strtol scan */
	extern long strtol();

	if((k = (key_t)strtol(kp, &tp, 0)) == IPC_PRIVATE || *tp != '\0') {
		(void) fprintf(stderr, "illegal key: %s\n", kp);
		return 0;
	}
	return k;
}
