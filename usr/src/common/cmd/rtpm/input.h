#ident	"@(#)rtpm:input.h	1.3"

struct keyin {
	int intval;
	void (*func)();
	char *str;
};
/*
 *	input keystrokes
 */

/*	downward cursor motions					*/
#define	 KEYSEQ_DOWN_IDX	0
#define	 KEYSEQ_DOWN	gettxt("RTPM:28", "jJ\016")	/* \016: ^N */

/* 	upward cursor motions					*/
#define	 KEYSEQ_UP_IDX		1
#define	 KEYSEQ_UP	gettxt("RTPM:29", "kK\020")	/* \020: ^P */

/*	left cursor motions					*/
#define	 KEYSEQ_LEFT_IDX	2
#define	 KEYSEQ_LEFT	gettxt("RTPM:30", "hH\002")	/* \002: ^B */

/*	right cursor motions					*/
#define	 KEYSEQ_RIGHT_IDX	3
#define	 KEYSEQ_RIGHT 	gettxt("RTPM:31", "lL\006")	/* \006: ^F */

/*	either plot or push the current screen			*/
#define	 KEYSEQ_PUSH_IDX	4
#define	 KEYSEQ_PUSH	gettxt("RTPM:32", " \n\r")

/*	pop the current screen, returning to the previous one	*/
#define	 KEYSEQ_POP_IDX		5
#define	 KEYSEQ_POP	gettxt("RTPM:33", "\033")		/* \033: Esc */

/*	toggle whether program is locked in memory via plock()	*/
#define	 KEYSEQ_LOCK_IDX	6
#define	 KEYSEQ_LOCK	gettxt("RTPM:34", "xX")

/*	display the help screen					*/
#define	 KEYSEQ_HELP_IDX	7
#define	 KEYSEQ_HELP	gettxt("RTPM:35", "?")

/*	limit processes displayed on the lwp screen		*/
#define	 KEYSEQ_USR_IDX		8
#define	 KEYSEQ_USR	gettxt("RTPM:36", "uU")
#define	 KEYSEQ_SYS_IDX		9
#define	 KEYSEQ_SYS	gettxt("RTPM:37", "sS")
#define	 KEYSEQ_ALL_IDX		10
#define	 KEYSEQ_ALL	gettxt("RTPM:38", "aA")

/*	change the plot display type				*/
#define	 KEYSEQ_PLOT_IDX	11
#define	 KEYSEQ_PLOT	gettxt("RTPM:39", "pP")

/*	scroll the bargraph up and down				*/
#define	 KEYSEQ_SCRLLUP_IDX	12
#define	 KEYSEQ_SCRLLUP	gettxt("RTPM:40", "^")
#define	 KEYSEQ_SCRLLDN_IDX	13
#define	 KEYSEQ_SCRLLDN	gettxt("RTPM:41", "vV")

/*	scroll per-resource metrics left and right		*/
#define	 KEYSEQ_SCRLLFT_IDX	14
#define	 KEYSEQ_SCRLLFT	gettxt("RTPM:42", "<")
#define	 KEYSEQ_SCRLLRT_IDX	15
#define	 KEYSEQ_SCRLLRT	gettxt("RTPM:43", ">")

/*	change the sampling/display interval			*/
#define	 KEYSEQ_INCR_IDX	16
#define	 KEYSEQ_INCR	gettxt("RTPM:44", "+")
#define	 KEYSEQ_DECR_IDX	17
#define	 KEYSEQ_DECR	gettxt("RTPM:45", "-")

/*	clear the bargraph or the oldest plot on the screen	*/
#define	 KEYSEQ_CLR_IDX		18
#define	 KEYSEQ_CLR	gettxt("RTPM:46", "cC")

/*	display the bargraph					*/
#define	 KEYSEQ_BAR_IDX		19
#define	 KEYSEQ_BAR	gettxt("RTPM:47", "bB")

/*	use '_' instead of termattr				*/
#define	 KEYSEQ_UNDERSCORE_IDX	20
#define	 KEYSEQ_UNDERSCORE	gettxt("RTPM:659", "_")

/*	redraw the screen					*/
#define	 KEYSEQ_REDRAW_IDX	21
#define	 KEYSEQ_REDRAW	gettxt("RTPM:48", "\014")	/* \014: ^L */

/*	quit the program					*/
#define	 KEYSEQ_QUIT_IDX	22
#define	 KEYSEQ_QUIT	gettxt("RTPM:49", "qQ\004")	/* \004: ^D */

#define NKEYS	( sizeof( keystrokes ) / sizeof( struct keyin ) )

/*
 * functions called on behalf of user input
 */
void move_down( void );
void move_up( void );
void move_left( void );
void move_right( void );
void toggle_memlock( void );
void plot_or_push( void );
void pop_back( void );
void get_help( void );
void set_user_procs( void );
void set_sys_procs( void );
void set_all_procs( void );
void change_plot_type( void );
void scroll_up( void );
void scroll_down( void );
void scroll_left( void );
void scroll_right( void );
void increment_interval( void );
void decrement_interval( void );
void chg_interval( int incr );
void clear_a_plot( void );
void toggle_bargraph( void );
void redraw_the_screen( void );
void bad_input_keystroke( void );
void toggle_underscore( void );
/*
 * other functions internal to input.c
 */
int pop( void );
int clrbar( void );
void mv_field( int dir );
void push_help( void );
int scroll_met( int incr, int hilight_flag );
void scroll_bar( int incr );
int find_plot( void );
