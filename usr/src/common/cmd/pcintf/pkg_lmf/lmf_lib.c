#ident	"@(#)pcintf:pkg_lmf/lmf_lib.c	1.1.1.3"
/* SCCSID(@(#)lmf_lib.c	7.5	LCC)	/* Modified: 16:56:28 3/9/92 */

/*
 *  LMF Message File Library
 */

#include <fcntl.h>
#include <string.h>
#ifdef MSDOS
#include <dos.h>
#endif
#include "lmf.h"
#include "lmf_int.h"


int lmf_errno;			/* Error information */
int lmf_message_length;		/* Length of message from lmf_get_message */


struct msg_file {
	struct msg_file	       *mf_next;	/* next open file */
	int			mf_fd;		/* file descriptor */
	struct lmf_file_header	mf_hdr;		/* LMF file header */
};

static struct msg_file *first_msg_file = NULL;


struct domain {
	struct domain	*d_next;		/* next domain */
	struct domain	*d_stack;		/* next domain in stack */
	struct msg_file	*d_file;		/* file where stored */
	int		 d_attr;		/* attributes of domain */
	char		*d_data;		/* domain contents */
	int		 d_length;		/* length of contents */
	char		 d_name[1];		/* full name of domain */
};

#define DA_ROOT		0x0001		/* domain is the root */
#define DA_FAST		0x0002		/* domain is fast */
#define DA_DUMMY	0x0004		/* domain is a dummy */

static struct domain *domain_cache = NULL;
static struct domain *current_domain = NULL;

static int flipBytes = 0;		/* non-zero = flip */

static char *buf = NULL;		/* buffer for library use  allocated */
static int   buf_len = 0;		/* size of current buffer */
char *lmf_get_buffer();		/* function to get a buffer */

char *malloc();			/* default declaration of malloc */


/*
 *  int lmf_open_file(file, default_lang, default_nlspath)
 *	char *file, *default_lang, *default_nlspath;
 *
 *	returns  n  A message file handle which can be closed using the
 *			lmf_close_file function.
 *		-1  An error occured.  Errors returned in lmf_errno:
 *		    LMF_ERR_NOMEM    No memory available.
 *		    LMF_ERR_NOFILE   The message file couldn't be found or
 *					opened.
 *		    LMF_ERR_BADFILE  The message file is not in the correct
 *					format.
 *
 *	This function opens a message file found using the file parameter,
 *	the LANG environment variable, and the NLSPATH environment variable.
 *
 *	The LANG variable defines what the current language is.  It is of the
 *	form language[_territory].
 *
 *	The NLSPATH variable is used to define the path where the message
 *	file is stored.  It expands the fields %L for the LANG variable,
 *	and %N for the file parameter.  On unix, multiple potential paths
 *	may be specified by seperating them with colons.
 *
 *	The default parameters are used if the environment variables are not
 *	found.  If they are NULL, the function fails.
 *
 *	The environment variables are compatible with the XOPEN standard.
 *
 *	Multiple message files may be opened if the domains stored inside
 *	them do not collide with other message files.
 *
 *	If a message file cannot be opened using the lang and nlspath
 *	as defined in the environment, then try opening a message file
 *	based on the defaults.
 *
 *	Calls "lmf_open_thisfile()" to do the actual opening.
 */

int
lmf_open_file(file, default_lang, default_nlspath)
char *file, *default_lang, *default_nlspath;
{
	int fd;
	char *lang, *nlspath;
	char *getenv();

	/* determine machine byte ordering */
	flipBytes = lmf_byteorder();

	if ((lang = getenv("LANG")) == NULL)
		lang = default_lang;
	if ((nlspath = getenv("NLSPATH")) == NULL)
		nlspath = default_nlspath;

	if ((fd = lmf_open_thisfile(file, lang, nlspath)) != -1)
		return fd;
	/* That didn't work, try another combination */
	if (nlspath != default_nlspath) {
		/* Try default nlspath */
		if ((fd = lmf_open_thisfile(file, lang, default_nlspath)) != -1)
			return fd;
	}
	if (lang != default_lang) {
		/* Try default lang */
		if ((fd = lmf_open_thisfile(file, default_lang, nlspath)) != -1)
			return fd;
	}
	/* Last try: use all defaults. */
	if ((fd = lmf_open_thisfile(file, default_lang, default_nlspath)) != -1)
		return fd;
	return -1;
}

