#ident	"@(#)instlist.c	15.1"

/*
 * This command generates the data for the contents file and writes it to
 * stdout, assigns MAC levels, and sets file privileges.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <search.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <mac.h>
#include <sys/mkdev.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#define PERMCHANGED	1
#define NOEXIST		2
#define WDMSK		0177777L
#define	HTAB_SIZE	40000

struct priv {
	level_t macid;
	char *fix;
	char *inh;
};

struct perm {
	mode_t mode;
	uid_t owner;
	gid_t group;
};

struct check {
	off_t size;
	unsigned sum;
	time_t time;
};

struct line {
	unchar flags;
	char type;
	char *path;
	char *pkgclass;
	char *lpath; /* sl */
	struct perm perm; /* fvxdbce */
	struct priv priv; /* fvxdbce */
	struct check check; /* fve */
	major_t major;
	minor_t minor;
};

static char **List = NULL;
static size_t Lines = 0;
static char *errstr = "instlist: Error:";

int main();
static void to_array(), installf(), removef(), pkgadd(), new_ent(), doadd(),
	prline(), set_file_privs();
static int calc_cksum(), cmp();
static int   owner(),      group();
static char *owner_str(), *group_str();

#ifdef DEBUG
#define mknod 		mymknod
#define chmod 		mychmod
#define chown 		mychown
#define mkdir 		mymkdir
#define mkfifo 		mymkfifo
#define link 		mylink
#define symlink 	mysymlink
#define lvlfile 	mylvlfile

static int
mknod(path, mode, dev)
const char *path;
mode_t mode;
dev_t dev;
{
	fprintf(stderr, "=== Called mknod %s %lo %lu\n", path, mode, dev);
	return 0;
}
static int
chmod(path, mode)
const char *path;
mode_t mode;
{
	fprintf(stderr, "=== Called chmod %s %lo\n", path, mode);
	return 0;
}
static int
chown(path, owner, group)
const char *path;
uid_t owner;
gid_t group;
{
	fprintf(stderr, "=== Called chown %s %ld %ld\n", path, owner, group);
	return 0;
}
static int
mkdir(path, mode)
const char *path;
mode_t mode;
{
	fprintf(stderr, "=== Called mkdir %s %lo\n", path, mode);
	return 0;
}
static int
mkfifo(path, mode)
const char *path;
mode_t mode;
{
	fprintf(stderr, "=== Called mkfifo %s %lo\n", path, mode);
	return 0;
}
static int
link(path1, path2)
const char *path1, *path2;
{
	fprintf(stderr, "=== Called link %s %s\n", path1, path2);
	return 0;
}
static int
symlink(path1, path2)
const char *path1, *path2;
{
	fprintf(stderr, "=== Called symlink %s %s\n", path1, path2);
	return 0;
}
static int
lvlfile(path, cmd, levelp)
const char *path;
int cmd;
level_t *levelp;
{
	fprintf(stderr, "=== Called lvlfile %s %d %lu\n", path, cmd, *levelp);
	return 0;
}
#endif

