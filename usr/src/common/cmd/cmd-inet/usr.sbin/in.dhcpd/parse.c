#ident "@(#)parse.c	1.2"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1996 Computer Associates International, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

/*
 * This file contains code for parsing the DHCP configuration file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <arpa/dhcp.h>
#include <syslog.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <string.h>
#include <memory.h>
#include "dhcpd.h"
#include "proto.h"

static FILE *config_fp;
static int line_num;
static int error_flag;

static time_t config_file_mtime;

/*
 * The following table maps hardware type names to type codes.
 */

typedef struct {
	char *name;
	u_char code;
	int len;
} HwType;

static HwType hardware_types[] = {
	"ethernet", HTYPE_ETHERNET, HLEN_ETHERNET,
	"ieee802", HTYPE_IEEE802, HLEN_IEEE802,
};

#define NUM_HARDWARE_TYPES	(sizeof(hardware_types) / sizeof(HwType))

static char *left_brace = "{";
static char *right_brace = "}";

/*
 * config_ok is set to 1 if the configuration file was parsed successfully,
 * or 0 if not.
 */

int config_ok;

/*
 * parse_error -- generate a parsing error message
 * A message indicating a parsing error is generated and syslog'ed.
 * fmt is a printf format string, and may be followed by arguments.
 * The message is preceded by the config file name and line number.
 * A 1200-byte buffer is used to generate the message, so the message
 * can't be larger than that (this size is used in case some bogus line
 * has a very long token on it (lines can be up to 1023 bytes) and
 * an error message is generated using that token).
 * parse_error sets error_flag to 1.
 */

static void
parse_error(char *fmt, ...)
{
	va_list p;
	static char msgbuf[1200];

	va_start(p, fmt);
	vsprintf(msgbuf, fmt, p);
	va_end(argp);
	report(LOG_ERR, "Error in config file %s, line %d: %s",
		config_file, line_num, msgbuf);
	error_flag = 1;
}

/*
 * get_line -- get the next non-comment/blank line
 * The line is broken up into white space-separated tokens.  The number
 * of tokens is returned in *argc_ret, and a pointer to an array
 * of pointers to the tokens is returned in *argv_ret.  If a token
 * was a quoted string, the initial quote is left in place so that the
 * parsing code can tell that the token was quoted.  Backslash escapes and
 * the closing quote are removed.  All tokens are stored in a static
 * area, so they must be copied if they need to be saved.
 * The return value is 1 if ok, 0 if EOF, -1 if a parsing error occurs,
 * or -2 if unable to allocate memory (memory is allocated for the token
 * list).
 */

static int
get_line(int *argc_ret, char ***argv_ret)
{
	static char linebuf[MAX_LINE_LEN + 1];
	static char **argv;
	static int argv_size = 0;
	int argc;
	char **nargv;
	char *p, *to;
	int len, c, esc, nsize;

#define ARGV_CHUNK	16

	if (argv_size == 0) {
		if (!(argv = tbl_alloc(char *, ARGV_CHUNK))) {
			malloc_error("get_line");
			return -2;
		}
		argv_size = ARGV_CHUNK;
	}

	for (;;) {
		line_num++;
		if (!fgets(linebuf, sizeof(linebuf), config_fp)) {
			return 0;
		}
		len = strlen(linebuf);
		if (linebuf[len - 1] != '\n') {
			parse_error("Line too long.");
			while ((c = getc(config_fp)) != '\n' && c != EOF)
				;
			return -1;
		}
		linebuf[len - 1] = '\0';
		argc = 0;
		p = linebuf;
		for (;;) {
			/*
			 * Skip white space.
			 */
			p += strspn(p, " \t");
			/*
			 * If comment or end of line, we're done.
			 */
			if (*p == '\0' || *p == '#') {
				break;
			}
			/*
			 * Store a pointer to the first char in argv.
			 * Make sure there's enough room.
			 */
			if (argc == argv_size) {
				nsize = argv_size + ARGV_CHUNK;
				if (!(nargv = tbl_grow(argv, char *, nsize))) {
					malloc_error("get_line");
					return -2;
				}
				argv = nargv;
				argv_size = nsize;
			}
			argv[argc++] = p;
			/*
			 * Check for quoted string.
			 */
			if (*p == '"') {
				/*
				 * Process backslash escapes & find the
				 * closing quote.  Backslashes are removed
				 * by moving subsequent characters up.
				 * The closing quote is removed, but the
				 * initial quote is left in place.
				 */
				esc = 0;
				to = p;
				for (;;) {
					c = *++p;
					if (c == '\0') {
						parse_error("No closing quote.");
						return -1;
					}
					if (esc) {
						*++to = c;
						esc = 0;
						continue;
					}
					switch (c) {
					case '\\':
						esc = 1;
						continue;
					case '"':
						*++to = '\0';
						p++;
						break;
					default:
						*++to = c;
						continue;
					}
					break;
				}
				/*
				 * There should only be white space (or
				 * end of line) after the closing quote.
				 */
				if (*p != ' ' && *p != '\t' && *p != '\0') {
					parse_error("Illegal character after closing quote.");
					return -1;
				}
			}
			else {
				/*
				 * Find the next white space character
				 * and put a null byte there.  p is left
				 * pointing at the next character.  If this
				 * is the end of the line, leave p pointing
				 * to the null byte.
				 */
				p += strcspn(p, " \t");
				if (*p != '\0') {
					*p++ = '\0';
				}
			}
		}
		/*
		 * If there was something on this line, return it.
		 */
		if (argc > 0) {
			*argc_ret = argc;
			*argv_ret = argv;
			return 1;
		}
	}
}

/*
 * get_* -- convert a token from the config file into a value of the
 * appropriate type.  The value is returned in *val_ret.  For string
 * and binary objects, the length is returned in *len_ret (len_ret is
 * not used for other types).  Functions return 1 if ok, 0 if error.
 * parse_error is called is an error is detected.
 */

/*
 * get_*int* -- convert a string to an integer value of the appropriate type
 * The value is returned in *val_ret.
 * Return value is 1 if ok or 0 if str contains an invalid value.
 */

static int
get_uint8(char *str, void *val_ret, int *len_ret)
{
	long val;
	char *ptr;

	val = strtol(str, &ptr, 10);
	/*
	 * ptr must point to a null byte
	 */
	if (*ptr != '\0' || val < 0 || val > 255) {
		parse_error("Illegal value for unsigned 8-bit integer.");
		return 0;
	}
	else {
		*((u_char *) val_ret) = val;
		return 1;
	}
}

