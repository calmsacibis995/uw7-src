/*
 * File parse.c
 *
 *      Copyright (C) The Santa Cruz Operation, 1994-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#pragma comment(exestr, "@(#) @(#)parse.c	8.1")

#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <ctype.h>

#include "sys/scodlpi.h"
#include "dlpiut.h"

int		srvterm = 0;	/* flag to terminate server */
cmd_t	cmd;

static int	parseargs(void);
static int	singlearg(int index, char *str, char *ostr);
static int	atoh(char *str);
static int	parseaddr(char *dst, char *src);
static int	parseroute(char *dst, char *src);
static int	parseparms(char *dst, char *src);
static int	parsetable(char *str);
static void	lprf(int indent, char *Fmt, ...);
static void	cout(uchar c, int indent);

static arg_t parg[] =
{
	{
		"",				/* We do not use the zero entry */
		NULL,
		T_NONE,
		NULL
	},
#define	A_FD			1
	{
		"fd=",
		"file descriptor",
		T_INT,
		&cmd.c_fd
	},
#define	A_DEVICE		2
	{
		"device=",
		"device name",
		T_STR,
		cmd.c_device
	},
#define	A_INTERFACE		3
	{
		"interface=",
		"interface name is mdi or dlpi",
		T_INTERFACE,
		&cmd.c_interface
	},
#define	A_MEDIA			4
	{
		"media=",
		"media type is token or ethernet",
		T_MEDIA,
		&cmd.c_media
	},
#define	A_SAP			5
	{
		"sap=",
		"sap is a hex integer",
		T_HEXINT,
		&cmd.c_sap
	},
#define	A_ERROR			6
	{
		"error=",
		"is the command expected to succeed or fail (y/n)",
		T_YESNO,
		&cmd.c_error
	},
#define	A_OMCHNADDR		7
	{
		"omchnaddr=",
		"MAC address of other machine",
		T_ADDR,
		cmd.c_omchnaddr
	},
#define	A_OURDSTADDR	8
	{
		"ourdstaddr=",
		"MAC address of our machine",
		T_ADDR,
		cmd.c_ourdstaddr
	},
#define	A_ODSTADDR		9
	{
		"odstaddr=",
		"MAC address other machine is sending to (bcst maybe)",
		T_ADDR,
		cmd.c_odstaddr
	},
#define	A_TABLE			10
	{
		"table=",
		"multicast table is addr1,addr2,addr3...",
		T_TABLE,
		0
	},
#define	A_MSG			11
	{
		"msg=",
		"message string enclosed in quotes",
		T_STR,
		cmd.c_msg
	},
#define	A_TIMEOUT		12
	{
		"timeout=",
		"timeout is in seconds",
		T_INT,
		&cmd.c_timeout
	},
#define	A_FRAMING		13
	{
		"framing=",
		"framing types are ethernet, "
				"802.3, llc802.3, snap802.3, xns, 802.5, llc802.5, snap802.5",
		T_FRAME,
		&cmd.c_framing
	},
#define	A_DELAY			14
	{
		"delay=",
		"delay in ms between frames or frame groups (windows)",
		T_INT,
		&cmd.c_delay
	},
#define	A_LOOP			15
	{
		"loop=",
		"loopback frames expected (y/n)",
		T_YESNO,
		&cmd.c_loop
	},
#define A_MATCH			16
	{
		"match=",
		"message exchanged is verified or merely output (y/n)",
		T_YESNO,
		&cmd.c_match
	},
#define A_LEN			17
	{
		"len=",
		"frame length to use",
		T_INT,
		&cmd.c_len
	},
#define A_SRMODE		18
	{
		"srmode=",
		"source routine mode is non/auto/stack",
		T_SRMODE,
		&cmd.c_srmode
	},
#define	A_WINDOWSZ		19
	{
		"windowsz=",
		"window size in frames must from 1 to 1000",
		T_INT,
		&cmd.c_windowsz
	},
#define A_VERIFY		20
	{
		"verify=",
		"% of frames to verify data, from 0 to 100",
		T_INT,
		&cmd.c_verify
	},
#define A_DURATION		21
	{
		"duration=",
		"time duration in HHMMSS format",
		T_HMS,
		&cmd.c_duration
	},
#define A_MULTISYNC		22
	{
		"multisync=",
		"should load test delay 20 sec before starting (y/n)",
		T_YESNO,
		&cmd.c_multisync
	},
#define	A_FILE			23
	{
		"file=",
		"name of file to transfer",
		T_STR,
		cmd.c_file
	},
#define A_BILOOP		24
	{
		"biloop=",
		" two-way DLPI loopback",
		T_YESNO,
		&cmd.c_biloop
	},
#define A_ROUTE			25
	{
		"route=",
		"route info",
		T_ROUTE,
		cmd.c_route
	},
#define A_PARMS			26
	{
		"parms=",
		"asr parameters",
		T_PARMS,
		cmd.c_parms
	},
	{
		"",				/* Null string is end-of-table */
		NULL,
		T_NONE,
		NULL
	}
};

