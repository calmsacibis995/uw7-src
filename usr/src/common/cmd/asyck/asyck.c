/* Copyright (c) 1997 SCO Ltd. All rights reserved 			*/

/* SCCS identifier	*/

#ident "@(#)asyck.c	1.2"
#ident "$Header$"

char asychk_copyright[] = "Copyright 1997 SCO Ltd."; 

/* 
 * asyck
 * 
 * Standard serial port hardware utility. For use with PC-AT compatible 
 * UART hardware, upwards compatible with NatSemi NS8250 device. Examples
 * are NS16450, NS16550, ST16650, TI16C750 devices. 
 * 
 * Modern UART devices implement various features which are not reflected 
 * in the termiox/termios state definitions - this utility allows tuning 
 * such data (eg. Rx FIFO trigger level - this setting should be chosen to 
 * get the lowest interrupt overhead consistent with not losing data due to 
 * UART internal overruns. Its value wil depend on factors such as: the load
 * on the host system; the response speed (to flow control) of the remote 
 * system; the transmit burst size of the remote system; the error handling 
 * called when data is lost. The statistics display allows the driver internal 
 * operating behaviour to be observed and corrective action taken, if needed.
 * But only by experts. 
 * 
 */  

/*** Include files ***/ 

#include <sys/types.h>
#include <unistd.h>		
#include <stdio.h>		
#include <stropts.h>		
#include <sys/errno.h>
#include <fcntl.h> 
#include <ctype.h> 
#include <sys/stat.h> 
#include <termios.h>
#include <sys/termiox.h>
#include "asyck.h"

/*** External (library) data ***/

int getopt (int argc, char *const *argv, const char *optstring);

extern char *optarg;
extern int optind, opterr, optopt;

/*** Global data ***/

char		inv_progname[255];			
char		devarg[255] = "/dev/tty00t"; 		/* tty00 default port 	*/
char		invname[255] = "asyck"; 			/* default name 		*/
int			fd = -1; 			
boolean_t	verbose = 0; 	

/* 
 * Error messages
 */ 

const char err1[]="Invocation command line syntax error"; 
const char err2[]="Unrecoverable command line syntax error"; 
const char err3[]="[De]Installation of signal handlers failed: OS error"; 
const char err4[]="Could not open device file"; 
const char err5[]="Could not create device node"; 
const char err6[]="Could not allocate ioctl(2) data space"; 
const char err7[]="Too many commands"; 
const char err8[]="Bad OS compatibility  argument"; 
const char err9[]="Too many port arguments"; 
const char err10[]="Unrecognised ioctl call"; 
const char err11[]="Too many verbose flags";
const char err12[]="Cant open tty device node"; 
const char err13[]="tty device node: name too large"; 
const char err14[]="Not enough memory"; 
const char err15[]="Garbled command line arguments (>1 function)"; 
const char err16[]="Command line arguments incomplete (No function)"; 
const char err17[]="ASY_GETUARTMODE ioctl failed"; 
const char err18[]="ASY_SETUARTMODE ioctl failed"; 
const char err19[]="ASY_GETSTATS ioctl failed"; 
const char err20[]="Cant open device"; 


/* 
 * Warning messages.
 */

const char warn1[]= "Function returned error code"; 

/* 
 * 
 * asychk functions
 *	
 * -h 			:	Display usage 				
 * 
 * -v			: 	Verbose output 
 * -f <device>	: 	Perform command on special device file argument. Defaults 
 *					to /dev/tty00 if not specified. 
 * 
 * -u 			: 	Display UART device type  (eg. chipset)
 * -s			: 	Display driver operating stats (resets parameter counts) 
 * 
 * 		(Depends on UART hardware).
 * -r <ival> 	:	Set UART Rx FIFO's IRQ trigger level
 * -t <ival> 	:	Set UART Tx burst size  
 *
 * 		(Flow control values are percentages of buffer size).
 * -l <ival>	: 	Set driver ISR buffer low water (flow control OFF level)
 * -w <ival> 	: 	Set driver ISR buffer high water (flow control ON level)
 *
 * 		(Frequency is set in unsigned 32b usecs (default = 20000 (20ms))). 
 * -p <ival>	: 	Set 2ary I/O processing sample rate. Sets delay from input
 *					data to sending upstream, and vice versa (if quiescent). 
 * 
 * -c 			: 	Set SCO OSr5 compatibility - ignores DCD state after 
 *					opening a modem control port (RD/WR data if DCD off). 
 * 
 * 	Syntax for these commands is trivial:
 * 		-h 					: ignores all other arguments 
 * 		-v 					: goes with any other args 
 * 		
 * 		-[usrtlwfc] 		: all require -p <port> arg 
 * 
 * 	Exit code is 0 on success, -1 on failure. 
 * 
 */