static int
get_int8(char *str, void *val_ret, int *len_ret)
{
	long val;
	char *ptr;

	val = strtol(str, &ptr, 10);
	/*
	 * ptr must point to a null byte
	 */
	if (*ptr != '\0' || val < -128 || val > 127) {
		parse_error("Illegal value for signed 8-bit integer.");
		return 0;
	}
	else {
		*((char *) val_ret) = val;
		return 1;
	}
}

static int
get_uint16(char *str, void *val_ret, int *len_ret)
{
	long val;
	char *ptr;

	val = strtol(str, &ptr, 10);
	/*
	 * ptr must point to a null byte
	 */
	if (*ptr != '\0' || val < 0 || val > 65535) {
		parse_error("Illegal value for unsigned 16-bit integer.");
		return 0;
	}
	else {
		*((u_short *) val_ret) = val;
		return 1;
	}
}

static int
get_int16(char *str, void *val_ret, int *len_ret)
{
	long val;
	char *ptr;

	val = strtol(str, &ptr, 10);
	/*
	 * ptr must point to a null byte
	 */
	if (*ptr != '\0' || val < -32768 || val > 32767) {
		parse_error("Illegal value for signed 16-bit integer.");
		return 0;
	}
	else {
		*((short *) val_ret) = val;
		return 1;
	}
}

static int
get_uint32(char *str, void *val_ret, int *len_ret)
{
	u_long val;
	char *p;
	int c;

	val = 0;
	for (p = str; c = *p; p++) {
		if (c < '0' || c > '9') {
			parse_error("Illegal value for unsigned 32-bit integer.");
			return 0;
		}
		c -= '0';
		/*
		 * Check for overflow.
		 */
		if (val > 429496729 || (val == 429496729 && c > 5)) {
			parse_error("Illegal value for unsigned 32-bit integer.");
			return 0;
		}
		val = val * 10 + c;
	}

	*((u_long *) val_ret) = val;
	return 1;
}

static int
get_int32(char *str, void *val_ret, int *len_ret)
{
	char *p;
	int sign, last_digit, c;
	u_long val;

	p = str;
	if (*p == '-') {
		sign = 1;
		last_digit = 8;
		p++;
	}
	else {
		sign = 0;
		last_digit = 7;
	}

	val = 0;
	for (; c = *p; p++) {
		if (c < '0' || c > '9') {
			parse_error("Illegal value for signed 32-bit integer.");
			return 0;
		}
		c -= '0';
		/*
		 * Check for overflow.
		 */
		if (val > 214748364 || (val == 214748364 && c > last_digit)) {
			parse_error("Illegal value for signed 32-bit integer.");
			return 0;
		}
		val = val * 10 + c;
	}

	if (sign) {
		*((long *) val_ret) = -val;
	}
	else {
		*((long *) val_ret) = val;
	}
	return 1;
}

/*
 * get_boolean -- convert a string ("true" or "false") to a boolean value
 * The value (1 for true, 0 for false) is returned in *val_ret.
 * The return value is 1 if ok, or 0 if not.
 */

static int
get_boolean(char *str, void *val_ret, int *len_ret)
{
	if (strcmp(str, "true") == 0) {
		*((u_char *) val_ret) = 1;
		return 1;
	}
	else if (strcmp(str, "false") == 0) {
		*((u_char *) val_ret) = 0;
		return 1;
	}
	else {
		parse_error("Invalid boolean value.");
		return 0;
	}
}

/*
 * get_string -- return a pointer to a copy of the string represented by str
 * If there is an initial quote (left by get_line), it is not included.
 * Returns 1 if ok, 0 if unable to allocate memory.
 */

static int
get_string(char *str, void *val_ret, int *len_ret)
{
	if (*str == '"') {
		str++;
	}
	if (!(*((char **) val_ret) = strdup(str))) {
		malloc_error("get_string");
		return 0;
	}
	*len_ret = strlen(str);
	return 1;
}

/*
 * get_binary -- convert hex string str to a string of bytes
 * A pointer to malloc'ed space containing the byte string is returned
 * in *val_ret, and the length is returned in *len_ret.
 * The string must consist of valid hex digits (letters may be upper or lower)
 * and must be of even length.
 * Returns 1 if ok, 0 if invalid string or unable to allocate memory.
 */

static int
get_binary(char *str, void *val_ret, int *len_ret)
{
	int len, c, which;
	char *p;
	u_char last, *buf, *bp;

	len = strlen(str);

	/*
	 * Check for even length.
	 */

	if ((len % 2) == 1) {
		parse_error("Binary value must contain an even number of digits.");
		return 0;
	}

	len /= 2;

	if (!(buf = malloc(len))) {
		malloc_error("get_binary");
		return 0;
	}

	bp = buf;
	which = 0;
	for (p = str; *p; p++) {
		c = *p;
		if (c >= '0' && c <= '9') {
			c -= '0';
		}
		else if (c >= 'a' && c <= 'f') {
			c = c - 'a' + 10;
		}
		else if (c >= 'A' && c <= 'F') {
			c = c - 'A' + 10;
		}
		else {
			free(buf);
			parse_error("Invalid digit in binary value.");
			return 0;
		}
		if (which == 0) {
			last = c;
			which = 1;
		}
		else {
			*bp++ = (last << 4) | c;
			which = 0;
		}
	}

	*((u_char **) val_ret) = buf;
	*len_ret = len;
	return 1;
}

/*
 * get_addr -- convert str to IP address
 * The string can either be an address in dot notation or a host name.
 * If the string is not a valid address in dot notation, it is assumed
 * to be a host name.  An error message is printed if the host name
 * lookup fails.
 * Returns 1 if ok, 0 if host lookup fails.
 */

static int
get_addr(char *str, void *val_ret, int *len_ret)
{
	struct hostent *hp;

	if (inet_aton(str, (struct in_addr *) val_ret)) {
		return 1;
	}

	if (!(hp = gethostbyname(str))) {
		report(LOG_ERR, "Unknown host \"%s\".", str);
		return 0;
	}

	memcpy((char *) val_ret, hp->h_addr, sizeof(struct in_addr));
	return 1;
}

/*
 * get_nb_node -- convert string to NetBIOS node type
 * Allowed values are 'b', 'p', 'm', and 'h'.
 * Returns 1 if ok or 0 if invalid.
 */

