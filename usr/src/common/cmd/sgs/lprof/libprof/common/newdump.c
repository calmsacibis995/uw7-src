#ident	"@(#)lprof:libprof/common/newdump.c	1.6"
/*
* Line profiling runtime support functions.
*/
#include "hidelibc.h"
#include "hidelibelf.h"

#include <fcntl.h>
#include <limits.h>
#include <pfmt.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "dprof.h"
#include "mach_type.h"

static const char tempmsg[] = "uxcds:1694:unable to create temporary filename\n";
static const char covprefix[] = COV_PREFIX;

static char *progpath;		/* a.out's pathname */
static volatile int signo;	/* dumpcnts() got interrupted */

static void
caught(int sig)
{
	signo = sig;
}

static void
erropts(const char *p) /* complain about bad PROFOPTS */
{
	char msg[256]; /* generally long enough */
	int ch, n;
	char *s;

	if ((s = strchr(p, ',')) == 0)
		s = strchr(p, '\0');
	ch = *s;
	*s = '\0'; /* breaking constness here */
	n = snprintf(msg, sizeof(msg),
		gettxt("uxcds:1344", "unrecognized PROFOPTS option: %s"), p);
	*s = ch;
	if (n <= 0)
		n = strlen(msg);
	msg[n] = '\n';
	write(2, msg, n + 1);
}

static char *
skipopts(const char *s, const char *pre, const char **ptr)
{
	const char *p = s;
	int ch;

	/*
	* First byte in pre is known to match the first in p.
	*/
	while ((ch = *++pre) != '\0') {
		if (*++p != ch)
			goto bad;
	}
	if ((ch = *++p) == '\0' || ch == ',') {
	bad:;
		erropts(s);
		return 0;
	}
	*ptr = p;
	if ((s = strchr(p, ',')) == 0)
		s = strchr(p, '\0');
	return (char *)s;
}

static struct options *
profopts(void) /* return structure based on "PROFOPTS" */
{
	static struct options ans = {0, 0, ~0ul};
	const char *p, *s;
	unsigned long fl;
	char *q, **pp;
	size_t n;

	if (ans.flags != ~0ul) /* already have the answer */
		return &ans;
	ans.flags = LPO_VERBOSE;
	if ((p = getenv("PROFOPTS")) == 0) /* use the default settings */
		return &ans;
	if (*p == '\0') {
		ans.flags = LPO_NOPROFILE;
		return &ans;
	}
	for (;;) {
		switch (*p) {
		case '\0':
			return &ans;
		case ' ':
		case '\t':
		case ',':
			p++;
			continue;
		case 'd': /* dir=string */
			s = "dir=";
			pp = &ans.dir;
		str:;
			if ((p = skipopts(p, s, &s)) == 0)
				return &ans;
			if ((q = *pp) != 0)
				free((void *)q);
			n = p - s;
			if ((q = malloc(n + 1)) == 0) {
				perror("line profiling--PROFOPTS allocation");
				return &ans;
			}
			memcpy(q, s, n);
			q[n] = '\0';
			*pp = q;
			continue;
		case 'f': /* file=string */
			s = "file=";
			pp = &ans.file;
			goto str;
		case 'm': /* merge=y/n or msg=y/n */
			if (p[1] == 'e') {
				s = "merge=";
				fl = LPO_MERGE;
			} else if (p[1] == 's') {
				s = "msg=";
				fl = LPO_VERBOSE;
			} else {
				erropts(p);
				return &ans;
			}
		yorn:;
			if ((q = skipopts(p, s, &s)) == 0)
				return &ans;
			if (*s == 'y' || *s == 'Y')
				ans.flags |= fl;
			else if (*s == 'n' || *s == 'N')
				ans.flags &= ~fl;
			else {
				erropts(p);
				return &ans;
			}
			p = q;
			continue;
		case 'p': /* pid=y/n */
			s = "pid=";
			fl = LPO_USEPID;
			goto yorn;
		default:
			erropts(p);
			return &ans;
		}
	}
}

static void
fatal(const char *fmt, const char *arg)
{
	pfmt(stderr, MM_ERROR, fmt, arg);
	fflush(0); /* act like _cleanup() */
	_exit(1);
}

static int
writeout(int fd, const char *start, const char *end)
{
	size_t n = end - start;

	if (write(fd, start, n) != n) {
		pfmt(stderr, MM_ERROR,
			"uxcds:1353:***unable to write profiling data\n");
		fflush(stderr); /* in case stderr is fully buffered */
		return -1;
	}
	return 0;
}

