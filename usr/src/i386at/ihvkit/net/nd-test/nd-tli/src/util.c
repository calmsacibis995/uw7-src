#ident	"%W"

#include "util.h"
#include "stress.h"

static char	devicename[128];
/*-----------------------------------------------------------------------*/
/*  Routine: BindReserved
/*
/*  Purpose: 
/*	
/*
/*  Entry:
/*	
/*
/*
/*
/*	Exit:	
/*
/*-----------------------------------------------------------------------*/
int
BindReserved(hostname, proto, port, callptr)
	char	*hostname;
	char	*proto;
	int 	port;
	struct t_call	**callptr;
{

	char	port_num[16];
	int	fd,x,i;
	struct  t_bind *bind2;
	struct  t_bind *bound2;
	struct	netbuf *netbufp;
	struct	nd_hostserv nd_hostserv;	/* used for verification */
	struct	nd_addrlist *nd_addrlistp = NULL;
	int	bound = FALSE;

	void 	*handlep;
	struct	netconfig	*netconfigp;
	int	invalid;

	if (hostname == NULL)
		hostname = HOST_SELF;

	TLI_DEBUG3(2,"BindReserved:INFO: hostname=%s, proto=%s, port=%x\n",hostname, proto, port);
	netbufp = get_addrs(hostname, proto, port);
	if (netbufp == NULL) {
		TLI_DEBUG(1,"BindReserved:ERROR: get_addrs failed\n");
		return -1;
	}

	if ((fd = t_open(devicename,  O_RDWR, NULL)) < 0) {
		TLI_DEBUG1(1,"BindReserved: ERROR:t_open: %s",devicename);
		return(-1);
	}
	if ((bind2 = (struct t_bind *) t_alloc(fd, T_BIND, T_ADDR)) == NULL )
	{
		tli_error("BindReserved:t_alloc ",TO_SYSLOG);
		return (-1);
	}
	if ((bound2 = (struct t_bind *) t_alloc(fd, T_BIND, T_ADDR)) == NULL )
	{
		tli_error("BindReserved:t_alloc ",TO_SYSLOG);
		(void) t_free( (char *)bind2, T_BIND);
		return (-1);
	}
	/*
	 *	Bind to the port
	 */
	TLI_DEBUG2(2,"BindReserved:INFO: maxlen=%d, len=%d buf=[", netbufp->maxlen, netbufp->len);
	for (i = 0; i < netbufp->len;i++)
		TLI_DEBUG1(2,"%02x", netbufp->buf[i]&0xff);
	TLI_DEBUG(2,"]\n");
	(void) memcpy(bind2->addr.buf, netbufp->buf, (int) bind2->addr.maxlen);
	bind2->addr.len = bind2->addr.maxlen;
	bind2->qlen = 8;

	if(t_bind(fd,bind2,bound2) < 0 ) {
		tli_error("BindReserved:t_bind ",TO_SYSLOG);
		(void) t_free( (char *)bind2, T_BIND);
		(void) t_free( (char *)bound2, T_BIND);
		return (-1);
	}
	/*
	 *  Verify the address we were bound to.
	 */
	if ((bind2->addr.len != bound2->addr.len) ||
		memcmp(bind2->addr.buf, bound2->addr.buf, bind2->addr.len)) {
		t_unbind(fd);
		tli_error("BindReserved:t_bind ",TO_SYSLOG);
		(void) t_free( (char *)bind2, T_BIND);
		(void) t_free( (char *)bound2, T_BIND);
		return (-1);
	}
	(void) t_free( (char *)bind2, T_BIND);
	(void) t_free( (char *)bound2, T_BIND);
	/*
	 *  Allocate a library structure for this endpoint. All fields.
	 */
	if((*callptr = (struct t_call *) t_alloc(fd,T_CALL,T_ALL)) == NULL) {
		TLI_DEBUG(1,"BindReserved:ERROR: t_alloc");
		t_unbind(fd);
		t_close(fd);
		return(-1);
	}
	return(fd);
}
/*-----------------------------------------------------------------------*/
/*  Routine: Listen
/*
/*  Purpose: 
/*
/*  Entry: 	fd	fd that the connection request is to be received on
/*		callptr	were to stick the connection information
/*
/*  Exit:	 >0		alls well, connection accepted.
/*		 -1		couldn't accept the connection
/*				connection transport endpoint CLOSED.
/*-----------------------------------------------------------------------*/
int 
Listen(fd,callptr)
	int 		fd;
	struct t_call	*callptr;
{
	/* wait for the connection */
	if (t_listen(fd, callptr) < 0) {
		tli_error("Listen:ERROR: t_listen",TO_SYSLOG);
		return -1;
	}
	TLI_DEBUG(2,"Listen:INFO: incoming connection\n");
}

