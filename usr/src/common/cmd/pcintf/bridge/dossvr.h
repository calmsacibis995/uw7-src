#ident	"@(#)pcintf:bridge/dossvr.h	1.2"
/*
**	@(#)dossvr.h	1.6	2/20/92	15:18:09
**	Copyright 1991  Locus Computing Corporation
**
**	extern function declarations
*/

#include "pci_proto.h"
#include "pci_types.h"

extern void	vfInit		PROTO((void));
extern int	swap_out	PROTO((unsigned));
extern void	delete_file	PROTO((unsigned));
extern void	deldir_pid	PROTO((int));
extern void	dump_file	PROTO((void));
extern int	swap_in		PROTO((unsigned, u_short));
extern int	create_file	PROTO((char *, int, int, int, int, int));
extern int	close_file	PROTO((unsigned, u_short));
extern int	oldest_swappable	PROTO((unsigned));
extern void	unlink_on_termsig	PROTO((void));
extern void	delfile_pid	PROTO((int));
extern void	changename	PROTO((char *, char *));
extern int	write_done	PROTO((unsigned));
extern void	file_written	PROTO((unsigned));
extern long	get_dos_time	PROTO((int));
extern void	pci_timedate	PROTO((int, int, int, int, struct output *));
extern void	del_fname	PROTO((char *));
extern void	close_all	PROTO((void));
extern int	get_vdescriptor	PROTO((ino_t));
extern void	stopService	PROTO((int, int));
extern void	childExit	PROTO((int, int));
extern void	pci_getcwd	PROTO((char *, char *));
extern int	mapfilename	PROTO((char *, char *));
extern int	mapdirent	PROTO((char *, char *, ino_t));
extern void	pci_chdir	PROTO((char *, struct output *, int));
extern void	pci_chmod	PROTO((struct input *, struct output *));
extern void	pci_exit	PROTO((int, struct output *));
extern void	pci_lseek	PROTO((int, long, int, u_short, struct output *, int));
extern void	pci_mapname	PROTO((char *, int, struct output *));
extern void	pci_mkdir	PROTO((char *, struct output *, int));
extern void	pci_open	PROTO((char *, int, int, int, struct output *, int));
extern void	pci_rmdir	PROTO((char *, struct output *, int));
extern void	pci_pwd		PROTO((int, struct output *));
extern int	pci_truncate	PROTO((long, int));
extern int	unmapfilename	PROTO((char *, char *));
extern void	server		PROTO((struct input *));
extern int	find_printx	PROTO((int));
extern void	err_handler	PROTO((unsigned char *, int, char *));
extern void	pci_get_ext_err	PROTO((struct output *));
extern void	pci_close	PROTO((int, long, u_short, int, int, char *, struct output *, int));
extern int	s_print		PROTO((int, int, int, char *));
extern void	pci_delete	PROTO((char *, int, int, struct output *, int));
extern void	pci_conread	PROTO((int, int, int, int, struct output *, int));
extern int	exec_cmd	PROTO((char *, char **));
extern int	fork_wait	PROTO((int *));
extern struct temp_slot	*remove_redirdir	PROTO((struct temp_slot *));
extern struct temp_slot	*redirdir		PROTO((char *, struct temp_slot *));
extern struct output	*pci_ack_read	PROTO((int));
extern int		pciran_read	PROTO((int, long, int, u_short, struct output *));

