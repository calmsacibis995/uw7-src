/*		copyright	"%c%" 	*/

#ident	"@(#)unpack:unpack.c	1.32"
#ident "$Header$"
/*
 *	Huffman decompressor
 *	Usage:	pcat filename...
 *	or	unpack filename...
 */

#include <stdio.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <locale.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <libgen.h>

static	get_word();	/* name changed from getwd() due to name conflict */

struct utimbuf {
	time_t	actime;		/* access time */
	time_t	modtime;	/* modification time */
} usertimes;

#ifdef lint
	int	_void_;
#	define VOID	_void_ = (int)
#else
#	define VOID
#endif

jmp_buf env;
struct	stat status;
char	*argvk;
int	rmflg = 0;	/* rmflg, when set it's ok to rm arvk file on caught signals */
long	errorm;

static	const	char
	badread[]    = ":28:Read error: %s",
	badwrite[]   = ":29:Write error: %s";

#define SUF0	'.'
#define SUF1	'z'
#define ZSUFFIX	0x40000
#define US	037
#define RS	036

/* variables associated with i/o */
char	*filename = (char *)NULL;
short	infile;
short	outfile;
short	inleft;
char	*inp;
char	*outp;
char	inbuff[BUFSIZ];
char	outbuff[BUFSIZ];

/* the dictionary */
long	origsize;
short	maxlev;
short	intnodes[25];
char	*tree[25];
char	characters[256];
char	*eof;

/* read in the dictionary portion and build decoding structures */
/* return 1 if successful, 0 otherwise */
getdict ()
{
	register int c, i, nchildren;

	/*
	 * check two-byte header
	 * get size of original file,
	 * get number of levels in maxlev,
	 * get number of leaves on level i in intnodes[i],
	 * set tree[i] to point to leaves for level i
	 */
	eof = &characters[0];

	inbuff[6] = 25;
	inleft = read (infile, &inbuff[0], BUFSIZ);
	if (inleft < 0) {
		eprintf (ZSUFFIX|MM_ERROR, badread, strerror(errno));
		return (0);
	}
	if (inbuff[0] != US)
		goto goof;

	if (inbuff[1] == US) {		/* oldstyle packing */
		if (setjmp (env))
			return (0);
		expand ();
		return (1);
	}
	if (inbuff[1] != RS)
		goto goof;

	inp = &inbuff[2];
	origsize = 0;
	for (i=0; i<4; i++)
		origsize = origsize*256 + ((*inp++) & 0377);
	maxlev = *inp++ & 0377;
	if (maxlev > 24) {
goof:		eprintf (ZSUFFIX|MM_ERROR, ":61:Not in packed format");
		return (0);
	}
	for (i=1; i<=maxlev; i++)
		intnodes[i] = *inp++ & 0377;
	for (i=1; i<=maxlev; i++) {
		tree[i] = eof;
		for (c=intnodes[i]; c>0; c--) {
			if (eof >= &characters[255])
				goto goof;
			*eof++ = *inp++;
		}
	}
	*eof++ = *inp++;
	intnodes[maxlev] += 2;
	inleft -= inp - &inbuff[0];
	if (inleft < 0)
		goto goof;

	/*
	 * convert intnodes[i] to be number of
	 * internal nodes possessed by level i
	 */

	nchildren = 0;
	for (i=maxlev; i>=1; i--) {
		c = intnodes[i];
		intnodes[i] = nchildren /= 2;
		nchildren += c;
	}
	return (decode ());
}

/* unpack the file */
/* return 1 if successful, 0 otherwise */
decode ()
{
	register int bitsleft, c, i;
	int j, lev;
	char *p;

	outp = &outbuff[0];
	lev = 1;
	i = 0;
	while (1) {
		if (inleft <= 0) {
			inleft = read (infile, inp = &inbuff[0], BUFSIZ);
			if (inleft < 0) {
				eprintf (ZSUFFIX|MM_ERROR, badread, strerror(errno));
				return (0);
			}
		}
		if (--inleft < 0) {
uggh:			eprintf (ZSUFFIX|MM_ERROR, ":62:Unpacking error");
			return (0);
		}
		c = *inp++;
		bitsleft = 8;
		while (--bitsleft >= 0) {
			i *= 2;
			if (c & 0200)
				i++;
			c <<= 1;
			if ((j = i - intnodes[lev]) >= 0) {
				p = &tree[lev][j];
				if (p == eof) {
					c = outp - &outbuff[0];
					if (write (outfile, &outbuff[0], c) != c) {
wrerr:						eprintf (MM_ERROR, badwrite,
							strerror(errno));
						return (0);
					}
					origsize -= c;
					if (origsize != 0)
						goto uggh;
					return (1);
				}
				*outp++ = *p;
				if (outp == &outbuff[BUFSIZ]) {
					if (write (outfile, outp = &outbuff[0], BUFSIZ) != BUFSIZ)
						goto wrerr;
					origsize -= BUFSIZ;
				}
				lev = 1;
				i = 0;
			} else
				lev++;
		}
	}
}

