/*		copyright	"%c%" 	*/

#ident	"@(#)uusched.c	1.2"
#ident "$Header$"

#include	"uucp.h"

#define USAGE	"[-xNUM] [-uNUM]"
#define MAXGRADE	52

struct m {
	char	mach[MAXBASENAME+1];
	char	jgrade[MAXGRADE+1];
	struct m *prev;
	struct m *next;
} *M = NULL;

int Mcount = 0;

short Uopt;
void cleanup(), exuucico();

void logent(){}		/* to load ulockf.c */

main(argc, argv, envp)
char *argv[];
char **envp;
{
	struct m *m;
	void machine();
	DIR *spooldir, *subdir, *gradedir;
	char f[256], g[256], fg[256], subf[256];
	int numgrade;
	char *gradelist;
	short num, snumber;
	char lckname[MAXFULLNAME];
	struct limits limitval;
	int i, maxnumb;
	FILE *fp;

	Uopt = 0;
	Env = envp;

	(void) strcpy(Progname, "uusched");
	while ((i = getopt(argc, argv, "u:x:")) != EOF) {
		switch(i){
		case 'x':
			Debug = atoi(optarg);
			if (Debug <= 0) {
				fprintf(stderr,
				"WARNING: %s: invalid debug level %s ignored, using level 1\n",
				Progname, optarg);
				Debug = 1;
			}
#ifdef SMALL
			fprintf(stderr,
			"WARNING: uusched built with SMALL flag defined -- no debug info available\n");
#endif /* SMALL */
			break;
		case 'u':
			Uopt = atoi(optarg);
			if (Uopt <= 0) {
				fprintf(stderr,
				"WARNING: %s: invalid debug level %s ignored, using level 1\n",
				Progname, optarg);
				Uopt = 1;
			}
			break;
		default:
			(void) fprintf(stderr, "\tusage: %s %s\n",
			    Progname, USAGE);
			cleanup(1);
		}
	}
	if (argc != optind) {
		(void) fprintf(stderr, "\tusage: %s %s\n", Progname, USAGE);
		cleanup(1);
	}

	DEBUG(9, "Progname (%s): STARTED\n", Progname);
	if (scanlimit("uusched", &limitval) == FAIL) {
	    DEBUG(1, "No limits for uusched in %s\n", LIMITS);
	} else {
	    maxnumb = limitval.totalmax;
	    if (maxnumb < 0) {
		DEBUG(4, "Non-positive limit for uusched in %s\n", LIMITS);
		DEBUG(1, "No limits for uusched\n%s", "");
	    } else {
		DEBUG(4, "Uusched limit %d -- ", maxnumb);

		for (i=0; i<maxnumb; i++) {
		    (void) sprintf(lckname, "%s.%d", S_LOCK, i);
		    if ( mklock(lckname) == SUCCESS )
			break;
		}
		if (i == maxnumb) {
		    DEBUG(4, "found %d -- cleaning up\n ", maxnumb);
		    cleanup(0);
		}
		DEBUG(4, "continuing\n", maxnumb);
	    }
	}

	if (chdir(SPOOL) != 0 || (spooldir = opendir(SPOOL)) == NULL)
		cleanup(101);		/* good old code 101 */
	while (gdirf(spooldir, f, SPOOL) == TRUE) {
	    subdir = opendir(f);
	    ASSERT(subdir != NULL, Ct_OPEN, f, errno);
	    while (gdirf(subdir, g, f) == TRUE) {
		(void) sprintf(fg, "%s/%s", f, g);
		gradedir = opendir(fg);
		ASSERT(gradedir != NULL, Ct_OPEN, g, errno);
		while (gnamef(gradedir, subf) == TRUE) {
		    if (subf[1] == '.') {
		        if (subf[0] == CMDPRE) {
			    /* Note - we can break now, since we
			     * have found a job grade with at least
			     * one C. file.
			    */
			    machine(f, g, 0L);
			    break;
			}
		    }
		}
		closedir(gradedir);
	    }
	    closedir(subdir);
	}

	DEBUG(5, "Execute num=%d \n", Mcount);

	while (Mcount > 0) {
	    snumber = (time((long *) 0) % (long) (Mcount));  /* random number */

	    for (i=snumber, m=M; i > 0; i--)
		m = m->next;

	    DEBUG(5, "number=%d, ", Mcount);
	    DEBUG(5, "entry=%d, ", snumber);
	    DEBUG(5, "remote=%s, ", m->mach);
	    DEBUG(5, "job grade list=%s\n", m->jgrade);

	    numgrade = strlen(m->jgrade);

	    for (i=0; i<numgrade; i++) {
		(void) sprintf(lckname, "%s.%s.%c", LOCKPRE, m->mach, m->jgrade[i]);
		/* if any grade is locked, move on to next machine */
		if (cklock(lckname) != SUCCESS)
			break;

		/* if we can't call now, move on to the next machine */
		if (callok(m->mach) != 0)
			break;

		/* if this is the last grade, then call uucico */
		if (i == numgrade - 1) {
		    DEBUG(5, "call exuucico(%s)\n", m->mach);
		    exuucico(m);
		}
	    }
	    
	    Mcount--;
	    if (m->next)
		m->next->prev = m->prev;
	    if (m->prev)
		m->prev->next = m->next;
	    else
		M = m->next;
	    (void) free(m);
	
	}

	cleanup(0);

}

void
machine(name, grade)
char	*name;
char	*grade;
{
	struct m *m;
	size_t	namelen;

	DEBUG(9, "machine(%s) called\n", name);

	for (m = M; m != NULL; m = m->next) {
		/* match on overlap? */
		if (EQUALSN(name, m->mach, MAXBASENAME)) {
			/* use longest name */
			if (strlen(name) > strlen(m->mach))
				(void) strncpy(m->mach, name, MAXBASENAME);
			m->jgrade[strlen(m->jgrade)] = *grade;
			break;
		}
	}

	if (m == NULL) { /* no entry, so allocate a new one */
		m = calloc((size_t) 1, (size_t) sizeof(struct m));
		if (m == NULL) {
			errent("MACHINE TABLE ENTRY LOST", "", UUSTAT_TBL,
				__FILE__, __LINE__);
		} else { /* put the new entry at the head of the list */
			if (M)
				M->prev = m;
			m->next = M;
			M = m;
			Mcount++;
			(void) strncpy(m->mach, name, MAXBASENAME);
			m->jgrade[0] = *grade;
		}
	}

	return;
}

void
exuucico(m)
struct m *m;
{
	char cmd[BUFSIZ];
	int status;
	pid_t pid, ret;
	char uopt[5];
	char sopt[BUFSIZ];

	(void) sprintf(sopt, "-s%s", m->mach);
	if (Uopt)
	    (void) sprintf(uopt, "-x%.1d", Uopt);

	if ((pid = vfork()) == 0) {
	    if (Uopt)
	        (void) execle(UUCICO, "UUCICO", "-r1", uopt, sopt, (char *) 0, Env);
	    else
	        (void) execle(UUCICO, "UUCICO", "-r1", sopt, (char *) 0, Env);

	    cleanup(100);
	}
	while ((ret = wait(&status)) != pid)
	    if (ret == -1 && errno != EINTR)
		break;

	DEBUG(3, "ret=%ld, ", (ret == pid ? (long) status : (long) ret));
	return;
}


void
cleanup(code)
int	code;
{
	rmlock(CNULL);
	exit(code);
}
