/*		copyright	"%c%" 	*/

#ident	"@(#)sh:common/cmd/sh/name.h	1.7.5.2"
#ident "$Header$"
/*
 *	UNIX shell
 */


#define	N_ENVCHG 0020
#define N_RDONLY 0010
#define N_EXPORT 0004
#define N_ENVNAM 0002
#define N_FUNCTN 0001

#define N_DEFAULT 0

struct namnod
{
	struct namnod	*namlft;
	struct namnod	*namrgt;
	unsigned char	*namid;
	unsigned char	*namval;
	unsigned char	*namenv;
	int	namflg;
};

