#ident	"@(#)ppptalk.c	1.8"

/*
 * This module provides an administrative interface to the PPP Daemon
 *
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>
#include <syslog.h>
#include <dlfcn.h>
#include <setjmp.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <locale.h>
#include <cs.h>
#include <sys/ppp.h>
#include <priv.h>

#include "fsm.h"
#include "psm.h"
#include "ppp_proto.h"
#include "ppptalk.h"
#include "ulr.h"
#include "act.h"
#include "auth.h"
#include "pathnames.h"
#include "ppptalk_msg.h"
#include "edit.h"

/*
 * Global variables
 */
char *ppp_version = PPP_VERSION_STRING;
char *def_prompt = ">	";
char *cmd_prompt = "ppp >";
int quit = 0;	/* Set when we are finished */
enum { COMMAND, DEFINITION }; /* Interactive modes */
int mode = COMMAND;
int bracket = 0;
int so;		/* Socket to pppd */
int qflg = 0;	/* If set, indicates quiet error messages ... for GUI */
int fline = 0;	/* Current line in the input file (for error messages) */
int pline;	/* Line being parsed */
int version_checked = 0; /* When loading, we must have the version checked */
int def_count = 0; /* Number of definitions counted */
int interactive = 0;	/* Read commands from command line */
extern int ppp_debug;	/* Set if ppp debugging is enabled */
int is_a_tty = 0;
int editor_mode = 0;
char cmdbuf[MAXLINE + 1];
char host[MAXHOSTNAMELEN + 1];
struct ulr_prim *gmsg = NULL;
jmp_buf intr_env;

struct cp_tab_s {
	char *cp_proto;
	void *cp_handle;
	struct psm_entry_s *cp_entry;
} cp_tab[MAX_CP];

nl_catd catd;

/*
 * This function is responsible for deciding which messages are output.
 */
void
pmsg(int level, int msgno, char *fmt, ...)
{
        va_list args;
	FILE *fp;

	/*
	 * Select output file descriptor, always use stdout
	 * if we are in interactive mode.
	 */
	fp = stdout;

	/*
	 * Use our normal reporting format
	 */

	va_start(args, fmt);

	/*
	 * Get the message from msg catalog
	 */
	switch (msgno) {
	case -1:
		break;
	case 0:
#ifdef DEBUG_I12N
		fprintf(fp, "Needs I12N : ");
#endif
		break;
	default:
		fmt = MSGSTR(msgno, fmt);
		break;
	}

	/*
	 * If quiet mode requested always use this format
	 */
	if (qflg) {
		if (level <= MSG_INFO)
			vfprintf(fp, fmt, args);
		else
			fprintf(fp, "MSG:%d\n", msgno);

		fflush(fp);
		return;
	}


	switch (level) {
	case MSG_INFO:
		vfprintf(fp, fmt, args);
		break;

	case MSG_WARN:
		fprintf(fp, MSGSTR(PPPTALK_WARN, "WARNING: "));
		vfprintf(fp, fmt, args);
		break;

	case MSG_ERROR:
		fprintf(fp, MSGSTR(PPPTALK_ERROR, "ERROR: "));
		vfprintf(fp, fmt, args);
		break;

	case MSG_DEF_ERROR:
		fprintf(fp, MSGSTR(PPPTALK_ERROR, "ERROR: "));
		if (pline > 0)
			fprintf(fp, "Line %d : ", pline);
		vfprintf(fp, fmt, args);
		break;
	case MSG_DEBUG:
		fprintf(fp, "DEBUG: ");
		vfprintf(fp, fmt, args);
		break;
	}

	fflush(fp);
	va_end(args);
}

/*
 * Called when a user interrupts
 */
void
user_intr(int sig)
{
	if (editor_mode != 0) {
		edit_cooked_mode();
	}

	pmsg(MSG_INFO, PPPTALK_INTR, "\nInterrupted.\n");
	sigrelse(SIGINT);
	if (!interactive)
		exit (1);
	longjmp(intr_env, -1);
}