/*
 * indices match defines C_OPEN...
 */
parse_t pcmd[] =
{
	{
		"open",
		"open a device, output fd=<fd>",
		I_BOTH,
		{ A_DEVICE, A_INTERFACE },
		{0}
	},
	{
		"close",
		"close the fd",
		I_BOTH,
		{ A_FD },
		{0}
	},
	{
		"bind",
		"do a bind",
		I_BOTH,
		{ A_FD, A_SAP, A_ERROR, A_FRAMING },
		{0}
	},
	{
		"unbind",
		"do an unbind",
		I_DLPI,
		{ A_FD },
		{0}
	},
	{
		"sbind",
		"do a subsequent bind",
		I_DLPI,
		{ A_FD, A_SAP, A_ERROR },
		{0}
	},
	{
		"addmca",
		"add a multicast address",
		I_BOTH,
		{ A_FD, A_OURDSTADDR, A_ERROR },
		{0}
	},
	{
		"delmca",
		"delete a multicast address",
		I_BOTH,
		{ A_FD, A_OURDSTADDR, A_ERROR },
		{0}
	},
	{
		"getmca",
		"get the multicast table and compare it to the expected value",
		I_BOTH,
		{ A_FD, A_TABLE },
		{0}
	},
	{
		"getaddr",
		"get the current phys address and output addr=<addr>",
		I_BOTH,
		{ A_FD },
		{0}
	},
	{
		"promisc",
		"set promiscuous mode, (not supported)",
		I_MDI,
		{ A_FD },
		{0}
	},
	{
		"syncsend",
		"sync up the two machines via sync-up protocol, "
										"sender side, output is msg=<str>",
		I_BOTH,
		{
			A_FD, A_MSG, A_TIMEOUT, A_OMCHNADDR, A_OURDSTADDR,
			A_ODSTADDR, A_FRAMING, A_SAP, A_LOOP, A_MATCH
		},
		{0}
	},
	{
		"syncrecv",
		"sync up the machines via sync-up protocol, "
										"receive side, output is msg=<str>",
		I_BOTH,
		{
			A_FD, A_MSG, A_TIMEOUT, A_OMCHNADDR, A_OURDSTADDR,
			A_ODSTADDR, A_FRAMING, A_SAP, A_LOOP, A_MATCH
		},
		{0}
	},
	{
		"sendloop",
		"loop every possible frame size with the given framing type, "
													"send side",
		I_BOTH,
		{
			A_FD, A_FRAMING, A_SAP, A_OMCHNADDR, A_OURDSTADDR,
			A_ODSTADDR, A_DELAY, A_LOOP
		},
		{0}
	},
	{
		"recvloop",
		"loop every possible frame size with the given framing type, "
													"receive side",
		I_BOTH,
		{
			A_FD, A_FRAMING, A_SAP, A_OMCHNADDR,
			A_OURDSTADDR, A_ODSTADDR, A_DELAY, A_LOOP
		},
		{0}
	},
	{
		"send",
		"send a single frame",
		I_BOTH,
		{ A_FD, A_FRAMING, A_SAP, A_OURDSTADDR, A_LOOP, A_LEN },
		{0}
	},
	{
		"txid",
		"transmit XID frame, (not supported)",
		I_BOTH,
		{ A_FD, A_SAP, A_OMCHNADDR, A_OURDSTADDR, A_ODSTADDR, A_FRAMING },
		{0}
	},
	{
		"rxid",
		"receive XIDi, verify, and generate a response, (not supported)",
		I_BOTH,
		{ A_FD, A_SAP, A_OMCHNADDR, A_OURDSTADDR, A_ODSTADDR, A_FRAMING },
		{0}
	},
	{
		"ttest",
		"transmit TEST frame, (not supported)",
		I_BOTH,
		{ A_FD, A_SAP, A_OMCHNADDR, A_OURDSTADDR, A_ODSTADDR, A_FRAMING },
		{0}
	},
	{
		"rtest",
		"receive and verify TEST frame, (not supported)",
		I_BOTH,
		{ A_FD, A_SAP, A_OMCHNADDR, A_OURDSTADDR, A_ODSTADDR, A_FRAMING },
		{0}
	},
	{
		"srmode",
		"set source routine mode",
		I_DLPI,
		{ A_FD, A_SRMODE },
		{0}
	},
	{
		"sendload",
		"load test, repetitively send a window of frames and expect an ack",
		I_DLPI,
		{
			A_FD, A_FRAMING, A_SAP, A_OURDSTADDR, A_LOOP,
			A_LEN, A_WINDOWSZ, A_DURATION, A_DELAY,
			A_VERIFY, A_TIMEOUT, A_MULTISYNC
		},
		{0}
	},
	{
		"recvload",
		"load test, repetitively receive a window of frames and send an ack",
		I_DLPI,
		{
			A_FD, A_FRAMING, A_SAP, A_OURDSTADDR, A_LOOP,
			A_LEN, A_WINDOWSZ, A_DURATION, A_DELAY, A_VERIFY,
			A_TIMEOUT, A_MULTISYNC
		},
		{0}
	},
	{
		"sendfile",
		"very primitive file transfer, send side",
		I_BOTH,
		{ A_FD, A_FRAMING, A_SAP, A_OURDSTADDR, A_LOOP, A_FILE },
		{0}
	},
	{
		"recvfile",
		"very primitive file transfer, recv side",
		I_BOTH,
		{ A_FD, A_FRAMING, A_SAP, A_OURDSTADDR, A_LOOP, A_FILE },
		{0}
	},
	{
		"biloopmode",
		"set DLPI biloop mode",
		I_DLPI,
		{ A_FD, A_BILOOP },
		{0}
	},
	{
		"bilooprx",
		"dlpi dummy read generation",
		I_DLPI,
		{ A_FD, A_BILOOP, A_FRAMING, A_SAP, A_ODSTADDR, A_OMCHNADDR, A_ROUTE },
		{0}
	},
	{
		"bilooptx",
		"dlpi looped back write test",
		I_DLPI,
		{ A_FD, A_BILOOP, A_FRAMING, A_SAP, A_OURDSTADDR, A_ROUTE },
		{0}
	},
	{
		"srclr",
		"dlpi source route clear command",
		I_DLPI,
		{ A_FD, A_OURDSTADDR },
		{0}
	},
	{
		"setsrparms",
		"dlpi set sr parameters command",
		I_DLPI,
		{ A_FD, A_PARMS },
		{0}
	},
	{
		"getraddr",
		"get the factory phys address and output addr=<addr>",
		I_BOTH,
		{ A_FD },
		{0}
	},
	{
		"setaddr",
		"set the current phys address",
		I_BOTH,
		{ A_FD, A_OURDSTADDR, A_ERROR },
		{0}
	},
	{
		"setallmca",
		"set all multicast address reception",
		I_BOTH,
		{ A_FD, A_ERROR },
		{0}
	},
	{
		"delallmca",
		"delete all multicast address reception",
		I_BOTH,
		{ A_FD, A_ERROR },
		{0}
	},
	{
		NULL
	}
};