main (argc, argv)
	int argc;
	char *argv[];
{
	register i, k;
	int sep, pcat = 0;
	register char *p1, *cp;
	char label[MAXLABEL+1];	/* Space for the catalogue label */
	int fcount = 0;		/* failure count */
	int (*onsig)();
	long name_len;		/* length of file name */
	long name_max;		/* limit on file name length */
	long path_max;		/* limit on path length */

	(void)setlocale (LC_ALL, "");
	(void)setcat("uxdfm");

	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
#ifdef __STDC__
		signal((int)SIGHUP,(void (*)(int)) onsig);
#else
		signal((int)SIGHUP,(void (*)) onsig);
#endif
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
#ifdef __STDC__
		signal((int)SIGINT,(void (*)(int)) onsig);
#else
		signal((int)SIGINT,(void (*)) onsig);
#endif
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
#ifdef __STDC__
		signal((int)SIGTERM,(void (*)(int)) onsig);
#else
		signal(SIGTERM, onsig);
#endif

	(void)setlabel("UX:unpack");

	/*
	 * Determine whether unpack or pcat was invoked
	 */
	p1 = basename(argv[0]);

	if (strcmp(p1, "pcat") == 0) {
		pcat++;
		(void)setlabel("UX:pcat");
	}

	/* skip first argument if it is "--" */
	k=1;
	if (strcmp(argv[k], "--") == 0)
		k++;

	for (; k<argc; k++) {
		/* clear errno; it may have been set in previous iteration */
		errno=0;

		/* Free filename from previous malloc, if needed. */
		if (filename != (char *)NULL) {
			free(filename);
		}

		fcount++;	/* expect the worst */

		errorm = -1;
		sep = 0;
		argvk = argv[k];
		name_len = strlen(argvk);

		/*
		 * Allocate space for filename.  (Add 3: 1 for 
		 * terminating NULL and 2 for ".z".)
		 */
		if ((filename = (char *)malloc(name_len + 3)) == (char *)NULL) {
			pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
			exit(1);
		}

		strcpy(filename, argvk);
		cp = filename;

		/*
		 * Go to end of filename string. Keep up
		 * with the position of the last '/'.
		 */
		i = 0;
		while (*cp != '\0') {
			i++;
			if (*cp++ == '/')
				sep = i;
		}

		/* If no ".z", append it onto filename. */
		if (cp[-1] != SUF1 || cp[-2] != SUF0) {
			*cp++ = SUF0;
			*cp++ = SUF1;
			*cp = '\0';
			name_len = name_len + 2;
		}
		else {
			/* If ".z" is already there, remove it for
			 * argvk; argvk is later used for the resulting
			 * unpacked filename, which will have no ".z".
			 */
			argvk[name_len - 2] = '\0';
		}

		/* Get path length limit. */
		if ((path_max = pathconf(filename, _PC_PATH_MAX)) == -1) {
			pfmt (stderr, MM_NOGET, "%s: %s\n", filename, strerror(errno));
			continue;
		}

		if (name_len > path_max) {
			eprintf (ZSUFFIX|MM_ERROR, ":66:Path name too long");
			goto done;
		}

		/* Get file name length limit. */
		if (((name_max = pathconf(filename, _PC_NAME_MAX)) == -1) && (errno != 0)) {
			pfmt (stderr, MM_NOGET, "%s: %s\n", filename, strerror(errno));
			continue;
		}

		if (name_max == -1) {
			if (creat_check(argvk) == 0) {
				eprintf (ZSUFFIX|MM_ERROR, ":66:Path name too long");
				continue;
			}
		} else {
			if ((name_len - sep) > name_max) {
				eprintf (ZSUFFIX|MM_ERROR, "uxsyserr:81:File name too long");
				goto done;
			}
		}

		if ((infile = open (filename, O_RDONLY)) == -1) {
			eprintf (ZSUFFIX|MM_ERROR, ":64:Cannot open: %s",
				strerror(errno));
			goto done;
		}

		if (pcat)
			outfile = 1;	/* standard output */
		else {
			if (stat (argvk, &status) != -1) {
				eprintf (MM_ERROR, ":38:Already exists");
				goto done;
			}
			VOID fstat (infile, &status);
			if (status.st_nlink != 1)
				eprintf (ZSUFFIX|MM_WARNING,
					":37:File has links");
			if ((outfile = creat (argvk, status.st_mode)) == -1) {
				eprintf (MM_ERROR, ":39:Cannot create: %s",
					strerror(errno));
				goto done;
			}

			rmflg = 1;
		}

		if (getdict ()) {	/* unpack */
			fcount--; 	/* success after all */
			if (!pcat) {
				/*
				 * preserve acc & mod dates
				 */
				usertimes.actime = status.st_atime;
				usertimes.modtime = status.st_mtime;
				if(utime(argvk,&usertimes)!=0)
			    		eprintf(MM_WARNING, 
			    			":49:Cannot change times: %s", strerror (errno));
				if (chmod (argvk, status.st_mode) != 0)
			    		eprintf(MM_WARNING, 
			    			":50:Cannot change mode to %o: %s",
			    			status.st_mode, strerror (errno));
				VOID chown (argvk, status.st_uid, status.st_gid);
				rmflg = 0;
				eprintf (MM_INFO, ":63:Unpacked");
				VOID unlink (filename);

			}
		}
		else
			if (!pcat)
				VOID unlink (argvk);
done:		if (errorm != -1)
			VOID fprintf (stderr, "\n");
		VOID close (infile);
		if (!pcat)
			VOID close (outfile);
	}
	return (fcount);
}