int
Accept(int fd, struct t_call *callptr)
{
	int	newfd;

	/* Create a bew transport endpoint */
	if ((newfd = t_open(devicename,  O_RDWR, NULL)) < 0) {
		TLI_DEBUG1(1,"Accept: ERROR:t_open: %s",devicename);
		return(-1);
	}
		
	/* bind any local address */
	if (t_bind(newfd, NULL, NULL) < 0) {
		tli_error("ListenAndAccept:ERROR: t_listen",TO_SYSLOG);
		t_close(newfd);
		return -1;
	}

	/* accept the connection on the new descripter */
	if (t_accept(fd, newfd, callptr) < 0) {
		t_close(newfd);
		if (t_errno == TLOOK) 
			return( Look(fd) );
		tli_error("AcceptConnection():t_accept:",TO_SYSLOG);
		t_close(fd);
		return (-1);
	}
	return (newfd);
}

int
ReceiveData(int fd, char *buf, int maxlen)
{
	int flags;
	register nRecv;

	flags = 0;

	if ((nRecv = t_rcv(fd, buf, maxlen, &flags)) < 0) {
		if (t_errno == TLOOK) 
			return( Look(fd) );
		t_error("ReceiveData:ERROR: t_rcv failed");
		return -1;
	}
	return(nRecv);
}	/* ReceiveData */

int
SendData(int fd, char *buf, int len)
{
	int	n;

	if ((n = t_snd(fd, buf, len, 0)) != len) {
		if (errno != EINTR)
			tli_error("SendData:ERROR: t_snd failed", TO_SYSLOG);
	}
	return n;
}
int
Look(int fd)
{
	int t;

       	t = t_look(fd);
	switch(t) {
       	   case T_LISTEN :
       		TLI_DEBUG(2,"Look:INFO: T_LISTEN\n");
		break;
       	   case T_CONNECT :
       		TLI_DEBUG(2,"Look:INFO: T_CONNECT\n");	    
		break;
       	   case T_DATA :
       		TLI_DEBUG(2,"Look:INFO: T_DATA\n");	    
		break;
  	   case T_EXDATA :
       		TLI_DEBUG(2, "Look:INFO: T_EXDATA\n");	    
		break;
  	   case T_DISCONNECT :
       		TLI_DEBUG(2, "Look:INFO: T_DISCONNECT\n");	    
		t_rcvdis(fd, NULL);	
		break;
  	   case T_UDERR :
       		TLI_DEBUG(2, "Look:INFO: T_UNDER\n");	    
		break;
  	   case T_ORDREL :
       		TLI_DEBUG(2, "Look:INFO: T_ORDREL\n");	    
		if (t_rcvrel(fd) < 0) 
			tli_error("Look:ERROR: t_rcvrel",TO_SYSLOG);
		break;
  	}
	errno = EINTR;
	t_close(fd);
	return(0);
}
/*-----------------------------------------------------------------------*/
/*  Routine: tli_error
/*
/*  Purpose: Send the system generated tli error message to the
/*			 syslog or stderr.  Deamons would normally go to syslog
/*			 while clients would send to stderr.
/*
/*  Entry:	 msg 		routine identifier string pointer.
/*		 dir		TO_STDERR = error messages to stderr
/*				TO_SYSLOG = error messages to syslog
/*		 flags		contains priority flags for syslog
/*
/*	Exit:	 
/*	
/*-----------------------------------------------------------------------*/
void
tli_error(char *msg,int dir)
{
	extern int	t_nerr;
	extern char	*t_errlist[];

	if (dir == TO_STDERR )
	{
		t_error(msg);
	}
	else if ( dir == TO_SYSLOG )
	{
		if ( t_errno > 0 && t_errno < t_nerr ) {
			TLI_DEBUG2(1,"%s %s\n",msg,t_errlist[t_errno]);
		}
		else {
			TLI_DEBUG2(1,"%s t_errno = %d\n",msg,t_errno);
		}
	}
}