/*
 * Trim of any leading/trailing white space
 */
char *
trimline(char *line)
{
	char *p;

	/* Find  the start */

	while (*line && isspace(*line))
		line++;

	/* Find the end */

	if (strlen(line) > 0) {
		p = line + strlen(line) - 1;
		while (p > line && isspace(*p))
			p--;

		if (*(p + 1))
			*(p + 1) = 0;
	}

	return(line);
}

/*
 * Get a token
 */
char *
gettok(char *p, char *tok)
{
	int len = 0;
	
	/* Find the token */
	while (*p && !isspace(*p) && (*p != '=') && len < MAXTOK) {
		tok[len++] = *p;
		p++;
	}
	tok[len] = 0;

	/*
	 * If we have a token that is too long ... skip over its
	 * end .. until we find a white space character
	 */
	if (len == MAXTOK) {
		if (*p && !isspace(*p))
			pmsg(MSG_WARN, PPPTALK_TOKLONG,
			     "Token too long, truncated (%s)\n", tok);
		while(*p && !isspace(*p))
			p++;
	}

	/* Move p to next none space character */

	while(*p && isspace(*p))
		p++;

	return(p);
}

/*
 * Get user input .. a line at a time
 */
char *
getinput(FILE *fp, char *prompt)
{
	static char local_cmdbuf[MAXLINE + 1];
	char *p;
	int n;

	if (editor_mode != 0) {
		edit_raw_mode();
		n = edit_read(local_cmdbuf, MAXLINE, prompt);
		edit_cooked_mode();

		if (n == 0) {		
			cmdbuf[0] = 0;
			local_cmdbuf[0] = 0;
			quit = 1;
		}
	} else {
		/* Display prompt */
		if (prompt)
			pmsg(MSG_INFO, -1, "%s", prompt);

		p = fgets(local_cmdbuf, MAXLINE, fp);

		if (!p) {
			cmdbuf[0] = 0;
			local_cmdbuf[0] = 0;
			quit = 1;
		} else {
			p += strlen(p) - 1;

		/* Drop the trailing newline */
			if (*p == 0x0a)
				*p = 0;
		}
	}
	strcpy(cmdbuf, local_cmdbuf);
	return(local_cmdbuf);
}

/*
 * Send a message to the Daemon
 */
int
ulr_send(int so, struct ulr_prim *msg, int len)
{
	int ret;

	ASSERT(len > 0);

	ret = send(so, (char *)msg, len, 0);	
	if (ret < 0) {
		pmsg(MSG_ERROR, PPPTALK_FAIL2D,
		     "Failed to send message to the PPP Daemon\n");
		return(ret);
	} else if (ret < len) {
		pmsg(MSG_ERROR, PPPTALK_FAIL2D,
		     "Failed to send message to the PPP Daemon\n");
		return(ret);
	}
	return(0);
}

/*
 * Read a message from the Daemon
 */
int
ulr_read(int so, struct ulr_prim *msg, int len)
{
	int rval;
	char *p;

	p = (char *)msg;
	memset(p, 0, len);

	rval = recv(so, p, len, 0);
	if (rval <= 0) {
		pmsg(MSG_ERROR, PPPTALK_FAILFD,
		     "Failed reading message from the PPP Daemon %d, %d\n",
		     rval, errno);
		return rval;
	}
	return 0;
}

/*
 * Send a message to the Daemon .. then wait for a reply
 */
int
ulr_sr(int so, struct ulr_prim *msg, int slen, int rlen)
{
	int ret;

	ret = ulr_send(so, msg, slen);
	if (ret)
		return(ret);

	/* Looks like things went well, wait for response */

	ret = ulr_read(so, msg, rlen);
	if (ret)
		return ret;

	if (msg->error)
		return msg->error;

	return ret;
}


void
cp_init()
{
	int i;
	for (i = 0; i<MAX_CP; i++)
		cp_tab[i].cp_handle = NULL;
}

/* 
 * Given a protocol name, eg ip, fund the shared object that
 * supports this protocol.
 */
