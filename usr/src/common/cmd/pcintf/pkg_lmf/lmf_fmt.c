#ident	"@(#)pcintf:pkg_lmf/lmf_fmt.c	1.2"
/* SCCSID(@(#)lmf_fmt.c	7.2	LCC)	/* Modified: 13:56:12 12/5/90 */

/*
 *  char *lmf_format_string(buffer, buffer_size, string, fmtspec, arg1, ...)
 *	char *buffer, *string, *fmtspec;
 *	int buffer_size;
 *
 *	returns a pointer to the resulting string.  If buffer is NULL,
 *		a static buffer is used.
 *
 *	buffer is an optional buffer into which the result is placed.
 *	buffer_size is the maximum number of bytes which can be placed into
 *		buffer.
 *	string is the NLS string normally obtained from lmf_get_message().
 *	fmtspec is a list of formatting directives ala printf.
 *
 *	This function will insert the formatted args into the string based
 *	on where the positional parameters are located.  The output string
 *	will be null terminated.
 *
 *	An argument specifier greater than the number of arguments passed
 *	will be removed from the output string.  There is a limit of nine
 *	arguments.
 *
 *	example:
 *	  lmf_format_string(buf, 80, "arg2 = %2, arg1 = %1\n", "%s%d", "1", 2);
 */

#define LMF_NO_EXTERNS	1	/* Don't define lmf_format_string in header */

#include <varargs.h>
#include "lmf.h"

static char *buffer = NULL;
static int buffer_length = 0;

static char *arg_buf = NULL;
static int arg_buf_len = 0;


char *malloc();			/* default declaration of malloc */
char *realloc();		/* default declaration of realloc */


char *
lmf_format_string(va_alist)
va_dcl
{
	va_list ap;
	char *buf;
	int buf_len;
	char *fmt;
	char *ptypes;
	register char *p, *p2;
	char *args[9], *argp;
	int   nargs, tmp;

	va_start(ap);
	buf = va_arg(ap, char *);	/* pointer to our buffer or NULL */

	buf_len = va_arg(ap, int);	/* length of buffer or 0 */

	fmt = va_arg(ap, char *);	/* lmf style format string 
					 * e.g.:
					 * "foo %1 bar %2"
					 */

	ptypes = va_arg(ap, char *);	/* conversion characters 
					 * e.g.:
					 * "%-14.14s%3d"
					 */

	nargs = 0;
	argp = arg_buf;
	while (*ptypes) {	/* while we still have conversion characters */

		/* allocate (more) space for the argument buffer if needed */
		if ((&arg_buf[arg_buf_len] - argp) < 100) {
			arg_buf_len += 512;
			if (arg_buf != NULL) {
				tmp = argp - arg_buf;
				arg_buf = (char *)realloc(arg_buf, arg_buf_len);
				argp = &arg_buf[tmp];
			} else
				argp = arg_buf = (char *)malloc(arg_buf_len);
			if (arg_buf == NULL) {
				arg_buf_len = 0;
				lmf_errno = LMF_ERR_NOMEM;
				return NULL;
			}
		}

		/* point p to the NEXT '%' */
		for (p = ptypes; *++p && *p != '%'; )
			;

		tmp = *p;	/* if p == NULL, tmp = 0 */
		*p = '\0';
		/* at this point p[-n] contains the current conversion string */
		switch (p[-1]) {
		case 's':
			sprintf(argp, ptypes, va_arg(ap, char *));
			break;
		case 'd':
		case 'i':
		case 'o':
		case 'u':
		case 'x':
		case 'X':
			if (p[-2] == 'l') {
				sprintf(argp, ptypes, va_arg(ap, long));
				break;
			}

		case 'c':
			sprintf(argp, ptypes, va_arg(ap, int));
			break;

		case 'e':
		case 'E':
		case 'f':
		case 'g':
		case 'G':
			sprintf(argp, ptypes, va_arg(ap, double));
			break;

		default:
			sprintf(argp, "Invalid");
			break;
		}
		args[nargs] = argp;
		argp = (&argp[strlen(argp)]) + 1;
		if (++nargs == 9)
			break;

		/* reset *p if tmp != 0 */
		if (tmp)
			*p = '%';
		ptypes = p;
	}

	/* determine the length of the output buffer */
	tmp = 0;
	for (p = fmt; *p; p++) {
		if (*p == '%' && *++p > '0' && (*p - '1') < nargs)
			tmp += strlen(args[*p - '1']);
		else
			tmp++;
	}


	if (buf == NULL) { /* allocate buffer space if it is not provided */
		if (buffer_length < ++tmp) {
			if (buffer != NULL)
				free(buffer);
			while (buffer_length < tmp)
				buffer_length += 512;
			if ((buffer = (char *)malloc(buffer_length)) == NULL){
				buffer_length = 0;
				lmf_errno = LMF_ERR_NOMEM;
				return NULL;
			}
		}
		buf = buffer;
		buf_len = tmp;
	}

	/* fill the buffer with output */
	p = buf;
	while (*fmt) {
		/* copy expanded arguments into buffer */
		if (*fmt == '%' && *++fmt > '0' && *fmt <= '9') {
			if ((*fmt - '1') < nargs) {
				p2 = args[*fmt - '1'];
				while (*p2) {
					/* fail if we run out of buffer space */
					if (buf_len-- == 1) {
						*p = '\0';
						lmf_errno = LMF_ERR_NOSPACE;
						return NULL;
					}
					*p++ = *p2++;
				}
				fmt++;
			}
		} else if (*fmt) {	/* copy text into buffer */
			/* fail if we run out of buffer space */
			if (buf_len-- == 1) {
				*p = '\0';
				lmf_errno = LMF_ERR_NOSPACE;
				return NULL;
			}
			*p++ = *fmt++;
		}
	}
	*p = '\0';
	return buf;
}