struct netbuf *
get_addrs(char *hostname, char *proto, int port)
{
	struct netbuf *netbufp;

	char	port_num[16];
	struct	nd_hostserv nd_hostserv;	/* used for verification */
	struct	nd_addrlist *nd_addrlistp = NULL;
	struct	netconfig	*netconfigp;
	int	invalid;

	void 	*handlep;

	if ((handlep = setnetpath()) == NULL) {
		nc_perror("get_addrs");
		return NULL;
	}

	while ((netconfigp = getnetpath(handlep)) != NULL) {
		TLI_DEBUG3(3,"get_addrs:INFO: netid %s proto %s device %s\n", netconfigp->nc_netid, netconfigp->nc_proto, netconfigp->nc_device);

		if (strcmp(netconfigp->nc_proto, proto) == 0)
				break;
	}
	
	if (netconfigp == NULL) {
		TLI_DEBUG1(1,"get_addrs: ERROR:protocol %s not found\n",proto);
		return NULL;
	}
#ifdef UNCOMMENT
	/*
	 *	Find the correct range of reserved ports for this transport
	 *	type.
	 */
	invalid = FALSE;
	if ( strcmp(netconfigp->nc_netid,"tcp") == 0 )
		if ((port >= IPPORT_RESERVED - 1) || (port < IPPORT_RESERVED/2))
			invalid = TRUE;	/* invalid */
	else if ( strcmp(netconfigp->nc_netid,"spx") == 0 )
		if ((port >= IPXPORT_RESERVED) || (port < STARTPORT))
			invalid = TRUE;	/* invalid */
	else
	{
		TLI_DEBUG1("1,get_addrs:ERROR: %s != (TCP || SPX)", netconfigp->nc_netid);
		return NULL;
	}

	if (invalid) {
		TLI_DEBUG1(1,"get_addrs: ERROR: invalid port %d\n", port);
		return NULL;
	}
#endif /* UNCOMMENT */
	sprintf(port_num,"%4.4d",port);
	nd_hostserv.h_host = hostname;
	nd_hostserv.h_serv = port_num;
	TLI_DEBUG2(2,"get_addrs:INFO: hostname=%s, port=%s\n",hostname, port_num);
	if (netdir_getbyname(netconfigp, &nd_hostserv, &nd_addrlistp) == 0) {
		netbufp = nd_addrlistp->n_addrs;
	}
	else
		netbufp = NULL;	

	strcpy(devicename,netconfigp->nc_device);
	endnetconfig(handlep);
	return(netbufp);
}
/*-----------------------------------------------------------------------*/
/*  Routine: BindAndConnect
/*
/*  Purpose: Do all necessary step to connect to a transport endpoint.
/*		 Binding is done to a transport provided port number.
/*		 Port number and remote system address for connection are
/*           determined from the argumented netbuf structure.
/*
/*  Entry:	 netconfigp	pointer to transport provider information.
/*		 netbufp	Address to connect to.
/*
/*	Exit:	 >0		alls well, fd is valid
/*		 -1		Can't open device,can't bind
/*				can't connect.               
/*-----------------------------------------------------------------------*/
int
BindAndConnect(hostname, proto, port)
	char 	*hostname;
	char 	*proto;
	int	port;
{
	struct t_call	*callptr;
    	struct netbuf	*netbufp;
	int		fd;
	int		i;
	
	TLI_DEBUG3(2,"BindAndConnect:INFO: hostname=%s proto=%s port=%x\n",hostname, proto, port);
	netbufp = get_addrs(hostname, proto, port);
	if (netbufp == NULL) {
		TLI_DEBUG(1,"BindAndConnect:ERROR: get_addrs failed\n");
		return -1;
	}
	if ((fd = t_open(devicename,  O_RDWR, NULL)) < 0) {
		TLI_DEBUG1(1,"BindAndConnect:ERROR: t_open: %s",devicename);
		return(-1);
	}

	if(t_bind(fd,NULL,NULL) < 0 ) 
	{
		tli_error("BindAndConnect:ERROR:t_bind:",TO_SYSLOG);
		t_close(fd);
		return(-1);
	}
	/*
 	*  Allocate a library structure for this endpoint. All fields.
 	*/
   	if((callptr = (struct t_call *) t_alloc(fd,T_CALL,T_ALL)) == NULL)
   	{
		tli_error("BindAndConnect():t_alloc:",TO_SYSLOG);
		t_unbind(fd);
		t_close(fd);
		return(-1);
   	}
	TLI_DEBUG2(2,"BindAndConnect:INFO: maxlen=%d, len=%d buf=[", netbufp->maxlen, netbufp->len);
	for (i = 0; i < netbufp->len;i++)
		TLI_DEBUG1(2,"%02x", netbufp->buf[i]&0xff);
	TLI_DEBUG(2,"]\n");
	callptr->addr.buf = netbufp->buf;
	callptr->addr.len = netbufp->len;
	callptr->addr.maxlen = netbufp->maxlen;

	callptr->opt.buf = NULL;
	callptr->opt.len = 0;
	callptr->opt.maxlen = 0;

	callptr->udata.buf = NULL;
	callptr->udata.len = 0;
	callptr->udata.maxlen = 0;
	if ( t_connect(fd,callptr,NULL) < 0 ) {
		tli_error("BindAndConnect():t_connect:",TO_SYSLOG);
		(void) t_free((char *)callptr,T_CALL);
		t_close(fd);
		return(-1);
	}
	(void) t_free((char *)callptr,T_CALL);
	return(fd);
}