struct psm_entry_s *
cp_get_tab(char *proto)
{
	int i;
	void *handle;
	char buf[256];
	struct cp_tab_s *tp = cp_tab;

	/* Check if already opened this protocol */
	/*printf("cp_get_tab: %s\n", proto);*/

	for (i = 0; i < MAX_CP; i++, tp++) {
		if (tp->cp_handle) {
			if (strcmp(tp->cp_proto, proto) == 0) {
				/*printf("Have loaded this protocol already\n"); */
				return tp->cp_entry;
			}
		} else {
			/* Not loaded ... do it now */

			sprintf(buf, "%s%s_parse.so", PSM_SO_PATH, proto);
			handle = dlopen(buf, RTLD_LAZY);
			if (!handle)
				return NULL;

			/* Find the entry point */

			tp->cp_entry = (struct psm_entry_s *)
				dlsym(handle, "psm_entry");

			if (!tp->cp_entry) {
				return NULL;
			}

			tp->cp_handle = handle;
			tp->cp_proto = strdup(proto);
			return tp->cp_entry;
		}
	}
	return NULL;
}


/*
 * The message sent to us has its ID set to that required.
 * Try searching each definition type an entry with the same id.
 *
 * Must be passed a piece of memory at least ULR_MAX_MSG in size.
 */
int
cfg_findid(int so, struct ulr_cfg *msg, char *id, int type)
{
	int ret;

	ASSERT(type >= 0 && type < DEF_MAX);

	msg->prim = ULR_GET_CFG;
	strcpy(msg->ucid, id);
	msg->type = type;

	ret = ulr_sr(so, (struct ulr_prim *)msg,
		     sizeof(struct ulr_cfg), ULR_MAX_MSG);
	if (ret) {
		if (ret != ENOENT)
			pmsg(MSG_ERROR, PPPTALK_UNEXP,
			     "Unexpected error from pppd (%d)\n", ret);
		return(-1);
	} else 
		return(msg->type);
}

/*
 * Parser
 */
parsecmd(char *cmd)
{

 	static char tok[MAXTOK + 1], id[MAXTOK + 1];
	char *args, *last;

	cmd = trimline(cmd);

	/* Skip empty lines or a comments */
	if (strlen(cmd) == 0 || *cmd == '#')
		return;

	/* Get the first token .. this should be the command */
	args = gettok(cmd, tok);

	/* Pointer to the last character */
	last = cmd + strlen(cmd) - 1;

	if (mode == COMMAND) {
		/* Check if syntax is a definition start */

		if (*last == '{') {

			/*
			 * Tok should point to the definition type, and
			 * args contains the dewfinition id and braket
			 */

			args = gettok(args, id);

			if (args != last) {
				printf("Bad definition syntax.\n");
				return;
			}

			/*printf("definition of %s - id %s\n", tok, id);*/
			mode = DEFINITION;
			bracket++;

			def_new(id);
			def_store("definition", tok);
			return;
		}
	} 

	if (mode == DEFINITION) {

		/* Do bracket checks */
		if (*last == '{') {
			printf("No nested structures\n");
			mode = COMMAND;
			return;
		} else if (*last == '}') {
			bracket--;
			if (bracket == 0) {
				/* End of definition - process it */
				def_process();
				mode = COMMAND;
				return;
			}
		}

		if (*args == '=') {
			args++;
			args = trimline(args);
		} else {
			printf("Options must be option = value\n");
			mode = COMMAND;
			return;
		}

		/* Store the line */
		def_store(tok, args);
	} else
		/* Lookup command in table */
		cmdlookup(tok, args);
}

static void
get_editor()
{
	char *ev;

	is_a_tty = isatty(0);
	if (is_a_tty) {
		ev = getenv("EDITOR");
		if (!ev)
			ev = getenv("VISUAL");
		
		if (ev) {
			if (strcmp(ev, "emacs") == 0)
				editor_mode = EMACS;
			else if (strcmp(ev, "vi") == 0)
				editor_mode = VIRAW;
			
			if (editor_mode != 0)
				edit_init(EMACS);
		}	
	}
}