/*
 *  int lmf_open_thisfile(file, lang, nlspath)
 *	char *file, *lang, *default_nlspath;
 *
 *	returns  n  A message file handle which can be closed using the
 *			lmf_close_file function.
 *		-1  An error occured.  Errors returned in lmf_errno:
 *		    LMF_ERR_NOMEM    No memory available.
 *		    LMF_ERR_NOFILE   The message file couldn't be found or
 *					opened.
 *		    LMF_ERR_BADFILE  The message file is not in the correct
 *					format.
 *
 */

int
lmf_open_thisfile(file, lang, nlspath)
char *file;
char *lang;
char *nlspath;
{
	char *code_ptr = NULL;
	char *dot_ptr;
	register char *bp, *pp;
	struct msg_file *mf;
	int fd;

#ifdef MSDOS
	char cp_buf[6];	/* for dos code page */

	sprintf(cp_buf, "%03d", dos_codepage());
	code_ptr = cp_buf;
#endif
	if ((dot_ptr = strrchr(lang, '.')) != NULL) {
		/* Our code set is just beyond the '.' */
		code_ptr = dot_ptr + 1;
		*dot_ptr = '\0'; /* NULL out the '.' */
	}
	bp = lmf_get_buffer(1024);
	for (pp = nlspath; *pp; pp++) {
		if (*pp == '%') {
			if (*++pp == 'L')
				strcpy(bp, lang);
			else if (*pp == 'N')
				strcpy(bp, file);
			else if (*pp == 'C' && code_ptr != NULL)
				strcpy(bp, code_ptr);
			else if (*pp == 'C')
				strcpy(bp, "8859");
			else
				*bp = '\0';
			bp = &bp[strlen(bp)];
		} else
			*bp++ = *pp;
	}
#ifdef MSDOS
	*bp++ = ';';
#else
	*bp++ = ':';
#endif
	for (pp = nlspath; *pp; pp++) {
		if (*pp == '%') {
			if (*++pp == 'L')
				strcpy(bp, lang);
			else if (*pp == 'N')
				strcpy(bp, file);
			else if (*pp == 'C')
				strcpy(bp, "lmf");
			else
				*bp = '\0';
			bp = &bp[strlen(bp)];
		} else
			*bp++ = *pp;
	}
	*bp = '\0';

	if ( dot_ptr != NULL) /* we did match  and NULL out the '.' in LANG */
		*dot_ptr = '.';	/* fix LANG up */

	if ((mf = (struct msg_file *)malloc(sizeof(struct msg_file))) == NULL) {
		lmf_errno = LMF_ERR_NOMEM;
		return -1;
	}
	fd = -1;
	nlspath = buf;
	for (bp = nlspath; *bp; bp++) {
#ifdef MSDOS
		if (*bp != ';')
#else
		if (*bp != ':')
#endif
			continue;
		*bp++ = '\0';
		if ((fd = open(nlspath, O_RDONLY|O_BINARY)) >= 0)
			break;
		nlspath = bp;
	}
	if (fd == -1 && (fd = open(nlspath, O_RDONLY|O_BINARY)) < 0) {
		free(mf);
		lmf_errno = LMF_ERR_NOFILE;
		return -1;
	}
	if (read(fd, &mf->mf_hdr, sizeof(struct lmf_file_header)) !=
		sizeof(struct lmf_file_header) ||
	    strcmp(mf->mf_hdr.lmfh_magic, "LMF") != 0) {
		close(fd);
		free(mf);
		lmf_errno = LMF_ERR_BADFILE;
		return -1;
	}
	if (flipBytes) {
	    sflip(mf->mf_hdr.lmfh_base_length);
	    lflip(mf->mf_hdr.lmfh_base_offset);
	}
	mf->mf_fd = fd;
	mf->mf_next = first_msg_file;
	first_msg_file = mf;
	return fd;
}


/*
 *  int lmf_close_file(lmf_file_handle)
 *	int lmf_file_handle;
 *
 *	This function closes a message file opened with lmf_open_file.
 */

