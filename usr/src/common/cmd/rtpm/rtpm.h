#ident	"@(#)rtpm:rtpm.h	1.6.1.1"

/*
 * the default alarm interval
 */
#define DEFAULT_ALARM	2
/*
 * types of actions for cooking metrics
 */
#define NONE 0		/* don't do anything to the metric	*/
#define RATE 1		/* compute rate as diff / time		*/
#define MEAN 2		/* compute percentage			*/
#define INSTANT 3	/* report instantaneous value		*/

/* PARANOID covers up a cache consistency bug where the metric page 
 * is occasionally a stale one from several samples in the past,
 * which may result in very large rates being calculated.  If PARANOID
 * is set, samples that are accumulated are discarded if the current
 * value is less than the previous value.
 */
#define PARANOID
/*

 * an arbitrary id number above range of valid id's
 */
#define CALC_MET	4096	
/*
 * The maximum number of screens that can be pushed
 */
#define MAXSCREENDEPTH	5
/*
 * Bit masks for "help" and "cannot plock" messages
 */
#define HELP_MSG_BIT	1
#define LOCK_MSG_BIT	2
/*
 * set peak hold capability in the bargraph generator
 */
#define PEAK_HOLD
/*
 * show peak within past 10 samples
 */
#define HOLDTIME 10
/*
 * defs for min and max
 */
#define max(y,z)	(((y)>(z))?(y):(z))
#define min(y,z)	(((y)<(z))?(y):(z))

/*
 * two dimensional array subscripts for metrics
 * (pretend a malloc buf is a row/column table)
 */
#define	subscript( X, Y )	((X)*dimy+(Y))	/* two dimens. array subscripts */

/*
 * types of plots that can be displayed
 */
#define FULLBARS 1		/* vertical bars			*/
#define SCATTER  2		/* asterisks on points			*/
#define FLOWERPLOT  3		/* bars topped with asterisks		*/
#define CONNECTBARS 4		/* asterisks connected with bars	*/
/*
 * types of screen updates
 */
#define PLOTS		1
#define METS		2
#define PLOT_UPDATE	4
#define ALL	( PLOTS | METS )
/*
 * structure for tracking lwpstats
 */
struct lwpstat {
	int sleep;	/* pr_state == SSLEEP 	'S' 			*/
	int run;	/* pr_state == SRUN 	'R'			*/
	int zombie;	/* pr_state == 0 	'Z'			*/
	int stop;	/* pr_state == SSTOP 	'T' 			*/
	int idle;	/* pr_state == SIDL 	'I' (not fully created)	*/
	int onproc;	/* pr_state == SONPROC	'O'			*/
	int other;	/* pr_state == ????	'?'			*/
	int total;
	int nproc;
};

struct net_total {
	int tcp;
	int udp;
	int icmp;
	int ip;
	int errs;
};

/*
 * the metric table: everything is kept and cooked here
 */
struct metric {
	metid_t id;			/* id number of the metric	*/
	uint32 ndim;			/* number of dimensions		*/
	resource_t reslist[2];		/* resource id numbers		*/
	uint32 action;			/* recipe for cooking		*/
	struct metval {
		float scaleval;	/* scale conversion factor	*/
		caddr_t met_p;		/* ptr to raw metric		*/
		float intv;		/* calculated interval		*/
		float *cooked; 		/* cooked metric history	*/
		union {
			dl_t dbl;	/* double long version		*/
			uint32 sngl;	/* single version		*/
		} met;			/* current metric value		*/
	} *metval;
	uint32 objsz;			/* size of metric in bytes	*/
	uint32 resval[2];		/* values of resources		*/
	char *title;			/* char string for met name	*/
	struct color_range *color;	/* display colors		*/
	int inverted;
}; 
/*
 * field structure, a linked list of screen areas that contain metrics
 * or other selectable items
 */
struct field {
	struct field *nxt;		/* next on list (must be FIRST)	*/
	int row;
	int col;
	int len;
	char *fmt;
	float *met;			/* met to display		*/
	struct metric *mp;		/* struct met for above		*/
	float *met2;			/* met to plot			*/
	struct metric *mp2;		/* struct met for above		*/
	int r1;				/* 1st resource value		*/
	int r2;				/* 2nd resource value		*/
	void (*func)();			/* func to call if subscreen hdr*/
};

/*
 * struct plotdata has information about something we have plotted
 */