eprintf (flag, s, a1, a2)
	int  flag;
	char *s, *a1, *a2;
{
	int loc_flag = flag & ~ZSUFFIX;
	if (errorm == -1 || errorm != flag) {
		if (errorm != -1)
			fprintf(stderr, "\n");
		errorm = flag;
		pfmt(stderr, (loc_flag | MM_NOGET),
			flag & ZSUFFIX ? "%s.z" : "%s", argvk);
	}
	pfmt(stderr, MM_NOSTD, "uxsyserr:2:: ");
	pfmt(stderr, MM_NOSTD, s, a1, a2);
}

/*
 * This code is for unpacking files that
 * were packed using the previous algorithm.
 */

int	Tree[1024];

expand ()
{
	register tp, bit;
	short word;
	int keysize, i, *t;

	outp = outbuff;
	inp = &inbuff[2];
	inleft -= 2;
	origsize = ((long) (unsigned) get_word ())*256*256;
	origsize += (unsigned) get_word ();
	t = Tree;
	for (keysize = get_word (); keysize--; ) {
		if ((i = getch ()) == 0377)
			*t++ = get_word ();
		else
			*t++ = i & 0377;
	}

	bit = tp = 0;
	for (;;) {
		if (bit <= 0) {
			word = get_word ();
			bit = 16;
		}
		tp += Tree[tp + (word<0)];
		word <<= 1;
		bit--;
		if (Tree[tp] == 0) {
			putch (Tree[tp+1]);
			tp = 0;
			if ((origsize -= 1) == 0) {
				write (outfile, outbuff, outp - outbuff);
				return;
			}
		}
	}
}

getch ()
{
	if (inleft <= 0) {
		inleft = read (infile, inp = inbuff, BUFSIZ);
		if (inleft < 0) {
			eprintf (ZSUFFIX|MM_ERROR, badread, strerror(errno));
			longjmp (env, 1);
		}
	}
	inleft--;
	return (*inp++ & 0377);
}

static
get_word ()
{
	register char c;
	register d;
	c = getch ();
	d = getch ();
	d <<= 8;
	d |= c & 0377;
	return (d);
}

void
onsig()
{
	/* could be running as unpack or pcat	*/
	/* but rmflg is set only when running	*/
	/* as unpack and only when file is	*/
	/* created by unpack and not yet done	*/
	if (rmflg == 1)
		VOID unlink(argvk);
	exit(1);
}

putch (c)
	char c;
{
	register n;

	*outp++ = c;
	if (outp == &outbuff[BUFSIZ]) {
		n = write (outfile, outp = outbuff, BUFSIZ);
		if (n < BUFSIZ) {
			eprintf (MM_ERROR, badwrite, strerror(errno));
			longjmp (env, 2);
		}
	}
}

/* 
 * creat_check accounts for some file systems types (e.g. nfs) where a 
 * file name limit cannot be determined via pathconf.  In this case, we try
 * to create the file,if is doesn't exist, and then read the directory to
 * see if the file is there, or a truncated version (without the Z or without
 * .Z).  If the .Z file name is not there, we return 0.  If the file did
 * not exist before entering this function, it is removed.
 */
creat_check(filename)
char *filename;
{
	FILE *filed;
	DIR *dirp;
	struct dirent *direntp;
	char *basefile;
	int found = 0, exists = 0;

	if ((filed = fopen(filename,"r")) != NULL) 
		exists = 1;		/* file exists in some form */
	else if ((filed = fopen(filename,"a+")) == NULL) 
		return(0);		/* can't create file  	    */

	basefile = basename(filename);
	dirp = opendir(dirname(filename));
	while (((direntp = readdir(dirp)) != NULL) && !found)
		if (strcmp(direntp->d_name,basefile) == 0)
			found = 1;

	closedir(dirp);
	fclose(filed);
	if (exists == 0)
		unlink(filename);

	return(found);
}

