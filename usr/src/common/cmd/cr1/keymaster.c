#ident	"@(#)keymaster.c	1.2"
#ident  "$Header$"

#define confirm(a) (a)->type = CONFIRM
#define reject(a) (a)->type = REJECT

/*  Command/daemon for management of key database for the IAF cr1 scheme  */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stropts.h>
#include <sys/utsname.h>

#include <crypt.h>
#include "cr1.h"
#include "keymaster.h"

#include <poll.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <wait.h>

#include <pfmt.h>
#include <locale.h>

void failure(int, char *);

extern int errno;
extern char *sys_errlist[];

/* Global variables used by the deamon */

static char *pipename;

static CORE_KEY *keys;
long mypid;
char role = ' ';
FILE *logfp;
static char *scheme = DEF_SCHEME;
static FILE *openlog();
	int x_type;

/*  The Keymaster daemon  */

static struct pollfd pollinfo[NPOLLFDS];   /*  structures for poll  */
static Ids pipeinfo[NPOLLFDS];	/*  structures for UIDs and GIDs  */
static int lastfd;		/*  index to last fd in poll list  */

static int
key_lock(char *scheme)
{
	int fd;
	mode_t oumask;
	char filename[128];

	/* create a lock file with the pid of the active daemon */

	sprintf(filename, "%s/%s/%s", DEF_KEYDIR, scheme , DEF_KMPID);
	oumask = umask(0022);
	if ((fd = open(filename, O_RDWR | O_CREAT, (mode_t) 0644)) >= 0) {
		/* try to lock pid file */
		if (lockf(fd, F_TLOCK, 0L) == 0) {
			/* I should be the  daemon */
			LOG("Keymaster obtained lock '%s'.\n", filename);
		} else {
			/* someone else is already the daemon */
			DLOG("Another process is already the daemon.%s\n", "");
			(void)close(fd);
			fd = -1;
		}
	} else {
		LOG("Keymaster cannot allocate '%s'\n", filename);
		failure(CR_KMLOCK, filename);
	}
	(void) umask(oumask);
	return(fd);
}

static int
x_set(char *name)
{
	char *p;

	if ( (p = strrchr(name, '.')) != NULL) {
		p++;
		if (strcmp(p, "enigma") == 0)
			return(X_ENIGMA);
	}
	
	return(X_DES);
}	

static FILE *
openlog(char *scheme)
{
	char filename[128];
	FILE *fp;

	sprintf(filename, "%s/%s/%s", DEF_LOGDIR, scheme , DEF_KMLOG);
	fp = fopen(filename, "a+");
	chmod(filename, 0660);
	return(fp);
}

static uid_t
prin_uid(Principal P, int index)
{
	struct passwd *entry;
	Principal name;
	char *at_p;

	(void)strncpy(name, P, PRINLEN);
	if ( (at_p = strchr(name, '@')) != NULL )
		*at_p = '\0';

	if ((entry = getpwnam(name)) == NULL)
		return(-1);
	
	if (entry->pw_uid != pipeinfo[index].uid)
		return(-1);
	
	return(0);

}

static char *
prin_name(uid_t uid)
{
	struct passwd *entry;

	if ((entry = getpwuid(uid)) == NULL)
		return("UNIDENTIFIED");
	else
		return(entry->pw_name);
}

static CORE_KEY *
findkey(Principal A, Principal B)
{
	CORE_KEY *temp;
	Principal Alt;		/* principal A with/without system name */
	char *temp_ptr;

	if (strchr(A, '@') == NULL) {
		/* if no '@', add one at the end */
		(void)strcat(A, "@");
	}

	(void) strcpy(Alt, A);	/* make a copy */
	
	temp_ptr = strchr(Alt, '@');

	if (*(++temp_ptr) == '\0') {
		/* need to add a system name */
		struct utsname sysinfo;	/* System information */

		uname(&sysinfo);
		(void)strcpy(temp_ptr,sysinfo.nodename);
	} else {
		/* need to trim system name */
		*temp_ptr = '\0';
	}

	if (strchr(B, '@') == NULL) {
		/* need to prepend an '@' */
		temp_ptr = strdup(B);
		(void) sprintf(B, "@%s", temp_ptr);
		free(temp_ptr);
	}

	temp = keys;
	while ( temp->next != NULL ) {
		if ( (strcmp(temp->next->local, A) == 0)
			|| (strcmp(temp->next->local, Alt) == 0) )
			if ( strcmp(temp->next->remote, B) == 0) {
				break;
			}
		temp = temp->next;
	}
	
	return(temp);
}

