/* $XConsortium: fsio.c,v 1.23 92/05/14 16:52:27 gildea Exp $ */
/*
 * Copyright 1990 Network Computing Devices
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Network Computing Devices not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  Network Computing
 * Devices makes no representations about the suitability of this software
 * for any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * NETWORK COMPUTING DEVICES DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL NETWORK COMPUTING DEVICES BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OR PERFORMANCE OF THIS SOFTWARE.
 *
/XLOCAL_


 * Author:  	Dave Lemke, Network Computing Devices, Inc
 */
/*
 * font server i/o routines
 */

#include	<X11/Xos.h>

#ifdef NCD
#include	<fcntl.h>
#endif

#include	"FS.h"
#include	"FSproto.h"
#include	<stdio.h>
#include	<signal.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<errno.h>
#include	"FSlibos.h"
#include	"fontmisc.h"
#include	"fsio.h"

#if defined (SVR4) && !defined (ARCHIVE) /* Note: this #define has been added */
#define select _abi_select               /* for ABI compliance. This #define  */
#endif /* SVR4 */                        /* appears in XlibInt.c, XConnDis.c  */
                                         /* NextEvent.c(libXt), Xmos.c(libXm) */
                                         /* fsio.c (libfont), and Berk.c      */					 /* (libX11.so.1)                     */
#ifdef SVR4
#include	<sys/utsname.h>
#include 	<sys/stream.h>
#include 	<sys/stropts.h>
#include 	<sys/stat.h>
#include 	<sys/inode.h>
#include	<sys/select.h>
#define		LOCAL_FS_PATH "/dev/X/xfont."
#endif /* SVR4 */

extern int  errno;

/* check for both EAGAIN and EWOULDBLOCK, because some supposedly POSIX
 * systems are broken and return EWOULDBLOCK when they should return EAGAIN
 */

#if defined(EAGAIN) && defined(EWOULDBLOCK)
#define ETEST(err) (err == EAGAIN || err == EWOULDBLOCK)
#else

#ifdef EAGAIN
#define ETEST(err) (err == EAGAIN)
#else
#define ETEST(err) (err == EWOULDBLOCK)
#endif

#endif

static int  padlength[4] = {0, 3, 2, 1};
unsigned long fs_fd_mask[MSKCNT];

static int  _fs_wait_for_readable();

_fs_name_to_address(servername, inaddr)
    char       *servername;
    struct sockaddr_in *inaddr;
{
    int         servernum = 0;
    char        hostname[256];
    char       *sp;
    unsigned long hostinetaddr;
    struct hostent *hp;

    /* XXX - do any service name lookup to get a hostname */

    (void) strncpy(hostname, servername, sizeof(hostname));

    /* get port */
    if ((sp = index(hostname, ':')) == NULL)
	return -1;

    *(sp++) = '\0';

    /* missing server number */
    if (*sp == '\0')
	return -1;
    servernum = atoi(sp);

    /* find host address */
    sp = hostname;
    if (!strncmp(hostname, "tcp/", 4)) {
	sp += 4;
    }
/* XXX -- this is all TCP specific.  other transports need to hack */
    hostinetaddr = inet_addr(sp);
    if (hostinetaddr == -1) {
	if ((hp = gethostbyname(sp)) == NULL) {
	    /* no such host */
	    errno = EINVAL;
	    return -1;
	}
	inaddr->sin_family = hp->h_addrtype;
	bcopy((char *) hp->h_addr, (char *) &inaddr->sin_addr,
	      sizeof(inaddr->sin_addr));
    } else {
	inaddr->sin_addr.s_addr = hostinetaddr;
	inaddr->sin_family = AF_INET;
    }
    inaddr->sin_port = htons(servernum);

    return 1;
}

#ifdef SIGNALRETURNSINT
#define SIGNAL_T int
#else
#define SIGNAL_T void
#endif

/* ARGSUSED */
static      SIGNAL_T
_fs_alarm(foo)
    int         foo;
{
    return;
}