int
lmf_close_file(fd)
int fd;
{
	struct msg_file *mf, *m;
	struct domain *dp, *ldp, *ndp;

	if (first_msg_file == NULL) {
		lmf_errno = LMF_ERR_NOFILE;
		return -1;
	}
	if (first_msg_file->mf_fd == fd) {
		mf = first_msg_file;
		first_msg_file = mf->mf_next;
	} else {
		for (m = first_msg_file;
		     m->mf_next && m->mf_next->mf_fd != fd;
		     m = m->mf_next)
			;
		if (m->mf_next == NULL) {
			lmf_errno = LMF_ERR_NOFILE;
			return -1;
		}
		mf = m->mf_next;
		m->mf_next = mf->mf_next;
	}

	/*
	 *  Free cached domains associated with this message file
	 */
	for (ldp = dp = current_domain; dp; dp = ndp) {
		ndp = dp->d_stack;
		if (dp->d_file == mf) {
			if (dp == current_domain)
				current_domain = ndp;
			else
				ldp->d_stack = ndp;
			if (!(dp->d_attr & DA_FAST)) {
				if (!(dp->d_attr & (DA_DUMMY|DA_ROOT)))
					free(dp->d_data);
				free(dp);
			}
		} else
			ldp = dp;
	}
	for (ldp = dp = domain_cache; dp; dp = ndp) {
		ndp = dp->d_next;
		if (dp->d_file == mf) {
			if (dp == domain_cache)
				domain_cache = ndp;
			else
				ldp->d_next = ndp;
			free(dp->d_data);
			free(dp);
		} else
			ldp = dp;
	}

	close(mf->mf_fd);
	free(mf);
	return 0;
}


/*
 *  int lmf_push_domain(domain)
 *	char *domain;
 *
 *	Returns  0  Success.
 *		-1  Error.  Errors returned in lmf_errno:
 *		    LMF_ERR_NOMEM	No memory available.
 *		    LMF_ERR_NOMSGFILE	No message file open.
 *		    LMF_ERR_NOTFOUND	The specified domain is not stored in
 *					an open message file.
 *		    LMF_ERR_BADFILE	The message file is corrupted.
 *
 *	This function is used to set the current message domain.  All subsequent
 *	lmf_get_message calls will prepend domain to their token.  The previous
 *	domain is pushed so that lmf_pop_domain will restore it.
 *	Initially, the null domain is current.
 */

int
lmf_push_domain(dom)
char *dom;
{
	struct domain *dp;
	struct domain *ndp;
	struct msg_file *mf;
	char *bp;
	long offset;
	int len;
	int ret;

	if (dom == NULL)
		dom = "";

	/*
	 *  Check current domain stack for domain
	 */
	if (*dom) {
		for (dp = current_domain;
		     dp && strcmp(dp->d_name, dom);
		     dp = dp->d_stack)
			;
	}
	if (dp || *dom == '\0') {
		if ((ndp = (struct domain *)malloc(sizeof(struct domain) +
						   strlen(dom))) == NULL) {
			lmf_errno = LMF_ERR_NOMEM;
			return -1;
		}
		ndp->d_next = NULL;
		ndp->d_stack = current_domain;
		if (dp) {
			ndp->d_file = dp->d_file;
			ndp->d_attr = DA_DUMMY;
			ndp->d_data = dp->d_data;
			ndp->d_length = dp->d_length;
			strcpy(ndp->d_name, dom);
		} else {
			ndp->d_file = NULL;
			ndp->d_attr = DA_ROOT;
			ndp->d_data = NULL;
			ndp->d_length = 0;
			ndp->d_name[0] = '\0';
		}
		current_domain = ndp;
		return 0;
	}

	/*
	 *  Check cached domains for domain
	 */
	for (dp = domain_cache; dp && strcmp(dp->d_name, dom); dp = dp->d_next)
		;
	if (dp) {
		dp->d_stack = current_domain;
		current_domain = dp;
		return 0;
	}

	/*
	 *  Search message files for domain
	 */
	ret = lmf_find_token(dom, &mf, &offset, &len);
	if (ret != LMFA_DOMAIN) {
		if (ret == 0)
			lmf_errno = LMF_ERR_NOTFOUND;
		return -1;
	}
	if ((ndp = (struct domain *)malloc(sizeof(struct domain) +
					   strlen(dom))) == NULL) {
		lmf_errno = LMF_ERR_NOMEM;
		return -1;
	}
	if ((bp = (char *)malloc(len)) == NULL) {
		free(ndp);
		lmf_errno = LMF_ERR_NOMEM;
		return -1;
	}
	lseek(mf->mf_fd, offset, 0);
	if (read(mf->mf_fd, bp, len) != len) {
		free(ndp);
		free(bp);
		lmf_errno = LMF_ERR_BADFILE;
		return -1;
	}
	if (flipBytes) 
	    lmf_flip_domain(bp, len);
	ndp->d_next = NULL;
	ndp->d_stack = current_domain;
	ndp->d_file = mf;
	ndp->d_attr = 0;
	ndp->d_data = bp;
	ndp->d_length = len;
	strcpy(ndp->d_name, dom);
	current_domain = ndp;
	return 0;
}


