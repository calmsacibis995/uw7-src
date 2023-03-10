/*		copyright	"%c%" 	*/

# ident "$Header$"
# ident	"@(#)extern.h	1.3"

extern	void	log();
extern	void	initialize();
extern	void	openlog();
extern	void	opendebug();
extern	void	debug();
extern	void	insert();
extern	void	startpms();
extern	void	startit();
extern	void	pollpms();
extern	void	pollfail();
extern	void	startpoll();
extern	void	sigpoll();
extern	void	sendpmmsg();
extern	void	purge();
extern	void	parse();
extern	void	read_table();
extern	void	readpipe();
extern	void	error();
extern	void	replace();
extern	void	reap();
extern	void	sendack();
extern	void	account();
extern	void	cleanut();
extern	void	readutmp();
extern	void	quit();
extern	void	usage();

extern	FILE	*open_temp();
extern	FILE	*open_sactab();

extern	struct	sactab	*read_entry();
extern	struct	sactab	*findpm();

extern	char	*trim();
extern	char	*pstate();
extern	char	*nexttok();
extern	char	**mkargv();
extern	char	*make_tempname();
extern	char	**dump_table();

extern	long	time();
extern	long	atol();
extern	char	*ctime();
extern	char	*strchr();
extern	char	*strrchr();
extern	char	*strcat();
extern	char	*strpbrk();
extern	char	*strtok();
extern	char	*strcpy();
extern	char	*fgets();
extern	char	*malloc();
extern	char	*calloc();
extern	char	*mktemp();
extern	void	free();
extern	void	setutent();
extern	void	endutent();
extern	struct	utmp	*getutid();
extern	struct	utmp	*getutent();
extern	struct	utmp	*makeut();
extern	struct	group	*getgrgid();
extern	struct	passwd	*getpwuid();

extern	int	N_msgs;
extern	int	Stime;
extern	int	Sfd;
extern	int	Cfd;
extern	int	Nentries;
extern	struct	sactab	*Sactab;
extern	struct	errmsg	Msgs[];
extern	char	Scratch[];

extern	char	Comment[];
extern	int	Saferrno;

extern	char	**environ;
extern	int	errno;
extern	char	*optarg;
extern	int	optind;
extern	int	opterr;