/*
 * Get a connection to the PPP Daemon
 */
static void
talk_init()
{
	cp_init();

	if (gethostname(host, MAXHOSTNAMELEN) < 0) {
		printf("WARNING Failed to obtain hostname.\n");
		host[0] = 0;
	}

	so = ppp_sockinit();
	if (so < 0) {
		perror("Cannot connect to pppd");
		exit(1);
	}
}

/*
 * Tell the PPP Daemon we have finished
 */
static void
talk_finish()
{
	gmsg->prim = ULR_END;
	write(so, (char *)gmsg, sizeof(*gmsg));
	close(so);
}

/*
 * Used to create command buffers when ppptalk is used as pppattach, etc
 */
static void
talk_extracmd(char *cmd, int argc, char *argv[])
{
	int i;
	sprintf(cmdbuf, "%s ", cmd);
	for (i = 1; i < argc; i++) {
		strcat(cmdbuf, argv[i]);
		strcat(cmdbuf, " ");
	}
}

#define PPPTALK "ppptalk"
#define PPPPPP "ppp"

char *basename(char *path);

main(int argc, char *argv[])
{
	char *cmd;
	int c;
	extern char *optarg;
	extern int optind;
	char *ifile = NULL;
	int errflg = 0, load = 0;
 
        setlocale(LC_ALL,"");
        catd = catopen(MF_PPPTALK, MC_FLAGS);

	/*
	 * Allocate an area of memory that is globally accessible.
	 * It's gernerally used to create messages to send to
	 * the PPP Daemon, or as a receive buffer in which messages
	 * from the Daemon are stored.
	 */
	gmsg = (struct ulr_prim *)malloc(ULR_MAX_MSG);
	if (!gmsg) {
		pmsg(MSG_ERROR, 0, "Failed to allocate memory\n");
		exit(1);
	}

	cmdbuf[0] = 0;

	/*
	 * Which command are we ?
	 */
	if (strcmp(basename(argv[0]), PPPTALK) != 0 
	    && strcmp(basename(argv[0]), PPPPPP) != 0
	    && strncmp(basename(argv[0]), PPPPPP, 3) == 0)
		/* This will fill-in cmdbuf will a ppptalk command */
		talk_extracmd(basename(argv[0]) + 3, argc, argv);
	else {

		/*
		 * Process ppptalk command line options
		 */
		while ((c = getopt(argc, argv, "lq")) != EOF) {
			switch (c) {
			case 'l':
				/* Do loading here ! */
				talk_init();
				cmd_load();
				talk_finish();
				exit(0);
				break;
			case 'q':
				qflg++;
				break;
			case '?':
				errflg++;
				break;
			}
		}

		if (errflg) {
			(void)fprintf(stderr, "usage: ppptalk [cmds]\n");
			exit(1);
		}
		/*
		 * If we have no extra arguments then
		 * we select interactive mode
		 */
		if (optind < argc) {
			for (;optind < argc; optind++) {
				strcat(cmdbuf, argv[optind]);
				strcat(cmdbuf, " ");
			}
		} else {
			interactive = 1;

			/* Check if we need a command line editor */
			get_editor();
		}
	}

	/* Perform initialisation */
	talk_init();

	/* Get debug flags */
	get_state();

	/* Handle a couple of signals*/
	if (setjmp(intr_env) != 0) {
		mode = COMMAND;
		bracket = 0;
	}
	signal(SIGINT, user_intr);
	signal(SIGPIPE, SIG_IGN);

	if (interactive) {
		do {
			cmd = getinput(stdin,
				       mode == COMMAND ?
				       cmd_prompt : def_prompt);

			/* Parse & execute command string */
			parsecmd(cmd);

		} while (!quit);

		if (strlen(cmd) < 2)
			pmsg(MSG_INFO, -1, "\n");
	} else 
		parsecmd(cmdbuf);

	talk_finish();
	exit(0);
}