static bool_t
authorized(Kmessage *kmsg, int index, CORE_KEY *authkey)
{
	bool_t ret = FALSE;
	char *request;
	char buffer[BUFSIZ] = "";

	if ((pipeinfo[index].uid == pipeinfo[0].uid)
		&& (kmsg->type != MASTER_KEY))
		return(TRUE);
	
	switch(kmsg->type) {
	
	case START:
		request = "START";
		break;

	case STOP:
		request = "STOP";
		if (pipeinfo[index].uid == pipeinfo[0].uid)
			ret = TRUE;
		break;

	case MASTER_KEY:
		request = "MASTER_KEY";
		if (strncmp(authkey->key, kmsg->key1, KEYLEN) == 0 )
			ret = TRUE;
		break;

	case ADD_KEY:
		request = "ADD_KEY";
		/* users can only add keys for themselves */
		if (prin_uid(kmsg->principal1, index) != -1)
			ret = TRUE;
		sprintf(buffer, "Principal '%s' to '%s'", 
			kmsg->principal1, kmsg->principal2);
		break;

	case DELETE_KEY:
		request = "DELETE_KEY";
		/* users can only delete keys for themselves */
		if ( (prin_uid(kmsg->principal1, index) != -1)
		   /* and only if they know the old key */
		   &&(strncmp(kmsg->key1, authkey->key, KEYLEN) == 0) )
			ret = TRUE;
		sprintf(buffer, "Principal '%s' to '%s'", 
			kmsg->principal1, kmsg->principal2);
		break;

	case CHANGE_KEY:
		request = "CHANGE_KEY";
		/* users can only change keys for themselves */
		if ( (prin_uid(kmsg->principal1, index) != -1)
		   /* and only if they know the old one */
		   &&(strncmp(kmsg->key1, authkey->key, KEYLEN) == 0) )
			ret = TRUE;
		sprintf(buffer, "Principal '%s' to '%s'", 
			kmsg->principal1, kmsg->principal2);
		break;

	case GET_KEY:
		request = "GET_KEY";
		/* users can only get keys for themselves */
		if (prin_uid(kmsg->principal1, index) != -1)
			ret = TRUE;
		sprintf(buffer, "Principal '%s' to '%s'", 
			kmsg->principal1, kmsg->principal2);
		break;

	case SEND_KEY:
		request = "SEND_KEY";
		break;

	case CONFIRM:
		request = "CONFIRM";
		break;

	case REJECT:
		request = "REJECT";
		break;

	default:
		request = "INVALID";
		break;

	}

	/* if authorization failed, log the failure and REJECT the request */

	if (ret == FALSE) {
		char buf2[BUFSIZ];

		sprintf(buf2,
			"%%s: %s by <%s>%s%s.\n",
			request, prin_name(pipeinfo[index].uid),
			*buffer ? ": " : "", buffer);
		LOG(buf2, "UNAUTHORIZED");
		reject(kmsg);
	}

	return(ret);

}