static int
get_nb_node(char *str, void *val_ret, int *len_ret)
{
	u_char val;

	switch (str[0]) {
	case 'b':
		val = NB_NODE_B;
		break;
	case 'p':
		val = NB_NODE_P;
		break;
	case 'm':
		val = NB_NODE_M;
		break;
	case 'h':
		val = NB_NODE_H;
		break;
	default:
		parse_error("Invalid NetBIOS node type.");
		return 0;
	}

	if (str[1] == '\0') {
		*((u_char *) val_ret) = val;
		return 1;
	}
	else {
		parse_error("Invalid NetBIOS node type.");
		return 0;
	}
}

/*
 * check_* -- make sure setting is valid for the option
 * An error message is printed if the value is invalid.
 * Functions return 1 if valid, 0 if not.
 */

static int
check_uint8(OptionSetting *osp, void *valp)
{
	OptionDesc *opt = osp->opt;
	u_char val;

	val = *((u_char *) valp);
	if ((opt->min_val && val < *((u_char *) opt->min_val))
	    || (opt->max_val && val > *((u_char *) opt->max_val))) {
		parse_error("Invalid value for %s", opt->name);
		return 0;
	}
	return 1;
}

static int
check_int8(OptionSetting *osp, void *valp)
{
	OptionDesc *opt = osp->opt;
	char val;

	val = *((char *) valp);
	if ((opt->min_val && val < *((char *) opt->min_val))
	    || (opt->max_val && val > *((char *) opt->max_val))) {
		parse_error("Invalid value for %s", opt->name);
		return 0;
	}
	return 1;
}

static int
check_uint16(OptionSetting *osp, void *valp)
{
	OptionDesc *opt = osp->opt;
	u_short val;

	val = *((u_short *) valp);
	if ((opt->min_val && val < *((u_short *) opt->min_val))
	    || (opt->max_val && val > *((u_short *) opt->max_val))) {
		parse_error("Invalid value for %s", opt->name);
		return 0;
	}
	return 1;
}

static int
check_int16(OptionSetting *osp, void *valp)
{
	OptionDesc *opt = osp->opt;
	short val;

	val = *((short *) valp);
	if ((opt->min_val && val < *((short *) opt->min_val))
	    || (opt->max_val && val > *((short *) opt->max_val))) {
		parse_error("Invalid value for %s", opt->name);
		return 0;
	}
	return 1;
}

static int
check_uint32(OptionSetting *osp, void *valp)
{
	OptionDesc *opt = osp->opt;
	u_long val;

	val = *((u_long *) valp);
	if ((opt->min_val && val < *((u_long *) opt->min_val))
	    || (opt->max_val && val > *((u_long *) opt->max_val))) {
		parse_error("Invalid value for %s", opt->name);
		return 0;
	}
	return 1;
}

static int
check_int32(OptionSetting *osp, void *valp)
{
	OptionDesc *opt = osp->opt;
	long val;

	val = *((long *) valp);
	if ((opt->min_val && val < *((long *) opt->min_val))
	    || (opt->max_val && val > *((long *) opt->max_val))) {
		parse_error("Invalid value for %s", opt->name);
		return 0;
	}
	return 1;
}

static int
check_boolean(OptionSetting *osp, void *valp)
{
	return 1;
}

static int
check_string(OptionSetting *osp, void *valp)
{

	if (osp->len < osp->opt->min_len || osp->len > osp->opt->max_len) {
		parse_error("Invalid string length.");
		return 0;
	}
	else {
		return 1;
	}
}

static int
check_binary(OptionSetting *osp, void *valp)
{
	OptionDesc *opt = osp->opt;

	if (osp->len < opt->min_len || osp->len > opt->max_len) {
		parse_error("Invalid binary string length.");
		return 0;
	}
	else {
		return 1;
	}
}

static int
check_addr(OptionSetting *osp, void *valp)
{
	return 1;
}

static int
check_nb_node(OptionSetting *osp, void *valp)
{
	return 1;
}

/*
 * order* -- do byte order conversion on an option value
 */

static void
order2(void *val)
{
	*((u_short *) val) = htons(*((u_short *) val));
}

static void
order4(void *val)
{
	*((u_long *) val) = htonl(*((u_long *) val));
}

/*
 * Table of information for option types, indexed by option type code.
 */

typedef struct {
	char *name;	/* name used in user-defined options */
	int (*get_func)(char *, void *, int *);
	int (*check_func)(OptionSetting *, void *);
	void (*order_func)(void *);
	/*
	 * size is the size in bytes of a value of this type.  If size is
	 * 0, the type is a string type (text or binary).
	 */
	int size;
} OptionType;

OptionType option_types[] = {
	"uint8", get_uint8, check_uint8, NULL, 1,
	"int8", get_int8, check_int8, NULL, 1,
	"uint16", get_uint16, check_uint16, order2, 2,
	"int16", get_int16, check_int16, order2, 2,
	"uint32", get_uint32, check_uint32, order4, 4,
	"int32", get_int32, check_int32, order4, 4,
	"boolean", get_boolean, check_boolean, NULL, 1,
	"string", get_string, check_string, NULL, 0,
	"binary", get_binary, check_binary, NULL, 0,
	"ip_address", get_addr, check_addr, NULL, sizeof(struct in_addr),
	"", get_nb_node, check_nb_node, NULL, 1
};

#define NUM_OPTION_TYPES	(sizeof(option_types) / sizeof(OptionType))

/*
 * get_simple_setting -- get an option setting for a non-array type
 * get_simple_setting attempts to create an OptionSetting structure
 * for the given option and token from the config file.  Memory is
 * allocated for the OptionSetting structure and the value itself.
 * Returns a pointer to a malloc'ed OptionSetting structure if ok,
 * If the OTYPE_AUTO bit is set, and the setting is "auto", the val
 * field is set to NULL.
 * or NULL if the setting was invalid or memory could not be allocated.
 */

static OptionSetting *
get_simple_setting(OptionDesc *opt, char *str)
{
	OptionSetting *osp;
	OptionType *otp;

	if (!(osp = str_alloc(OptionSetting))) {
		malloc_error("get_simple_setting");
		return NULL;
	}

	osp->opt = opt;

	otp = &option_types[OTYPE(opt->type)];

	/*
	 * Check for "auto"
	 */

	if ((opt->type & OTYPE_AUTO) && strcmp(str, "auto") == 0) {
		osp->val = NULL;
		osp->len = 0;
		osp->size = otp->size;
		return osp;
	}

	if (otp->size > 0) {
		/*
		 * This is a fixed size item.  Allocate space for it.
		 */
		if (!(osp->val = malloc(otp->size))) {
			malloc_error("get_simple_setting");
			free(osp);
			return NULL;
		}
		osp->size = otp->size;
		/*
		 * Get the value, make sure it's ok, and do byte
		 * order conversion if necessary.
		 */
		if ((*otp->get_func)(str, osp->val, &osp->len)
		    && (*otp->check_func)(osp, osp->val)) {
			if (otp->order_func) {
				(*otp->order_func)(osp->val);
			}
			return osp;
		}
		else {
			free(osp->val);
			free(osp);
			return NULL;
		}
	}
	else {
		/*
		 * This is string type (text or binary).
		 * The get function will allocate memory to store the value
		 * and return a pointer to it.
		 */
		if ((*otp->get_func)(str, &osp->val, &osp->len)
		    && (*otp->check_func)(osp, osp->val)) {
			osp->size = osp->len;
			return osp;
		}
		else {
			free(osp);
			return NULL;
		}
	}
}

