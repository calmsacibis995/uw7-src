/*		copyright	"%c%" 	*/

#ident	"@(#)postprint.h	1.2"
#ident	"$Header$"

/*
 *
 * Definitions used by the PostScript translator for ASCII files.
 *
 */

#define LINESPP		66
#define TABSTOPS	8
#define POINTSIZE	10
#define CS0		0
#define CS1		1
#define CS2		2
#define CS3		3
#define YES		1
#define NO		0
#define MAX_LINE	200
#define MAX_STR		150
#define SS2		0x8E
#define SS3		0x8F
/*
 *
 * An array of type Fontmap helps convert font names requested by users into
 * legitimate PostScript names. The array is initialized using FONTMAP, which must
 * end with an entry that has NULL defined as its name field. The only fonts that
 * are guaranteed to work well are the constant width fonts.
 *
 */

/* maybe useful for next release

typedef struct {

	char	*name;			* user's font name *
	char	*val;			* corresponding PostScript name *

} Fontmap;

 */

typedef struct {

	char	*name;			/* user's encoding vector name */
	char	*val;			/* full encoding vector name */

} Encodemap;


/*
 *
 * Some of the non-integer functions in postprint.c.
 *
 */

 /* char	*get_font();  for next release probably */

char 	*get_encode(char *code);
size_t	strlen();
void init_signals(void);
void header(void);
void options(void);
void setup(void);
void arguments(void);
void done(void);
void account(void);
void text(void);
void formfeed(void);
void newline(void);
void spaces(int ch);
void startline(void);
void endline(void);
void oput(int ch);
void sput(int ch);
void redirect(int pg);
void switch_font( int n );
void read_aliases(void);

/*
 *
 * Integer functions in postprint.c.
 *
 */
int out_list();
int saverequest();
int writerequest();
int in_olist();

/*
 *
 * dummy argument to keep lint happy in calls to error()
 *
 */
unsigned dummy;
