#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:internet/route_util.c	1.9"
#endif


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <limits.h>
#include <string.h>
#include <libgen.h>
#include <wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#include "route_util.h"

/* fcn prototypes */
static void		cleanupLinks(ConfigLine_t *, FileKind_t);
static ConfigLine_t *	queryConfigFile(ConfigFile_t *, QueryConfig_t *);
static int		createNetmaskFromAddr(char *, Class_t *, char **);
static void		normalizeAddr(char *, char **);
static int		checkNetstatOutput(char **);
static int		getDefaultDevice(char **);

/* macros */
#define	GOBBLEWHITE(p)	while (isspace(*p)) ++p
#define	TRAVERSETOKEN(p)	while (*p != 0 && !isspace(*p)) ++p

/*
 *	return:	0 if success
 *		1 if failure
 */
int
readConfigFile(ConfigFile_t * cf)
{
	ConfigLine_t *	currentLine;
	ConfigLine_t **	nextLine;
	void *		token;
	FILE *		fd;
	char		buf[BUFSIZ];
	char *		tmp;
	char *		tmp2;
	int		i, tokCount;	
	char *		tokens[10];
	char		path[128];
	
	if (!cf) {
		return 1;
	}
	if (cf->cf_kind == ConfigFile) {
		strcpy(path, CONFIGPATH);
	} else {
		strcpy(path, INTERFACEPATH);
	}
	if ((fd = fopen(path, "r")) == NULL) {
		return 1;
	}
	cf->cf_fileName = strdup(path);

	cf->cf_numLines = 0;

	nextLine = &cf->cf_lines;

	while (fgets(buf, BUFSIZ, fd)) {
		currentLine = (ConfigLine_t *)calloc(1, sizeof(ConfigLine_t));
		if (!currentLine) {
			return 1;
		}
		*nextLine = currentLine;
		nextLine = &currentLine->cl_next;
		cf->cf_numLines++;
		currentLine->cl_origLine = strdup(buf);

		tmp = buf;
		GOBBLEWHITE(tmp);
		if (*tmp == '#') {
			currentLine->cl_kind = CommentLine;
			continue;
		} else {
			if ((!(*tmp)) || (*tmp == '\n')) {
				currentLine->cl_kind = BlankLine;
				continue;
			} else {
				currentLine->cl_kind = DataLine;
			}
		}

		tokCount=0;
		while ((tmp2 = strchr(tmp, ':')) != NULL) {
			if (tmp2 != tmp) {
				*tmp2 = '\0';
				tokens[tokCount] = strdup(tmp);
			} else {
				tokens[tokCount] = NULL;
			}
			tmp = ++tmp2;
			tokCount++;
		}
		if (*tmp != '\n') {
			tmp2 = tmp;
			while (*tmp2 != '\n') {
				++tmp2;
			}
			tokens[tokCount] = strndup(tmp, tmp2-tmp);
			tokCount++;
		}
		if (tokCount != cf->cf_minFields) {
			currentLine->cl_kind = BadLine;
			for (i = 0; i < tokCount; i++) {
				if (tokens[i]) {
					free(tokens[i]);
				}
			}
			continue;
		}

		if (cf->cf_kind == ConfigFile) {
			token = (ConfigToken_t *)calloc(1, sizeof(ConfigToken_t));
			currentLine->cl_tokens = token;
			((ConfigToken_t *)currentLine->cl_tokens)->ct_level = strdup(tokens[C_LEVEL]);
			((ConfigToken_t *)currentLine->cl_tokens)->ct_fullCmdPath = strdup(tokens[C_FULLPATH]);
			((ConfigToken_t *)currentLine->cl_tokens)->ct_overridingCmdPath = strdup(tokens[C_OVERRIDE]);
			((ConfigToken_t *)currentLine->cl_tokens)->ct_isRunning = strdup(tokens[C_ISRUNNING]);
			((ConfigToken_t *)currentLine->cl_tokens)->ct_configFile = strdup(tokens[C_CONFIGFILE]);
			((ConfigToken_t *)currentLine->cl_tokens)->ct_cmdOptions = strdup(tokens[C_OPTIONS]);
		} else {
			token = (InterfaceToken_t *)calloc(1, sizeof(InterfaceToken_t));
			currentLine->cl_tokens = token;
			((InterfaceToken_t *)currentLine->cl_tokens)->it_prefix = strdup(tokens[I_PREFIX]);
			((InterfaceToken_t *)currentLine->cl_tokens)->it_unit = strdup(tokens[I_UNIT]);
			((InterfaceToken_t *)currentLine->cl_tokens)->it_addr = strdup(tokens[I_ADDR]);
			((InterfaceToken_t *)currentLine->cl_tokens)->it_device = strdup(tokens[I_DEVICE]);
			((InterfaceToken_t *)currentLine->cl_tokens)->it_ifconfigOptions = strdup(tokens[I_IFCONFIGOPT]);
			((InterfaceToken_t *)currentLine->cl_tokens)->it_slinkOptions = strdup(tokens[I_SLINKOPT]);
		}
		for (i = 0; i < (int)cf->cf_minFields; i++) {
			if (tokens[i]) {
				free(tokens[i]);
			}
		}
	}
	fclose(fd);
	return 0;	
}