int
main()
{
	level_t tst_lvl = (level_t)0;
	level_t default_lvl = (level_t)4; /* USER_PUBLIC */
	int lvl_yes;
	ulong j;
	char buf[5120];
	char *array[50];
	struct line *line;
	ENTRY ent, *pent;

	if (lvlfile("/etc/passwd", MAC_GET, &tst_lvl) != 0) {
		fprintf(stderr, "%s lvlfile() to get level failed: %s\n",
		    errstr, strerror(errno));
		return 1;
	}
	errno = 0;
	/* Check to see whether the root file system supports MAC levels. */
	(void) lvlfile("/etc/passwd", MAC_SET, &tst_lvl);
	switch (errno) {
	case 0:
		/*
		 * The root file system supports MAC levels.  Therefore,
		 * set the MAC level on every object whose pkgmap entry or
		 * installf line specifies a level.  Also, set inheritable
		 * privileges on every object whose pkgmap entry or installf
		 * line specifies inheritable privileges.
		 */
		lvl_yes = 1;
		break;
	case ENOSYS:
		/*
		 * The root file system does *not* support MAC levels.
		 * To save installation time, do not set levels on *any*
		 * objects, and do not set *any* inheritable privileges. 
		 * This works because the MAC module and the Least Privilege
		 * Module (LPM) are always installed together by the Enhanced
		 * Security Utilities package.  The implication is that, if MAC
		 * cannot be installed (because the root file system does not
		 * support MAC levels), then LPM cannot be installed.  If
		 * neither MAC nor LPM can be installed, then don't spend time
		 * trying to set MAC levels and setting inheritable privileges.
		 *
		 * If LPM and MAC ever become separately installable, then
		 * this program needs to change.
		 *
		 * Fixed privileges are set unconditionally (as specified in
		 * pkgmap files or installf command lines).
		 */
		lvl_yes = 0;
		break;
	default:
		fprintf(stderr, "%s lvlfile() to set level failed with \
unexpected errno: %s\n", errstr, strerror(errno));
		return 1;
	}

	if (hcreate(HTAB_SIZE) == 0) {
		fprintf(stderr, "%s cannot create hash-table\n", errstr);
		exit(1);
	}
	while(gets(buf)) {
		if (!buf[0])
			continue;
		to_array(buf, array);
		if (strcmp(array[0], "installf") == 0)
			installf(array + 1);
		else if (strcmp(array[0], "removef") == 0)
			removef(array + 1);
		else
			pkgadd(array);
	}
	qsort(List, Lines, sizeof(char *), cmp);

	for (j = 0; j < Lines; j++) {
		ent.key = List[j];
		if (!(pent = hsearch(ent, FIND))) {
			fprintf(stderr, "%s cannot find %s\n", errstr,
			    List[j]);
			continue;
		}
		if (!pent->data)
			continue;
		line = (struct line *) pent->data;
		if (line->flags & NOEXIST) {
			switch (line->type) {
			case 'd':
				mkdir(line->path, line->perm.mode);
				line->flags |= PERMCHANGED;
				break;
			case 'b':
				mknod(line->path, S_IFBLK|line->perm.mode,
				    makedev(line->major, line->minor));
				line->flags |= PERMCHANGED;
				break;
			case 'c':
				mknod(line->path, S_IFCHR|line->perm.mode,
				    makedev(line->major, line->minor));
				line->flags |= PERMCHANGED;
				break;
			case 'p':
				mkfifo(line->path, line->perm.mode);
				line->flags |= PERMCHANGED;
				break;
			case 'l':
				link(line->lpath, line->path);
				/* mode, owner, group not used for links */
				line->flags &= ~(PERMCHANGED);
				break;
			case 's':
				symlink(line->lpath, line->path);
				/* mode, owner, group not used for symlinks */
				line->flags &= ~(PERMCHANGED);
				break;
			default:
				/*
				 * Cannot create, so cannot
				 * change permissions
				 */
				line->flags &= ~(PERMCHANGED);
			}
		}
		if (line->flags & PERMCHANGED) {
			chown(line->path, line->perm.owner, line->perm.group);
			chmod(line->path, line->perm.mode);
		}
		if (line->priv.macid) {
			if (lvl_yes) {
				lvlfile(line->path, MAC_SET, &line->priv.macid);
			}
			set_file_privs(line, lvl_yes);
		} else if (lvl_yes) {
			/*
			 * Set the level to the default only if the object
			 * does not have a level.
			 */
			lvlfile(line->path, MAC_GET, &tst_lvl);
			if (tst_lvl == 0) {
				lvlfile(line->path, MAC_SET, &default_lvl);
			}
		}
		prline(pent->data);
	}
	return lvl_yes ? 0 : 10;
}

static void
to_array(buf, array)
char *buf;
char **array;
{
	int i;

	array[0] = strtok(buf, " =");
	for (i = 1; array[i] = strtok(NULL, " ="); i++)
		;
}