static CORE_KEY *
read_keys(Key old_key)
{
	int fd;
	int encrypted = -1;
	char filename[128];
	static char *file, *ftemp;
	CORE_KEY *ltemp, *llast;
	struct stat kstat;
	char *temp, *nl;
	char *key_p = old_key;

	/*  Open the keys file  */

	sprintf(filename, "%s/%s/%s", DEF_KEYDIR, scheme, DEF_KEYFIL);
	if ((fd = open(filename, O_RDWR | O_CREAT, (mode_t)0600)) < 0) {
		LOG("Couldn't open key file '%s'.\n", filename);
		failure(CR_DBOPEN, filename);
	}

	if ( fstat(fd, &kstat) == -1 ) {
		LOG("Stat of key file '%s' failed\n", filename);
		failure(CR_DBSTAT, filename);
	}

	if (kstat.st_size) {
		if ( (file = (char *) malloc((size_t)kstat.st_size+1)) == NULL ) {
			LOG("Couldn't allocate %ld bytes for keys.\n",
				(long) kstat.st_size);
			failure(CR_MEMORY, NULL);
		}
		if ( read(fd, file, kstat.st_size) != kstat.st_size ) {
			LOG("Read of keys file '%s' failed.\n", filename);
			failure(CR_DBREAD, filename);
		}
		file[kstat.st_size] = '\0';
		if (strspn(file, " \t\n") == strlen(file))
			kstat.st_size = 0;
	}

	if (kstat.st_size == 0) {
		file = strdup(CLEARTEXT_FILE);
		kstat.st_size = strlen(file);
		DLOG("using default file\n%s", "");
	}

	ftemp = file;
	ltemp = llast = NULL;
	while (ftemp && (ftemp < (file + kstat.st_size))) {
		if (!ltemp) {
			if ((ltemp = calloc(1, sizeof(CORE_KEY))) == NULL) {
				LOG("Couldn't allocate key structure.%s\n", "");
				failure(CR_MEMORY, NULL);
			}
		}
		nl = strchr(ftemp, '\n');
		if (!encrypted && nl)
			*nl = '\0';
		if (temp = strtok(ftemp, " \t\n")) {
			if (encrypted == -1) {
				keys = llast = ltemp;
				encrypted = (strcmp(temp, CLEARTEXT) ? 1 : 0);
				if (!encrypted) {
					LOG("Keys file is %s\n", encrypted ?
						"encrypted" : "unencrypted");
					*(temp + strlen(temp)) = ' ';
					*nl = '\0';
					temp = strtok(ftemp, " \t\n");
				}
			}
			strncpy(ltemp->local, temp, PRINLEN);
			DLOG("Local  Prin: %s\n", temp);
			if(temp = strtok(NULL, " \t\n")) {
				strncpy(ltemp->remote, temp, PRINLEN);
			DLOG("Remote Prin: %s\n", temp);
				if (encrypted) {
					temp += strlen(temp) + 1;
					memcpy(ltemp->key, temp, KEYLEN);
					cryptbuf(ltemp->key, KEYLEN, key_p, NULL, x_type | X_CBC | X_DECRYPT);
					key_p = NULL;
					ftemp = temp + KEYLEN + 1;
				} else {
					if (temp = strtok(NULL, " \t\n")) {
						strncpy(ltemp->key, temp, KEYLEN);
					}
				DLOG("Prin Key: %s\n", temp ? temp : "NULL");
					ftemp = nl + 1;
				}

				llast->next = ltemp;
				ltemp->next = (CORE_KEY *)NULL;
				llast = ltemp;
				ltemp = NULL;
				continue;

			}
		}

	LOG("Corrupted key file.%s\n", "");
	failure(CR_DBBAD, filename);

	}

	free(file);

	if ( strncmp(keys->key, (char *)old_key, KEYLEN) != 0 ) {
		LOG("Master key '%s' does not match.\n", old_key);
		failure(CR_MASTER, NULL);
	}

	return(keys);
}

static void
write_keys(CORE_KEY *master)
{
	int encrypted;
	int i, fd;
	FILE *fp;
	CORE_KEY *list;
	char filename[128];
	char tempname[128];
	char *key_p = master->key;

	/*  Open the keys file  */

	sprintf(tempname, "%s/%s/TMP.%ld", DEF_KEYDIR, scheme, mypid);
	sprintf(filename, "%s/%s/%s", DEF_KEYDIR, scheme, DEF_KEYFIL);
	if ((fd = open(tempname, O_WRONLY | O_CREAT | O_EXCL, (mode_t)0600)) < 0) {
		LOG("Couldn't open temp key file '%s'.\n", tempname);
		failure(CR_DBTEMP, tempname);
	}
	fp = fdopen(fd, "w");

	list = master;
	
	encrypted = (strcmp(list->local, CLEARTEXT) ? 1 : 0) ;
	while (list) {
		fprintf(fp, "%s\t%s\t", list->local, list->remote);
		if (encrypted) {
			Key crypt_key;
			(void) memset(crypt_key, 0, KEYLEN);
			(void) strncpy(crypt_key, list->key, KEYLEN);
			cryptbuf(crypt_key, KEYLEN, key_p, NULL, x_type | X_CBC | X_ENCRYPT);
			key_p = NULL;
			for (i=0; i<KEYLEN; i++)
				fprintf(fp, "%c", crypt_key[i]);
		} else
			fprintf(fp, "%.*s", KEYLEN, list->key);
		fprintf(fp, "\n");
		list = list->next;
	}
	fclose(fp);

	(void) unlink(filename);
	if ( rename(tempname, filename) == -1 ) {
		LOG("Rename failed, new key file left as '%s'\n", tempname);
		failure(CR_DBLINK, filename);
	}
		
	return;
}