int
parse()
{
	register char *cp;
	register parse_t *pp;

	if (strcmp(ibuf, "stop") == 0) {
		srvterm = 1;
		return(0);
	}
	memset(&cmd, 0, sizeof(cmd_t));

	cp = ibuf;
	for (pp = pcmd; *pp->p_name; pp++)
		if (strcmp(cp, pp->p_name) == 0)
			break;
	if (*pp->p_name == 0) {
		error("command %s not found, use no arguments for usage.", cp);
		return(0);
	}
	cmd.c_cmd = pp - pcmd;

	cp += strlen(ibuf) + 1;
	if (parseargs() == 0)
		return(0);;
	return(1);
}

/*
 * now that we have the command, get the arguments
 */

static int
parseargs()
{
	register arg_t *argp;
	register parse_t *pp;
	int i;
	int index;
	int argndx;
	char *cp;
	char *cpe;
	int len;

	pp = &pcmd[cmd.c_cmd];
	memset(pp->p_got, 0, sizeof(pp->p_got));

	/* start at second parameter */
	cp = ibuf + strlen(ibuf) + 1;
	for (; *cp; cp += strlen(cp) + 1) {
		for (argp = &parg[1]; *argp->a_name; argp++) {
			len = strlen(argp->a_name);
			if (strncmp(cp, argp->a_name, len) == 0)
				break;
		}
		if (*argp->a_name == 0) {
			error("Unknown parameter: %s", cp);
			return(0);
		}
		argndx = argp - parg;
		for (index = 0; pp->p_arg[index]; index++)
			if (pp->p_arg[index] == argndx)
				break;
		if (pp->p_arg[index] == 0) {
			error("Unknown parameter: %s", cp);
			return(0);
		}
		if (pp->p_got[index]) {
			error("Duplicate parameter: %s", cp);
			return(0);
		}
		pp->p_got[index] = 1;
		cpe = cp + len;
		if (singlearg(argndx, cpe, cp) == 0)
			return(0);
	}

	/* check that we got all of the parameters */
	for (i = 0; pp->p_arg[i]; i++)
		if (pp->p_got[i] == 0)
			break;
	if (pp->p_arg[i] && !pp->p_got[i]) {
		error("Missing parameter: %s", parg[pp->p_arg[i]].a_name);
		return(0);
	}
	return(1);
}