void
cleanupConfigFile(ConfigFile_t * cf)
{
	if (!cf) {
		return;
	}
	cleanupLinks(cf->cf_lines, cf->cf_kind);
	if (cf->cf_fileName) {
		free(cf->cf_fileName);
	}
}

static void
cleanupLinks(ConfigLine_t * ptr, FileKind_t kind)
{
	if (ptr) {
		if (ptr->cl_tokens) {
			if (kind == ConfigFile) {
				if (((ConfigToken_t *)ptr->cl_tokens)->ct_level)
					free(((ConfigToken_t *)ptr->cl_tokens)->ct_level);
				if (((ConfigToken_t *)ptr->cl_tokens)->ct_fullCmdPath)
					free(((ConfigToken_t *)ptr->cl_tokens)->ct_fullCmdPath);
				if (((ConfigToken_t *)ptr->cl_tokens)->ct_overridingCmdPath)
					free(((ConfigToken_t *)ptr->cl_tokens)->ct_overridingCmdPath);
				if (((ConfigToken_t *)ptr->cl_tokens)->ct_isRunning)
					free(((ConfigToken_t *)ptr->cl_tokens)->ct_isRunning);
				if (((ConfigToken_t *)ptr->cl_tokens)->ct_configFile)
					free(((ConfigToken_t *)ptr->cl_tokens)->ct_configFile);
				if (((ConfigToken_t *)ptr->cl_tokens)->ct_cmdOptions)
					free(((ConfigToken_t *)ptr->cl_tokens)->ct_cmdOptions);
			} else {
				if (((InterfaceToken_t *)ptr->cl_tokens)->it_prefix)
					free(((InterfaceToken_t *)ptr->cl_tokens)->it_prefix);
				if (((InterfaceToken_t *)ptr->cl_tokens)->it_unit)
					free(((InterfaceToken_t *)ptr->cl_tokens)->it_unit);
				if (((InterfaceToken_t *)ptr->cl_tokens)->it_addr)
					free(((InterfaceToken_t *)ptr->cl_tokens)->it_addr);
				if (((InterfaceToken_t *)ptr->cl_tokens)->it_device)
					free(((InterfaceToken_t *)ptr->cl_tokens)->it_device);
				if (((InterfaceToken_t *)ptr->cl_tokens)->it_ifconfigOptions)
					free(((InterfaceToken_t *)ptr->cl_tokens)->it_ifconfigOptions);
				if (((InterfaceToken_t *)ptr->cl_tokens)->it_slinkOptions)
					free(((InterfaceToken_t *)ptr->cl_tokens)->it_slinkOptions);
			}
			free(ptr->cl_tokens);
		}
		if (ptr->cl_origLine)
			free(ptr->cl_origLine);
		if (ptr->cl_newLine) {
			free(ptr->cl_newLine);
		}
		cleanupLinks(ptr->cl_next, kind);
		if (ptr)
			free(ptr);
	}
}