static void
put_msg(Kmessage *kmsg, int index)
{
	int pos;		/*  position in message buffer  */
	char kbuf[MLEN];
	XDR xdrs;		/*  XDR stream  */
	
	xdrmem_create(&xdrs, kbuf, sizeof(kbuf), XDR_ENCODE);
	
	if (!xdr_kmessage(&xdrs, kmsg))
		failure(CR_XDROUT, "xdr_kmessage");

	pos = xdr_getpos(&xdrs);

	/*  Send message to requester  */

	if (write(pollinfo[index].fd, kbuf, pos) != pos) {
		LOG("Write of return message failed on fd=%d\n", pollinfo[index].fd);
		failure(CR_PIPE, "write");
	}

	return;
}

static int
get_msg(Kmessage *kmsg, int index)
{
	char kbuf[MLEN];
	XDR xdrs;		/*  XDR stream  */
	
	if (read(pollinfo[index].fd, kbuf,sizeof(kbuf)) > 0) {
		/*  Save message  */
		xdrmem_create(&xdrs, kbuf, sizeof(kbuf), XDR_DECODE);
		if(!xdr_kmessage(&xdrs, kmsg)) {
			LOG("Could not get msg.%s\n","");
			return(-1);
		}
		xdr_destroy(&xdrs);

		return(0);
	}

	return(-1);
}

static void
stop(Kmessage *kmsg, int index)
{
	/*  Stop the daemon, if authorized. */

 	if (authorized(kmsg, index, keys) == FALSE)
		return;

	LOG("The daemon is stopping...%s\n", "");

	confirm(kmsg);

	put_msg(kmsg, index);

	/*  Get rid of well-known pipe and file  */

	while (lastfd >= 0)
		(void) close(pollinfo[lastfd--].fd);

	(void) unlink("/etc/iaf/cr1/.kmpipe");
	(void) unlink("/etc/iaf/cr1/kmpid");

	/*  Terminate the daemon  */

	LOG("The daemon has stopped.%s\n", "");
	
	exit(0);
}

static void
master_key(Kmessage *kmsg, int index)
{
	char *old = kmsg->key1,
	     *new = kmsg->key2;

	cryptbuf(new, KEYLEN, keys->key, NULL, kmsg->xtype | X_DECRYPT);
	cryptbuf(old, KEYLEN, new, NULL, kmsg->xtype | X_DECRYPT);

	if (authorized(kmsg, index, keys) == FALSE)
		return;

	LOG("Old MASTER key matches. Setting new MASTER key.%s\n", "");

	if ( strncmp(kmsg->key2, "", KEYLEN) == 0 ) {
		strncpy(keys->local, CLEARTEXT, PRINLEN);
		memset(keys->key, 0, KEYLEN);
	} else {
		memcpy(keys->key, kmsg->key2, KEYLEN);
		strncpy(keys->local, CIPHERTEXT, PRINLEN);
	}

	write_keys(keys);

	confirm(kmsg);

	return;

}

static void
add_key(Kmessage *kmsg, int index)
{
	CORE_KEY *temp;

	char buffer[128];

	sprintf(buffer, "%s' to '%s", kmsg->principal1, kmsg->principal2);

	if (authorized(kmsg, index, NULL) == FALSE)
		return;

	temp = findkey(kmsg->principal1, kmsg->principal2);

	if (temp->next != NULL) {
		LOG("Failed: ADD_KEY: Principal '%s' key exists.\n", buffer);
		reject(kmsg);
		return;
	}

	if ( (temp->next = (CORE_KEY *) calloc(1, sizeof(CORE_KEY))) == NULL ) {
		LOG("Failed to allocate an additional CORE_KEY%s\n", "");
		reject(kmsg);
		return;
	}
	
	temp = temp->next;

	strcpy(temp->local, kmsg->principal1);
	strcpy(temp->remote, kmsg->principal2);
	memcpy(temp->key, kmsg->key2, KEYLEN);
	
	write_keys(keys);
	LOG("Successfully added '%s' key.\n", buffer);
	confirm(kmsg);
	return;
}