/*
 *  void lmf_pop_domain()
 *
 *	This function restores the previous message domain.  If there is no
 *	previous domain, the null domain is made current.
 */

void
lmf_pop_domain()
{
	struct domain *dp;

	dp = current_domain;
	if (dp) {
		current_domain = dp->d_stack;
		dp->d_stack = NULL;
		if (!(dp->d_attr & DA_FAST)) {
			if (!(dp->d_attr & (DA_DUMMY|DA_ROOT)))
				free(dp->d_data);
			free(dp);
		}
	}
}


/*
 *  int lmf_fast_domain(domain)
 *	char *domain;
 *
 *	This function tells the message system that the specified domain will
 *	be used frequently so the message system may cache it to improve
 *	performance.
 */

int
lmf_fast_domain(dom)	/* %%% arg for subdomains, loading messages */
char *dom;
{
	struct domain *dp;
	int ret;

	if (dom == NULL || *dom == '\0') {
		lmf_errno = LMF_ERR_NOTFOUND;
		return -1;
	}

	/*
	 *  Check cached domains for domain
	 */
	for (dp = domain_cache; dp && strcmp(dp->d_name, dom); dp = dp->d_next)
		;
	if (dp)
		return 0;

	/*
	 *  Check current domain stack for domain
	 */
	for (dp = current_domain;
	     dp && strcmp(dp->d_name, dom) && (dp->d_attr & DA_DUMMY);
	     dp = dp->d_stack)
		;
	if (dp) {
		dp->d_attr |= DA_FAST;
		dp->d_next = domain_cache;
		domain_cache = dp;
		return 0;
	}

	/*
	 *  The domain is not currently loaded.  use push_domain to load it.
	 */
	if ((ret = lmf_push_domain(dom)) == 0) {
		dp = current_domain;
		current_domain = dp->d_stack;
		dp->d_stack = NULL;
		dp->d_next = domain_cache;
		domain_cache = dp;
		dp->d_attr |= DA_FAST;
	}
	return ret;
}


/*
 *  char *lmf_get_message_internal(token, allocate)
 *	char *token;
 *	int allocate;
 *
 *	Returns NULL if the token is not found or there is no open message
 *	file.
 *
 *	This function gets a message.  The returned message will be overwritten
 *	or allocated based on the allocate argument.
 *
 *	The actual message length will be stored in the external variable
 *	lmf_message_length.
 */

char *
lmf_get_message_internal(token, allocate)
char *token;
int allocate;
{
	char tok_buf[256];
	struct msg_file *mf;
	long offset;
	int len;
	int ret;
	char *bp;

	if (current_domain && !(current_domain->d_attr & DA_ROOT)) {
		if (strlen(current_domain->d_name)+strlen(token) > 
							(unsigned int)(254)) {
			lmf_errno = LMF_ERR_NOMEM;
			return NULL;
		}
		sprintf(tok_buf, "%s.%s", current_domain->d_name, token);
		token = tok_buf;
	}
	ret = lmf_find_token(token, &mf, &offset, &len);
	if (ret > 0)
		lmf_errno = LMF_ERR_NOTFOUND;
	if (ret != 0)
		return NULL;
	if (allocate)
		bp = (char *)malloc(len + 1);
	else
		bp = lmf_get_buffer(len + 1);
	if (bp == NULL) {
		lmf_errno = LMF_ERR_NOMEM;
		return NULL;
	}
	lseek(mf->mf_fd, offset, 0);
	if (read(mf->mf_fd, bp, len) != len) {
		lmf_errno = LMF_ERR_BADFILE;
		return NULL;
	}
	lmf_message_length = len;
	bp[len] = '\0';
	return bp;
}