static int
dumpcnts(SOentry *so, const char *cnt) /* write line profiling data */
{
	static const char openfail[] = "uxcds:1394:can't open file %s\n";
	static const char scnfail[] = "uxcds:1695:unable to get section header in %s\n";
	static const char statfail[] = "uxcds:1684:unable to get status for %s\n";
	char outdata[4096], *endp, *outp, *strp;
	struct sigaction intr, hup, quit, new;
	unsigned long nlocal, ncov;
	struct caHEADER hdr;
	char *p, *s, *objf;
	int cfd, efd, ret;
	Elf32_Shdr *eshdp;
	Elf32_Ehdr *ehdp;
	Elf_Data *symtab;
	Elf_Data *strtab;
	Elf32_Sym *symp;
	struct stat st;
	Elf_Scn *escnp;
	Elf_Kind ekind;
	caCOVWORD nbb;
	size_t sz;
	Elf *elfp;

	/*
	* Install signal handler so that we can clean up if
	* interrupted while creating this file.
	*/
	signo = 0;
	sigemptyset(&new.sa_mask);
	new.sa_flags = 0;
	new.sa_handler = caught;
	(void)sigaction(SIGINT, &new, &intr);
	(void)sigaction(SIGHUP, &new, &hup);
	(void)sigaction(SIGQUIT, &new, &quit);
	ret = 0; /* assume success */
	/*
	* Open the object file (a.out or SO), check the elf
	* library version, locate the symbol table, get its
	* contents and that of its matching string table.
	*/
	objf = so->SOpath;
	if (objf[0] == '\0') /* the a.out */
		objf = progpath;
	if ((efd = open(objf, O_RDONLY)) < 0)
		fatal(openfail, objf);
	if (elf_version(EV_CURRENT) == EV_NONE)
		fatal("uxcds:1366:elf library out of date\n", 0);
	if ((elfp = elf_begin(efd, ELF_C_READ, 0)) == 0)
		fatal("uxcds:1696:unable to read (begin) file %s\n", objf);
	if ((ekind = elf_kind(elfp)) != ELF_K_COFF && ekind != ELF_K_ELF)
		goto interrupted; /* not a shared object/a.out */
	if ((ehdp = elf32_getehdr(elfp)) == 0)
		fatal("uxcds:1697:unable to get elf header in %s\n", objf);
	escnp = 0;
	while ((escnp = elf_nextscn(elfp, escnp)) != 0) {
		if (signo != 0)
			goto interrupted;
		if ((eshdp = elf32_getshdr(escnp)) == 0)
			fatal(scnfail, objf);
		if (eshdp->sh_type == SHT_SYMTAB)
			goto gotsymtab;
	}
	fatal("uxcds:1698:cannot find symbol table section in %s\n", objf);
gotsymtab:;
	if ((symtab = elf_getdata(escnp, 0)) == 0)
		fatal("uxcds:1699:unable to get symbol table of %s\n", objf);
	if ((strtab = elf_getdata(elf_getscn(elfp, eshdp->sh_link), 0)) == 0)
		fatal("uxcds:1700:unable to get string table of %s\n", objf);
	if ((cfd = open(cnt, O_WRONLY | O_CREAT | O_TRUNC , 0666)) < 0)
		fatal(openfail, cnt);
	/*
	* Set up the buffering pointers.
	*/
	outp = outdata;
	endp = &outdata[sizeof(outdata)];
	/*
	* Write the count data file header information.
	* It is assumed that the buffer size is sufficient
	* to cover a caHEADER structure.  Note that it is
	* rewritten if all is successful.
	*/
	hdr.size = sizeof(hdr);
	hdr.mach = MACH_TYPE;
	hdr.vers = VERSION;
	hdr.okay = 0;
	memcpy(hdr.ident, ehdp->e_ident, sizeof(hdr.ident));
	hdr.type = ehdp->e_type;
	hdr.ncov = 0;
	if (strlen(objf) >= sizeof(hdr.name) && (s = strrchr(objf, '/')) != 0)
		s++;
	else
		s = objf;
	strncpy(hdr.name, s, sizeof(hdr.name) - 1);
	hdr.name[sizeof(hdr.name) - 1] = '\0';
	hdr.time = 0;
	memcpy(outp, &hdr, sizeof(hdr));
	outp += sizeof(hdr);
	/*
	* Scan through the local symbols looking for data objects
	* with the right prefix, as defined by COV_PREFIX.
	*/
	ncov = 0;
	strp = strtab->d_buf;
	nlocal = eshdp->sh_info;
	for (symp = (Elf32_Sym *)symtab->d_buf; nlocal-- != 0; symp++) {
		if (signo != 0)
			goto interrupted;
		if (symp->st_info != ELF32_ST_INFO(STB_LOCAL, STT_OBJECT))
			continue;
		p = &strp[symp->st_name];
		if (strncmp(p, covprefix, sizeof(covprefix) - 1) != 0)
			continue;
		/*
		* Determine number of basic block counters, while
		* checking the validity of st_size.  The size in
		* bytes is sizeof(caCOVWORD) * (1 + 2 * nbb).
		*/
		sz = symp->st_size;
		if (sz <= sizeof(caCOVWORD))
			continue;
		nbb = sz / sizeof(caCOVWORD);
		if (nbb * sizeof(caCOVWORD) != sz)
			continue;
		if ((nbb & 0x1) == 0)
			continue;
		nbb /= 2;
		/*
		* Write the number of basic blocks and optionally
		* an adjusted address, followed by the rest of the
		* coverage data structure.
		*/
		ncov++;
		p = symp->st_value + (char *)so->baseaddr;
		if (outp + 2 * sizeof(nbb) > endp) {
			if (writeout(cfd, outdata, outp) != 0)
				goto err;
			outp = outdata;
		}
		memcpy(outp, &nbb, sizeof(nbb));
		outp += sizeof(nbb);
		/*
		* For shared libraries, we need to adjust the
		* coverage address to match that specified in
		* the symbol table.  Room for this value in the
		* buffer is guaranteed by the test above.
		*/
		if (so->SOpath[0] != '\0') {
			memcpy(&nbb, p, sizeof(nbb));
			p += sizeof(nbb);
			sz -= sizeof(nbb);
			nbb -= (caCOVWORD)so->baseaddr;
			memcpy(outp, &nbb, sizeof(nbb));
			outp += sizeof(nbb);
		}
		if (sz >= sizeof(outdata)) {
			if (writeout(cfd, outdata, outp) != 0)
				goto err;
			outp = outdata;
			if (writeout(cfd, p, p + sz) != 0)
				goto err;
		} else {
			if (endp - outp < sz) {
				if (writeout(cfd, outdata, outp) != 0)
					goto err;
				outp = outdata;
			}
			memcpy(outp, p, sz);
			outp += sz;
		}
	}
	if (outp > outdata && writeout(cfd, outdata, outp) != 0)
		goto err;
	if (ncov == 0) {
		if (profopts()->flags & LPO_VERBOSE) {
			pfmt(stderr, MM_INFO,
				"uxcds:1701:no coverage data for %s\n", objf);
			fflush(stderr); /* in case stderr is fully buffered */
		}
	interrupted:;
		remove(cnt);
		if (ret == 0)
			ret = 1;
	} else if (ret == 0) {
		/*
		* Update caHEADER to reflect success.
		*/
		hdr.okay = 1;
		hdr.ncov = ncov;
		if (fstat(efd, &st) != 0) {
			remove(cnt);
			fatal(statfail, objf);
		}
		hdr.time = st.st_mtime;
		if (lseek(cfd, 0, SEEK_SET) != 0) {
			remove(cnt);
			fatal("uxcds:1702:unable to rewind file %s\n", cnt);
		}
		if (writeout(cfd, (char *)&hdr, (char *)(1 + &hdr)) != 0) {
		err:;
			ret = -1;
			goto interrupted;
		}
	}
	elf_end(elfp);
	close(efd);
	close(cfd);
	(void)sigaction(SIGINT, &intr, 0);
	(void)sigaction(SIGHUP, &hup, 0);
	(void)sigaction(SIGQUIT, &quit, 0);
	if (signo != 0)
		raise(signo); /* post the signal now that it's safe */
	return ret;
}