static void
delete_key(Kmessage *kmsg, int index)
{
	CORE_KEY *temp;

	char buffer[128];
	sprintf(buffer, "%s' to '%s",
		kmsg->principal1,
		kmsg->principal2);

	temp = findkey(kmsg->principal1, kmsg->principal2);

	if (authorized(kmsg, index, temp->next) == FALSE)
		return;

	if (temp->next == NULL) {
		LOG("Failed: DELETE_KEY: Principal '%s' key does not exist.\n", buffer);
		reject(kmsg);
		return;
	}

	strcpy(temp->next->local, "");
	strcpy(temp->next->remote, "");
	memset(temp->next->key, 0, KEYLEN);
	temp->next = temp->next->next;
	
	write_keys(keys);
	LOG("Successfully deleted '%s' key.\n", buffer);
	confirm(kmsg);

	return;
}

static void
change_key(Kmessage *kmsg, int index)
{
	CORE_KEY *temp;

	char buffer[128];
	sprintf(buffer, "%s' to '%s",
		kmsg->principal1,
		kmsg->principal2);

	temp = findkey(kmsg->principal1, kmsg->principal2);

	if (authorized(kmsg, index, temp->next) == FALSE)
		return;

	if (temp->next == NULL) {
		LOG("Failed: CHANGE_KEY: Principal '%s' key does not exist.\n", buffer);
		reject(kmsg);
		return;
	}

	memcpy(temp->next->key, kmsg->key2, KEYLEN);
	
	write_keys(keys);
	confirm(kmsg);
	LOG("Successfully changed key '%s'.\n", buffer);
	return;
}

static void
get_key(Kmessage *kmsg, int index)
{
	CORE_KEY *temp;

	char buffer[128];
	sprintf(buffer, "%s' to '%s",
		kmsg->principal1,
		kmsg->principal2);

	temp = findkey(kmsg->principal1, kmsg->principal2);

	if (authorized(kmsg, index, temp->next) == FALSE)
		return;

	if (temp->next == NULL) {
		LOG("Failed: GET_KEY: Principal '%s' key does not exist.\n", buffer);
		reject(kmsg);
		return;
	}

	kmsg->type = SEND_KEY;

	strncpy(kmsg->key1, temp->next->key, KEYLEN);

	return;
}

static void
proc_msg(int index)
{
	Kmessage kmsg;		/*  message structure through pipe	 */

	/* Get message */

	if (get_msg(&kmsg, index) == 0) {

		/*  Process request  */

		switch(kmsg.type) {
		case START:
			confirm(&kmsg);
			break;
		case STOP:
			stop(&kmsg, index);
			break;
		case MASTER_KEY:
			master_key(&kmsg, index);
			break;
		case ADD_KEY:
			add_key(&kmsg, index);
			break;
		case CHANGE_KEY:
			change_key(&kmsg, index);
			break;
		case DELETE_KEY:
			delete_key(&kmsg, index);
			break;
		case GET_KEY:
			get_key(&kmsg, index);
			break;
		case CONFIRM:
			confirm(&kmsg);
			break;
		case REJECT:
			reject(&kmsg);
			break;
		default:
			reject(&kmsg);
			break;
		}
		
		put_msg(&kmsg, index);
	
	}

	return;

}

