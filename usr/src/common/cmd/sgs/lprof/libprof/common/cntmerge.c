#ident	"@(#)lprof:libprof/common/cntmerge.c	1.3"

/*
* Routines to merge line profiling data files.
* Called from both runtime and from lprof.
*/

#include <fcntl.h>
#include <pfmt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "dprof.h"
#include "mach_type.h"

int
_CAmapcntf(struct caCNTMAP *cmp, const char *path, int oflag)
{
	struct caHEADER *hdr;
	const char *fmt;
	struct stat st;
	size_t len;
	int fd;

	if ((fd = open(path, oflag)) < 0)
	{
		fmt = "uxcds:1394:can't open file %s\n";
	emit:;
		pfmt(stderr, MM_ERROR, fmt, path);
		fflush(stderr); /* in case stderr is fully buffered */
		return -1;
	}
	if (fstat(fd, &st) < 0)
	{
		fmt = "uxcds:1684:unable to get status for %s\n";
		goto err;
	}
	if ((len = st.st_size) < sizeof(struct caHEADER))
	{
		if (len != 0) /* empty file is a special case */
			goto hderr;
		if ((oflag & (O_RDONLY | O_WRONLY | O_RDWR)) == O_RDONLY)
		{
			fmt = "uxcds:1685:empty profiling file: %s\n";
			goto err;
		}
		cmp->head = 0;
		cmp->cov = 0;
		cmp->aft = 0;
		cmp->nbyte = 0;
		return fd;
	}
	hdr = mmap(0, len, PROT_READ, MAP_SHARED, fd, 0);
	if (hdr == (struct caHEADER *)-1)
	{
		fmt = "uxcds:1686:unable to map file %s\n";
		goto err;
	}
	else if (hdr->size != sizeof(struct caHEADER) || hdr->okay == 0
		|| hdr->mach != MACH_TYPE || hdr->vers != VERSION)
	{
	hderr:;
		fmt = "uxcds:1687:invalid profile file header: %s\n";
	err:;
		close(fd);
		goto emit;
	}
	/*
	* Success.
	*/
	cmp->head = hdr;
	cmp->cov = (caCOVWORD *)&hdr[1];
	cmp->aft = (caCOVWORD *)(len + (char *)hdr);
	cmp->nbyte = len;
	return fd;
}