typedef struct termios termios_t; 
typedef struct termiox termiox_t; 

/*** Prototypes ***/ 

int 	main(int argc, const char *argv[], const char *envp[]); 

void	display_usage(void); 		/* Display usage syntax and synopsis */
void	display_uart_type(void); 	/* Display UART type		*/
void 	display_asystats (astats_t	*sp); 
void 	display_driver_parms(getasydat_t *); 

int		install_sighandlers(void);
int		allsigs_handler(); 
int		uninstall_sighandlers(void);

int		parse_command_line(int argc, char *argv[]); 

boolean_t	is_num(char *); 

int		open_device (char * devname); 		
void 	close_device (int fd); 

void	fatal_error(const char *errmsg, int retcode); 

int		do_getstats_ioctl(astats_t *data); 
int		do_getmode_ioctl(getasydat_t *data); 
int		do_setmode_ioctl(setasyc_t *data); 

/*** Function source ***/ 

void 	
display_driver_parms(getasydat_t *gap)
{ 
	uart_data_t		*udp; 

	udp = & gap->udata; 

	printf("IO 0x%x, IRQ %d, Type Code: %d, Status: 0x%x, OS5 Compat %d\n", 
				udp->iobase, udp->irqnum, udp->uart, udp->status, udp->compat); 
	printf("Rx FIFO: %d, Tx Burst: %d, termio * 0x%x, termiox * 0x%x\n", 
				udp->rxfifo, udp->txfifo, udp->termp, udp->termxp ); 
	return; 
} 

/* 
 * display_asycstats(astats_t *dp)
 * Display the fields of operating statistics. 
 */ 

#define _SDSP(X)    printf ( "   "  #X "= %d", statsptr->X ) ;
#define _SDSN(X)    printf ( "   "  #X "= %d\n", statsptr->X ) ;

void 
display_asystats (astats_t	*sp)
{ 
	astats_t    *statsptr;

	statsptr = sp; 

		_SDSP(  rxdataisr)
		_SDSP(  rawrxchars)
		_SDSP(  spclrxchars)
		_SDSN(  txdatasvcreq)
		_SDSP(  isrmodint)
		_SDSP(  isrnointid)

		_SDSP(  rcvdvstart)
		_SDSN(  rcvdvstop)
		_SDSP(  rcvdixany)

		_SDSP(  ierrcnt)
		_SDSP(  softrxovrn1)
		_SDSN(  softrxovrn2)
		_SDSP(  rxoverrun)
		_SDSP(  rxperror)
		_SDSP(  rxfrmerror)
		_SDSN(  rxbrkdet)

		_SDSP(  rxrealdata)

		_SDSP(  droprxcread)
		_SDSP(  droprxopen)
		_SDSN(  droprxnobuf)
		_SDSP(  nobufchars)

		_SDSP(  txdatachars)
		_SDSP(  nointrpending)

		_SDSN(  vstop_xhre_spin)
		_SDSP(  vstart_xhre_spin)
		_SDSP(  restart_xhre_spin)
		_SDSP(  rsteotpend)
		_SDSN(  badcarrsts)

		_SDSP(  notxttstate )
		_SDSP(  notxhwfc )
		_SDSP(  notxswfc )
		_SDSN(  notxnodata )
		_SDSP(  notxqlockinuse )

} 

void 
display_uart_type( void )
{ 
	getasydat_t		gds; 

	bzero(gds, sizeof(getasydat_t)); 

	if (do_getmode_ioctl(&gds) < 0)
		fatal_error(err17,0); 

	if (verbose)
		display_driver_parms(&gds); 
	else 	
		printf("UART type: %d\n", (int) gds.udata.uart); 

	return; 
} 

void 
display_driver_stats ( void )
{ 
	astats_t	sds; 
	
	if ( do_getstats_ioctl(&sds) < 0)  
		fatal_error(err19, 0); 

	display_asystats(&sds); 
	return; 
} 