static void
installf(array)
char **array;
{
	ENTRY ent, *pent;
	int i = 0;
	int new;
	int exist;
	int major_given, mode_given;
	struct line *line;
	char *class, *pkg, *lpath;
	struct stat statbuf;
	unsigned cksum;
	register char old_type, new_type;

	if (strcmp(array[0], "-f") == 0)
		return;
	if (strcmp(array[0], "-c") == 0) {
		class = array[1];
		i += 2;
	}
	else
		class = NULL;
	pkg = array[i++];
	ent.key = array[i++];
	exist = lstat(ent.key, &statbuf) == 0;
	if (array[i] && strchr(array[i], '/'))
		lpath = array[i++];
	if (!exist && (!array[i] || strchr("fve", array[i][0]))) {
		/*
		 * The file does not exist, and either the input does not
		 * specify a file type, or the file is type f, v, or e.  We
		 * cannot install such an entry, so just return.
		 */
		fprintf(stderr,
		    "%s %s does not exist -- cannot installf it.\n",
		    errstr, ent.key);
		return;
	}
	if (!(pent = hsearch(ent, FIND)) || !pent->data) {
		new_ent(&pent, &line, ent.key);
		if (!exist)
			line->flags |= NOEXIST;
		if (!array[i]) {
			switch (statbuf.st_mode & S_IFMT) {
			case S_IFREG:
			case S_IFLNK: /* compatible with installf command */
				line->type = 'f';
				break;
			case S_IFCHR:
				line->type = 'c';
				break;
			case S_IFBLK:
				line->type = 'b';
				break;
			}
		}
		else
			line->type = array[i][0];
		new = 1;
	}
	else {
		line = (struct line *) pent->data;
		new = 0;
	}
	if (array[i] && !new) {
		old_type = line->type;
		new_type = array[i][0];
		if (old_type == 'l' && new_type == 'f')
			/*
			 * Input is trying to change the type of a file
			 * from l to f.  We'll ignore the type change
			 * and keep going.
			 */
			lpath = strdup(line->lpath);
		else if (old_type == 'f' && new_type == 'v')
			line->type = 'v';
		else if (old_type == 'v' && new_type == 'f')
			/*
			 * Legal, but ignore.  Once a file is volatile,
			 * it's always volatile.
			 */
			 ;
		else if (old_type != new_type) {
			fprintf(stderr,
			    "%s %s: Illegal file type change from %c to %c.\n",
			    errstr, line->path, old_type, new_type);
			return;
		}
	}
	if (array[i])
		i++;

	doadd(line->pkgclass, pkg, class);

	if (strchr("sl", line->type))
		line->lpath = strdup(lpath);

	if (strchr("bc", line->type)) {
		major_given = 0;
		if (array[i]) {
			if (array[i][0] != '?') {
				major_given = 1;
				line->major = atoi(array[i++]);
				if (array[i])
					line->minor = atoi(array[i++]);
				else {
					line->minor = (minor_t)0;
					fprintf(stderr,
					    "%s No minor number for %s\n",
					    errstr, line->path);
				}
			}
			else
				i += 2;
		}
		if (new && !major_given) {
			line->major = major(statbuf.st_rdev);
			line->major = minor(statbuf.st_rdev);
		}
	}
	if (strchr("fvxdbce", line->type)) {
		mode_given = 0;
		if (array[i]) {
			if (array[i][0] != '?') {
				mode_given = 1;
				line->flags |= PERMCHANGED;
				line->perm.mode = strtoul(array[i++], NULL, 8);
				if (array[i])
					line->perm.owner = owner(array[i++]);
				else {
					line->perm.owner = (uid_t)0;
					fprintf(stderr,
					    "%s No owner for %s\n", errstr,
					    line->path);
				}
				if (array[i])
					line->perm.group = group(array[i++]);
				else {
					line->perm.group = (gid_t)0;
					fprintf(stderr,
					    "%s No group for %s\n", errstr,
					    line->path);
				}
			}
			else
				i += 3;
		}
		if (new && !mode_given) {
			line->perm.mode = statbuf.st_mode &07777;
			line->perm.owner = statbuf.st_uid;
			line->perm.group = statbuf.st_gid;
		}
	}
	if (new && strchr("fve", line->type)) {
		line->check.size = statbuf.st_size;
		line->check.time = statbuf.st_mtime;
		if (calc_cksum(line->path, &cksum) != 0) {
			fprintf(stderr, "%s checksum failed on %s: %s\n",
			    errstr, line->path, strerror(errno));
			line->check.sum = 0;
		} else {
			line->check.sum = cksum;
		}
	}
	if (strchr("fvxdbce", line->type)) {
		if (array[i] && (array[i][0] != '?')) {
			/*
			 * Privileges are always considered "changed",
			 * since the installation software doesn't
			 * do privileges.
			 */
			line->priv.macid = strtoul(array[i++], NULL, 8);
			if (array[i])
				line->priv.fix = strdup(array[i++]);
			else {
				line->priv.fix = "NULL";
				fprintf(stderr,
				    "%s No fixed privs for %s\n", errstr,
				    line->path);
			}
			if (array[i])
				line->priv.inh = strdup(array[i++]);
			else {
				line->priv.inh = "NULL";
				fprintf(stderr,
				    "%s No inheritable privs for %s\n", errstr,
				    line->path);
			}
		}
	}
}

