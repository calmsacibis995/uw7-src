#ident	"@(#)peer.h	1.2"
#ident	"$Header$"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/* peer.h - Definition of data structures related to peers */

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

#define	BUFSIZE	2048

typedef struct _data_area {
    char  *data;
    int    len;
} DA;		/* to store the excess data read on the TCP endpoint */


struct smuxPeer {
    struct smuxPeer *pb_forw;		/* doubly-linked list */
    struct smuxPeer *pb_back;		/*   .. */

    int	    pb_index;			/* smuxPindex */

    int	    pb_fd;			
    struct sockaddr_in pb_address;
    char    pb_source[30];
    OID	    pb_identity;		/* smuxPidentity */
    char   *pb_description;		/* smuxPdescription */
    int	    pb_sendCoR;
    int	    pb_priority;		/* minimum allowed priority */
    int	    pb_newstatus;		/* for setting smuxPstatus */
    int	    pb_invalid;
    DA     *pb_data;			/* excess TCP data received */
};


struct smuxTree {
    struct smuxTree *tb_forw;		/* doubly-linked list */
    struct smuxTree *tb_back;		/*   .. */

#define	TB_SIZE	30			/* object instance */
    unsigned int    tb_instance[TB_SIZE + 1];
    int	    tb_insize;
    VarEntry   *tb_subtree;		/* smuxTsubtree */
    int	    tb_priority;		/* smuxTpriority */
    struct smuxPeer *tb_peer;		/* smuxTindex */
    struct smuxTree *tb_next;		/* linked list for ot_smux */
    int	    tb_newstatus;		/* for setting smuxPstatus */
    int	    tb_invalid;
};


struct smuxReserved {
    char  *rb_text;
    OID   rb_name;
};