static void
inactmerge(SOentry *aso, const char *cnt)
{
	SOentry *so, **sop;

	/*
	* Look for a previously dlclose()d instances of this
	* same SO.  If one exists, merge it into this one.
	* In this fashion, there will only be one "live"
	* output file per unique SO pathname.  Note that this
	* also depends on our SO not yet having been placed
	* on the inactive SO list.
	*/
	for (sop = &_inact_SO;; sop = &so->next_SO) {
		if ((so = *sop) == 0)
			return;
		if (so->tmppath != 0 && strcmp(aso->SOpath, so->SOpath) == 0)
			break;
	}
	if (_CAcntmerge(cnt, so->tmppath, 0) != 0)
		exit(1);
	remove(so->tmppath);
	free(so->tmppath);
	*sop = so->next_SO; /* remove it from list */
	free(so);
}

static ssize_t
mvfile(const char *old, const char *new)
{
	char buf[4096];
	int fdo, fdn;
	ssize_t sz;

	if (rename(old, new) == 0)
		return 0;
	if ((fdo = open(old, O_RDONLY)) < 0)
		return -1;
	if ((fdn = open(new, O_WRONLY)) < 0) {
		close(fdo);
		return -1;
	}
	sighold(SIGQUIT);
	sighold(SIGINT);
	sighold(SIGHUP);
	while ((sz = read(fdo, buf, sizeof(buf))) > 0) {
		if (write(fdn, buf, sz) != sz) {
			remove(new);
			sz = -1;
			break;
		}
	}
	close(fdo);
	close(fdn);
	if (sz >= 0)
		remove(old);
	sigrelse(SIGHUP);
	sigrelse(SIGINT);
	sigrelse(SIGQUIT);
	return sz;
}

