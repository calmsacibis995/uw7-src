#ident	"@(#)route_util.c	1.2"
#ident	"$Header: "

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
#include <sys/utsname.h>

#include "route_util.h"

/* fcn prototypes */
static ConfigLine_t *	queryConfigFile(ConfigFile_t *, QueryConfig_t *);
static int		createNetmaskFromAddr(char *, Class_t *, char **);
static int		getDefaultDevice(char **);
static char *		strndup(char *, int);
static char *		getAddrFromName(char *);
static char *		getNameFromAddr(char *);

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
#define	max_number_tokens 10
	char *		tokens[max_number_tokens];
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
		currentLine->cl_lineNumber = ++cf->cf_numLines;
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
		if ((tokCount != cf->cf_minFields) &&
		    (tokCount != cf->cf_minFields-1)) {
			currentLine->cl_kind = BadLine;
			for (i = 0; i < tokCount; i++) {
				if (tokens[i]) {
					free(tokens[i]);
				}
			}
			continue;
		}
		for (i = tokCount; i < max_number_tokens; i++) {
			tokens[i] = strdup("");
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
		for (i = 0; i < max_number_tokens; i++) {
			if (tokens[i]) {
				free(tokens[i]);
			}
		}
	}
	fclose(fd);
	return 0;	
}

ConfigFile_t *
allocReadConfigFile(FileKind_t kind)
{
	ConfigFile_t *retval;	
	retval = (ConfigFile_t *)calloc(sizeof(ConfigFile_t),1);
	if (retval) {
		retval->cf_kind = kind;
		if (ConfigFile == kind)
			retval->cf_minFields = IF_FIELDS;
		else	retval->cf_minFields = CF_FIELDS;

		if (0 == readConfigFile(retval))
			return retval;
	}
	return NULL;
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
	return line; /* line will be null if no match */
}

/*
 *	return:	0 if success
 *		1 if failure
 */
