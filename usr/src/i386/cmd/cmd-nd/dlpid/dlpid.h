#ident "@(#)dlpid.h	26.1"
#ident "$Header$"

/*
 *
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#define STREAMS_PIPE_DEVICE "/dev/spx"

#define PATHSZ	64

struct dlpidev {
	char		dlpi_name[PATHSZ];
	char		dlpi_path[PATHSZ];
	int		dlpi_fd;
	char		dlpi_reopening;		/* 1=Attempt a reopen in 1 min*/
	struct dlpidev	*next;
	struct mdidev	*mdi;

	char		mdi_active;		/* 1=MDI driver is up */
	int		mdi_fd;

	char		mc_valid;		/* 1=M'Cast table is valid */
};

struct mdidev {
	char		mdi_name[PATHSZ];
	char		mdi_path[PATHSZ];
	char		mdi_backup_name[PATHSZ];
	struct mdidev	*next;
	struct mdidev	*prev;
};

#define RESTART_INTERVAL	15/*60*/			/* Seconds */
#define _PATH_DLPIDPID     "/etc/dlpid.pid"

/*******************************************************************************
 * Function prototypes
 ******************************************************************************/
extern void AddInput(int , void (*)());
extern void RemoveInput(int);

extern struct dlpidev * AddInterface(char *, char *);
extern void DumpIfStructs();
extern int OpenPipes();

extern int ReadHWFailInd(struct dlpidev *);
extern int ReadMCTable(struct dlpidev *);
extern int RemoveIf(struct dlpidev *, struct mdidev *);
extern int RemoveInterface(char *, char *);
extern int RestartIf(struct dlpidev *, int);
extern int RestartInterface(char *);
extern int StartIf(struct dlpidev *);
extern int StartInterface(char *);
extern int StopIf(struct dlpidev *, int);
extern int StopInterface(char *);
extern int WriteMCTable(struct dlpidev *);
extern void pipeHandler(int);
extern void ForAllInterfaces( int (*)(char *));

extern void startRestartHandler();
extern void stopRestartHandler();
extern void restartHandler(int);
extern void ignoreHandler(int);
extern struct sigaction alrmhandler, alrmnohandler;

#ifdef _REENTRANT
#define MUTEX_LOCK(lock)    mutex_lock(lock)
#define MUTEX_UNLOCK(lock)  mutex_unlock(lock)
#define MUTEX_TRYLOCK(lock) mutex_trylock(lock)
#else /* _REENTRANT */
#define MUTEX_LOCK(lock)    0
#define MUTEX_UNLOCK(lock)  0
#define MUTEX_TRYLOCK(lock) 0
#endif /* _REENTRANT */

extern int netisl;