/*
 * array_line -- add the list of settings on a line to the given array
 * Returns 1 if ok, or 0 if not.
 */

static int
array_line(OptionSetting *osp, int argc, char *argv[])
{
	OptionDesc *opt;
	OptionType *otp;
	int size, i, dummy;
	char *nval, *p;

	opt = osp->opt;
	otp = &option_types[OTYPE(opt->type)];
	size = otp->size;

	/*
	 * If this is the first line of values for this option,
	 * allocate space according to the number of arguments.
	 * Otherwise, increase the allocated space by that amount.
	 */

	if (!osp->val) {
		nval = malloc(argc * size);
	}
	else {
		nval = realloc(osp->val, (osp->len + argc) * size);
	}
	if (!nval) {
		malloc_error("array_line");
		return 0;
	}
	osp->val = nval;
	p = ((char *) osp->val) + (osp->len * size);
	osp->len += argc;

	/*
	 * Convert each argument and store it in the array.
	 */

	for (i = 0; i < argc; i++) {
		if (!(*otp->get_func)(argv[i], p, &dummy)
		    || !(*otp->check_func)(osp, p)) {
			return 0;
		}
		if (otp->order_func) {
			(*otp->order_func)(p);
		}
		p += size;
	}

	return 1;
}

/*
 * get_array_setting -- get an option setting for an array type
 * get_array_setting attempts to create an OptionSetting structure
 * for the given option and config file line.  Multi-line entries
 * (between { and }) are handled automatically.
 * Returns a pointer to a malloc'ed OptionSetting structure if successful,
 * or NULL if not.
 */

static OptionSetting *
get_array_setting(OptionDesc *opt, int argc, char **argv)
{
	OptionSetting *osp;
	int ret;

	if (!(osp = str_alloc(OptionSetting))) {
		malloc_error("get_array_setting");
		return NULL;
	}

	osp->opt = opt;
	osp->val = NULL;
	osp->len = 0;

	if (argc >= 2 && strcmp(argv[1], left_brace) == 0) {
		if (argc != 2) {
			parse_error("Syntax error.");
			goto error;
		}
		/*
		 * Multi-line entry.  Process lines until we find the
		 * closing brace.
		 */
		while ((ret = get_line(&argc, &argv)) == 1) {
			if (strcmp(argv[0], right_brace) == 0) {
				if (argc != 1) {
					parse_error("Syntax error.");
					goto error;
				}
				break;
			}
			if (!array_line(osp, argc, argv)) {
				parse_error("Syntax error.");
				goto error;
			}
		}
		if (ret == 0) {
			parse_error("End of file encountered in multi-line entry.");
			goto error;
		}
		else if (ret != 1) {
			goto error;
		}
	}
	else {
		/*
		 * Entry is contained on this line.  Skip argv[0], which
		 * is the option name.
		 */
		if (!array_line(osp, argc - 1, argv + 1)) {
			goto error;
		}
	}

	/*
	 * Check the length of the array against min & max.
	 */
	
	if (osp->len < osp->opt->min_len || osp->len > osp->opt->max_len) {
		parse_error("Invalid array length.");
		goto error;
	}

	osp->size = osp->len * option_types[OTYPE(osp->opt->type)].size;

	return osp;

error:
	if (osp->val) {
		free(osp->val);
	}
	free(osp);
	return NULL;
}

/*
 * is_numeric -- verify that a string contains only digits
 * Returns 1 if numeric, 0 if not.
 */

static int
is_numeric(char *str)
{
	char *p;

	for (p = str; *p; p++) {
		if (*p < '0' || *p > '9') {
			return 0;
		}
	}

	return 1;
}

/*
 * proc_option_setting -- process a config file entry believed to be
 * an option setting.  The argument vector from the config file line
 * is supplied.  This function may cause subsequent lines to be
 * consumed if this is a multi-line entry.
 * The new option is added to the list of options pointed to by *opt_list,
 * provided that the option is not already on the list.
 * Returns 1 if ok, or 0 if an error occurs (including duplicate options).
 */

int
proc_option_setting(OptionSetting **opt_list, int argc, char **argv)
{
	OptionSetting *osp, *losp, **link;
	OptionDesc *opt;
	int len, num;

	/*
	 * Look up the option, first in the predefined option table, then in
	 * the user-defined option table.  If found, call get_simple_setting
	 * or get_array_setting to handle it.
	 */
	
	if (!(opt = lookup_option(argv[0]))) {
		/*
		 * See if the option name is "optionNNN", where NNN is
		 * a number between 0 and 255, which means this is a reference
		 * to a user-defined option.
		 */
		len = strlen(argv[0]);
		if (len > 6  && len <= 9 && strncmp(argv[0], "option", 6) == 0
		    && is_numeric(&argv[0][6])
		    && (num = atoi(&argv[0][6])) >= MIN_USER_OPTION
		    && num <= MAX_USER_OPTION) {
			opt = user_options[num];
		}
	}

	if (opt) {
		if (opt->array) {
			osp = get_array_setting(opt, argc, argv);
		}
		else {
			if (argc != 2) {
				parse_error("%s takes a single value.",
					opt->name);
				return 0;
			}
			osp = get_simple_setting(opt, argv[1]);
		}
		if (!osp) {
			return 0;
		}
	}
	else {
		parse_error("Unknown option %s", argv[0]);
		return 0;
	}

	/*
	 * Go through the list and make sure it's not already there.
	 * If it isn't, add it to the end.
	 */
	
	for (link = opt_list, losp = *opt_list; losp;
	    link = &losp->next, losp = losp->next) {
		if (losp->opt == osp->opt) {
			parse_error("Duplicate option %s", osp->opt->name);
			if (osp->val) {
				free(osp->val);
			}
			free(osp);
			return 0;
		}
	}

	osp->next = NULL;
	*link = osp;

	return 1;
}