/*
 *	return:	0 if success
 *		1 if failure
 */
int
writeConfigFile(ConfigFile_t * cf)
{
	FILE *		fd;
	ConfigLine_t *	line;

	if (!cf) {
		return 1;
	}
	if ((fd = fopen(cf->cf_fileName, "w")) == NULL) {
		return 1;
	}
	line = cf->cf_lines;
	while (line) {
		if (line->cl_newLine) {
			(void)fputs(line->cl_newLine, fd);
		} else {
			(void)fputs(line->cl_origLine, fd);
		}
		line = line->cl_next;
	}
	fflush(fd);
	fclose(fd);
	return 0;
}

/*
 *	return:	ptr to ConfigLine_t structure where match occurred
 *		NULL if no match found
 */
static ConfigLine_t *
queryConfigFile(ConfigFile_t * cf, QueryConfig_t * qc)
{
	ConfigLine_t *	line;
	QueryConfig_t *	pqc;
	char *		ptr;
	char *		regc;
	uchar_t		error = 0;

	if (!cf || !qc) {
		return NULL;
	}
	line = cf->cf_lines;
	while (line) {
		if (line->cl_kind != DataLine) {
			line = line->cl_next;
			continue;
		}
		pqc = qc;
		while (!(pqc->qc_flags & QC_END)) {
			if (!(pqc->qc_flags & QC_MATCH)) {
				pqc++;
				continue;
			}
			if (pqc->qc_flags & QC_ISREGULAR) {
				regc = regcmp(pqc->qc_string, (char *)0);
				ptr = regex(regc, line->cl_origLine);
				free(regc);
			} else {
				ptr = strstr(line->cl_origLine, pqc->qc_string);
			}
			if (!ptr) {
				error = 1;
			}
			pqc++;
		}
		if (error == 0) {
			break;
		}
		line = line->cl_next;
		error = 0;
	}
	return line; /* should be null if no match */
}

/*
 *	return:	0 if success
 *		1 if failure
 */
static int
getDefaultDevice(char ** dev)
{
	FILE *	fd;
	char	buf[BUFSIZ];
	char *	protocol = NULL;
	char *	p1;
	char *	p2;
	int	ret;

	if ((fd = fopen(NETDRIVERSPATH, "r")) == NULL) {
		return 1;
	}
	while (fgets(buf, BUFSIZ, fd)) {
		p1 = buf;
		if ((!(*p1)) || (*p1 == '\n')) {
			continue;
		}
		GOBBLEWHITE(p1);
		TRAVERSETOKEN(p1);
		/* carefully check for whitespace leading to EOL... */
		while(isspace(*p1)) {
			if (*p1 == '\n') {
				break;
			}
			++p1;
		}
		if (*p1 == '\n') {
			continue;
		}
		p2 = p1;
		TRAVERSETOKEN(p2);
		protocol = strndup(p1, p2-p1);
		if (!strcmp(protocol, "inet")) {
			/* we have what we're looking for */
			p1 = buf;
			p2 = p1;
			TRAVERSETOKEN(p2);
			*dev = strndup(p1, p2-p1);
			free(protocol);
			fclose(fd);

			return 0;
		}
		free(protocol);
	}
	fclose(fd);
	return 1;
}

/*
 *	return:	0 if success
 *		1 if failure
 */