static void
realcnts(SOentry *so, struct options *op, const char *tmp)
{
	static const char failed[] =
		"uxcds:1377:*** Unable to put profiling data in `%s'.\n";
	static const char nomerge[] =
		"uxcds:1375:*** Unable to merge results.\n";
	char path[PATH_MAX], *p;
	const char *s, *t;
	int justmade;
	size_t sz;
	int i;

	/*
	* First create the target pathname in path,
	* taking into account the option controls.
	*/
	sz = sizeof(path);
	p = path;
	*p = '\0';
	if (op->dir != 0) {
		i = snprintf(p, sz, "%s/", op->dir);
		if (i > 0) {
			sz -= i;
			p += i;
		}
	}
	if (op->flags & LPO_USEPID) {
		i = snprintf(p, sz, "%d.", getpid());
		if (i > 0) {
			sz -= i;
			p += i;
		}
	}
	if (op->file != 0)
		snprintf(p, sz, "%s", op->file);
	else {
		s = so->SOpath;
		if (*s == '\0') /* the a.out */
			s = progpath;
		if ((t = strrchr(s, '/')) != 0)
			s = t + 1;
		snprintf(p, sz, "%s.cnt", s);
	}
	/*
	* Second, set "t" to refer to our immediate result pathname.
	* If it is different from path, we'll then need to merge.
	*/
	t = path;
	if (op->flags & LPO_MERGE && access(path, R_OK|W_OK|EFF_ONLY_OK) == 0)
		t = tmp;
	/*
	* Cause the count data file to be created, unless we're
	* being called to move an SO's data file into the final
	* location.
	*/
	if (tmp != 0 && tmp == so->tmppath) /* already created */
		justmade = 0;
	else if (dumpcnts(so, t) == 0)
		justmade = 1;
	else
		return; /* dumpcnts() failed */
	/*
	* If "path" is our target, we still need to move if we
	* didn't just make the data file above.  We use the
	* pfmt() format string as our indication of success.
	*/
	s = 0;
	if (t == path) {
		if (justmade || mvfile(tmp, path) == 0)
			s = "uxcds:1376:CNTFILE `%s' created\n";
	} else if (_CAcntmerge(path, tmp, 0) == 0)
		s = "uxcds:1374:CNTFILE `%s' updated\n";
	/*
	* Announce failure or success, as appropriate.  Also, for
	* SOs only, merge any inactive SO count date into this file.
	*/
	if (s == 0) {
		pfmt(stderr, MM_ERROR, t == path ? failed : nomerge, path);
		/*
		* Move the file to a safe place if we just created it
		* because it's in a temporary place.
		*/
		if (justmade) {
			if ((p = tempnam(0, "lxp")) == 0)
				return;
			if (mvfile(t, p) != 0) {
				free(p);
				return;
			}
			tmp = p;
		}
		pfmt(stderr, MM_INFO, "uxcds:1380:Data from this " /*CAT*/
			"execution can be retrieved from `%s'.\n", tmp);
		fflush(stderr); /* in case stderr is fully buffered */
		if (justmade)
			free(p);
	} else {
		if (so->SOpath[0] != '\0' && justmade)
			inactmerge(so, path); /* only for SOs */
		if (op->flags & LPO_VERBOSE) {
			pfmt(stderr, MM_INFO, s, path);
			fflush(stderr); /* in case stderr is fully buffered */
		}
	}
}

