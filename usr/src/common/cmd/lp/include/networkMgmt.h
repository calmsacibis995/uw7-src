/*		copyright	"%c%" 	*/


#ifndef	NETWORK_MGMT_H
#define	NETWORK_MGMT_H
/*==================================================================*/
/*
*/
#ident	"@(#)networkMgmt.h	1.2"
#ident	"$Header$"

#include	"_networkMgmt.h"
#include	"xdrMsgs.h"
#include	"lists.h"
#include	"boolean.h"

#define	DEFAULT_SCHEME	"cr1"
#define	DEFAULT_LID	((level_t) 2)	/*  SYS_PRIVATE  */
#define	DEFAULT_OWNER	"guest"		/*  Err on the side of caution?  */

typedef	struct
{
	int	size;
	void	*data_p;

}  dataPacket;

typedef	struct
{
	uchar_t		versionMajor;
	uchar_t		versionMinor;
	networkMsgType	msgType;

}  networkMsgTag;

typedef struct
{
        boolean		endOfFile;
        uid_t		uid;
        gid_t		gid;
        level_t		lid;
        mode_t		mode;
        size_t		sizeOfFile;
	char *		ownerp;
        char *		destPathp;
        size_t		fraglen;
	caddr_t 	fragp;

}  fileFragmentMsg;

/*------------------------------------------------------------------*/
/*
**	Interface definition.
*/
#ifdef	__STDC__

int		SendJob (connectionInfo *, list *, list *, list *);
int		NegotiateJobClearance (connectionInfo *);
int		ReceiveJob (connectionInfo *, list **, list **);
char		*ReceiveFile (connectionInfo *, fileFragmentMsg *);
void		SetJobPriority (int);
void		FreeNetworkMsg (networkMsgType, void **);
void		FreeDataPacket (dataPacket **);
boolean		SetDefaultFileAttributes (char *, uid_t, uid_t,
			level_t, mode_t);
boolean		JobPending (connectionInfo *);
boolean		SendFile (connectionInfo *, boolean, char *, char *);
boolean		SendData (connectionInfo *, boolean, void *, int);
boolean		EncodeNetworkMsgTag (connectionInfo *, networkMsgType);
boolean		SendJobControlMsg (connectionInfo *, jobControlCode);
boolean		SendSystemIdMsg (connectionInfo *, void *, int);
boolean		SendFileFragmentMsg (connectionInfo *, boolean,
			fileFragmentMsg *);
dataPacket	*NewDataPacket (int);
networkMsgTag	*ReceiveNetworkMsg (connectionInfo *, jobControlMsg **,
			void **);
networkMsgTag	*DecodeNetworkMsg (connectionInfo *, jobControlMsg **,
			void **);

#else

int		SendJob ();
int		NegotiateJobClearance ();
int		ReceiveJob ();
char		*ReceiveFile ();
void		SetJobPriority ();
void		FreeNetworkMsg ();
void		FreeDataPacket ();
boolean		SetDefaultFileAttributes ();
boolean		JobPending ();
boolean		SendFile ();
boolean		SendData ();
boolean		EncodeNetworkMsgTag ();
boolean		SendJobControlMsg ();
boolean		SendSystemIdMsg ();
boolean		SendFileFragmentMsg ();
dataPacket	*NewDataPacket ();
networkMsgTag	*ReceiveNetworkMsg ();
networkMsgTag	*DecodeNetworkMsg ();

#endif
/*==================================================================*/
#endif