/*
 *  char *lmf_get_message(token, default)
 *	char *token, *default;
 *
 *	Returns default if the token is not found or there is no open message
 *	file.
 *
 *	This function gets a message.  The returned message will be overwritten
 *	by subsequent calls the the lmf library with the exception of
 *	lmf_format_string.
 *
 *	The actual message length will be stored in the external variable
 *	lmf_message_length.
 */

char *
lmf_get_message(token, default_msg)
char *token, *default_msg;
{
	char *ret;

	ret = lmf_get_message_internal(token, 0);
	if (ret == NULL)
		return default_msg;
	lmf_errno = 0;
	return ret;
}

/*
 *  char *lmf_get_message_copy(token, default)
 *	char *token, *default;
 *
 *	Returns default if the token is not found or there is no open message
 *	file.
 *
 *	This function gets a message.  The returned message is allocated with
 *	malloc.
 *
 *	The actual message length will be stored in the external variable
 *	lmf_message_length.
 */

char *
lmf_get_message_copy(token, default_msg)
char *token, *default_msg;
{
	char *ret;

	ret = lmf_get_message_internal(token, 1);
	if (ret == NULL)
		return default_msg;
	lmf_errno = 0;
	return ret;
}

/*
 *  int lmf_scan_domain(data, len, name, offset_ptr, length_ptr)
 *	char *data;
 *	int   len;
 *	char *name;
 *	long *offset_ptr;
 *	int  *length_ptr;
 *
 *	This function scans a loaded domain for name.  It returns the
 *	domain entry attribute, or -1.  offset_ptr and length_ptr are
 *	used to store the offset and length of the entry's location in
 *	the file.
 */

int
lmf_scan_domain(data, len, name, offp, lenp)
char *data;
int len;
char *name;
long *offp;
int *lenp;
{
	struct lmf_dirent *de;

	while (len > 0) {
		de = (struct lmf_dirent *)data;
		if (!strcmp(de->lmfd_name, name)) {
			*offp = de->lmfd_offset;
			*lenp = de->lmfd_length;
			return (de->lmfd_attr & LMFA_DOMAIN);
		}
		len -= de->lmfd_len;
		data += de->lmfd_len;
	}
	lmf_errno = LMF_ERR_NOTFOUND;
	return -1;
}


/*
 *  int lmf_find_token(token, mf_ptr, offset_ptr, length_ptr)
 *	char *token;
 *	struct msg_file **mf_ptr;
 *	long *offset_ptr;
 *	int  *length_ptr;
 *
 *	Returns  1  The token was found and is a domain.
 *		 0  The token was found and is a message.
 *		-1  The token wasn't found.  Errors returned in lmf_errno:
 *		    LMF_ERR_NOMSGFILE  No open message file.
 *		    LMF_ERR_NOTFOUND   The token was not found.
 *
 *	This function searches for the token in the current open message files.
 *	token is a fully qualified name.
 */