static void
removef(array)
char **array;
{
	ENTRY ent, *pent;
	int i;
	struct line *line;
	char *pkg;
	char *p;
	char pkgclass[50];
	size_t count;

	if (strcmp(array[0], "-f") == 0)
		return;
	pkg = array[0];
	for (i = 1; array[i]; i++) {
		ent.key = array[i];
		if (!(pent = hsearch(ent, FIND)) || !pent->data)
			continue;
		line = (struct line *) pent->data;
		if (strcmp(line->pkgclass, pkg) == 0)
			pent->data = NULL; /* Remove the object */
		else {
			if ((p = strstr(line->pkgclass, pkg)) == NULL)
				/*
				 * If p is NULL, it means that a package is
				 * trying to removef a file that it did not
				 * install.  This is not valid, so we silently
				 * ignore the request.
				 */
				continue;
			if (strchr(p, ' ') == NULL)
				strcpy(pkgclass, p);
			else {
				count = strcspn(p, " ");
				strncpy(pkgclass, p, count);
				pkgclass[count] = '\0';
			}
			if (strcmp(line->pkgclass, pkgclass) == 0) {
				pent->data = NULL; /* Remove the object */
				continue;
			}
			/* Remove this pkgclass from the list */
			if (p == line->pkgclass)
				/* p is at beginning of line->pkgclass */
				strcpy(p, p + strlen(pkgclass) + 1);
			else
				/* p is at middle or end of line->pkgclass */
				strcpy(p - 1, p + strlen(pkgclass));
		}
	}
}

static void
pkgadd(array)
char **array;
{
	ENTRY ent, *pent;
	int i = 0;
	char *lpath;
	char *class;
	register char old_type, new_type;
	struct line *line;

	ent.key = array[i++];
	if (strchr(array[i], '/'))
		lpath = array[i++];
	if (!(pent = hsearch(ent, FIND)) || !pent->data) {
		new_ent(&pent, &line, ent.key);
		line->type = array[i][0];
	}
	else {
		line = (struct line *) pent->data;
		old_type = line->type;
		new_type = array[i][0];
		if (old_type == 'l' && new_type == 'f')
			/*
			 * Input is trying to change the type of a file
			 * from l to f.  We'll ignore the type change
			 * and keep going.
			 */
			lpath = strdup(line->lpath);
		else if (old_type == 'f' && new_type == 'v')
			line->type = 'v';
		else if (old_type == 'v' && new_type == 'f')
			/*
			 * Legal, but ignore.  Once a file is volatile,
			 * it's always volatile.
			 */
			 ;
		else if (old_type != new_type) {
			fprintf(stderr,
			    "%s %s: Illegal file type change from %c to %c.\n",
			    errstr, line->path, old_type, new_type);
			return;
		}
	}
	i++;
	class = array[i++];
	if (strchr("sl", line->type))
		line->lpath = strdup(lpath);
	if (array[i] && (array[i][0] != '?') && strchr("bc", line->type)) {
		line->major = atoi(array[i++]);
		line->minor = atoi(array[i++]);
	}
	if (array[i] && (array[i][0] != '?') && strchr("fvxdbce", line->type)) {
		line->perm.mode = strtoul(array[i++], NULL, 8);
		line->perm.owner = owner(array[i++]);
		line->perm.group = group(array[i++]);
	}
	if (array[i] && (array[i][0] != '?') && strchr("fve", line->type)) {
		line->check.size = atoi(array[i++]);
		line->check.sum = atoi(array[i++]);
		line->check.time = atoi(array[i++]);
	}
	if (array[i] && isdigit(array[i][0]) && strchr("fvxdbce", line->type)) {
		line->priv.macid = strtoul(array[i++], NULL, 8);
		line->priv.fix = strdup(array[i++]);
		line->priv.inh = strdup(array[i++]);
	}
	/* packages */
	if (array[i]) {
		while (array[i]) {
			if ((array[i][0] == '+') || (array[i][0] == '-'))
				array[i]++;
			doadd(line->pkgclass, array[i++], class);
		}
	}
}