int
modifyDefaultRouter(ConfigFile_t * cf, char * router)
{
	static QueryConfig_t	qc[] = {
		{ ROUTEPATH,		(QC_MATCH) },
		{ "add[ \t]*default",	(QC_MATCH | QC_ISREGULAR) },
		{ NULL,			(QC_END) }
	};
	ConfigToken_t *	tok;
	ConfigLine_t *	cl;
	char *		ptr;
	char *		ptr1;
	char *		ptr2;
	char		tmpbuf[BUFSIZ];

	if (!cf || !router) {
		return 1;
	}
	cl = queryConfigFile(cf, qc);
	if (cl == NULL) {
		/* we don't have a router line and must append one */
		for (cl = cf->cf_lines; cl->cl_next; cl = cl->cl_next)
			; /*empty*/
		cl->cl_next = (ConfigLine_t *)calloc(1, sizeof(ConfigLine_t));
		if (!cl->cl_next) {
			return 1;
		}
		cl = cl->cl_next;
		sprintf(tmpbuf, "%s:%s::y::add default %s 1:\n",
			ROUTETOK, ROUTEPATH, router);
		cl->cl_newLine = strdup(tmpbuf);
		return 0;
	}
	/* we have a router line */
	cl->cl_newLine = (char *)calloc(strlen(cl->cl_origLine) + 20, sizeof(char));
	if (!cl->cl_newLine) {
		return 1;
	}
	tok = (ConfigToken_t *)cl->cl_tokens;
	sprintf (
		cl->cl_newLine, "%s:%s:%s:%s:%s:",
		tok->ct_level,
		tok->ct_fullCmdPath,
		tok->ct_overridingCmdPath,
		"y",
		tok->ct_configFile
	);
	ptr1 = tok->ct_cmdOptions;
	GOBBLEWHITE(ptr1);
	ptr2 = strstr(ptr1, "add");
	/* copy everything preceding "add" */
	strncat(cl->cl_newLine, ptr1, ptr2-ptr1);
	sprintf(tmpbuf, "add default %s 1:\n", router);
	strcat(cl->cl_newLine, tmpbuf);

	return 0;
}

void
breakupAddr(char * addrIn, char * a, char * b, char * c, char * d)
{
	static char *	re = \
"(([0-9]){1,4})$0\\.\
(([0-9]){1,4})$1\\.\
(([0-9]){1,4})$2\\.\
(([0-9]){1,4})$3";

	char *	naddr;
	char *	rep;

	if (!addrIn) {
		return;
	}
	normalizeAddr(addrIn, &naddr);
	/* break up addr into four parts */
	rep = regcmp(re, (char *)0);
	(void)regex(rep, naddr, a, b, c, d);
	free(rep);
	free(naddr);
	return;
}

void
mungeAddr(char * addrIn, char ** addr1Out, char ** addr2Out, Class_t class)
{
	char	a[4], b[4], c[4], d[4];
	char	buf[BUFSIZ];

	if (!addrIn) {
		return;
	}

	breakupAddr(addrIn, a, b, c, d);

	/* do the munging */
	switch (class) {
		case ClassA: {
			sprintf(buf, "%s.#.#.#", a);
			*addr1Out = strdup(buf);
			sprintf(buf, "#.%s.%s.%s", b, c, d);
			*addr2Out = strdup(buf);
			break;
		}
		case ClassB: {
			sprintf(buf, "%s.%s.#.#", a, b);
			*addr1Out = strdup(buf);
			sprintf(buf, "#.#.%s.%s", c, d);
			*addr2Out = strdup(buf);
			break;
		}
		case ClassC: {
			sprintf(buf, "%s.%s.%s.#", a, b, c);
			*addr1Out = strdup(buf);
			sprintf(buf, "#.#.#.%s", d);
			*addr2Out = strdup(buf);
			break;
		}
		default: {
			break;
		}
	}

	return;
}

/*	return:	0 is success
 *		1 if failure
 */
int
decideAddrClass(char * addrIn, Class_t * classOut, char ** netmaskOut)
{
	char	a[4], b[4], c[4], d[4];
	int	firstOctet;
	int	ret;

	if (!addrIn) {
		return 1;
	}
	breakupAddr(addrIn, a, b, c, d);
	firstOctet = atoi(a);
	if (firstOctet <= 0x7f) {
		*netmaskOut = strdup(CLASS_A_NETMASK);
		*classOut = ClassA;
	}
	if (firstOctet >= 0x80 && firstOctet <= 0xbf) {
		*netmaskOut = strdup(CLASS_B_NETMASK);
		*classOut = ClassB;
	}
	if (firstOctet >= 0xc0 && firstOctet <= 0xdf) {
		*netmaskOut = strdup(CLASS_C_NETMASK);
		*classOut = ClassC;
	}
	if (firstOctet > 0xdf) {
		ret = createNetmaskFromAddr(addrIn, classOut, netmaskOut);
	}

	return 0;
}

