/*		copyright	"%c%" 	*/

#ident	"@(#)lidload.h	1.2"
#ident "$Header$"

#include <sys/types.h>
#include <sys/tiuser.h>

/*
 * struct lap: format of an entry in /etc/dfs/lid_and_priv
 * is the first arg to makelist()
 */
struct lap {
	char	domainname[1024];
	char	hostname[1024];
	char	lvlname[1024];
	char	privlist[1024];
};

/*
 * struct lpgen: the converted internal form of an entry
 */
struct lpgen {
	char		*lp_hostname;
	struct netbuf	*lp_addr;
	struct netbuf	*lp_mask;
	lid_t		lp_lid;
	pvec_t		lp_priv;
	int		lp_valid;	
};
