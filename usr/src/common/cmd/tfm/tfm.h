/*		copyright	"%c%" 	*/

#ident	"@(#)tfm.h	1.2"
#ident "$Header$"
#ifndef TFM
#define	TFM	1	/*Flag to say we have been here*/

#define	TFM_CMAX	512

typedef	char	tfm_cname[TFM_CMAX + 1];
typedef	char	tfm_name[NAME_MAX + 1];
typedef	char	tfm_priv[(2 * sizeof(unsigned))+1];
typedef	tfm_name tfm_namelist[1];

typedef	struct {
	char		cmd_path[PATH_MAX];
	tfm_priv	cmd_priv;
}	tfm_cmd;

typedef	struct	{
	char		inv_path[PATH_MAX];
	priv_t		*inv_priv;
	unsigned	inv_count;
}	tfm_invoke;

typedef	struct	{
	int		ncmds;
	tfm_cmd		*cmds;
	tfm_cname	*names;
} tfm_cmdlst;

#define	TFM_ROOT	"/etc/security/tfm"

#define	TFM_TEXTSZ	PATH_MAX+128

#define	TFM_UMASK	0022
#define	TFM_CMASK	0644
#define	TFM_DMASK	0755

#define	unhex(c)	((((c) <= 'f') && ((c) >= 'a')) ?\
				(c) - 'a' + 10 :\
				((((c) <= 'F') && ((c) >= 'A')) ?\
					(c) - 'A' + 10 :\
					((((c) <= '9') && ((c) >= '0')) ?\
						(c) - '0' :\
						0)))

extern	void	tfm_rmpriv();
extern	int	tfm_gotpriv();
extern	int	parselist();
extern	void	inv_vect();
extern	void	cmd2inv();
extern	void	inv2cmd();
extern	int	tfm_setup();
extern	int	tfm_getrcmd();
extern	int	tfm_getucmd();
extern	long	tfm_roles();
extern	long	tfm_ucmds();
extern	long	tfm_rcmds();
extern	long	tfm_getusers();
extern	long	tfm_getroles();
extern	int	tfm_ckdb();
extern	int	tfm_ckrole();
extern	int	tfm_ckuser();
extern	int	tfm_newdb();
extern	int	tfm_newrole();
extern	int	tfm_newuser();
extern	int	tfm_killrole();
extern	int	tfm_killuser();
extern	int	tfm_putucmd();
extern	int	tfm_putrcmd();
extern	int	tfm_putroles();
extern	int	tfm_err();
#endif