/*
 *	return: 0 if success
 *		1 if failure
 */
int
getNetmask(ConfigFile_t * cf, char * addr, char ** netmaskOut, Class_t * classOut)
{
	InterfaceToken_t *	tok;
	ConfigLine_t *		cl;
	int			ret;
	char *			dev = NULL;		/* alloc'd */
	char *			netmask = NULL;	/* alloc'd */
	char *			ptr1 = NULL;
	char *			ptr2 = NULL;
	QueryConfig_t		q[] = {
		{ NULL,		(QC_MATCH) },
		{ NULL,		(QC_END) }
	};
	char *			reg1;		/* alloc'd */
	char *			reg2;		/* alloc'd */
	char *			reg3;		/* alloc'd */
	ulong_t			mask;

	if (!cf || !addr) {
		return 1;
	}
	if ((ret = getDefaultDevice(&dev)) != 0) {
		if (dev) {
			free(dev);
		}
		return 1;
	}
	q[0].qc_string = strdup(dev);
	free(dev);
	cl = queryConfigFile(cf, q);
	if (!cl) {
		free(q[0].qc_string);
		return 1;
	}
	tok = (InterfaceToken_t *)cl->cl_tokens;
	ptr1 = tok->it_ifconfigOptions;
	ptr1 = strstr(ptr1, "netmask");
	if (!ptr1) {
		ret = decideAddrClass(addr, classOut, netmaskOut);
		free(q[0].qc_string);
		return 0;
	}
	ptr1 = strpbrk(ptr1, " \t");
	GOBBLEWHITE(ptr1);
	ptr2  = ptr1;
	TRAVERSETOKEN(ptr2);
	netmask = strndup(ptr1, ptr2-ptr1);
	reg1 = regcmp("0x[0-9a-fA-F]", (char *)0);
	reg2 = regcmp("([0-9a-fA-F]){1,4}\\.", (char *)0);
	reg3 = regcmp("[a-zA-Z]", (char *)0);
	ptr1 = regex(reg1, netmask);
	if (ptr1) {
		ptr1 = netmask;
		while (*ptr1 != 0) {
			if (*ptr1 == 'x') {
				++ptr1;
				continue;
			}
			*ptr1 = toupper(*ptr1);
			++ptr1;
		}
		if (!strcmp(netmask, CLASS_A_NETMASK)) {
			*classOut = ClassA;
		} else {
			if (!strcmp(netmask, CLASS_B_NETMASK)) {
				*classOut = ClassB;
			} else {
				if (!strcmp(netmask, CLASS_C_NETMASK)){
					*classOut = ClassC;
				} else {
					*classOut = ClassOther;
				}
			}
		}
		*netmaskOut = strdup(netmask);
		goto getout;
	}
	ptr1 = regex(reg2, netmask);
	if (ptr1) {
		ret = createNetmaskFromAddr(netmask, classOut, netmaskOut);
		goto getout;
	}
	ptr1 = regex(reg3, netmask);
	if (ptr1) {
		/*
		 * this is not correct. we should parse /etc/networks to find
		 * matching token and addr. for now, assume mask field is
		 * either dot-decimal or hex.
		 */
		ret = decideAddrClass(addr, classOut, netmaskOut);
	}

getout:
	free(reg1);
	free(reg2);
	free(reg3);
	free(netmask);
	free(q[0].qc_string);

	return 0;
}

char *
strndup(char * str, int	len)
{
	char *	p;

	if (p = malloc(len + 1)) {
		memcpy(p, str, len);
		p[len] = '\0';
	}
	return p;
}

/*
 *	return:	0 if success
 *		1 if failure
 */