struct plotdata {
	struct plotdata *nxt;		/* nxt on list (must be FIRST) */
	int zrow;	/* zero rows */
	int zcol;	/* zero cols */
	int rows;	/* actual rows */
	int cols;	/* actual cols */
	int plotrows;	/* max rows	*/
	int plotcols;	/* max cols	*/
	int barg_flg;	/* this is a bar graph */
	int curx;	/* current x-pos in plot */
	int lastpos;	/* last y-pos: (-2) init val, -1 <==> met was 0.0*/
	int digits;	/* number of digits on y axis */
	float yincr;	/* y axis increment */
	float maxx;	/* max y value seen */
	float yscale;	/* total y-axis scale */
	float *met;	/* ptr to metric data */
	struct metric *mp;	/* ptr to metrics struct */
	int r1;		/* value of first resource */
	int r2;		/* value of second resource */
	int offset;	/* amount to push down in multi col plots */
	char title[256];/* title of the plot, handed to mk_field  */
};
/*
 * functions exported by input.c
 */
void init_keyseq( void );
void do_input( int inch );
int setbar( void );
int push( void );
void quit( void );
int setplot( struct metric *mp, float *met, int r1, int r2 );
/*
 * functions exported by metcook.c
 */
void calc_interval_data( void );
void snap_mets( void );
void calc_intv( struct metric *metp, struct metval *mp, uint32 sz );
void set_oldtime( void );
/*
 * functions exported by mtbl.c
 */
void set_met_titles( void );
void alloc_mets( void );
void set_nmets( void );
void *histalloc( size_t sz );
/*
 * function exported by ether.c
 */
int ether_stat( void );
/*
 * function exported by inet.c
 */
int net_stat( void );
/*
 * function exported by netware.c
 */
void netware_stat( void );
/*
 * function exported by nswap.c
 */
void get_mem_and_swap( void );
/*
 * functions in output.c
 */
struct field *mk_field( struct metric *metp, struct metric *metp2, int r1, int r2, int row, int col, int len, char *fmt, float *met, float *met2, void (*func)() );
void clr_fields( void );
void clr_metfields( void );
int clrplot( void );
void nuke_plot( void );
void print_hdr( int row, int col, resource_t resource, int resval );
void rprint_metric( struct metric *metp, struct metric *metp2, int row, int col, int digits );
void justify( int *roff, int *coff );
void print_mets( void );
void highlight_currfield( void );
void lolight_currfield( void );
int plot(int zrow, int zcol, int plotrows, int plotcols, struct plotdata *pd);
void make_plot_title( struct plotdata *pd );
void make_bar_title( struct plotdata *pd, int col );
void plot_restitle( int res_id, int res_val, char *p );
double setscale( double maxx, int *rows, int plotrows );
int set_format( double yscale, double yincr, char *str );
void plot_point( int zrow, int zcol, int rows, int curx, double met, double yincr, int *lastpos, struct metric *mp, int inverted );
int bar( int row, int col, int plotrows, int plotcols, struct plotdata *pd );
void barline( float *sys, float *usr, double xincr, int row, int col );
void print_uname( void );
void print_plock( void );
void print_time( void );
void print_help_msg( void );
void print_interval( void );
void print_sum(int row, int col );
void print_cputime( int row, int col );
void print_mem( int row, int col );
void print_lwp( int row, int col );
void print_pgin( int row, int col );
void print_pgout( int row, int col );
void print_paging( int row, int col );
void print_syscalls( int row, int col );
void print_queue( int row, int col );
void print_tty( int row, int col );
void print_disk( int row, int col );
void print_fscalls( int row, int col );
void print_igets_dirblks( int row, int col );
void print_inoreclaim( int row, int col );
void print_bufcache( int row, int col );
void print_fstbls( int row, int col );
void print_fs(int row, int col );
void print_ethtraffic( int row, int col );
void print_ethinerr( int row, int col );
void print_ethouterr( int row, int col );
void print_eth(int row, int col );
void print_ip(int row, int col );
void print_tcp(int row, int col );
void print_icmp( int row, int col );
void print_net(int row, int col );
void print_netware(int row, int col );
void print_rip( int row, int col );
void print_ipx( int row, int col );
void print_spx( int row, int col );
void print_spx_snd( int row, int col );
void print_spx_rcv( int row, int col );
void print_spx_msc( int row, int col );
void print_sap( int row, int col );
void print_sap_lan( int row, int col );
void print_ipx_lan( int row, int col );
void print_ipx_sock( int row, int col );
void print_help( int row, int col );
/*
 * functions exported by proc.c
 */
int initproc( void );
int set_ps_option_user( void );
int set_ps_option_sys( void );
int set_ps_option_all( void );
int read_proc( void );
int print_proc( int row, int col );
int scroll_ps( int incr );
int readata( void );
void getdev( void );
void getpasswd( void );
/*
 * functions exported by color.c
 */
void set_metric_color( double met, struct color_range *color, int attr, int plotflg );
void set_message_color( int attr );
void set_default_color( int attr );
void set_plot_color( int attr );
void set_label_color( int attr );
void set_header_color( int attr );
void color_clear( void );
void color_clrtoeol( int row, int col );
void color_clrtobot( int row, int col );
void init_rtpmcolor( void );
/*
 * functions exported by netware.c
 */
int netware_stats( void );