static int
_fs_connect(servername, timeout)
    char       *servername;
    int         timeout;
{
    int         fd;
    struct sockaddr *addr;
    struct sockaddr_in inaddr;
    unsigned    oldTime;

    SIGNAL_T(*oldAlarm) ();
    int         ret;
#ifdef SVR4
    struct utsname name;
    char   machine[SYS_NMLN], port[10], *name_p, *tmpservername;
    int i, local = 0;
#endif /* SVR4 */

#ifdef SVR4
    if( uname(&name) == -1 ) {
       fprintf(stderr, "uname() failed, errno is %d\n", errno);
       exit(1);
    }
    tmpservername = strdup(servername);
    name_p = tmpservername;

    while( *name_p++ != '/' ) ; /* loop to first character beyond the / */
	
    for( i = 0; *name_p != ':'; i++, name_p++ ) 
       machine[i] = *name_p; 
    machine[i] = '\0';   
    for( i = 0, name_p++; *name_p != '\0' ; i++, name_p++ )
       port[i] = *name_p;
    port[i] = '\0';
    free(tmpservername);

    if( !strcmp(machine, name.nodename) ) /* setup local connection */
    {
    char server_path[64];
    struct stat filestat;
    extern int isastream();
    struct strrecvfd str;

    (void) sprintf(server_path,"%s%s",LOCAL_FS_PATH, port);
#ifdef DEBUG
    fprintf(stderr,"Local FS connection, trying [%s]\n",server_path);
#endif /* DEBUG */
    if (stat(server_path, &filestat) != -1) {
	if ((filestat.st_mode & IFMT) == IFIFO) {
	    if ((fd = open(server_path, O_RDWR)) >= 0) {
#ifdef I_BIGPIPE
		if( ioctl( fd, I_BIGPIPE, &str ) == -1 ) 
#ifdef DEBUG
		  fprintf(stderr, "I_BIGPIPE failed, errno is %d", errno);
#else
		; /* do nothing, we'll get by without it */
#endif /* DEBUG */		
#endif /* I_BIGPIPE */
		if (isastream(fd) > 0) {
#ifdef DEBUG
		    fprintf(stderr,"Local FS connection [%s] succeeded\n",
			      server_path);
#endif /* DEBUG */
		    local = 1; 
		}
	 	else 
	            (void) close(fd);
	    }
	} else {
#ifdef DEBUG
	    fprintf(stderr,"XLOCAL_NAMED non-pipe node.\n");
#endif /* DEBUG */
	    local = 0;
	} 
    }
    if ( local == 0 ) 
           fprintf(stderr,"XLOCAL_NAMED failed\n");
}
if ( local == 0 ){
#endif /* SVR4 */

    if (_fs_name_to_address(servername, &inaddr) < 0)
	return -1;

    addr = (struct sockaddr *) & inaddr;
    if ((fd = socket((int) addr->sa_family, SOCK_STREAM, 0)) < 0)
	return -1;

    oldTime = alarm((unsigned) 0);
    oldAlarm = signal(SIGALRM, _fs_alarm);
    alarm((unsigned) timeout);
    ret = connect(fd, addr, sizeof(struct sockaddr_in));
    alarm((unsigned) 0);
    signal(SIGALRM, oldAlarm);
    alarm(oldTime);
    if (ret == -1) {
	(void) close(fd);
	return -1;
    }
#ifdef SVR4
}
#endif /* SVR4 */
    /* ultrix reads hang on Unix sockets, hpux reads fail */

#if defined(O_NONBLOCK) && (!defined(ultrix) && !defined(hpux))
    (void) fcntl(fd, F_SETFL, O_NONBLOCK);
#else

#ifdef FIOSNBIO
    {
	int         arg = 1;

	ioctl(fd, FIOSNBIO, &arg);
    }
#else
    (void) fcntl(fd, F_SETFL, FNDELAY);
#endif

#endif

    return fd;
}