static int
createNetmaskFromAddr(char * addrIn, Class_t * classOut, char ** netmaskOut)
{
	char	octet[4];
	char	buf[BUFSIZ];
	char	mask[11];
	char *	ptr1;
	char *	ptr2;
	int	i;
	int	intOctet;
	char *	naddr;

	if (!addrIn) {
		return 1;
	}
	normalizeAddr(addrIn, &naddr);
	i = 0;
	ptr1 = naddr;
	strcpy(mask, "0x");
	while (ptr1 && (ptr2 = strchr(ptr1, '.'))) {
		strncpy(octet, ptr1, ptr2-ptr1);
		if (!strcmp(octet, "255")) {
			++i;
		}
		intOctet = atoi(octet);
		sprintf(buf, "%x", intOctet);
		strcat(mask, buf);
		ptr1 = ptr2 + 1;
	}
	switch (i) {
		case 1: {
			*classOut = ClassA;
			strcat(mask, "000000");
			break;
		}
		case 2: {
			*classOut = ClassB;
			strcat(mask, "0000");
			break;
		}
		case 3: {
			*classOut = ClassC;
			strcat(mask, "00");
			break;
		}
		default: {
			free(naddr);
			return 1;
		}
	}

	*netmaskOut = strdup(mask);

	free(naddr);
	return 0;
}

/*
 *	return: 0 if success
 *		1 if failure
 */
int
getDefaultRouter(ConfigFile_t * cf, char ** routerOut, AddrKind_t * kind)
{
	ConfigToken_t *		tok;
	ConfigLine_t *		cl;
	int			ret;
	char *			router;		/* alloc'd */
	char *			ptr1;
	char *			ptr2;
	QueryConfig_t		q[] = {
	{ "add[ \t]*default",	(QC_MATCH | QC_ISREGULAR) },
	{ NULL,			(QC_END) }
	};
	char *			reg1;		/* alloc'd */
	char *			reg2;		/* alloc'd */

	if (!cf) {
		return 1;
	}
	*kind = 0;
	cl = queryConfigFile(cf, q);
	if (!cl) {
		/*
		 * ret = checkNetstatOutput(routerOut);
		 * if (ret) {
		 *	*routerOut = NULL;
		 * }
		 * *kind = IsName;
		 * return 0;
		 */
		*routerOut = NULL;
		return 0;
	}
	tok = (ConfigToken_t *)cl->cl_tokens;
	ptr1 = tok->ct_cmdOptions;
	reg1 = regcmp("add[ \t]*default", (char *)0);
	ptr2 = regex(reg1, ptr1);
	free(reg1);
	GOBBLEWHITE(ptr2);
	ptr1 = ptr2;
	TRAVERSETOKEN(ptr2);
	router = strndup(ptr1, ptr2-ptr1);
	reg1 = regcmp("([0-9a-fA-F]){1,4}\\.", (char *)0);
	reg2 = regcmp("[a-zA-Z]", (char *)0);
	ptr1 = regex(reg1, router);
	if (ptr1) {
		*routerOut = strdup(router);
		*kind = IsIPAddr;
		goto getout;
	}
	ptr1 = regex(reg2, router);
	if (ptr1) {
		*routerOut = strdup(router);
		*kind = IsName;
	}

getout:
	free(router);
	free(reg1);
	free(reg2);

	return 0;
}

/*
 *	return: 0 if success
 *		1 if failure
 */
static int
checkNetstatOutput(char ** routerOut)
{
	FILE *	fd;
	char	buf[BUFSIZ];
	char *	ptr;
	char *	ptr2;
	int	ret;

	ret = system("netstat -r | grep default > /tmp/netstatout 2>&1");
	if ((ret == -1) || WEXITSTATUS(ret) != 0) {
		*routerOut = NULL;
		unlink("/tmp/netstatout");
		return 1;
	}
	if ((fd = fopen("/tmp/netstatout", "r")) == NULL) {
		unlink("/tmp/netstatout");
		return 1;
	}
	fgets(buf, BUFSIZ, fd);
	ptr = buf;
	GOBBLEWHITE(ptr);
	if (!strcmp(ptr, "default")) {
		unlink("/tmp/netstatout");
		return 1;
	}
	TRAVERSETOKEN(ptr);
	GOBBLEWHITE(ptr);
	ptr2 = ptr;
	TRAVERSETOKEN(ptr2);
	*routerOut = strndup(ptr, ptr2-ptr);
	fclose(fd);
	unlink("/tmp/netstatout");

	return 0;
}

