#ident	"@(#)ksh93:src/cmd/ksh93/include/io.h	1.1"
#pragma prototyped
/*
 *	UNIX shell
 *	David Korn
 *
 */

#include	<ast.h>
#include	<sfio.h>

#ifndef IOBSIZE
#   define  IOBSIZE	4096
#endif /* IOBSIZE */
#define IOMAXTRY	20

/* used for output of shell errors */
#define ERRIO		2

#define IOREAD		001
#define IOWRITE		002
#define IODUP 		004
#define IOSEEK		010
#define IONOSEEK	020
#define IOTTY 		040
#define IOCLEX 		0100
#define IOCLOSE		(IOSEEK|IONOSEEK)


/*
 * The remainder of this file is only used when compiled with shell
 */

#ifdef KSHELL

#define sh_inuse(f2)	(sh.fdptrs[f2])

extern int	sh_iocheckfd(int);
extern void 	sh_ioinit(void);
extern int 	sh_iomovefd(int);
extern int	sh_iorenumber(int,int);
extern void 	sh_pclose(int[]);
extern void 	sh_iorestore(int);
extern Sfio_t 	*sh_iostream(int);
struct ionod;
extern int	sh_redirect(struct ionod*,int);
extern void 	sh_iosave(int,int);
extern void 	sh_iounsave(void);
extern int	sh_chkopen(const char*);
extern int	sh_ioaccess(int,int);

/* the following are readonly */
extern const char	e_pexists[];
extern const char	e_pexists_id[];
extern const char	e_query[];
extern const char	e_query_id[];
extern const char	e_history[];
extern const char	e_history_id[];
extern const char	e_argtype[];
extern const char	e_argtype_id[];
extern const char	e_create[];
extern const char	e_create_id[];
extern const char	e_tmpcreate[];
extern const char	e_tmpcreate_id[];
extern const char	e_exists[];
extern const char	e_exists_id[];
extern const char	e_file[];
extern const char	e_file_id[];
extern const char	e_file2[];
extern const char	e_file2_id[];
extern const char	e_formspec[];
extern const char	e_formspec_id[];
extern const char	e_badregexp[];
extern const char	e_badregexp_id[];
extern const char	e_open[];
extern const char	e_open_id[];
extern const char	e_toomany[];
extern const char	e_toomany_id[];
extern const char	e_pipe[];
extern const char	e_pipe_id[];
extern const char	e_flimit[];
extern const char	e_unknown[];
extern const char	e_unknown_id[];
extern const char	e_devnull[];
extern const char	e_profile[];
extern const char	e_sysprofile[];
extern const char	e_stdprompt[];
extern const char	e_supprompt[];
extern const char	e_ambiguous[];
extern const char	e_ambiguous_id[];
#endif /* KSHELL */