static int  generationCount;

/* ARGSUSED */
static Bool
_fs_setup_connection(conn, servername, timeout)
    FSFpePtr    conn;
    char       *servername;
    int         timeout;
{
    fsConnClientPrefix prefix;
    fsConnSetup rep;
    int         setuplength;
    fsConnSetupAccept conn_accept;
    int         endian;
    int         i;
    int         alt_len;
    char       *auth_data = NULL,
               *vendor_string = NULL,
               *alt_data = NULL,
               *alt_dst;
    FSFpeAltPtr alts;
    int         nalts;

    conn->fs_fd = _fs_connect(servername, 5);
    if (conn->fs_fd < 0)
	return FALSE;

    conn->generation = ++generationCount;

    /* send setup prefix */
    endian = 1;
    if (*(char *) &endian)
	prefix.byteOrder = 'l';
    else
	prefix.byteOrder = 'B';

    prefix.major_version = FS_PROTOCOL;
    prefix.minor_version = FS_PROTOCOL_MINOR;

/* XXX add some auth info here */
    prefix.num_auths = 0;
    prefix.auth_len = 0;

    if (_fs_write(conn, (char *) &prefix, sizeof(fsConnClientPrefix)) == -1)
	return FALSE;

    /* read setup info */
    if (_fs_read(conn, (char *) &rep, sizeof(fsConnSetup)) == -1)
	return FALSE;

    conn->fsMajorVersion = rep.major_version;
    if (rep.major_version > FS_PROTOCOL)
	return FALSE;

    alts = 0;
    /* parse alternate list */
    if (nalts = rep.num_alternates) {
	setuplength = rep.alternate_len << 2;
	alts = (FSFpeAltPtr) xalloc(nalts * sizeof(FSFpeAltRec) +
				    setuplength);
	if (!alts) {
	    close(conn->fs_fd);
	    errno = ENOMEM;
	    return FALSE;
	}
	alt_data = (char *) (alts + nalts);
	if (_fs_read(conn, (char *) alt_data, setuplength) == -1) {
	    xfree(alts);
	    return FALSE;
	}
	alt_dst = alt_data;
	for (i = 0; i < nalts; i++) {
	    alts[i].subset = alt_data[0];
	    alt_len = alt_data[1];
	    alts[i].name = alt_dst;
	    bcopy(alt_data + 2, alt_dst, alt_len);
	    alt_dst[alt_len] = '\0';
	    alt_dst += (alt_len + 1);
	    alt_data += (2 + alt_len + padlength[(2 + alt_len) & 3]);
	}
    }
    if (conn->alts)
	xfree(conn->alts);
    conn->alts = alts;
    conn->numAlts = nalts;

    setuplength = rep.auth_len << 2;
    if (setuplength &&
	    !(auth_data = (char *) xalloc((unsigned int) setuplength))) {
	close(conn->fs_fd);
	errno = ENOMEM;
	return FALSE;
    }
    if (_fs_read(conn, (char *) auth_data, setuplength) == -1) {
	xfree(auth_data);
	return FALSE;
    }
    if (rep.status != AuthSuccess) {
	xfree(auth_data);
	close(conn->fs_fd);
	errno = EPERM;
	return FALSE;
    }
    /* get rest */
    if (_fs_read(conn, (char *) &conn_accept, (long) sizeof(fsConnSetupAccept)) == -1) {
	xfree(auth_data);
	return FALSE;
    }
    if ((vendor_string = (char *)
	 xalloc((unsigned) conn_accept.vendor_len + 1)) == NULL) {
	xfree(auth_data);
	close(conn->fs_fd);
	errno = ENOMEM;
	return FALSE;
    }
    if (_fs_read_pad(conn, (char *) vendor_string, conn_accept.vendor_len) == -1) {
	xfree(vendor_string);
	xfree(auth_data);
	return FALSE;
    }
    xfree(auth_data);
    xfree(vendor_string);

    conn->servername = (char *) xalloc(strlen(servername) + 1);
    if (conn->servername == NULL)
	return FALSE;
    strcpy(conn->servername, servername);

    return TRUE;
}