static int
start(Key old_key)
{
	int revents;
	int fds[2];		/*  fds of request pipe  */
#define myend		fds[0]
#define theirend	fds[1]
	int i;			/*  index into poll list  */
	struct strrecvfd recvfd;/*  structure for I_RECVFD ioctl  */
	int fd;			/*  fd of connection  */
	int count;		/*  count of poll events */
	mode_t oumask;

	char filename[128];

	LOG("The '%s' daemon is starting...\n", scheme);

	/* set up master id in poll fds */

	pipeinfo[0].uid = geteuid();
	pipeinfo[0].gid = getegid();
	LOG("Privileged uid=%ld\n", geteuid());

	/*  Create entry with well-known name  */

	oumask = umask(0);
	sprintf(filename,"%s/%s/%s", DEF_KEYDIR, scheme, DEF_KMPIP);
	pipename = strdup(filename);
	if ( (fd = creat(pipename, (mode_t) 0666)) == -1) {
		LOG("Couldn't create pipe entry'%s'.\n", pipename);
		failure(CR_PIPE, "creat");
	}
	close(fd);
	(void) umask(oumask);

	/*  Create pipe to attach to a well-known file  */

	if ((pipe(fds)) == -1) {
		LOG("Couldn't create pipe.%s", "");
		failure(CR_PIPE, "pipe");
	}
	myend = fds[0];
	theirend = fds[1];

	/*  Push STREAMS module connld onto stream  */

	if (ioctl(theirend, I_PUSH, "connld")) {
		LOG("Couldn't push '%s'\n", "connld");
		failure(CR_PUSH, "connld");
	}

	/*  Attach pipe to well-known file  */

	if (fattach(theirend, pipename) != 0) {
		LOG("fattach() to '%s' failed\n", pipename);
		failure(CR_FATTACH, pipename);
	}

	/* Read in the keys */

	keys = read_keys(old_key);

	/* Set up file descriptors for polling */

	pollinfo[0].fd = myend;
	pollinfo[0].events = POLLIN;
	lastfd = 0;
	for (i=1; i<NPOLLFDS; i++) {
		pollinfo[i].fd = -1;
		pollinfo[i].events = POLLIN | POLLHUP;
		pollinfo[i].revents = 0;
	}

	/* Loop continually polling */

	LOG("Begin polling...%s\n", "");

	while ( (count = poll(pollinfo, lastfd+1, -1)) != -1) {
		if (count == 0)
			continue;

		/* Handle individual file descriptors first */

		for (i=lastfd; i > 0; i--) {
			if ( !(revents = pollinfo[i].revents) )
				continue;
			fd = pollinfo[i].fd;
			pollinfo[i].revents = 0;
			if (revents & POLLIN) {
				/* process incoming message */
				proc_msg(i);
			} else if (revents & ~POLLIN) {
				/* not POLLIN means POLLHUP, or POLLNVAL */
				close(fd);
				/* Shift polled file descriptors */
				pollinfo[i] = pollinfo[lastfd];
				pipeinfo[i] = pipeinfo[lastfd];
				lastfd--;
			} else {
				LOG(" ERROR SHOULD NEVER GET HERE%s\n","");
			}

		}  /*  Finished processing one poll's events  */

		/* Now handle main daemon pipe */

		revents = pollinfo[0].revents;
		pollinfo[0].revents = 0;
		if (revents & POLLIN) {
			if ((++lastfd < NPOLLFDS) && 
			   	(ioctl(myend, I_RECVFD, &recvfd) != -1)) {
				pollinfo[lastfd].fd = recvfd.fd;
				pipeinfo[lastfd].uid = recvfd.uid;
				pipeinfo[lastfd].gid = recvfd.gid;
			} else {
				LOG("Bad message on daemon's pipe.%s\n", "");
				failure(CR_PIPE, "poll");
			}
		}

	}  /*  End of poll loop  */

	/*  Terminate  */

	failure(CR_END, NULL);

	/* NOTREACHED */
	
}

static int
do_fork(int pid_fd, Key old_key)
{
	char buffer[128];
	int ret;

	switch (ret = fork()) {
	case -1:	/* fork failed */
		break;
	case 0:		/* child */
		setsid();
		mypid = (long) getpid();
		fclose(stdin);
		fclose(stdout);
		if (stderr != logfp)
			fclose(stderr);
		lockf(pid_fd, F_LOCK, 0L);
		sprintf(buffer, "%8ld", mypid);
		write(pid_fd, buffer, strlen(buffer));
		start(old_key);
		break;
	default:	/* proud parent */
		LOG("Started daemon '%ld'.\n",(long)ret);
		break;
	}
	return(ret);
}

/* MAIN PROGRAM for keymaster command */