static int
singlearg(int index, char *str, char *ostr)
{
	register arg_t *argp;
	int i;
	char tm[3];

	argp = &parg[index];

	/* each argument type processed here */
	switch (argp->a_type)
	{
		case T_INT:
			i = atoi(str);
			if (i < 0) {
				error("Invalid integer: %s", ostr);
				return(0);
			}
			*((int *)(argp->a_address)) = i;
			break;
		case T_STR:
			strcpy((char *)argp->a_address, str);
			break;
		case T_HEXINT:
			i = atoh(str);
			*((int *)(argp->a_address)) = i;
			break;
		case T_YESNO:
			if (tolower(*str) == 'y')
				i = 1;
			else if (tolower(*str) == 'n')
				i = 0;
			else
			{
				error("Invalid %s, must be y/n", ostr);
				return(0);
			}
			*((int *)(argp->a_address)) = i;
			break;
		case T_ADDR:
			if (parseaddr((char *)argp->a_address, str) == 0)
				return(0);
			break;
		case T_TABLE:
			if (parsetable(str) == 0)
				return(0);
			break;
		case T_FRAME:
			if (strcmp(str, "ethernet") == 0)
				i = F_ETHERNET;
			else if (strcmp(str, "802.3") == 0)
				i = F_802_3;
			else if (strcmp(str, "xns") == 0)
				i = F_XNS;
			else if (strcmp(str, "llc802.3") == 0)
				i = F_LLC1_802_3;
			else if (strcmp(str, "snap802.3") == 0)
				i = F_SNAP_802_3;
			else if (strcmp(str, "802.5") == 0)
				i = F_802_5;
			else if (strcmp(str, "snap802.5") == 0)
				i = F_SNAP_802_5;
			else if (strcmp(str, "llc802.5") == 0)
				i = F_LLC1_802_5;
			else {
				error("Unrecognized framing type: %s", str);
				return(0);
			}
			*((int *)(argp->a_address)) = i;
			break;
		case T_INTERFACE:
			if (strcmp(str, "dlpi") == 0)
				i = I_DLPI;
			else if (strcmp(str, "mdi") == 0)
				i = I_MDI;
			else {
				error("Unrecognized interface type: %s", str);
				return(0);
			}
			*((int *)(argp->a_address)) = i;
			break;
		case T_MEDIA:
			if (strcmp(str, "ethernet") == 0)
				i = M_ETHERNET;
			else if (strcmp(str, "token") == 0)
				i = M_TOKEN;
			else {
				error("Unrecognized media type: %s", str);
				return(0);
			}
			*((int *)(argp->a_address)) = i;
			break;
		case T_SRMODE:
			if (strcmp(str, "non") == 0)
				i = SR_NON;
			else if (strcmp(str, "auto") == 0)
				i = SR_AUTO;
			else if (strcmp(str, "stack") == 0)
				i = SR_STACK;
			else {
				error("Unrecognized media type: %s", str);
				return(0);
			}
			*((int *)(argp->a_address)) = i;
			break;
		case T_HMS:
			if (strlen(str) != 6) {
				error("HHMMSS format invalid");
				return(0);
			}
			tm[2] = 0;
			strncpy(tm, str, 2);
			cmd.c_duration = atoi(tm) * 60 * 60;	/* hours */
			strncpy(tm, str+2, 2);
			cmd.c_duration += atoi(tm) * 60;	/* minutes */
			strncpy(tm, str+4, 2);
			cmd.c_duration += atoi(tm);		/* seconds */
			break;
		case T_ROUTE:
			if (parseroute((char *)argp->a_address, str) == 0)
				return(0);
			break;
		case T_PARMS:
			if (parseparms((char *)argp->a_address, str) == 0)
				return(0);
			break;
	}
	return(1);
}