/*
 *	return: 0 if success
 *		1 if failure
 *	in:	string representing process
 *	out:	process ID
 */
int
findProcess(char * procName, pid_t * procIDOut)
{
	FILE *	fd;
	char	buf[BUFSIZ];
	int	ret;
	char *	ptr1;
	char *	ptr2;
	char *	proc;
	ulong_t	value;

	*procIDOut = -1;
	sprintf(buf, "ps -ef | grep \"%s\" | grep -v grep > /tmp/findproc 2>&1",
		procName);
	if ((ret = system(buf)) == -1) {
		unlink("/tmp/findproc");

		return 1;
	}
	if (WEXITSTATUS(ret) == 0) {
		/* grep succeeded, must parse output file */
		if ((fd = fopen("/tmp/findproc", "r")) == NULL) {
			unlink("/tmp/findproc");

			return 1;
		}
		if ((ptr1 = fgets(buf, BUFSIZ, fd)) == NULL) {
			fclose(fd);
			unlink("/tmp/findproc");

			return 1;
		}
		/* point at second token (proc id) */
		GOBBLEWHITE(ptr1);
		TRAVERSETOKEN(ptr1);
		GOBBLEWHITE(ptr1);
		ptr2 = ptr1;
		TRAVERSETOKEN(ptr2);
		proc = strndup(ptr1, ptr2-ptr1);
		fclose(fd);
		value = strtoul(proc, (char **)NULL, 10);

		*procIDOut = (pid_t)value;

		free(proc);
		unlink("/tmp/findproc");

		return 0;
	} else {
		unlink("/tmp/findproc");

		return 1;
	}
}

/*
 *	return:	0 if success
 *		1 if failure
 */

int
modifyNetmask(ConfigFile_t * cf, char * netmask, char * baddr)
{
	static QueryConfig_t	qc[] = {
		{ NULL,	(QC_MATCH) },
		{ NULL,	(QC_END) }
	};
	InterfaceToken_t *	tok;
	ConfigLine_t *		cl;
	char *			ptr;
	char			tmpbuf[BUFSIZ];
	char *			token;
	char *			device;
	int			ret;
	uchar_t			netmaskExists = 0;
	uchar_t			broadcastExists = 0;

	if (!cf) {
		return 1;
	}
	if ((ret = getDefaultDevice(&device)) == 1) {
		if (device) {
			free(device);
		}
		return 1;
	}

	qc[0].qc_string = device;
	cl = queryConfigFile(cf, qc);
	if (cl == NULL) {
		free(device);
		return 1;
	}

	cl->cl_newLine = (char *)calloc(strlen(cl->cl_origLine) + 50, sizeof(char));
	if (!cl->cl_newLine) {
		free(device);
		return 1;
	}
	tok = (InterfaceToken_t *)cl->cl_tokens;
	sprintf (
		cl->cl_newLine, "%s:%s:%s:%s:",
		tok->it_prefix,
		tok->it_unit,
		tok->it_addr,
		tok->it_device
	);
	ptr = tok->it_ifconfigOptions;
	token = strtok(ptr, " \t");
	while (token) {
		if (!strcmp(token, "netmask")) {
			sprintf(tmpbuf, "netmask %s ", netmask);
			strcat(cl->cl_newLine, tmpbuf);
			token = strtok(NULL, " \t");
			netmaskExists = 1;
		} else if (!strcmp(token, "broadcast")) {
			sprintf(tmpbuf, "broadcast %s ", baddr);
			strcat(cl->cl_newLine, tmpbuf);
			token = strtok(NULL, " \t");
			broadcastExists = 1;
		} else {
			sprintf(tmpbuf, "%s ", token);
			strcat(cl->cl_newLine, tmpbuf);
		}
		token = strtok(NULL, " \t");
	}
	/* add netmask and broadcast address if not found */

	if (!netmaskExists && !broadcastExists) {
		sprintf(tmpbuf, "netmask %s broadcast %s", netmask, baddr);
		strcat(cl->cl_newLine, tmpbuf);
	} else if (broadcastExists && !netmaskExists) {
		sprintf(tmpbuf, "netmask %s", netmask);
		strcat(cl->cl_newLine, tmpbuf);
	} else if (netmaskExists && !broadcastExists) {
		sprintf(tmpbuf, "broadcast %s", baddr);
		strcat(cl->cl_newLine, tmpbuf);
	}

	sprintf(tmpbuf, ":%s:\n", tok->it_slinkOptions);
	strcat(cl->cl_newLine, tmpbuf);

	free(device);
	return 0;
}