main (int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	extern int opterr; 

	int c;			/*  return value for getopt()  */
	int ret;		/*  return value for program  */
	int cflag = 0;		/*  set to 1 if -c option is given  */
	int kflag = 0;		/*  set to 1 if -k option is given  */
	int nflag = 0;		/*  set to 1 if -n option is given  */

	pid_t child;

	Kmessage kmsg, smsg;

	char *new_key,		/* new master key alias into kmsg */
	     *old_key;		/* old master key alias into kmsg */
	Key  clr_key;		/* clear text key for startup */
	char *ver_key;		/* new master key for verification */
	char prompt[128];	/* prompts for getpass() routine */

	/* set up global variables */

	mypid = (long) getpid();

	(void) setlocale(LC_MESSAGES, "");
	(void) setlabel("UX:cr1");
	(void) setcat("uxnsu");

	/*  Get command line arguments  */

	kmsg.type = START;

	opterr = 0;	/*  Disable "illegal option" error message  */

	while ((c = getopt(argc, argv, "ckns:")) != -1)
		switch (c) {
		case 'c':
			kmsg.type = MASTER_KEY;
			cflag++;
			break;
		case 'k':
			kmsg.type = STOP;
			kflag++;
			break;
		case 'n':
			nflag++;
			break;
		case 's':
			scheme = optarg;
			break;
		case '?':
			failure(CR_KMUSAGE, "keymaster");
			break;
		}

	if (optind != argc)
		failure(CR_KMUSAGE, "keymaster");

	/*  Open error log  */

	if ((logfp = openlog(scheme)) == NULL)
		logfp = stderr;

	/* get encryption type */

	kmsg.xtype = x_type = x_set(scheme);

	LOG("A '%s' keymaster is starting...\n", scheme);

	if ( !kflag ) {
		/* we always need a master key (null is OK) */
		memset(clr_key, 0, KEYLEN);
		if ( !nflag ) {
			sprintf(prompt, gettxt(OLD_MASTERID, OLD_MASTER), scheme);
			if ((ver_key = getpass(prompt)) == NULL)
				failure(CR_INKEY, NULL);
			strncpy(clr_key, ver_key, KEYLEN);
		}
		/* we need new key if changing master key */
		if (cflag) {
			old_key = kmsg.key1;
			memcpy(old_key, clr_key, KEYLEN);

			new_key = kmsg.key2;
			memset(new_key, 0, KEYLEN);
			sprintf(prompt, gettxt(NEW_MASTERID, NEW_MASTER), scheme);
			if ((ver_key = getpass(prompt)) == NULL)
				failure(CR_INKEY, NULL);
			strncpy(new_key, ver_key, KEYLEN);
			sprintf(prompt, gettxt(VER_MASTERID, VER_MASTER), scheme);
			if ((ver_key = getpass(prompt)) == NULL)
				failure(CR_INKEY, NULL);
			if (strncmp(new_key, ver_key, KEYLEN))
				failure(CR_CONFIRM, NULL);
			cryptbuf(old_key, KEYLEN, new_key, NULL, x_type | X_ENCRYPT);
			cryptbuf(new_key, KEYLEN, clr_key, NULL, x_type | X_ENCRYPT);
		}
	}

	/* see if this process should become daemon */

	if ( (ret = key_lock(scheme)) != -1) {

		int fd = ret;

		/* there is no daemon */

		if ( kmsg.type == STOP ) {
			close(fd);
			LOG("Can't STOP daemon. None running.%s\n", "");
			failure(CR_KMSTOP, "keymaster");
		}

		ret = child = do_fork(fd, clr_key);

		close(fd);		/* free the lock on the pid file */

		if ( ret <= 0 )
			failure(CR_FORK, NULL);

		smsg.type = START;

		while ( (ret = send_msg(scheme, &smsg, CR_NOEXIT)) != 0 ) {
			if (waitpid(child, (int *)NULL, WEXITED|WNOHANG) != 0)
				failure(CR_END, NULL);
			sleep(5);
		}

		if (kmsg.type == MASTER_KEY)
			ret = send_msg(scheme, &kmsg, CR_EXIT);

	} else {

		/* there is already a daemon */

		ret = 0;

		if ( kmsg.type == START ) {
			LOG("Can't START daemon. Already running.%s\n", "");
			failure(CR_KMSTART, "keymaster");
		}

		ret = send_msg(scheme, &kmsg, CR_EXIT);

	}

	exit(0);

}