extern void	pci_create	PROTO((char *, int, int, int, struct output *, int));
extern int	table_init	PROTO((void));
extern int	set_tables	PROTO((int));
extern void	dos_nls_info	PROTO((unsigned short *, unsigned short, unsigned short, struct output *));
extern void	pci_rename	PROTO((char *, char *, int, int, struct output *, int));
extern void	get_version_string	PROTO((char *, int, struct output *));
extern void	pci_fsize	PROTO((char *, int, struct output *, int));
extern void	pci_setstatus	PROTO((char *, int, struct output *, int));
extern void	pci_fstatus	PROTO((struct output *, int));
extern void	pci_devinfo	PROTO((int, struct output *));
extern int	pci_ran_write	PROTO((int, long, u_short, int, struct output *, int));
extern void	pci_seq_write	PROTO((int, char *, int, int, long, struct output *, int));
extern void	pci_mid_write	PROTO((int, char *, unsigned, struct output *, int));
extern void	pci_end_write	PROTO((int, char *, unsigned, struct output *, int));
extern int	wildcard	PROTO((const char *, int));
extern void	getpath		PROTO((char *, char *, char *));
extern int	match		PROTO((char *, char *, int));
extern long	uexec		PROTO((char *, int, int, int, int, int));
extern void	uwait		PROTO((int, struct output *));
extern int	u_wait		PROTO((int *));
extern void	pci_ukill	PROTO((long, int, struct output *));
extern void	kill_uexecs	PROTO((void));

#if !defined(NOLOG)

extern char	*dosCName	PROTO((unsigned int));
extern char	*dosEName	PROTO((unsigned int));
extern void	logControl	PROTO((int));
extern void	logMessage	PROTO((unsigned char *, int));

#endif /* ~NOLOG */

extern int	cvt_to_unix		PROTO((char *, char *));
extern int	cvt_to_dos		PROTO((unsigned char *, unsigned char *));
extern int	cvt_fname_to_unix	PROTO((int, unsigned char *, unsigned char *));
extern int	cvt_fname_to_dos	PROTO((int, unsigned char *, unsigned char *, unsigned char *, ino_t));
extern int	scan_illegal		PROTO((char *, char *));
extern int	cover_illegal		PROTO((char *));

#if defined(RLOCK)

extern int		lock_file	PROTO((int, short, int, long, long));
extern struct vFile	*validFid	PROTO((int, pid_t));
extern int		coLockRec	PROTO((int, short, unsigned long, unsigned long));
extern int		coUnlockRec	PROTO((int, short, unsigned long, unsigned long));
extern int		open_file	PROTO((char *, int, int, int, int));
extern int		add_file	PROTO((int, char *, int, int, ino_t, int, int));
extern void		pci_lock	PROTO((int, int, unsigned short, long, long, struct output *, int));

#else

extern int		open_file	PROTO((char *, int, int, int));
extern int		add_file	PROTO((int, char *, int, ino_t, int, int));
extern void		pci_lock	PROTO((int, int, long, long, struct output *, int));

#endif

#if defined(JANUS)	/* Merge MEM_COPY support. */

extern struct output	*pciseq_read	PROTO((int, unsigned short, char *));

#else	/* not JANUS */

extern struct output	*pciseq_read	PROTO((int, unsigned short));

#endif	/* not JANUS */

#if !defined(NOIPC)

extern void	p_semget	PROTO((char *, struct output *));
extern void	p_semop		PROTO((char *, struct output *));
extern void	p_semctl	PROTO((char *, struct output *));
extern void	p_msgget	PROTO((char *, struct output *));
extern void	p_msgctl	PROTO((char *, struct output *));
extern void	p_msgrcv	PROTO((char *, struct output *));
extern void	p_msgsnd	PROTO((char *, struct output *));
extern void	p_msgsnd_new	PROTO((char *, int, struct output *));
extern void	p_msgsnd_ext	PROTO((char *, int, struct output *));
extern void	p_msgsnd_end	PROTO((char *, int, struct output *));

#endif	/* !NOIPC */

#if defined(XENIX)

extern struct output	*pci_sfirst	PROTO((char *, int, int, int, int));
extern struct output	*pci_snext	PROTO((char *, int, int, int, long, int));

#else

extern struct sio	*pci_sfirst	PROTO((char *, int, int, int, int));
extern struct sio	*pci_snext	PROTO((char *, int, int, int, long, int));

#endif	/* XENIX */