/*
 * proc_client_entry -- process a client entry in the config file
 * Returns 1 if ok, 0 if not.
 */

static int
proc_client_entry(int argc, char **argv)
{
	Client *client;
	int ht, ret;

	if (argc != 4 || strcmp(argv[3], left_brace) != 0) {
		parse_error("Syntax error.");
		return 0;
	}

	if (!(client = str_alloc(Client))) {
		malloc_error("proc_client_entry");
		return 0;
	}

	/*
	 * Initialize fields.
	 */

	client->addr.s_addr = 0;
	client->options = NULL;

	/*
	 * Determine hardware type.
	 */
	
	for (ht = 0; ht < NUM_HARDWARE_TYPES; ht++) {
		if (strcmp(argv[1], hardware_types[ht].name) == 0) {
			client->id_type = hardware_types[ht].code;
			break;
		}
	}

	/*
	 * Check for "opaque" or a numeric type.
	 */

	if (ht == NUM_HARDWARE_TYPES) {
		if (strcmp(argv[1], "opaque") == 0) {
			client->id_type = IDTYPE_OPAQUE;
		}
		else if (is_numeric(argv[1])) {
			if (!get_uint8(argv[1], &client->id_type, NULL)) {
				goto error;
			}
		}
		else {
			parse_error("Unknown hardware type.");
			goto error;
		}
	}

	/*
	 * Get the ID string.
	 */
	
	if (client->id_type == IDTYPE_OPAQUE && argv[2][0] == '"') {
		if (!(client->id = (u_char *) strdup(&argv[2][1]))) {
			malloc_error("proc_client_entry");
			goto error;
		}
		client->id_len = strlen(&argv[2][1]);
	}
	else if (!get_binary(argv[2], &client->id, &client->id_len)) {
		goto error;
	}

	if (client->id_len < 1 || client->id_len > MAX_OPT_LEN - 1) {
		parse_error("Client ID must be between 1 and %d bytes.",
			MAX_OPT_LEN - 1);
		goto error;
	}
	else if (ht < NUM_HARDWARE_TYPES
	    && client->id_len != hardware_types[ht].len) {
		parse_error("%s address must be %d bytes",
			hardware_types[ht].name, hardware_types[ht].len);
		goto error;
	}

	/*
	 * Now process the contents.  We do this until we find the
	 * closing brace.
	 */
	
	while ((ret = get_line(&argc, &argv)) == 1) {
		if (strcmp(argv[0], right_brace) == 0) {
			if (argc != 1) {
				parse_error("Syntax error.");
				goto error;
			}
			break;
		}
		/*
		 * There are two special keywords allowed here:
		 * ip_address and comment.  Options may also appear
		 * here.
		 */
		if (strcmp(argv[0], "comment") == 0) {
			continue;
		}
		else if (strcmp(argv[0], "ip_address") == 0) {
			if (argc != 2) {
				parse_error("Syntax error.");
				goto error;
			}
			if (!get_addr(argv[1], &client->addr, NULL)) {
				goto error;
			}
			continue;
		}
		if (!proc_option_setting(&client->options, argc, argv)) {
			goto error;
		}
	}
	if (ret == 0) {
		parse_error("End of file encountered in client entry.");
		goto error;
	}
	else if (ret < 0) {
		goto error;
	}

	if ((ret = add_client(client)) == -1) {
		parse_error("An entry already exists for this client.");
		goto error;
	}
	else if (ret == -2) {
		goto error;
	}

	return 1;

error:
	free(client->id);
	free_options(client->options);
	return 0;
}

static int
proc_user_class_entry(int argc, char **argv)
{
	UserClass *user_class;
	int ret;

	if (argc != 3 || strcmp(argv[2], left_brace) != 0) {
		parse_error("Syntax error.");
		return 0;
	}

	if (!(user_class = str_alloc(UserClass))) {
		malloc_error("proc_user_class_entry");
		return 0;
	}

	user_class->options = NULL;

	/*
	 * Get the class ID, which is a string.
	 */
	
	if (!get_string(argv[1], &user_class->class, &user_class->class_len)) {
		return 0;
	}

	if (user_class->class_len < 1 || user_class->class_len > MAX_OPT_LEN) {
		parse_error("User class must be between 1 and %d bytes.",
			MAX_OPT_LEN);
		goto error;
	}

	/*
	 * Process the contents.  Other than options, the only thing
	 * that can appear here is comment.
	 */

	while ((ret = get_line(&argc, &argv)) == 1) {
		if (strcmp(argv[0], right_brace) == 0) {
			if (argc != 1) {
				parse_error("Syntax error.");
				goto error;
			}
			break;
		}
		if (strcmp(argv[0], "comment") == 0) {
			continue;
		}
		if (!proc_option_setting(&user_class->options, argc, argv)) {
			goto error;
		}
	}
	if (ret == 0) {
		parse_error("End of file encountered in user class entry.");
		goto error;
	}
	else if (ret < 0) {
		goto error;
	}

	if ((ret = add_user_class(user_class)) == -1) {
		parse_error("An entry already exists for this user class.");
		goto error;
	}
	else if (ret == -2) {
		goto error;
	}

	return 1;

error:
	free(user_class->class);
	free_options(user_class->options);
	return 0;
}

static int
proc_vendor_class_entry(int argc, char **argv)
{
	VendorClass *vendor_class;
	int ret;

	if (argc != 3 || strcmp(argv[2], left_brace) != 0) {
		parse_error("Syntax error.");
		return 0;
	}

	if (!(vendor_class = str_alloc(VendorClass))) {
		malloc_error("proc_vendor_class_entry");
		return 0;
	}

	vendor_class->options = NULL;

	/*
	 * Get the class ID, which can be a quoted string or binary.
	 */
	
	if (argv[1][0] == '"') {
		if (!get_string(argv[1], &vendor_class->class,
		    &vendor_class->class_len)) {
			return 0;
		}
	}
	else if (!get_binary(argv[1], &vendor_class->class,
	      &vendor_class->class_len)) {
		return 0;
	}

	if (vendor_class->class_len < 1
	    || vendor_class->class_len > MAX_OPT_LEN) {
		parse_error("Vendor class must be between 1 and %d bytes.",
			MAX_OPT_LEN);
		goto error;
	}

	/*
	 * Process the contents.  Other than options, the only thing
	 * that can appear here is comment.
	 */

	while ((ret = get_line(&argc, &argv)) == 1) {
		if (strcmp(argv[0], right_brace) == 0) {
			if (argc != 1) {
				parse_error("Syntax error.");
				goto error;
			}
			break;
		}
		if (strcmp(argv[0], "comment") == 0) {
			continue;
		}
		if (!proc_option_setting(&vendor_class->options, argc, argv)) {
			goto error;
		}
	}
	if (ret == 0) {
		parse_error("End of file encountered in vendor class entry.");
		goto error;
	}
	else if (ret < 0) {
		goto error;
	}

	if ((ret = add_vendor_class(vendor_class)) == -1) {
		parse_error("An entry already exists for this vendor class.");
		goto error;
	}
	else if (ret == -2) {
		goto error;
	}

	return 1;

error:
	free(vendor_class->class);
	free_options(vendor_class->options);
	return 0;
}