int 
set_driver_parms (setasyc_t	*sp)
{
	if ( do_setmode_ioctl(sp) < 0)
		fatal_error(err18, 0); 

	return 0; 
} 

int 
get_driver_parms (getasydat_t *dp)
{ 
	return(do_getmode_ioctl(dp)); 
	
} 

/* 
 * do_*_ioctl ( <ioctl_data_type> ) 
 * 
 * Stage the ioctl: clear all the fields in the data structure, 
 * then send the ioctl(2) to the device already opened. 
 */ 

int 
do_getstats_ioctl(astats_t	*dp) 
{ 
	struct strioctl 	si;  

	si.ic_dp = (char *)dp; 
	si.ic_len = sizeof(astats_t); 
	si.ic_cmd = ASY_GETSTATS;
	si.ic_timout = 5; 				/* seconds to timeout if ! ACK/NAK */ 

	bzero(dp, sizeof(astats_t)); 

	return (ioctl(fd, I_STR, &si )); 
} 

int 
do_getmode_ioctl(getasydat_t *dp) 
{ 
	struct strioctl 	si;  

	si.ic_dp = (char *)dp; 
	si.ic_len = sizeof(getasydat_t); 
	si.ic_cmd = ASY_GETUARTDATA;
	si.ic_timout = 5; 				/* seconds to timeout if ! ACK/NAK */ 

	bzero(dp, sizeof(getasydat_t )); 

	return (ioctl(fd, I_STR, &si )); 
} 


int 
do_setmode_ioctl(setasyc_t *dp) 
{ 
	struct strioctl 	si;  

	si.ic_dp = (char *)dp; 
	si.ic_len = sizeof(setasyc_t); 
	si.ic_cmd = ASY_SETUARTMODE;
	si.ic_timout = 5; 				/* seconds to timeout if ! ACK/NAK */ 

	return (ioctl(fd, I_STR, &si )); 
} 


/* 
 * Attempts to open the passed in or defaulted (/dev/tty00) device file, 
 * and verifies that it is a special character device. Returns a valid 
 * file descriptor on success or -1 on failure. 
 */ 

int 
open_device ( char * devname )
{ 
	int				fd; 
	struct stat		statbuf; 

	/* 
	 * Print to warn user
	 */ 

	if (verbose) 
		printf("\nUsing device file : %s\n", devname ); 

	if ( stat ( (const char *) devname , &statbuf ) < 0 ) { 
		perror ( "stat failed" ); 
		return (-1); 
	} 
	
	if ( !( statbuf.st_mode & S_IFCHR )) { 
		printf( "\n%s: File not character/STREAMS special\n", devname );
		return (-1);
	} 

	if ( ( fd = open ( devname, O_RDWR) ) < 0 ) { 
		perror ( "open(R/W) failed " ); 
		return (-1); 
	} 

	return (fd ); 

} 

void 
close_device ( int fd )
{

	if ( verbose ) 
		printf("\nClosing device fd: %d\n", fd ); 

	if ( close ( fd ) < 0 ) 
		perror ( "close failed" ); 

	return ; 

} 

/* 
 * main 
 * 		Get command line
 *		Install signal handlers (if any SLOW calls)
 * 		Parse command line and do command (ioctl(2) call)
 * 		Uninstall handlers 
 * 		Exit (SUCCESS) 
 */ 

int 	
main(int argc, const char *argv[], const char *envp[])
{ 
	int			rv; 	
	
	if (strlen(argv[0]) < sizeof(invname))
		strcpy(invname, argv[0]); 

	/* 
	 * Some of the operations performed on the port may use 'slow'
 	 * system calls. Install signal handlers so that unexpected 
 	 * interrupts allow us to tidy serial port/driver state.
	 */ 

	if ((rv = install_sighandlers()) < 0) { 
		/*
		 *+ Couldn't install the signal handlers. Indicates an 
		 *+ error in the OS. Better to quit if OS in this state.
		 *+ Allow perror(3) to display syscall return code.
		 */ 
		fatal_error(err3, rv); 
		/* NOTREACHED */
	} 

	/*
	 * Parse command line arguments to determine what to do. 
	 * Do it inside the command loop. 
	 */ 

	if ( parse_command_line(argc, (char **) argv) < 0) { 
		/* 
		 *+ Bad invocation parameters: dont understand request, 
		 *+ or mandatory parameter missing.
		 */ 
		display_usage(); 
		fatal_error("Unrecoverable command line syntax error",0); 
		/* NOTREACHED */
	} 

	/* 
	 * If succesful , uninstall the signal handlers and quit. 
	 */ 

	if ((rv = uninstall_sighandlers()) < 0) { 
		/*
		 *+ Couldn't install the signal handlers. Indicates an 
		 *+ error in the OS. Better to quit if OS in this state.
		 *+ Allow perror(3) to display syscall return code.
		 */ 
		fatal_error(err3, rv); 
		/* NOTREACHED */
	} 

	exit(0); 

} 