static Bool
_fs_try_alternates(conn, timeout)
    FSFpePtr    conn;
    int         timeout;
{
    int         i;

    for (i = 0; i < conn->numAlts; i++)
	if (_fs_setup_connection(conn, conn->alts[i].name, timeout))
	    return TRUE;
    return FALSE;
}

#define FS_OPEN_TIMEOUT	    30
#define FS_REOPEN_TIMEOUT   10

FSFpePtr
_fs_open_server(servername)
    char       *servername;
{
    FSFpePtr    conn;

    conn = (FSFpePtr) xalloc(sizeof(FSFpeRec));
    if (!conn) {
	errno = ENOMEM;
	return (FSFpePtr) NULL;
    }
    bzero((char *) conn, sizeof(FSFpeRec));
    if (!_fs_setup_connection(conn, servername, FS_OPEN_TIMEOUT)) {
	if (!_fs_try_alternates(conn, FS_OPEN_TIMEOUT)) {
	    xfree(conn->alts);
	    xfree(conn);
	    return (FSFpePtr) NULL;
	}
    }
    return conn;
}

Bool
_fs_reopen_server(conn)
    FSFpePtr    conn;
{
    if (_fs_setup_connection(conn, conn->servername, FS_REOPEN_TIMEOUT))
	return TRUE;
    if (_fs_try_alternates(conn, FS_REOPEN_TIMEOUT))
	return TRUE;
    return FALSE;
}

/*
 * expects everything to be here.  *not* to be called when reading huge
 * numbers of replies, but rather to get each chunk
 */
_fs_read(conn, data, size)
    FSFpePtr    conn;
    char       *data;
    unsigned long size;
{
    long        bytes_read;

    if (size == 0) {

#ifdef DEBUG
	fprintf(stderr, "tried to read 0 bytes \n");
#endif

	return 0;
    }
    errno = 0;
    while ((bytes_read = read(conn->fs_fd, data, (int) size)) != size) {
	if (bytes_read > 0) {
	    size -= bytes_read;
	    data += bytes_read;
	} else if (ETEST(errno)) {
	    /* in a perfect world, this shouldn't happen */
	    /* ... but then, its less than perfect... */
	    if (_fs_wait_for_readable(conn) == -1) {	/* check for error */
		_fs_connection_died(conn);
		errno = EPIPE;
		return -1;
	    }
	    errno = 0;
	} else if (errno == EINTR) {
	    continue;
	} else {		/* something bad happened */
	    if (conn->fs_fd > 0)
		_fs_connection_died(conn);
	    errno = EPIPE;
	    return -1;
	}
    }
    return 0;
}

_fs_write(conn, data, size)
    FSFpePtr    conn;
    char       *data;
    unsigned long size;
{
    long        bytes_written;

    if (size == 0) {

#ifdef DEBUG
	fprintf(stderr, "tried to write 0 bytes \n");
#endif

	return 0;
    }
    errno = 0;
    while ((bytes_written = write(conn->fs_fd, data, (int) size)) != size) {
	if (bytes_written > 0) {
	    size -= bytes_written;
	    data += bytes_written;
	} else if (ETEST(errno)) {
	    /* XXX -- we assume this can't happen */

#ifdef DEBUG
	    fprintf(stderr, "fs_write blocking\n");
#endif
	} else if (errno == EINTR) {
	    continue;
	} else {		/* something bad happened */
	    _fs_connection_died(conn);
	    errno = EPIPE;
	    return -1;
	}
    }
    return 0;
}

_fs_read_pad(conn, data, len)
    FSFpePtr    conn;
    char       *data;
    int         len;
{
    char        pad[3];

    if (_fs_read(conn, data, len) == -1)
	return -1;

    /* read the junk */
    if (padlength[len & 3]) {
	return _fs_read(conn, pad, padlength[len & 3]);
    }
    return 0;
}