/*
 * get_lease_time -- convert a string to a lease time.
 * Returns 1 if ok or 0 if invalid.
 */

static int
get_lease_time(char *str, u_long *lease)
{
	if (strcmp(str, "infinite") == 0) {
		*lease = LEASE_INFINITY;
		return 1;
	}
	else if (!get_uint32(str, lease, NULL)) {
		return 0;
	}
	else if (*lease == 0) {
		parse_error("Invalid lease time.");
		return 0;
	}

	return 1;
}

/*
 * get_timer -- get a setting for T1 or T2, which are per thousand values
 * Returns 1 if valid, 0 if not.
 */

static int
get_timer(char *str, int *val)
{
	if (!is_numeric(str) || strlen(str) > 4
	    || (*val = atoi(str)) > 1000) {
		parse_error("A timer value must be between 0 and 1000.");
		return 0;
	}

	return 1;
}

static int
proc_subnet_entry(int argc, char **argv)
{
	Subnet *subnet;
	int ret, len;
	char *keyword;

	if (argc != 3 || strcmp(argv[2], left_brace) != 0) {
		parse_error("Syntax error.");
		return 0;
	}

	if (!(subnet = str_alloc(Subnet))) {
		malloc_error("proc_subnet_entry");
		return 0;
	}

	subnet->pool = NULL;
	subnet->options = NULL;
	subnet->lease_dflt = DFLT_LEASE_DFLT;
	subnet->lease_max = DFLT_LEASE_MAX;
	subnet->t1 = DFLT_T1;
	subnet->t2 = DFLT_T2;

	/*
	 * Get the subnet number.
	 */
	
	if (!inet_aton(argv[1], &subnet->subnet)) {
		parse_error("Invalid subnet number.");
		goto error;
	}

	/*
	 * We store the subnet number in host order.
	 */
	
	subnet->subnet.s_addr = ntohl(subnet->subnet.s_addr);

	/*
	 * Set the mask to the default for the address class.
	 */

	if (IN_CLASSA(subnet->subnet.s_addr)) {
		subnet->mask.s_addr = IN_CLASSA_NET;
	}
	else if (IN_CLASSB(subnet->subnet.s_addr)) {
		subnet->mask.s_addr = IN_CLASSB_NET;
	}
	else if (IN_CLASSC(subnet->subnet.s_addr)) {
		subnet->mask.s_addr = IN_CLASSC_NET;
	}
	else {
		parse_error("Invalid subnet number.");
		goto error;
	}

	/*
	 * Process contents.
	 */

	while ((ret = get_line(&argc, &argv)) == 1) {
		keyword = argv[0];
		if (strcmp(keyword, right_brace) == 0) {
			if (argc != 1) {
				parse_error("Syntax error.");
				goto error;
			}
			break;
		}
		if (strcmp(keyword, "mask") == 0) {
			if (argc != 2) {
				parse_error("Syntax error.");
				goto error;
			}
			if (!inet_aton(argv[1], &subnet->mask)) {
				parse_error("Invalid mask.");
				goto error;
			}
			subnet->mask.s_addr = ntohl(subnet->mask.s_addr);
		}
		else if (strcmp(keyword, "pool") == 0) {
			if (argc != 2) {
				parse_error("Syntax error.");
				goto error;
			}
			if (!get_string(argv[1], &subnet->pool, &len)) {
				goto error;
			}
		}
		else if (strcmp(keyword, "lease_dflt") == 0) {
			if (argc != 2) {
				parse_error("Syntax error.");
				goto error;
			}
			if (!get_lease_time(argv[1], &subnet->lease_dflt)) {
				goto error;
			}
		}
		else if (strcmp(keyword, "lease_max") == 0) {
			if (argc != 2) {
				parse_error("Syntax error.");
				goto error;
			}
			if (!get_lease_time(argv[1], &subnet->lease_max)) {
				goto error;
			}
		}
		else if (strcmp(keyword, "t1") == 0) {
			if (argc != 2) {
				parse_error("Syntax error.");
				goto error;
			}
			if (!get_timer(argv[1], &subnet->t1)) {
				goto error;
			}
		}
		else if (strcmp(keyword, "t2") == 0) {
			if (argc != 2) {
				parse_error("Syntax error.");
				goto error;
			}
			if (!get_timer(argv[1], &subnet->t2)) {
				goto error;
			}
		}
		else if (strcmp(keyword, "comment") == 0) {
			continue;
		}
		else if (!proc_option_setting(&subnet->options, argc, argv)) {
			goto error;
		}
	}
	if (ret == 0) {
		parse_error("End of file encountered in subnet entry.");
		goto error;
	}
	else if (ret < 0) {
		goto error;
	}

	if ((ret = add_subnet(subnet)) == -1) {
		parse_error("An entry already exists for this subnet.");
		goto error;
	}
	else if (ret == -2) {
		goto error;
	}

	return 1;

error:
	if (subnet->pool) {
		free(subnet->pool);
	}
	free_options(subnet->options);
	return 0;
}

static int
proc_global_entry(int argc, char **argv)
{
	int ret;

	if (global_options) {
		parse_error("Global options already defined.");
		return 0;
	}

	while ((ret = get_line(&argc, &argv)) == 1) {
		if (strcmp(argv[0], right_brace) == 0) {
			if (argc != 1) {
				parse_error("Syntax error.");
				goto error;
			}
			break;
		}
		if (!proc_option_setting(&global_options, argc, argv)) {
			goto error;
		}
	}
	if (ret == 0) {
		parse_error("End of file encountered in global entry.");
		goto error;
	}
	else if (ret < 0) {
		goto error;
	}

	return 1;

error:
	free_options(global_options);
	return 0;
}