/* 
 * int fatal_error(const char *msg, int error)
 * 
 * Call/Exit 
 *	Called when a fatal error occurs. Display the message as a 
 * user diagnostic, call perror(2) to display the error number (if non-zero),
 * then exit with the error as a return value.
 * 
 */ 

void 
fatal_error(const char *msg, int error)
{ 

	if (msg) { 
		printf("Fatal error: %s\n", msg); 
	} 

	if (error) { 
		perror("Fatal Error" ); 
	} 

	exit(error); 
} 
	
/* 
 * 
 * Call/Exit:
 * 
 * Remarks:
 *	Separate commands:  Main, qualifiers
 *	-h  	: Display usage 
 * 	-[usr] -p <port> 	: Commands requiring port specifier
 *	-v 	: Command qualifier (modify command operation) 
 *	-{ f <ftl> } -{ c SCO|UW }  	: Command requiring argument 
 *	
 */ 


void 
display_usage(void)
{ 

	printf ("Usage: %s { cmd <arg> } [ mode ] [ port ]\n" ,invname );  

	printf ("\n\t-h\t\t: Display help screen\n"); 
	printf ("\t-f <dev>\t: device special file\n");  
	printf ("\t-v\t\t: set verbose processing mode\n"); 

	printf ("\n\t-u\t\t: Display UART hardware type\n"); 
	printf ("\n\t-s\t\t: Display driver operation statistics\n"); 

	printf ("\n\t-l <%>\t\t: ISR buffer low water (flow control off)\n");  
	printf ("\t-w <%>\t\t: ISR buffer high water (flow control on)\n");  
	
	printf ("\t-r <n>\t\t: Rx FIFO IRQ trigger level\n");  
	printf ("\t-t <n>\t\t: Tx FIFO burst size \n\n");  

	exit(0); 

} 

/* 
 * 
 * Call/Exit:
 * 	Set up and return the requested cmd.i Verify the arguments 
 * are present and sensible. 
 * 
 * Remarks:
 *	getopt(3) enforces Unix cmdline standards.
 */ 


