#ident	"@(#)general.c	1.2"
#ident	"$Header$"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/* general.c - Miscellaneous routines needed by the agent */

/*
 *
 * Contributed by NYSERNet Inc. This work was partially supported by
 * the U.S. Defense Advanced Research Projects Agency and the Rome
 * Air Development Center of the U.S. Air Force Systems Command under
 * contract number F30602-88-C-0016.
 *
 */

/*
 * All contributors disclaim all warranties with regard to this
 * software, including all implied warranties of mechantibility
 * and fitness. In no event shall any contributor be liable for
 * any special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in action of contract, negligence or other tortuous action,
 * arising out of or in connection with, the use or performance
 * of this software.
 */

/*
 * As used above, "contributor" includes, but not limited to:
 * NYSERNet, Inc.
 * Marshall T. Rose
 */

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>
#include <unistd.h>

#include "snmp.h"
#include "snmpuser.h"
#include "snmpd.h"
#include "peer.h"

#if defined(SVR3) || defined(SVR4)
#include <sys/stream.h>
#include <sys/tiuser.h>

extern int t_errno;
#endif

#define OK	 0
#define NOTOK	-1

/*
 * Maximum amount of time to wait for data from
 * the peer.
 */
#define PEER_TIMEOUT_VAL	60

extern	int	errno;

/* Global variables */
extern	struct	smuxPeer *PHead;
extern	struct	smuxTree *THead;
extern	fd_set	ifds, sfds;
extern	int	log_level;


#ifdef SVR4
/*defined in libc*/
#include <search.h>

#else

struct qelem {
    struct qelem   *q_forw;
    struct qelem   *q_back;
    char            q_data[1];  /* extensible */
};

insque (elem, pred)
struct qelem   *elem, *pred;
{
    pred->q_forw->q_back = elem;
    elem->q_forw = pred->q_forw;
    elem->q_back = pred;
    pred->q_forw = elem;
}


remque (elem)
struct qelem   *elem;
{
    elem->q_forw->q_back = elem->q_back;
    elem->q_back->q_forw = elem->q_forw;
}
#endif /* SVR4 */


char *octet2str(OctetString *os)
{
    int   i;
    char *str1, *str2;

    if ((str1 = (char *) malloc ((unsigned) (os->length + 1))) == NULL)
	return (NULL);

    for (i = 0; i < os->length; i++)
	str1[i] = os->octet_ptr[i];

    str1[os->length] = '\0';

    return (str1);
}


void tb_free (register struct smuxTree *tb)
{
    register struct smuxTree *tp,
			     **tpp;

    if (tb == NULL)
        return;

    for (tpp = (struct smuxTree **) &tb->tb_subtree->smux;
             tp = *tpp;
             tpp = &tp->tb_next)
        if (tp == tb) {
            *tpp = tb->tb_next;
            break;
        }

    remque ((struct qelem *)tb);
    free ((char *) tb);
}


void	pb_free (register struct smuxPeer *pb)
{
    register struct smuxTree *tb, *ub;

    if (pb == NULL)
        return;

    for (tb = THead->tb_forw; tb != THead; tb = ub) {
        ub = tb->tb_forw;

        if (tb->tb_peer == pb)
            tb_free (tb);
    }
 
    if (pb->pb_fd != NOTOK) {
        (void) t_close (pb->pb_fd);
        FD_CLR (pb->pb_fd, &ifds);
        FD_CLR (pb->pb_fd, &sfds);
    }

    if (pb->pb_data) {
	if (pb->pb_data->data)
	    free (pb->pb_data->data);
	free ((char *) pb->pb_data);
    }
 
    if (pb->pb_identity)
        free_oid (pb->pb_identity);
    if (pb->pb_description)
        free ((char *) pb->pb_description);
 
    remque ((struct qelem *)pb);
    free ((char *) pb);
}

int peer_timeout(int sig)
{
  syslog(LOG_WARNING, gettxt(":90", "Timed out waiting for data from SMUX peer.\n"));
}