static int
proc_option_entry(int argc, char **argv)
{
	int ret, code, type;
	OptionDesc *opt;
	OptionType *otp;
	char type_set, min_len_set, max_len_set;
	char *keyword, *min_val_str, *max_val_str;
	int type_max_len;

	if (argc != 3 || strcmp(argv[2], left_brace) != 0) {
		parse_error("Syntax error.");
		return 0;
	}

	if (!is_numeric(argv[1]) || strlen(argv[1]) > 3
	    || (code = atoi(argv[1])) < MIN_USER_OPTION
	    || code > MAX_USER_OPTION) {
		parse_error("Invalid option number.");
		return 0;
	}

	if (user_options[code]) {
		parse_error("Redefinition of option %d", code);
		return 0;
	}

	if (!(opt = str_alloc(OptionDesc))
	    || !(opt->name = malloc(10))) {
		malloc_error("proc_option_entry");
		return 0;
	}

	opt->code = code;
	sprintf(opt->name, "option%d", code);
	opt->array = 0;
	opt->min_val = NULL;
	opt->max_val = NULL;
	type_set = 0;
	min_len_set = 0;
	max_len_set = 0;
	min_val_str = NULL;
	max_val_str = NULL;

	while ((ret = get_line(&argc, &argv)) == 1) {
		keyword = argv[0];
		if (strcmp(keyword, right_brace) == 0) {
			if (argc != 1) {
				parse_error("Syntax error.");
				goto error;
			}
			break;
		}
		if (strcmp(keyword, "name") == 0) {
			continue;
		}
		else if (strcmp(keyword, "type") == 0) {
			if (type_set) {
				parse_error("Type already defined.");
				goto error;
			}
			if (argc == 3 && strcmp(argv[2], "array") == 0) {
				opt->array = 1;
			}
			else if (argc != 2) {
				parse_error("Syntax error.");
				goto error;
			}
			for (type = 0; type < NUM_OPTION_TYPES; type++) {
				if (strcmp(argv[1],
				    option_types[type].name) == 0) {
					break;
				}
			}
			if (type == NUM_OPTION_TYPES) {
				parse_error("Unknown option type %s", argv[1]);
				goto error;
			}
			if (opt->array
			    && (type == OTYPE_STRING || type == OTYPE_BINARY)) {
				parse_error("Can't have array of string or binary.");
				goto error;
			}
			opt->type = type;
			type_set = 1;
			otp = &option_types[type];
		}
		else if (strcmp(keyword, "min_val") == 0) {
			if (min_val_str) {
				parse_error("Minimum value already defined.");
				goto error;
			}
			if (argc != 2) {
				parse_error("Syntax error.");
				goto error;
			}
			/*
			 * Just save the string for now -- we can't process
			 * this until the type is defined, so we just do
			 * it at the end.
			 */
			if (!(min_val_str = strdup(argv[1]))) {
				malloc_error("proc_option_entry");
				goto error;
			}
		}
		else if (strcmp(keyword, "max_val") == 0) {
			if (max_val_str) {
				parse_error("Maximum value already defined.");
				goto error;
			}
			if (argc != 2) {
				parse_error("Syntax error.");
				goto error;
			}
			/*
			 * Just save the string for now -- we can't process
			 * this until the type is defined, so we just do
			 * it at the end.
			 */
			if (!(max_val_str = strdup(argv[1]))) {
				malloc_error("proc_option_entry");
				goto error;
			}
		}
		else if (strcmp(keyword, "min_length") == 0) {
			if (min_len_set) {
				parse_error("Minimum length already defined.");
				goto error;
			}
			if (argc != 2) {
				parse_error("Syntax error.");
				goto error;
			}
			if (!is_numeric(argv[1]) || strlen(argv[1]) > 3) {
				parse_error("Invalid minimum length.");
				goto error;
			}
			opt->min_len = atoi(argv[1]);
			min_len_set = 1;
		}
		else if (strcmp(keyword, "max_length") == 0) {
			if (max_len_set) {
				parse_error("Maximum length already defined.");
				goto error;
			}
			if (argc != 2) {
				parse_error("Syntax error.");
				goto error;
			}
			if (!is_numeric(argv[1]) || strlen(argv[1]) > 3) {
				parse_error("Invalid maximum length.");
				goto error;
			}
			opt->max_len = atoi(argv[1]);
			max_len_set = 1;
		}
		else if (strcmp(keyword, "comment") == 0) {
			continue;
		}
		else {
			parse_error("Unknown keyword %s", keyword);
			goto error;
		}
	}
	if (ret == 0) {
		parse_error("End of file encountered in option entry.");
		goto error;
	}
	else if (ret < 0) {
		goto error;
	}

	/*
	 * Option type must be set.
	 */
	
	if (!type_set) {
		parse_error("Option type not specified.");
		goto error;
	}

	/*
	 * Min & max values are only allowed for integer types.
	 */

	if ((min_val_str || max_val_str)
	    && (type != OTYPE_UINT8 && type != OTYPE_INT8
	    && type != OTYPE_UINT16 && type != OTYPE_INT16
	    && type != OTYPE_UINT32 && type != OTYPE_INT32)) {
		parse_error("Can't have minimum or maximum values for type %s",
			otp->name);
		goto error;
	}

	/*
	 * Convert min & max value strings to the appropriate type.
	 */

	if (min_val_str) {
		if (!(opt->min_val = malloc(otp->size))) {
			malloc_error("proc_option_entry");
			goto error;
		}
		if (!(*otp->get_func)(min_val_str, opt->min_val, NULL)) {
			goto error;
		}
		free(min_val_str);
		min_val_str = NULL;
	}

	if (max_val_str) {
		if (!(opt->max_val = malloc(otp->size))) {
			malloc_error("proc_option_entry");
			goto error;
		}
		if (!(*otp->get_func)(max_val_str, opt->max_val, NULL)) {
			goto error;
		}
		free(max_val_str);
		max_val_str = NULL;
	}

	/*
	 * Only arrays and string types can have min & max length.
	 */

	if ((min_len_set || max_len_set)
	    && (type != OTYPE_STRING && type != OTYPE_BINARY && !opt->array)) {
		parse_error("Can't have minimum or maximum length for type %s",
			otp->name);
		goto error;
	}

	/*
	 * Check and/or set minimum & maximum lengths.
	 */
	
	if (type == OTYPE_STRING || type == OTYPE_BINARY) {
		if (min_len_set) {
			if (opt->min_len > MAX_OPT_LEN) {
				parse_error("Maximum length is %d",
					MAX_OPT_LEN);
				goto error;
			}
		}
		else {
			opt->min_len = 1;
		}
		if (max_len_set) {
			if (opt->max_len > MAX_OPT_LEN) {
				parse_error("Maximum length is %d",
					MAX_OPT_LEN);
				goto error;
			}
		}
		else {
			opt->max_len = MAX_OPT_LEN;
		}
		if (opt->min_len > opt->max_len) {
			parse_error("Minimum length greater than maximum length");
			goto error;
		}
	}
	else if (opt->array) {
		type_max_len = MAX_OPT_LEN / otp->size;
		if (min_len_set) {
			if (opt->min_len > type_max_len) {
				parse_error("Maximum length is %d",
					type_max_len);
				goto error;
			}
		}
		else {
			opt->min_len = 1;
		}
		if (max_len_set) {
			if (opt->max_len > type_max_len) {
				parse_error("Maximum length is %d",
					type_max_len);
				goto error;
			}
		}
		else {
			opt->max_len = type_max_len;
		}
		if (opt->min_len > opt->max_len) {
			parse_error("Minimum length greater than maximum length");
			goto error;
		}
	}

	user_options[code] = opt;
	return 1;

error:
	free(opt->name);
	if (opt->min_val) {
		free(opt->min_val);
	}
	if (opt->max_val) {
		free(opt->max_val);
	}
	if (min_val_str) {
		free(min_val_str);
	}
	if (max_val_str) {
		free(max_val_str);
	}
	free(opt);
	return 0;
}

