#ident	"@(#)Space.c	2.1"
#ident	"$Header$"

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/dlpi_ether.h>
#include <config.h>

/*
 * The N_BOARDS defines how many ee16 boards supported by this driver
 */
#define N_BOARDS        4

/*
 *  The N_SAPS define determines how many protocol drivers can bind to a single
 *  EE16  board.  A TCP/IP environment requires a minimum of two (IP and ARP).
 *  Putting an excessively large value here would waste memory.  A value that
 *  is too small could prevent a system from supporting a desired protocol.
 */
#define	N_SAPS		8

/*
 *  The STREAMS_LOG define determines if STREAMS tracing will be done in the
 *  driver.  A non-zero value will allow the strace(1M) command to follow
 *  activity in the driver.  The driver ID used in the strace(1M) command is
 *  equal to the ENET_ID value (generally 2101).
 *
 *  NOTE:  STREAMS tracing can greatly reduce the performance of the driver
 *         and should only be used for trouble shooting.
 */
#define	STREAMS_LOG	0

/*
 *  The IFNAME define determines the name of the the internet statistics
 *  structure for this driver and only has meaning if the inet package is
 *  installed.  It should match the interface prefix specified in the strcf(4)
 *  file and ifconfig(1M) command used in rc.inet.  The unit number of the
 *  interface will match the board number (i.e emd0, emd1, emd2) and is not
 *  defined here.
 */
#define	IFNAME	"ee16"

int		ee16strlog = STREAMS_LOG;	/* is STREAMS logging on? */
int		ee16nsaps = N_SAPS;		/* # possible protocols */
int		ee16nboards = N_BOARDS;
int		ee16cmajor = EE16_CMAJOR_0;	/* first major # in range */
char		*ee16_ifname = IFNAME;		/* interface name */