_fs_write_pad(conn, data, len)
    FSFpePtr    conn;
    char       *data;
    int         len;
{
    static char pad[3];

    if (_fs_write(conn, data, len) == -1)
	return -1;

    /* write the pad */
    if (padlength[len & 3]) {
	return _fs_write(conn, pad, padlength[len & 3]);
    }
    return 0;
}

/*
 * returns the amount of data waiting to be read
 */
int
_fs_data_ready(conn)
    FSFpePtr    conn;
{
    long        readable;

    if (BytesReadable(conn->fs_fd, &readable) < 0)
	return -1;
    return readable;
}

static int
_fs_wait_for_readable(conn)
    FSFpePtr    conn;
{
#ifdef SVR4
    fd_set r_mask;
    fd_set e_mask;
#else
    unsigned long r_mask[MSKCNT];
    unsigned long e_mask[MSKCNT];
#endif /* SVR4 */
    int         result;

#ifdef DEBUG
    fprintf(stderr, "read would block\n");
#endif
#ifdef SVR4
    FD_ZERO(&r_mask);
    FD_ZERO(&e_mask);
    do {
	FD_SET(conn->fs_fd, &r_mask);
	FD_SET(conn->fs_fd, &e_mask);
	result = select(conn->fs_fd + 1, &r_mask, (fd_set *)NULL, 
  			&e_mask, NULL);
#else
    CLEARBITS(r_mask);
    CLEARBITS(e_mask);
    do {
	BITSET(r_mask, conn->fs_fd);
	BITSET(e_mask, conn->fs_fd);
	result = select(conn->fs_fd + 1, r_mask, NULL, e_mask, NULL);
#endif /* SVR4 */
	if (result == -1) {
	    if (errno != EINTR)
		return -1;
	    else
		continue;
	}
	if (result && _fs_any_bit_set(e_mask))
	    return -1;
    } while (result <= 0);

    return 0;
}

int
_fs_set_bit(mask, fd)
    unsigned long *mask;
    int         fd;
{
    return BITSET(mask, fd);
}

int
_fs_is_bit_set(mask, fd)
    unsigned long *mask;
    int         fd;
{
    return GETBIT(mask, fd);
}

void
_fs_bit_clear(mask, fd)
    unsigned long *mask;
    int         fd;
{
    BITCLEAR(mask, fd);
}

int
_fs_any_bit_set(mask)
    unsigned long *mask;
{

#ifdef ANYSET
    return ANYSET(mask);
#else
    int         i;

    for (i = 0; i < MSKCNT; i++)
	if (mask[i])
	    return (1);
    return (0);
#endif
}

int
_fs_or_bits(dst, m1, m2)
    unsigned long *dst,
               *m1,
               *m2;
{
    ORBITS(dst, m1, m2);
}

_fs_drain_bytes(conn, len)
    FSFpePtr    conn;
    int         len;
{
    char        buf[128];

#ifdef DEBUG
    fprintf(stderr, "draining wire\n");
#endif

    while (len > 0) {
	if (_fs_read(conn, buf, (len < 128) ? len : 128) < 0)
	    return -1;
	len -= 128;
    }
    return 0;
}

_fs_drain_bytes_pad(conn, len)
    FSFpePtr    conn;
    int         len;
{
    _fs_drain_bytes(conn, len);

    /* read the junk */
    if (padlength[len & 3]) {
	_fs_drain_bytes(conn, padlength[len & 3]);
    }
}

_fs_eat_rest_of_error(conn, err)
    FSFpePtr    conn;
    fsError    *err;
{
    int         len = (err->length - (sizeof(fsReplyHeader) >> 2)) << 2;

#ifdef DEBUG
    fprintf(stderr, "clearing error\n");
#endif

    _fs_drain_bytes(conn, len);
#ident	"@(#)r5fonts:lib/font/fc/fsio.c	1.9"
}
