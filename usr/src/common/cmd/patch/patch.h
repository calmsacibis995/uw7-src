/*	copyright	"%c%"	*/


#ident	"@(#)patch_p2:patch.h	1.2"

int filemode = 0644;

FILE *ofp = Nullfp;		/* output file pointer */
FILE *rejfp = Nullfp;		/* reject file pointer */

bool using_plan_a = TRUE;	/* try to keep everything in memory */
bool out_of_mem = FALSE;	/* ran out of memory in plan a */

int filec = 0;			/* how many file arguments? */
bool ok_to_create_file = FALSE;
char *bestguess = Nullch;	/* guess at correct filename */

char *outname = Nullch;
char *oopt = Nullch;
int first_append = 1;

char *origprae = Nullch;

char TMPOUTNAME[] = "/tmp/patchoXXXXXX";
char TMPINNAME[] = "/tmp/patchiXXXXXX";	/* might want /usr/tmp here */
char TMPREJNAME[] = "/tmp/patchrXXXXXX";
char TMPPATNAME[] = "/tmp/patchpXXXXXX";
bool toutkeep = FALSE;
bool trejkeep = FALSE;

LINENUM last_offset = 0;
#ifdef DEBUGGING
int debug = 0;
#endif
LINENUM maxfuzz = 2;
bool force = FALSE;
bool verbose = TRUE;
bool reverse = FALSE;
bool noreverse = FALSE;
bool skip_rest_of_patch = FALSE;
int strippath = 957;
bool canonicalize = FALSE;
bool saveorig = FALSE;

int diff_type = 0;