int
lmf_find_token(token, mfp, offp, lenp)
char *token;
struct msg_file **mfp;
long *offp;
int *lenp;
{
	char *name;
	char *dom;
	char *bp;
	struct domain *dp;
	struct msg_file *mf;
	long offset;
	int len, ret;
	char *strchr(), *strrchr();

	if (first_msg_file == NULL) {
		lmf_errno = LMF_ERR_NOMSGFILE;
		return -1;
	}

	dom = token;
	if ((name = strrchr(token, '.')) == NULL) {
		dom = "";
		name = token;
	} else
		*name++ = '\0';

	for (dp = current_domain;dp && strcmp(dp->d_name, dom);dp = dp->d_stack)
		;
	if (dp)
		goto founddomain;

	for (dp = domain_cache; dp && strcmp(dp->d_name, dom); dp = dp->d_next)
		;
	if (dp)
		goto founddomain;

	for (mf = first_msg_file; mf; mf = mf->mf_next) {
		if (!strcmp(mf->mf_hdr.lmfh_base_domain, dom)) {
			if (name != token)
				name[-1] = '.';
			offset = mf->mf_hdr.lmfh_base_offset;
			len = mf->mf_hdr.lmfh_base_length;
			if ((bp = lmf_get_buffer(len)) == NULL) {
				lmf_errno = LMF_ERR_NOMEM;
				return -1;
			}
			lseek(mf->mf_fd, offset, 0);
			if (read(mf->mf_fd, bp, len) != len) {
				lmf_errno = LMF_ERR_BADFILE;
				return -1;
			}
			if (flipBytes)
			    lmf_flip_domain(bp, len);
			*mfp = mf;
			ret = lmf_scan_domain(bp, len, name, offp, lenp);
			if (ret >= 0)
				return ret;
		}
		if (name != token)
			name[-1] = '.';
		if (!strcmp(mf->mf_hdr.lmfh_base_domain, token)) {
			*mfp = mf;
			*offp = mf->mf_hdr.lmfh_base_offset;
			*lenp = mf->mf_hdr.lmfh_base_length;
			return (mf->mf_hdr.lmfh_base_attr & LMFA_DOMAIN);
		}
		if (name != token)
			name[-1] = '\0';
	}
	if (name != token) {
		ret = lmf_find_token(dom, &mf, &offset, &len);
		if (name != token)
			name[-1] = '.';
		if (ret != LMFA_DOMAIN) {
			lmf_errno = LMF_ERR_NOTFOUND;
			return -1;
		}
		if ((bp = lmf_get_buffer(len)) == NULL) {
			lmf_errno = LMF_ERR_NOMEM;
			return -1;
		}
		*mfp = mf;
		lseek(mf->mf_fd, offset, 0);
		if (read(mf->mf_fd, bp, len) != len) {
			lmf_errno = LMF_ERR_BADFILE;
			return -1;
		}
		if (flipBytes)
		    lmf_flip_domain(bp, len);
		return lmf_scan_domain(bp, len, name, offp, lenp);
	}
	if (name != token)
		*--name = '.';
	lmf_errno = LMF_ERR_NOTFOUND;
	return -1;

founddomain:
	if (name != token)
		name[-1] = '.';
	if (dp->d_attr & DA_ROOT) {
		lmf_errno = LMF_ERR_NOTFOUND;
		return -1;
	}
	*mfp = dp->d_file;
	return lmf_scan_domain(dp->d_data, dp->d_length, name, offp, lenp);
}


/*
 *  char *lmf_get_buffer(size)
 *	int size;
 *
 *	This function returns a buffer of at least size bytes.
 */

char *
lmf_get_buffer(size)
int size;
{
	if (buf && buf_len >= size)
		return buf;
	if (buf)
		free(buf);
	for (buf_len = 0; buf_len < size; buf_len += 512)
		;
	buf = (char *)malloc(buf_len);
	return buf;
}

#ifdef MSDOS
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * dos_codepage() return DOS code page number
 */
static int 
dos_codepage()
{
    union REGS regs;
    
    regs.x.ax = 0x6601;
    intdos(&regs,&regs);
    if (regs.x.bx == 0)			/* if bx == 0 use */
	return 437;			/* default code page of 437 */
    else
	return (regs.x.bx);		/* else return global code page */
}
#endif

/*
 *  int lmf_flip_domain(data, len)
 *	char *data;
 *	int   len;
 *
 *	This function scans a loaded domain and flips the appropriate
 *	values in it.
 */

int
lmf_flip_domain(data, len)
char *data;
int len;
{
	struct lmf_dirent *de;

	while (len > 0) {
		de = (struct lmf_dirent *)data;
		sflip(de->lmfd_length);
		lflip(de->lmfd_offset);
		len -= de->lmfd_len;
		data += de->lmfd_len;
	}
}

/*
 *  int lmf_byteorder()
 *
 *	This function determines the machine byte ordering.
 *	Returns 0 if flipping is not necessary.
 *	Returns 1 if flipping is necessary (i.e. big endian architecture).
 */
int
lmf_byteorder()
{
	int	ivar = 1;

	if (*(char *)&ivar)
		return 0;
	else 
		return 1;
}