void
_CAstartSO(unsigned long low) /* start up line profiling */
{
	extern int etext; /* end of text; type is bogus */
	struct options *op = profopts();
	const char *p, *s, *q;
	char path[PATH_MAX];
	SOentry *so;
	int n;

	if (op->flags & LPO_NOPROFILE)
		return;
	/*
	* Build the initial SO entry.
	*/
	if ((so = malloc(sizeof(SOentry))) == 0) {
		perror("line profiling--shared object allocation");
		/*
		* Disable further profiling.
		*/
	err:;
		op->flags |= LPO_NOPROFILE;
		return;
	}
	so->SOpath = ""; /* denotes the a.out */
	so->tmppath = 0;
	so->tcnt = 0;
	so->prev_SO = 0;
	so->next_SO = 0;
	so->baseaddr = 0; /* unused */
	so->textstart = low;
	so->endaddr = (unsigned long)&etext;
	so->size = 0;
	so->ccnt = 0;
	/*
	* Set progpath to what we believe to be the path to the a.out.
	*/
	if (___Argv == 0 || (p = ___Argv[0]) == 0) {
		perror("line profiling--no name for program");
		goto err;
	}
	if (strchr(p, '/') == 0 && (s = getenv("PATH")) != 0 && *s != '\0') {
		/*
		* Try each directory specified by the nonempty PATH.
		*/
		do {
			if ((q = strchr(s, ':')) == 0)
				n = strlen(s);
			else {
				if ((n = q - s) == 0) {
					s = ".";
					n = 1;
				}
				q++; /* start of next directory */
			}
			n = snprintf(path, sizeof(path), "%.*s/%s", n, s, p);
			if (n > 0 && access(path, EX_OK | EFF_ONLY_OK) == 0) {
				if ((progpath = strdup(path)) == 0) {
					perror("line profiling--" /*CAT*/
						"program path allocation");
					goto err;
				}
				goto out;
			}
		} while ((s = q) != 0);
	}
	/*
	* Otherwise, the original argv[0] name must be it.
	*/
	if (access(p, EX_OK | EFF_ONLY_OK) != 0) {
		perror("line profiling--unable to locate program");
		goto err;
	}
	progpath = (char *)p;
	/*
	* Success.  Make the active list the main SO.
	*/
out:;
	_inact_SO = 0;
	_act_SO = so;
	_last_SO = so;
	_curr_SO = 0;
}

void
_CAstopSO(SOentry *so) /* dump SO line counts to temporary file */
{
	struct options *op = profopts();
	unsigned long flags = op->flags;
	SOentry *nso;

	if (flags & LPO_NOPROFILE)
		return; /* nevermind */
	/*
	* Temporarily turn off any options (esp. LPO_VERBOSE)
	* since we are processing a dlclose()d SO.  They are
	* restored just before we leave.
	*/
	op->flags = 0;
	if ((so->tmppath = tempnam(0, "lp")) == 0)
		fatal(tempmsg, 0);
	if (dumpcnts(so, so->tmppath) == 0)
		inactmerge(so, so->tmppath);
	else {
		free(so->tmppath);
		so->tmppath = 0;
	}
	op->flags = flags;
}

void
_CAnewdump(void) /* shutdown line profiling */
{
	static const char dumpmsg[] =
		"uxcds:1381:Dumping profiling data from process '%s' . . .\n";
	struct options *op = profopts();
	SOentry *so;
	char *tmp;

	if (op->flags & LPO_NOPROFILE)
		return; /* nevermind */
	if (op->flags & LPO_VERBOSE) {
		pfmt(stderr, MM_INFO, dumpmsg, progpath);
		fflush(stderr); /* in case stderr is fully buffered */
	}
	/*
	* When merging is to occur, we'll need a temporary file
	* for the latest data.
	*/
	tmp = 0;
	if (op->flags & LPO_MERGE && (tmp = tempnam(0, "lxp")) == 0)
		fatal(tempmsg, 0);
	/*
	* Produce the data for a.out first.
	* Here we completely follow the options.
	*/
	so = _act_SO;
	if (so->SOpath[0] != '\0')
		fatal("uxcds:1703:internal error: a.out not first object\n", 0);
	realcnts(so, op, tmp);
	/*
	* Now any remaining SOs on the active list.
	* We do not allow overriding the filename here, since that
	* option is only intended for the application.
	*/
	if (op->file != 0) {
		free(op->file);
		op->file = 0;
	}
	while ((so = so->next_SO) != 0)
		realcnts(so, op, tmp);
	if (tmp != 0) {
		remove(tmp);
		free(tmp);
	}
	/*
	* Finally, for any remaining dlclose()d file count data
	* not yet merged into a "real" count file, make them "real".
	*/
	for (so = _inact_SO; so != 0; so = so->next_SO) {
		if (so->tmppath != 0) {
			realcnts(so, op, so->tmppath);
			free(so->tmppath);
		}
	}
}