int
parse_command_line(int ac, char *av[])
{ 
	int					c ; 
	int					ucnt,ccnt,scnt,rcnt,tcnt,lcnt,wcnt,pcnt; 
	uint_t				rxfarg, txfarg, lwmarg, wwmarg, pollarg; 
	setasyc_t			sp; 
	getasydat_t			gp; 	
	
	ucnt = ccnt = scnt = rcnt = tcnt = lcnt = wcnt = pcnt = 0; 

	while ((c = getopt(ac, av, "hvf:usr:t:l:w:p:c")) != EOF) { 

		switch (c) { 

			/* set device file arg */
			case 'f' : 
				if (strlen(optarg) < sizeof(devarg))
					strcpy(devarg, optarg); 
				else 
					fatal_error(err13,0); 
				break ; 
	
			/* set verbose flag */
			case 'v' : 
				verbose = B_TRUE; 	
				break ; 

			/* display help screen and exit */
			case 'h' : 
				display_usage(); 
				/* NOTREACHED */
				break; 

			/* Display UART chipset and exit 	*/ 
			case 'u' : 
				++ucnt; 
				break ; 

			/* Display Stats and exit 	*/ 
			case 's' : 
				++scnt; 
				break ; 

			/* Set Rx FIFO value */ 
			case 'r' : 
				if (rcnt++) 
					break; 
				if (!is_num(optarg))
					break; 
				rxfarg = atoi(optarg); 
				break ; 

			/* Set Tx FIFO value */ 
			case 't' : 
				if (tcnt++) 
					break; 
				if (!is_num(optarg))
					break; 
				txfarg = atoi(optarg); 
				break ; 

			/* Set low water mark value */ 
			case 'l' : 
				if (lcnt++) 
					break; 
				if (!is_num(optarg))
					break; 
				lwmarg = atoi(optarg); 
				break ; 

			/* Set high water mark value */ 
			case 'w' : 
				if (wcnt++) 
					break; 
				if (!is_num(optarg))
					break; 
				wwmarg = atoi(optarg); 
				break ; 

			/* Set poll frequency value */ 
			case 'p' : 
				if (pcnt++) 
					break; 
				if (!is_num(optarg))
					break; 
				pollarg = atoi(optarg); 
				break ; 

			/* Set SCO OSr5 compatible mode */ 
			case 'c' : 
				++ccnt ; 
				break ; 
	
		} 

	} 

	/* 
	 * Have to wait until the end since arguments like device type 
	 * can be anywhere on the command line. 
	 * 
	 * 	-u 	Unique 
	 *	-s 	Unique 
	 *	{-r/-t/-l/-w/-p/-c} in any combination. 
	 */ 

	if ( ucnt && (scnt || rcnt || tcnt || lcnt || wcnt || pcnt || ccnt) ) 
		fatal_error(err15, 0); 
	
	if ( scnt && (ucnt || rcnt || tcnt || lcnt || wcnt || pcnt || ccnt) ) 
		fatal_error(err15, 0); 

	if (!( scnt || ucnt || rcnt || tcnt || lcnt || wcnt || pcnt || ccnt ))
		fatal_error(err16, 0); 

	/* 
	 * Arguments make sense. Try and open the device specified (or 
	 * defaulted to) to send it an ioctl. 
	 */ 

	if ( (fd = open_device ( devarg )) < 0 )
		fatal_error( err20, 0 ); 

	if (ucnt) 
		display_uart_type(); 
	else if (scnt)
		display_driver_stats(); 
	else { 

		/* 
		 * Must be a driver setup parameters command.
		 */ 

		if (rcnt)
			sp.set_rxfifo = rxfarg ; 
		if (tcnt)
			sp.set_txfifo = txfarg ; 
		if (lcnt)
			sp.set_rxlowat = lwmarg; 
		if (wcnt)
			sp.set_rxhiwat = wwmarg; 
		if (pcnt)
			sp.set_pollfreq = pollarg; 

		set_driver_parms(&sp); 

		display_driver_parms(&gp); 
	} 

	/* 
	 * Close down the device node. 
	 */

	close_device (fd); 

	return ( 0 ); 
} 
	
/* 
 * int	
 * allsigs_handler(); 
 * 
 * Call/Exit:
 * 
 * Remarks:
 * 	Signal handler for QUIT, INTR. Ensure all resources returned, 
 * 	tidily.
 */ 
int	
allsigs_handler()
{ 
	if (verbose) 
		printf("Received signal(%d). Tidying up..."); 
	
	/* 
	 */

	return ; 
} 

/* 
 * int	
 * uninstall_sighandlers(void);
 * 
 * Call/Exit:
 * 
 * Remarks:
 *	Deinstalls the handler on QUIT, INTR signals. Essentially reset 
 *	sction to default. 
 *
 */ 

int	
uninstall_sighandlers(void)
{ 

	return ; 
} 
	
/* 
 * int	
 * install_sighandlers(void);
 *
 * Call/Exit:
 * 
 * Remarks:
 *	Installs the signal handler to catch QUIT, INTR signals. 
 *
 */ 

int	
install_sighandlers(void)
{ 

	return ; 
} 	

/* 
 * boolean_t 
 * is_num ( char * ) 
 * 
 * Call/Exit:
 * 
 * Remarks:
 *		Return B_TRUE if pointer to a valid decimal/hex number.
 * 
 */ 
boolean_t 
is_num ( char * s )
{ 
	boolean_t	hex; 

	if ((*s == '0') && ((*(s+1) == 'x')||(*(s+1)=='X'))){ 
		hex = B_TRUE; 
		s += 2; 
	} 

	while (*s++) { 
		if (hex) 
			if (!isxdigit(*s))
				return B_FALSE; 
		else 
			if (!isdigit(*s))
				return B_FALSE; 
	} 	

	return B_TRUE; 
}