static void
set_file_privs(line, lvl_yes)
struct line *line;
int lvl_yes;
{
	int i = 0;
#ifdef DEBUG
	int j;
#endif
	int fix_yes, inh_yes;
	char *filepriv_argv[7];

	if (line->priv.fix[0] == '?' && line->priv.inh[0] == '?')
		return;
	if (line->priv.fix[0] == '?' || line->priv.inh[0] == '?') {
		fprintf(stderr,
		    "%s %s:\n\tCannot specify '?' for one privilege and not \
the other.\n",
		    errstr, line->path);
		return;
	}
	fix_yes = (strcmp(line->priv.fix, "NULL") != 0);
	inh_yes = lvl_yes ? (strcmp(line->priv.inh, "NULL") != 0) : 0;
	if (!fix_yes && !inh_yes)
		return;
	else {
		filepriv_argv[i++] = "filepriv";
		if (fix_yes) {
			filepriv_argv[i++] = "-f";
			filepriv_argv[i++] = line->priv.fix;
		}
		if (inh_yes) {
			filepriv_argv[i++] = "-i";
			filepriv_argv[i++] = line->priv.inh;
		}
	}
	filepriv_argv[i++] = line->path;
	filepriv_argv[i] = NULL;
#ifdef DEBUG
	fprintf(stderr, "=== Called ");
	for (j = 0; j < i; j++)
		fprintf(stderr, " %s", filepriv_argv[j]);
	fputc('\n', stderr);
#else
	if (fork() == 0)
		(void) execv("/sbin/filepriv", filepriv_argv);
	else
		(void) wait((int *)0);
#endif
	return;
}

static void
prline(line)
struct line *line;
{
	char *class;
	char privbuf[300];

	if (class = strchr(line->pkgclass, ':')) {
		if (strchr(class + 1, ' '))
			class = "none";
		else {
			*class = '\0';
			class++;
		}
	}
	else
		class = "none";
	if (line->priv.macid)
		sprintf(privbuf, "%lu %s %s ", line->priv.macid,
		    line->priv.fix, line->priv.inh);
	else
		privbuf[0] = '\0';
	switch (line->type) {
	case 'e':
	case 'f':
	case 'v':
		printf("%s %c %s %4.4lo %s %s %lu %u %lu %s%s\n", line->path,
		    line->type, class, line->perm.mode,
		    owner_str(line->perm.owner), group_str(line->perm.group),
		    line->check.size, line->check.sum, line->check.time,
		    privbuf, line->pkgclass);
		break;
	case 'd':
	case 'x':
	case 'p':
		printf("%s %c %s %4.4lo %s %s %s%s\n", line->path, line->type,
		    class, line->perm.mode, owner_str(line->perm.owner),
		    group_str(line->perm.group), privbuf, line->pkgclass);
		break;
	case 'l':
	case 's':
		printf("%s=%s %c %s %s\n", line->path, line->lpath, line->type,
		    class, line->pkgclass);
		break;
	case 'b':
	case 'c':
		printf("%s %c %s %lu %lu %4.4lo %s %s %s%s\n", line->path,
		    line->type, class, line->major, line->minor,
		    line->perm.mode, owner_str(line->perm.owner),
		    group_str(line->perm.group), privbuf, line->pkgclass);
		break;
	default:
		fprintf(stderr, "%s unknown file type %c: %s\n", errstr,
		    line->type ? line->type : '?', line->path);
	}
}