int
_CAcntmerge(const char *dst, const char *src, int tso)
{
	caCOVWORD *dp, *ndp, *sp, *nsp, *p, *np, *min;
	caCOVWORD nbb, n, ncov;
	struct caCNTMAP dcm, scm;
	const char *fmt;
	char *output;
	int fd, ret;

	if ((fd = _CAmapcntf(&scm, src, O_RDONLY)) < 0)
		return -1;
	close(fd); /* no longer need this one */
	if ((fd = _CAmapcntf(&dcm, dst, O_RDWR)) < 0)
	{
		munmap(scm.head, scm.nbyte);
		return -1;
	}
	ret = 0; /* assume success */
	output = 0; /* for error processing */
	if (dcm.nbyte == 0) /* just copy src over dst */
		goto swap;
	/*
	* Check to see if the headers match each other.
	*/
	if (dcm.head->type != scm.head->type
		|| memcmp(dcm.head->ident, scm.head->ident,
			sizeof(dcm.head->ident)) != 0)
	{
	nomatch:;
		fmt = "uxcds:1688:profiling header mismatch: %s, %s\n";
		goto err;
	}
	if (strncmp(dcm.head->name, scm.head->name,
		sizeof(dcm.head->name)) != 0)
	{
		if (dcm.head->time != scm.head->time)
			goto nomatch;
		pfmt(stderr, MM_INFO, "uxcds:1689:timestamps match, " /*CAT*/
			"but filenames do not: %s, %s\n", dst, src);
		fflush(stderr); /* in case stderr is fully buffered */
		goto ncovcheck;
	}
	if (dcm.head->time != scm.head->time)
	{
		fmt = "uxcds:1690:filenames match, but timestamps do not: %s, %s\n";
		if (!tso)
			goto err;
		pfmt(stderr, MM_INFO, fmt, dst, src);
		fflush(stderr); /* in case stderr is fully buffered */
		/*
		* Arrange it so that dcm refers to the newer one.
		* Whether dcm refers to dst no longer matters.
		*/
		if (dcm.head->time < scm.head->time)
		{
			struct caCNTMAP tmp;

		swap:;
			tmp = dcm;
			dcm = scm;
			scm = tmp;
			if (scm.nbyte == 0) /* just copy src over dst */
			{
				output = (char *)dcm.head;
				goto dowrite;
			}
		}
	}
	else /* names and timestamps match */
	{
	ncovcheck:;
		if (dcm.head->ncov != scm.head->ncov)
		{
			fmt = "uxcds:1691:block counts mismatch: %s, %s\n";
			goto err;
		}
		tso = 0;
	}
	/*
	* The two headers look close enough to go forward with the merge.
	*
	* First allocate space for the merged result.  The result is the
	* same size as dcm, which (if they differ) is the newer.  Similarly,
	* if the input files differ in the number of basic blocks or the
	* function address, we believe the newer one.
	*
	* The coverage data looks like "ncov" instances of the following:
	*	caCOVWORD nbb;
	*	caCOVWORD addr;
	*	caCOVWORD cnt[nbb];
	*	caCOVWORD lno[nbb];
	*/
	if ((output = malloc(dcm.nbyte)) == 0)
	{
		fmt = "uxcds:1692:unable to allocate memory for merge: %s, %s\n";
		goto err;
	}
	memcpy(output, dcm.head, sizeof(struct caHEADER));
	p = (caCOVWORD *)&output[sizeof(struct caHEADER)];
	ncov = dcm.head->ncov;
	if (ncov > scm.head->ncov) /* loop over the minimum */
		ncov = scm.head->ncov;
	for (dp = dcm.cov, sp = scm.cov; ncov != 0; p = np, dp = ndp, sp = nsp)
	{
		ncov--;
		/*
		* Set the pointers to the start of the next coverage set.
		*/
		n = dp[0];
		np = &p[n * 2 + 2];
		ndp = &dp[n * 2 + 2];
		nsp = &sp[sp[0] * 2 + 2];
		/*
		* Check for matching numbers of basic blocks and
		* function addresses.  Set nbb to the minimum
		* number of basic blocks.
		*/
		if ((nbb = n) != sp[0]) /* number of basic blocks differs */
		{
			if (nbb > sp[0]) /* keep nbb the minimum */
				nbb = sp[0];
			goto differ;
		}
		else if (dp[1] != sp[1]) /* function address differs */
		{
		differ:;
			if (!tso)
			{
				fmt = "uxcds:1693:clashing coverage data: %s, %s\n";
				goto err;
			}
		}
		p[0] = n;
		p[1] = dp[1];
		if (n != 0)
		{
			/*
			* First copy all the information that is
			* unchanged by the merge, that is, all the
			* counters past nbb, the number shared
			* between the two files, and the line
			* numbers for these counters.
			*/
			n += n - nbb;
			p += 2;
			dp += 2;
			sp += 2;
			memcpy(&p[nbb], &dp[nbb], n * sizeof(caCOVWORD));
			if (nbb != 0)
			{
				/*
				* Here's what all the fuss was about.
				* Sum the counts for these basic blocks.
				*/
				p += nbb;
				dp += nbb;
				sp += nbb;
				do {
					*--p = *--dp + *--sp;
				} while (--nbb != 0);
			}
		}
	}
	/*
	* If there are more coverage data sets in dcm,
	* copy the rest unchanged.  If there are more in scm,
	* they are ignored, since dcm is the newer.
	*/
	if ((n = dcm.aft - dp) != 0)
		memcpy(p, dp, n * sizeof(caCOVWORD));
	/*
	* Protect this write() from interrupts so that we
	* don't create a "broken" output file.
	*/
dowrite:;
	sighold(SIGINT);
	sighold(SIGHUP);
	sighold(SIGQUIT);
	fmt = 0;
	if (write(fd, output, dcm.nbyte) != dcm.nbyte)
		fmt = "uxcds:1534:cannot write file %s\n";
	sigrelse(SIGINT);
	sigrelse(SIGHUP);
	sigrelse(SIGQUIT);
	if (fmt != 0)
	{
	err:;
		pfmt(stderr, MM_ERROR, fmt, dst, src);
		fflush(stderr); /* in case stderr is fully buffered */
		ret = -1;
	}
	close(fd);
	if (output != (char *)dcm.head)
		free(output);
	munmap(dcm.head, dcm.nbyte);
	if (scm.nbyte != 0)
		munmap(scm.head, scm.nbyte);
	return ret;
}