int
Release(int fd)
{
	struct t_info	info;
	int		result = 0;
	int		i;

	/* Get the transport information */

	if (t_getinfo(fd, &info) < 0) {
		tli_error("ReleaseAndClose:ERROR: t_getinfo",TO_SYSLOG);
		info.servtype = T_COTS;
		result = -1;
	}
	/* Release the connection */
	switch (info.servtype) {
	   case T_COTS_ORD:
		TLI_DEBUG(2,"ReleaseAndClose:INFO: T_COTS_ORD\n");
		if (t_sndrel(fd) < 0) {
			result = -1;
			tli_error("ReleaseAndClose:ERROR: t_sndrel",TO_SYSLOG);
		}
		TLI_DEBUG1(2,"ReleaseAndClose:INFO: t_sndrel result=%d\n",result);
		break;
	   case T_COTS:
	   default:
		sleep(1);
		if (t_snddis(fd,NULL) < 0) {
			result = -1;
			tli_error("ReleaseAndClose:ERROR: t_snddis",TO_SYSLOG);
		}
		TLI_DEBUG1(2,"ReleaseAndClose:INFO: t_snddis result=%d\n",result);
		break;
	}
	return (result);
}
void
Close(int fd)
{
	t_close(fd);
}		
ushort
checksum(buffer,len)
char buffer[];
register int len;
{
	register sum, val;
	int	 n;

	sum = 0;
	while(len--) {
		val = buffer[len];
		if(len&1)
		     sum += len+(val>>4)+(val>>8)+(val&0xf)+(val&0xff);
		else
		     sum += val-(val>>4)+(len>>8)+(len&0xf)+(val&0xff);
	}
	sum += len+(val>>4)+(val>>8)+(val&0xf)+(val&0xff);
	return((ushort)sum);
}
