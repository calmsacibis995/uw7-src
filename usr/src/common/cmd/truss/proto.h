/*		copyright	"%c%" 	*/

#ident	"@(#)truss:common/cmd/truss/proto.h	1.6.10.1"
#ident  "$Header$"

/*
 * Function prototypes for most external functions.
 */

extern	int	requested( process_t * , int );
extern	int	jobcontrol( process_t * );
extern	int	signalled( process_t * );
extern	void	faulted( process_t * );
extern	int	sysentry( process_t * );
extern	void	sysexit( process_t * );
extern	void	showbuffer( process_t * , long , int );
extern	void	showbytes( const char * , int , char * );
extern	void	accumulate( volatile timestruc_t * , timestruc_t * , timestruc_t * );

extern	const char *	ioctlname( int );
extern	const char *	fcntlname( int );
extern	const char *	sfsname( int );
extern	const char *	plockname( int );
extern	const char *	rfsysname( int );
extern	const char *	utscode( int );
extern	const char *	sigarg( int );
extern	const char *	openarg( int );
extern	const char *	msgflags( int );
extern	const char *	semflags( int );
extern	const char *	shmflags( int );
extern 	const char *	dshmflags( int );
extern	const char *	msgcmd( int );
extern	const char *	semcmd( int );
extern	const char *	shmcmd( int );
extern	const char *	dshmcmd( int );
extern	const char *	strrdopt( int );
extern	const char *	strevents( int );
extern	const char *	strflush( int );
extern	const char *	mountflags( int );
extern	const char *	svfsflags( int );
extern	const char *	sconfname( int );
extern	const char *	pathconfname( int );
extern	const char *	fuiname( int );
extern	const char *	fuflags( int );

extern	void	expound( process_t * , int , int );
extern	void	prtime( const char * , time_t );
extern	void	prtimestruc( const char * , timestruc_t );
extern	void	print_siginfo( siginfo_t *sip );

extern	void	increment( volatile int * );
extern	void	decrement( volatile int * );

extern	void	Flush(void);
extern	void	Eserialize(void);
extern	void	Xserialize(void);
extern	void	procadd( pid_t );
extern	void	procdel(void);
extern	int	checkproc( process_t * , char * , int );

extern	int	syslist( char * , sysset_t * , int * );
extern	int	siglist( char * , sigset_t * , int * );
extern	int	fltlist( char * , fltset_t * , int * );
extern	int	fdlist( char * , fileset_t * );

extern	char *	fetchstring( ulong_t , int );
extern	void	show_cred( process_t * , int );
extern	void	errmsg( const char * , const char * );
extern	void	abend( const char * , const char * );
extern	int	isprocdir( process_t * , const char * );

extern	void	outstring( const char * s );

extern	void	show_procset( process_t * , ulong_t );
extern	void	show_statloc( process_t * , ulong_t );
extern	const char *	woptions( int );

extern	const char *	errname( int );
extern	const char *	sysname( int , int );
extern	const char *	rawsigname( int );
extern	const char *	signame( int );
extern	const char *	codename( int, int );
extern	const char *	rawfltname( int );
extern	const char *	fltname( int );
extern	const char *	idtypename( int );

extern	void	show_xstat( process_t * , ulong_t );
extern	void	show_stat( process_t * , ulong_t );

extern	ulong_t	getargp(process_t *, int *);
extern	void	showargs(process_t *, int);
extern	void	set_upid(pid_t);
extern	int	execute( process_t * , int );
extern	int	checksyscall( process_t * );
struct systable;
extern	void	showpaths(const struct systable *);
extern	void	dumpargs(process_t *, ulong_t, const char *);