static void
normalizeAddr(char * addrIn, char ** addrOut)
{
	ulong_t		iaddr;
	struct in_addr	in;

	iaddr = inet_addr(addrIn);
	in.s_addr = iaddr;
	*addrOut = strdup(inet_ntoa(in));
}

/*
 *	return:	errno
 */
int
killProcess(pid_t proc)
{
	return kill(proc, SIGKILL);
}

/*
 *	return:	0 if success
 *		1 if failure
 */
int
modifyRoutedEntry(ConfigFile_t * cf, int enable)
{
	static QueryConfig_t	qc[] = {
		{ ROUTEDTOK,	(QC_MATCH) },
		{ NULL,	(QC_END) }
	};
	ConfigToken_t	 *	tok;
	ConfigLine_t *		cl;
	char			flag[2];

	if (!cf) {
		return 1;
	}
	cl = queryConfigFile(cf, qc);
	if (cl == NULL) {
		return 1;
	}

	cl->cl_newLine = (char *)calloc(strlen(cl->cl_origLine) + 50, sizeof(char));
	if (!cl->cl_newLine) {
		return 1;
	}

	if (enable) {
		strcpy(flag, "Y");
	} else {
		strcpy(flag, "N");
	}
	tok = (ConfigToken_t *)cl->cl_tokens;
	sprintf (
		cl->cl_newLine, "%s:%s:%s:%s:%s:%s:\n",
		tok->ct_level,
		tok->ct_fullCmdPath,
		tok->ct_overridingCmdPath,
		flag,
		tok->ct_configFile,
		tok->ct_cmdOptions
	);

	return 0;
}

/*
 *	return:	0 if success
 *		1 if failure
 */
int
getAddrFromName(char * name, char ** addrOut)
{
	struct hostent *	he;
	struct in_addr *	p;

	if ((he = gethostbyname(name)) == NULL){
		return 1;
	}
	*addrOut = strdup(inet_ntoa(*((struct in_addr *)*he->h_addr_list)));

	return 0;
}

/*	return:	0 if success
 *		1 if failure
 *
 */
int
getNameFromAddr(char * addr, char ** nameOut)
{
	ulong_t			bin;
	struct hostent *	he;

	if ((bin = inet_addr(addr)) == -1) {
		return 1;
	}
	if ((he = gethostbyaddr((char *)&bin, sizeof(struct in_addr), AF_INET)) == NULL) {
		return 1;
	}
	*nameOut = strdup(he->h_name);

	return 0;
}

/*
 *	return:	0 if valid
 *		1 if invalid
 */
int
validateNetmask(ulong_t netmask)
{
	ulong_t	mask;
	uchar_t	foundOnes;
	int	count;

	if (netmask == 0) {
		return 1;
	}
	count = 0;
	mask = 1 << count;
	foundOnes = 0;
	do {
		if (netmask & mask)
			foundOnes = 1;
		++count;
		mask = 1 << count;
	} while (!foundOnes);
	while (count < 32) {
		if (!(netmask & mask)) {
			return 1;
		}
		++count;
		mask = 1 << count;
	}
	return 0;
}

int
getBroadcastFromAddr(char * addrIn, ulong_t mask, char ** addrOut)
{
	ulong_t		iaddr;
	struct in_addr	in;

	iaddr = inet_addr(addrIn);
	in.s_addr = ((iaddr & ntohl(mask)) | ~(ntohl(mask)));
	*addrOut = inet_ntoa(in);

	return 0;
}
