#ident	"@(#)maillock.h	11.1"

#ifndef __MAILLOCK_H
# define __MAILLOCK_H

#define	MAILDIR		"/var/mail/"
#define	SAVEDIR		"/var/mail/:saved/"
#define	FWRDDIR		"/var/mail/:forward/"
#define	RDLKDIR		"/var/mail/:readlock/"
#define SPOOLSMTPQ	"/var/mail"

#define	L_SUCCESS	0
#define	L_NAMELEN	1	/* recipient name > 13 chars */
#define	L_TMPLOCK	2	/* problem creating temp lockfile */
#define L_TMPWRITE	3	/* problem writing pid into temp lockfile */
#define	L_MAXTRYS	4	/* cannot link to lockfile after N tries */
#define	L_ERROR		5	/* Something other than EEXIST happened */
#define	L_MANLOCK	6	/* cannot set mandatory lock on temp lockfile */

#if defined(__STDC__) || defined(__cplusplus)
# ifdef __cplusplus
extern "C" {
# endif
extern int maillock(char *user, int retrycnt);
extern int maildlock(char *user, int retrycnt, char *dir, int showerrs);
extern int mailunlock(void);
extern int mailrdlock(char *user);
extern int mailurdlock(void);
# ifdef __cplusplus
}
# endif
#else
extern int maillock();
extern int maildlock();
extern int mailunlock();
extern int mailrdlock();
extern int mailurdlock();
#endif

#endif /* __MAILLOCK_H */