static int
getDefaultDevice(char ** dev)
{
	FILE *	fd;
	char	device[BUFSIZ];
	char	interface[BUFSIZ];
	int	ret;

	if ((fd = fopen(NETDRIVERSPATH, "r")) == NULL) {
		return 1;
	}
	while ((ret = fscanf(fd, "%s %s", device, interface)) != EOF) {
		if (!strcmp(interface, "inet")) {
			*dev = strdup(device);
			fclose(fd);
			return 0;
		}
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
	char *		run_flag = "y";

	if (!cf) {
		return 1;
	}
	cl = queryConfigFile(cf, qc);
	if (cl == NULL) {
		if (!router || !*router)
			return 0;
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
	if (!router || !*router) {
		/* we have a null route address, negate with N */
		run_flag = "n";
		router = "";
	}
	sprintf (
		cl->cl_newLine, "%s:%s:%s:%s:%s:",
		tok->ct_level,
		tok->ct_fullCmdPath,
		tok->ct_overridingCmdPath,
		run_flag,
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

/*	return:	0 is success
 *		1 if failure
 */
static int
decideAddrClass(char * addrIn, Class_t * classOut, char ** netmaskOut)
{}

/*
 *	This function used to only retrieve or extrapolate legal netmasks.
 *	It has been made more general.  The old form is supported afterwards.
 *	The search_name (usually "netmask" or "broadcast") will specify what to
 *	operate on.
 *	When extract_only is true, the word after search_name will be
 *	returned in value_out without extrapolation or validation.
 *	Thus addr and classOut need only be supplied when extract_only
 *	is false.
 *	return: 0 if success
 *		1 if failure
 */
static int
internal_getIfconfigVal(ConfigFile_t * cf, char * addr, char ** value_out, Class_t * classOut, char * dev, char *search_name, boolean_t extract_only, ConfigLine_t ** configLineOut)
{
	InterfaceToken_t *	tok;
	ConfigLine_t *		cl;
	int			ret;
	boolean_t		allocd_dev = B_FALSE;	/* not yet alloc'd */
	char *			netmask;	/* alloc'd */
	char *			ptr1;
	char *			ptr2;
	QueryConfig_t		q[] = {
		{ NULL,		(QC_MATCH) },
		{ NULL,		(QC_END) }
	};
	char *			reg1;		/* alloc'd */
	char *			reg2;		/* alloc'd */
	char *			reg3;		/* alloc'd */

	if (!search_name || !*search_name || !cf || 
	    !extract_only && !addr || !extract_only && !classOut)
		return 1;

	if (NULL == dev) {
		if ((ret = getDefaultDevice(&dev)) != 0) {
			if (dev) {
				free(dev);
			}
			return 1;
		}
		allocd_dev = B_TRUE;
	}
	q[0].qc_string = strdup(dev);
	if (allocd_dev)
		free(dev);
	cl = queryConfigFile(cf, q);
	if (configLineOut)
		*configLineOut = cl;
	if (!cl) {
		free(q[0].qc_string);
		return 1;
	}
	tok = (InterfaceToken_t *)cl->cl_tokens;
	ptr1 = tok->it_ifconfigOptions;
	ptr1 = strstr(ptr1, search_name);
	if (!ptr1) {
		if (extract_only) {
			*value_out = (char *)calloc(1,1);
			free(q[0].qc_string);
			return 1;
		}
		ret = decideAddrClass(addr, classOut, value_out);
		free(q[0].qc_string);
		return 0;
	}
	ptr1 = strpbrk(ptr1, " \t");
	GOBBLEWHITE(ptr1);
	ptr2  = ptr1;
	TRAVERSETOKEN(ptr2);
	netmask = strndup(ptr1, ptr2-ptr1);
	if (extract_only) {
		*value_out = strdup(netmask);
		free(q[0].qc_string);
		return 0;
	}
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
		*value_out = strdup(netmask);
		goto getout;
	}
	ptr1 = regex(reg2, netmask);
	if (ptr1) {
		ret = createNetmaskFromAddr(netmask, classOut, value_out);
		goto getout;
	}
	ptr1 = regex(reg3, netmask);
	if (ptr1) {
		/*
		 * this is not correct. we should parse /etc/networks to find
		 * matching token and addr. for now, assume mask field is
		 * either dot-decimal or hex.
		 */
		ret = decideAddrClass(addr, classOut, value_out);
	}

getout:
	free(reg1);
	free(reg2);
	free(reg3);
	free(netmask);
	free(q[0].qc_string);

	return 0;
}

static int
getNetmask(ConfigFile_t * cf, char * addr, char ** netmaskOut, Class_t * classOut)
{
	return internal_getIfconfigVal(cf, addr, netmaskOut, classOut, NULL, "netmask", B_FALSE, NULL);
}


/* winxksh function pointer entry points */

/* this default replaced by /etc/hosts sepcific
 * {not dns,nis...} _tcpip_gethostbyname_func)(const char *)
 */
static	struct hostent *((*hostbyname_func)(const char *)) =
		gethostbyname;

/* this default replaced by /etc/hosts sepcific
 * {not dns,nis...} _tcpip_gethostbyaddr(const void *, size_t, int) 
 */
static	struct hostent *((*hostbyaddr_func)(const void *, size_t, int)) =
		gethostbyaddr;

/* winxksh shell variable assignment, env_set, is not standard.
 * no default possible.
 */
static	void(*winxksh_var_func)(char *) = NULL;

void
init_winxksh_var_name_addr(void(*Assign_p)(char *),
	struct hostent *((*Byname_p)(char *)),
	struct hostent *((*Byaddr_p)(char *, int, int)))
{
	if (NULL != Assign_p)
		winxksh_var_func=Assign_p;
	if (NULL != Byname_p)
		hostbyname_func=Byname_p;
	if (NULL != Byaddr_p)
		hostbyaddr_func=Byaddr_p;
}

/* call the winxksh shell variable assignment */
static void
winxksh_var(char *a, char *b)
{
	char *buffer;
	int lena, lenb;

	if (NULL==winxksh_var_func) {
		return;
	}
	if (NULL==a) a="illegal_null_var_name";
	if (NULL==b) b="";
	while (('\t'==*b) || (' '==*b)) b++;

	lena=strlen(a);
	lenb=strlen(b);
	buffer=malloc(lena+lenb+256);
	if (!buffer) {
		return;
	}
	strcpy(buffer, a);
	strcat(buffer, "=");
	strcat(buffer, b);

	(*winxksh_var_func)(buffer);
}

int
simple_getNetmask(ConfigFile_t * cf, char * dev, char * alternate)
{
	char		*buffer = NULL, *addr, *address, *name;
	int		ret=1;
	ConfigLine_t	*configLineOut = NULL;
	struct utsname	uts;	/* null entry gets `uname -n` */

	ret=internal_getIfconfigVal(cf, NULL, &buffer, NULL, dev, "netmask", B_TRUE, &configLineOut);
	if (buffer)
		winxksh_var("INET_get_netmask", buffer);
	if (configLineOut) {
		if ((InterfaceToken_t *)configLineOut->cl_tokens) {
			winxksh_var("INET_get_slink",
				((InterfaceToken_t *)configLineOut->cl_tokens)->it_slinkOptions);
			addr = ((InterfaceToken_t *)configLineOut->cl_tokens)->it_addr;
		}
		/* if the file entry is missing, and uname finds one, use uname */
		if (!addr || !*addr) {
			if ((uname(&uts) >= 0) && (uts.nodename[0])) {
				addr = &uts.nodename[0];
			}
		}
		winxksh_var("INET_get_nd_val", addr);
		address = getAddrFromName(addr);
		winxksh_var("INET_get_addr", address);

		if (addr && *addr && hostbyaddr_func &&
		    ((in_addr_t)-1 == inet_addr((const char *)addr)))
			name = addr;
		else	name = getNameFromAddr(addr);

		winxksh_var("INET_get_nodename", name);
	} else {
		/* though we did not find an interface entry,
		 * the user wants a gethostbyname on alternate
		 */
		address = getAddrFromName(alternate);
		winxksh_var("INET_get_addr", address);
	}
	return ret;
}

int
simple_getBroadcast(ConfigFile_t * cf, char * dev)
{
	char	* buffer = NULL;
	int	ret=1;

	ret=internal_getIfconfigVal(cf, NULL, &buffer, NULL, dev, "broadcast", B_TRUE, NULL);
	if (buffer)
		winxksh_var("INET_get_broadcast", buffer);
	return ret;
}

static char *
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
	return 0;
}

/*
 *	return: 0 if success
 *		1 if failure
 */
static int
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

	if (!cf || !kind) {
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

char *
simple_getDefaultRouter(ConfigFile_t * cf)
{
	char		*buffer_def_route;
	AddrKind_t	tmp_kind;

	if (getDefaultRouter(cf, &buffer_def_route, &tmp_kind))
		return NULL;
	else {
		if (buffer_def_route) {
			winxksh_var("INET_get_rt_name", buffer_def_route);
			winxksh_var("INET_get_router", getAddrFromName(buffer_def_route));
		}
		return buffer_def_route;
	}
}

/*
 *	return:	0 if success
 *		1 if failure
 */

int
modifyNetmask(ConfigFile_t * cf, char * netmask, char * baddr, char * device, char * address, char * slinkOpt)
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
	boolean_t		allocd_dev = B_FALSE;	/* not yet alloc'd */
	int			ret;
	uchar_t			netmaskExists = 0;
	uchar_t			broadcastExists = 0;
	char *			currentAddress;

	if (!cf) {
		return 1;
	}
	if (NULL == device) {
		if ((ret = getDefaultDevice(&device)) == 1) {
			if (device) {
				free(device);
			}
			return 1;
		}
		allocd_dev = B_TRUE;
	}

	qc[0].qc_string = device;
	cl = queryConfigFile(cf, qc);
	if (cl == NULL) {
		if (allocd_dev)
			free(device);
		return 1;
	}

	cl->cl_newLine = (char *)calloc(strlen(cl->cl_origLine) + 128, sizeof(char));
	if (!cl->cl_newLine) {
		if (allocd_dev)
			free(device);
		return 1;
	}
	tok = (InterfaceToken_t *)cl->cl_tokens;

	if (address)
		currentAddress = address;
	else	currentAddress = tok->it_addr;

	sprintf (
		cl->cl_newLine, "%s:%s:%s:%s:",
		tok->it_prefix,
		tok->it_unit,
		address,
		tok->it_device
	);
	ptr = tok->it_ifconfigOptions;
	token = strtok(ptr, " \t");
	while (token) {
		/* if we need to replace this netmask, and we have not
		 * yet done so, replace it now.  Same for broadcast.
		 */
		if (!strcmp(token, "netmask")) {
			if (netmask && *netmask && (0 == netmaskExists)) {
				sprintf(tmpbuf, "netmask %s ", netmask);
				strcat(cl->cl_newLine, tmpbuf);
			}
			/* netmaskExists will indicate complete */
			netmaskExists = 1;
			token = strtok(NULL, " \t");
		} else if (!strcmp(token, "broadcast")) {
			if (baddr && *baddr && (0 == broadcastExists)) {
				sprintf(tmpbuf, "broadcast %s ", baddr);
				strcat(cl->cl_newLine, tmpbuf);
			}
			/* broadcastExists will indicate complete */
			broadcastExists = 1;
			token = strtok(NULL, " \t");
		} else {
			sprintf(tmpbuf, "%s ", token);
			strcat(cl->cl_newLine, tmpbuf);
		}
		token = strtok(NULL, " \t");
	}

	/* Add netmask and broadcast address if not yet entered. */

	if (!netmaskExists && !broadcastExists &&
	    netmask && *netmask && baddr && *baddr) {
		/* neither yet entered and both pending, add both */
		sprintf(tmpbuf, "netmask %s broadcast %s", netmask, baddr);
		strcat(cl->cl_newLine, tmpbuf);
	} else if (!netmaskExists && !broadcastExists &&
		netmask && *netmask) {
		/* neither yet entered and one pending */
		sprintf(tmpbuf, "netmask %s", netmask);
		strcat(cl->cl_newLine, tmpbuf);
	} else if (!netmaskExists && !broadcastExists &&
		baddr && *baddr) {
		/* neither yet entered and one pending */
		sprintf(tmpbuf, "broadcast %s", baddr);
		strcat(cl->cl_newLine, tmpbuf);
	} else if (broadcastExists && !netmaskExists &&
		   netmask && *netmask) {
		/* one not yet entered and one pending */
		sprintf(tmpbuf, "netmask %s", netmask);
		strcat(cl->cl_newLine, tmpbuf);
	} else if (netmaskExists && !broadcastExists &&
		   baddr && *baddr) {
		/* one not yet entered and one pending */
		sprintf(tmpbuf, "broadcast %s", baddr);
		strcat(cl->cl_newLine, tmpbuf);
	}

	if (slinkOpt)
		sprintf(tmpbuf, ":%s:\n", slinkOpt);
	else 	sprintf(tmpbuf, ":%s:\n", tok->it_slinkOptions);

	strcat(cl->cl_newLine, tmpbuf);

	if (allocd_dev)
		free(device);
	return 0;
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
 *	return:	name string if success
 *		0 if failure
 */
static char *
getAddrFromName(char * name)
{
	struct hostent * he;

	if ((NULL == name) ||
	    (0 == *name) ||
	    (0 == hostbyname_func) ||
	    (he = (*hostbyname_func)((const char *)name)) == NULL) {
		return strdup("");
	}
	return strdup(inet_ntoa(*((struct in_addr *)*he->h_addr_list)));
}

/*	return:	address string if success
 *		0 if failure
 *
 */
static char *
getNameFromAddr(char * addr)
{
	in_addr_t		bin;
	struct hostent *	he;

	if ((NULL == addr) ||
	    (0 == *addr) ||
	    (0 == hostbyaddr_func) ||
	    ((in_addr_t)-1 == (bin = inet_addr(addr)))) {
		return strdup("");
	}
	if ((he = (*hostbyaddr_func)((const void *)&bin,
					sizeof(struct in_addr),
					AF_INET)) == NULL) {
		return strdup("");
	}
	return strdup(he->h_name);
}

/*
 *	return:	0 if success
 *		1 if failure
 */
int
modifyRoutedOptions(ConfigFile_t * cf, int enable)
{
	static QueryConfig_t	qc[] = {
		{ ROUTEDTOK,	(QC_MATCH) },
		{ NULL,	(QC_END) }
	};
	ConfigToken_t	 *	tok;
	ConfigLine_t *		cl;
	char			*flag, *free_flag_ptr,
				*built, *free_built_ptr,
				*comment, *end;
	int			len;
	boolean_t		need_dash, found_q;

	if (!cf) {
		return 1;
	}
	cl = queryConfigFile(cf, qc);
	if (cl == NULL) {
		return 1;
	}

	tok = (ConfigToken_t *)cl->cl_tokens;
	cl->cl_newLine = (char *)calloc(strlen(cl->cl_origLine) + 50, sizeof(char));
	free_flag_ptr  = flag  = (char *)calloc(
			2*(10+strlen(tok->ct_cmdOptions)), sizeof(char));
	free_built_ptr = built = (char *)calloc(
			2*(10+strlen(tok->ct_cmdOptions)), sizeof(char));
	if (!cl->cl_newLine || !flag || !built) {
		return 1;
	}

	if (tok->ct_cmdOptions)
		strcpy(flag, tok->ct_cmdOptions);
	comment = strchr(flag, '#');
	if (comment) {
		/* if a comment is found, we will append its
		 * contents to the end of the args later.
		 * For now, put a null where the # is and
		 * bump the char pointer. */
		*comment++ = 0;
	}
	/* skip leading white space */
	while ((*flag) && ((' ' == *flag) || ('\t' == *flag)))
		flag++;

	/* delete trailing white space */
	len = strlen(flag) - 1;
	end = flag + len;
	/* since len can be -1, can't just use 'len' as condition */
	while ((len > 0) && ((' ' == *end) || ('\t' == *end))) {
		len--;
		*end-- = 0;
	}

	if (enable) {
		strcpy(built, "-q ");
	}

	if (*flag && strcmp(flag, "-q")) {
		/* there are flags yet to process */
		for (; (flag &&
			*flag &&
			('-' == *flag) &&
			('-' != *(1+flag)));) {
			need_dash=B_TRUE;
			flag++;
			/* find and delete a series of q's */	
			end=strpbrk(flag, "q \t");
			do {
				if (!end || !*end) {
					flag = "";
					break;
				}

				if ('q' == *end) {
					found_q = B_TRUE;
					*end++ = 0;
					while (end && ('q' == *end))
						end++;
				} else {
					/* end of token with space or tab */
					found_q = B_FALSE;
					*end++ = 0;
				}
				/* there were other chars in the initial segment */
				if (*flag) {
					if (need_dash) {
						need_dash=B_FALSE;
						len = strlen(built) - 1;
						if ((len > 0) &&
						    ((' ' == *(built+len)) ||
						     ('\t' == *(built+len))))
							strcat(built, "-");
						else	strcat(built, " -");
					}
					strcat(built, flag);
				}
				flag = end;
				/* find and delete the first q */	
				end = strpbrk(flag, "q \t");
			} while (found_q);
			/* skip leading white space */
			while (flag && (*flag) && ((' ' == *flag) || ('\t' == *flag)))
				flag++;
		}

		if (flag && *flag) {
			strcat(built, " ");
			strcat(built, flag);
		}
	}

	/* delete leading and trailing white space */
	while ((' ' == *built) || ('\t' == *built))
		built++;
	len = strlen(built) - 1;
	end = built + len;
	/* since len can be -1, can't just use 'len' as condition */
	while ((len > 0) && ((' ' == *end) || ('\t' == *end))) {
		len--;
		*end-- = 0;
	}

	if (comment) {
		if (*built)
			strcat(built, " ");
		strcat(built, "#");
		strcat(built, comment);
	}
	sprintf (
		cl->cl_newLine, "%s:%s:%s:%s:%s:%s:\n",
		tok->ct_level,
		tok->ct_fullCmdPath,
		tok->ct_overridingCmdPath,
		"Y",
		tok->ct_configFile,
		built
	);

	free(free_flag_ptr);
	free(free_built_ptr);
	return 0;
}