/* Create a new entry */
static void
new_ent(ppent, pline, path)
ENTRY **ppent;
struct line **pline;
char *path;
{
	if (!*ppent)
		*ppent = (ENTRY *) malloc(sizeof(ENTRY));
	memset(*ppent, '\0', sizeof(ENTRY));
	(*ppent)->key = strdup(path);
	(*ppent)->data = (void *) malloc(sizeof(struct line));
	*pline = (struct line *) (*ppent)->data;
	memset(*pline, '\0', sizeof(struct line));
	(*pline)->path = (*ppent)->key;
	(*pline)->pkgclass = (char *) malloc(200);
	(*pline)->pkgclass[0] = '\0';
	if (hsearch(**ppent, ENTER) == NULL) {
		fprintf(stderr, "%s hash-table is full (%s)\n", errstr, path);
	}
	if (Lines % 512 == 0)
		List = (char **) realloc(List, (Lines + 512) * sizeof(char *));
	List[Lines++] = (*ppent)->key;
}

static void
doadd(orig, pkg, class)
char *orig, *pkg, *class;
{
	char pkgclass[200];
	char *p;
	int len;

	strcpy(pkgclass, orig);
	if (orig[0]) {
		len = strlen(pkg);
		for (p = strtok(pkgclass, " "); p; p = strtok(NULL, " ")) {
			switch (p[len]) {
			case '\0':
			case ':':
				if (strncmp(p, pkg, len) == 0)
					return;
			}
		}
		strcat(orig, " ");
	}
	strcat(orig, pkg);
	if (class && (strcmp(class, "none") != 0)) {
		strcat(orig, ":");
		strcat(orig, class);
	}
}

static int
calc_cksum(path, cksum)
char *path;
unsigned *cksum;
{
	struct part {
		short unsigned hi, lo;
	};
	static union hilo {
		/* this only works if short is 1/2 of long */
		struct part hl;
		long	lg;
	} tempa, suma;
	int ca;
	FILE *fp;
	unsigned sum;
	char *actkey;

	if ((fp = fopen(path, "r")) == NULL)
		return 1;

	sum = 0;
	suma.lg = 0;

	if ((actkey = getenv("ACTKEY")) != NULL && strcmp(actkey, "YES") == 0)
	{
		while((ca = getc(fp)) != EOF) {
			if (sum & 01)
				sum = (sum >> 1) + 0x8000;
			else
				sum >>= 1;
			sum += ca;
			sum &= 0xffff;
		}

		*cksum = sum;
	}
	else
	{
		while((ca = getc(fp)) != EOF)
			suma.lg += ca & WDMSK;
		tempa.lg = (suma.hl.lo & WDMSK) + (suma.hl.hi & WDMSK);
		*cksum = tempa.hl.hi + tempa.hl.lo;
	}

	(void) fclose(fp);

	return 0;
}

static int
owner(str)
char *str;
{
	struct passwd *pswd;

	pswd = getpwnam(str);
	return pswd == NULL ? 0 : (int)pswd->pw_uid;
}

static int
group(str)
char *str;
{
	struct group *grp;

	grp = getgrnam(str);
	return grp == NULL ? 0 : (int)grp->gr_gid;
}

static char *
owner_str(id)
uid_t id;
{
	struct passwd *pswd;

	pswd = getpwuid(id);
	return pswd == NULL ? NULL : pswd->pw_name;
}

static char *
group_str(id)
gid_t id;
{
	struct group *grp;

	grp = getgrgid(id);
	return grp == NULL ? NULL : grp->gr_name;
}

static int
cmp(pstr1, pstr2)
const void *pstr1, *pstr2;
{
	return strcmp(*(char **)pstr1, *(char **)pstr2);
}