static int
atoh(register char *str)
{
	register int x;

	x = 0;
	while (*str) {
		if (*str >= '0' && *str <= '9')
			x = ((x<<4) | (*str - '0'));
		else if (*str >= 'a' && *str <= 'f')
			x = ((x<<4) | (*str - ('a' - 10)));
		else
			x = ((x<<4) | (*str - ('A' - 10)));
		str++;
	}
	return(x);
}

static int
parseaddr(char *dst, char *src)
{
	int i;
	register char *icp;
	register char *ocp;
	char buf[3];

	if (strlen(src) != (ADDR_LEN*2)) {
		error("Invalid MAC address string: %s", src);
		return(0);
	}
	buf[2] = 0;
	icp = src;
	ocp = dst;
	for (i = 0; i < ADDR_LEN; i++) {
		strncpy(buf, icp, 2);
		*ocp++ = atoh(buf);
		icp += 2;
	}
	return(1);
}


static int
parseroute(char *dst, char *src)
{
	int i;
	int len;
	register char *icp;
	register char *ocp;
	char buf[3];

	if ((len = strlen(src)) > (ROUTE_LEN*2)) {
		error("Route too long: %s", src);
		return(0);
	}
	buf[2] = 0;
	icp = src;
	ocp = dst;
	for (i = 0; i < len; i++) {
		strncpy(buf, icp, 2);
		*ocp++ = atoh(buf);
		icp += 2;
	}
	return(1);
}

static int
parseparms(char *dst, char *src)
{
	int i;
	int len;
	register char *icp;
	register char *ocp;
	char buf[3];

	if ((len = strlen(src)) != (PARMS_LEN*2)) {
		error("parameter list wrong size: %s", src);
		return(0);
	}
	buf[2] = 0;
	icp = src;
	ocp = dst;
	for (i = 0; i < len; i++) {
		strncpy(buf, icp, 2);
		*ocp++ = atoh(buf);
		icp += 2;
	}
	return(1);
}

static int
parsetable(char *str)
{
	char *icp;
	char tmp[ADDR_LEN*2 + 1];

	icp = str;
	cmd.c_tabsiz = 0;
	while (*icp) {
		strncpy(tmp, icp, ADDR_LEN*2);
		tmp[ADDR_LEN*2] = 0;
		if (parseaddr((char *)cmd.c_table[cmd.c_tabsiz], tmp) == 0)
			return(0);
		icp += ADDR_LEN*2;
		cmd.c_tabsiz++;
		if (*icp == 0)
			break;
		if (*icp != ',') {
			error("syntax error for table parameter");
			return(0);
		}
		icp++;
		if (*icp == 0)
			break;
	}
	return(1);
}

/*
 * here we verify the data that was parsed.
 * check the file descriptor,
 * check that the command is valid for the interface type.
 */

int
verify()
{
	if (cmd.c_cmd == C_OPEN)
		return(1);
	if (fds[cmd.c_fd].f_open == 0) {
		error("Invalid file descriptor %d", cmd.c_fd);
		return(0);
	}
	if (!(fds[cmd.c_fd].f_interface & pcmd[cmd.c_cmd].p_interface)) {
		error("Command %s not valid for interface %s",
			pcmd[cmd.c_cmd].p_name,
			(fds[cmd.c_fd].f_interface == I_DLPI) ? "dlpi" : "mdi");
		return(0);
	}
	return(1);
}