static int
proc_server_entry(int argc, char **argv)
{
	int ret, len, val;
	char *keyword;

	if (server_params.set) {
		parse_error("Server parameters already set.");
		return 0;
	}

	while ((ret = get_line(&argc, &argv)) == 1) {
		keyword = argv[0];
		if (strcmp(keyword, right_brace) == 0) {
			if (argc != 1) {
				parse_error("Syntax error.");
				goto error;
			}
			break;
		}
		if (strcmp(keyword, "aas_server") == 0) {
			if (argc != 2) {
				parse_error("Syntax error.");
				goto error;
			}
			if (!get_addr(argv[1], &server_params.aas_server,
			    NULL)) {
				goto error;
			}
			server_params.aas_remote = 1;
		}
		else if (strcmp(keyword, "aas_password") == 0) {
			if (argc != 2) {
				parse_error("Syntax error.");
				goto error;
			}
			if (!get_string(argv[1], &server_params.aas_password,
			    &len)) {
				goto error;
			}
		}
		else if (strcmp(keyword, "option_overload") == 0) {
			if (argc != 2) {
				parse_error("Syntax error.");
				goto error;
			}
			if (!get_boolean(argv[1],
			    &server_params.option_overload, NULL)) {
				goto error;
			}
		}
		else if (strcmp(keyword, "lease_res") == 0) {
			if (argc != 2) {
				parse_error("Syntax error.");
				goto error;
			}
			if (!is_numeric(argv[1]) || strlen(argv[1]) > 4
			    || (server_params.lease_res = atoi(argv[1])) < 60
			    || server_params.lease_res > 3600) {
				goto error;
			}
		}
		else if (strcmp(keyword, "lease_pad") == 0) {
			if (argc != 2) {
				parse_error("Syntax error.");
				goto error;
			}
			if (!is_numeric(argv[1]) || strlen(argv[1]) > 4
			  || (val = atoi(argv[1])) > 100) {
				goto error;
			}
			server_params.lease_pad = ((double) val + 1000.0)
				/ 1000.0;
		}
		else if (strcmp(keyword, "address_probe") == 0) {
			if (argc != 2) {
				parse_error("Syntax error.");
				goto error;
			}
			if (!get_boolean(argv[1],
			    &server_params.address_probe, NULL)) {
				goto error;
			}
		}
		else {
			parse_error("Unknown keyword %s", keyword);
		}
	}
	if (ret == 0) {
		parse_error("End of file encountered in server entry.");
		goto error;
	}
	else if (ret < 0) {
		goto error;
	}

	if (server_params.aas_remote) {
		if (!server_params.aas_password) {
			parse_error("Password must be specified for remote address server.");
			goto error;
		}
	}

	server_params.set = 1;

	return 1;

error:
	if (server_params.aas_password) {
		free(server_params.aas_password);
		server_params.aas_password = NULL;
	}
	return 0;
}

void
configure(void)
{
	int ret, argc;
	char **argv;
	struct stat st;

	config_fp = NULL;

	if (!(config_fp = fopen(config_file, "r"))) {
		report(LOG_ERR, "Unable to open configuration file %s: %m",
			config_file);
		goto error;
	}

	/*
	 * Save the config file modification time.
	 */
	
	if (fstat(fileno(config_fp), &st) == -1) {
		report(LOG_ERR, "Unable to get status for configuration file %s: %m",
			config_file);
		goto error;
	}
	config_file_mtime = st.st_mtime;

	reset_database();

	line_num = 0;

	while ((ret = get_line(&argc, &argv)) == 1) {
		if (strcmp(argv[0], "client") == 0) {
			if (!proc_client_entry(argc, argv)) {
				goto error;
			}
		}
		else if (strcmp(argv[0], "user_class") == 0) {
			if (!proc_user_class_entry(argc, argv)) {
				goto error;
			}
		}
		else if (strcmp(argv[0], "vendor_class") == 0) {
			if (!proc_vendor_class_entry(argc, argv)) {
				goto error;
			}
		}
		else if (strcmp(argv[0], "subnet") == 0) {
			if (!proc_subnet_entry(argc, argv)) {
				goto error;
			}
		}
		else if (strcmp(argv[0], "global") == 0) {
			if (!proc_global_entry(argc, argv)) {
				goto error;
			}
		}
		else if (strcmp(argv[0], "option") == 0) {
			if (!proc_option_entry(argc, argv)) {
				goto error;
			}
		}
		else if (strcmp(argv[0], "server") == 0) {
			if (!proc_server_entry(argc, argv)) {
				goto error;
			}
		}
		else {
			parse_error("Unknown entry type %s", argv[0]);
			goto error;
		}
	}

	fclose(config_fp);
	config_ok = 1;
	return;

error:
	if (config_fp) {
		fclose(config_fp);
	}
	config_ok = 0;
	report(LOG_ERR, "Invalid configuration -- requests will be ignored.");
	return;
}

/*
 * config_file_changed -- returns 1 if the configuration file has been
 * modified since the last time it was read.  Otherwise, 0 is returned.
 */

int
config_file_changed(void)
{
	struct stat st;

	if (stat(config_file, &st) == -1) {
		report(LOG_ERR, "Unable to get status for configuration file %s: %m",
			config_file);
		return 0;
	}

	return (st.st_mtime > config_file_mtime);
}