/*
 * usage message generated from parse tables
 */

void
usage()
{
	parse_t	*pp;
	arg_t	*argp;
	int		i;
	char	buf[32];

	printf(
		"usage: dlpi [-n ipcname] cmd [options...]\n"
		"dlpiut will perform certain test operations at the LLI or MDI\n"
		"interfaces.  The output of dlpiut is designed to be parsed by\n"
		"the shell.  Multiple lines of output, each directed to a shell\n"
		"variable can occur.  When the output ERROR=\"str\" is returned,\n"
		"a non-zero exit code is returned.  The output LOG=\"str\" contains\n"
		"any useful information the program can report.  \n"
		"\n"
		"When dlpiut starts it automatically forks off a server in the\n"
		"background to allow the file descriptors to remain open while\n"
		"providing a line-by-line shell interface.\n"
		"dlpiut allows for multiple opens through a single server.  To\n"
		"create multiple servers, separate ipcnames need to be specified\n"
		"\n"
		"Command syntaxes:\n"
		"    stop\n");

	for (pp = pcmd; *pp->p_name; pp++)
	{
		lprf(11, "%-10s ", pp->p_name);
		for (i = 0; i < ARGS; i++)
		{
			if (pp->p_arg[i] == 0)
				continue;

			argp = &parg[pp->p_arg[i]];
			lprf(11, "%s", argp->a_name);
			switch (argp->a_type) {
			case T_INT:	lprf(11, "<num>"); break;
			case T_STR:	lprf(11, "<str>"); break;
			case T_HEXINT:	lprf(11, "<hexnum>"); break;
			case T_YESNO:	lprf(11, "y/n"); break;
			case T_ADDR:	lprf(11, "<addr>"); break;
			case T_TABLE:	lprf(11, "<addr,addr,...>"); break;
			case T_FRAME:	lprf(11, "<framing>"); break;
			case T_INTERFACE:lprf(11, "<interface>"); break;
			case T_MEDIA:	lprf(11, "<media>"); break;
			case T_SRMODE:	lprf(11, "<srmode>"); break;
			case T_HMS:	lprf(11, "HHMMSS"); break;
			case T_ROUTE:	lprf(11, "<route>"); break;
			case T_PARMS:	lprf(11, "<parms>"); break;
			}
			lprf(11, " ");
		}
		lprf(11, "\n");
	}

	lprf(11, "\ncommand  interface  description and output if any\n");
	for (pp = pcmd; *pp->p_name; pp++)
	{
		lprf(20, "%-10s ", pp->p_name);
		switch (pp->p_interface) {
		case I_MDI:	lprf(20, "MDI "); break;
		case I_DLPI:	lprf(20, "DLPI"); break;
		case I_BOTH:	lprf(20, "BOTH"); break;
		}
		lprf(20, "     %s\n", pp->p_usage);
	}
	lprf(11, "\nArgument   Description\n");
	for (argp = &parg[1]; *argp->a_name; argp++)
	{
		i = strlen(argp->a_name) - 1;
		strncpy(buf, argp->a_name, i);
		buf[i] = 0;
		lprf(11, "%-10s %s\n", buf, argp->a_usage);
	}
	exit(0);
}

static void
lprf(int indent, char *Fmt, ...)
{
	uchar			buf[256];
	register uchar	*cp;
	va_list			Args;

	va_start(Args, Fmt);
	vsprintf((char *)buf, Fmt, Args);
	va_end(Args);
	for (cp = buf; *cp; cp++)
		cout(*cp, indent);
}

static void
cout(uchar c, int indent)
{
	static char		line[256];
	static int		num = 0;
	static int		col;
	register char	*cp;
	int				i;

	line[num++] = c;
	if (c == '\n')
	{
		line[num] = 0;
		printf("%s", line);
		num = 0;
	}
	else
	{
		if (num > 78)
		{
			line[num] = 0;
			cp = &line[num-1];
			while (*cp != ' ')
				cp--;
			*cp = 0;
			printf("%s", line);
			printf("\n");
			for (i = 0; i < indent; i++)
				line[i] = ' ';
			line[i] = 0;
			strcat(line, cp + 1);
			num = strlen(line);
		}
	}
}

