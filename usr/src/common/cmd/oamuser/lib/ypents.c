#ident	"@(#)ypents.c	1.2"
#ident  "$Header$"

#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>

#define nisname(n)  (*n == '+' || *n == '-')

struct nislist {
	union {
		struct passwd pwent;
		struct group  grent;
	}u_ent;
	struct nislist *next, *tail;
};
#define pwent u_ent.pwent
#define grent u_ent.grent

static struct nislist *nispwents = NULL;
static struct nislist *nisgrents = NULL;

savenispwent(p)
struct passwd *p;
{
	register struct passwd *pwd;
	register struct nislist *nis; 

	nis = (struct nislist *)calloc(1,sizeof(struct nislist));
	if (nis == NULL)
		return(1);

	pwd = &nis->pwent;
	pwd->pw_name = strdup(p->pw_name);
	if (p->pw_passwd)
		pwd->pw_passwd = strdup(p->pw_passwd);
	pwd->pw_uid = p->pw_uid;
	pwd->pw_gid = p->pw_gid;
	if (p->pw_comment)
		pwd->pw_comment = strdup(p->pw_comment);
	if (p->pw_dir)
		pwd->pw_dir = strdup(p->pw_dir);
	if (p->pw_shell)
		pwd->pw_shell = strdup(p->pw_shell);

	if (nispwents) {
		if (nispwents->tail) {
			nispwents->tail->next = nis;
		} else{
			nispwents->next = nis;
		}
		nispwents->tail = nis;
	} else {
		nispwents = nis;
	}

	return(0);
}
putnispwents(fp)
FILE *fp;
{
	register struct nislist *nis, *tmp; 
	register struct passwd *pwd, *nisplus=NULL;

	for (nis = nispwents; nis;){
		pwd = &nis->pwent;
		/*
		 * Make sure a '+' entry is always last
		 */
		if (strcmp(pwd->pw_name, "+") == 0){
			nisplus = pwd;
			nis = nis->next;
			continue;
		}
		putpwent(pwd, fp);
		free(pwd->pw_name);
		if (pwd->pw_passwd)
			free(pwd->pw_passwd);
		if (pwd->pw_comment)
			free(pwd->pw_comment);
		if (pwd->pw_dir)
			free(pwd->pw_dir);
		if (pwd->pw_shell)
			free(pwd->pw_shell);
		tmp = nis;
		nis = nis->next;
		free(tmp);
	}
	if (nisplus) {
		pwd = nisplus;
		putpwent(pwd, fp);
		free(pwd->pw_name);
		if (pwd->pw_passwd)
			free(pwd->pw_passwd);
		if (pwd->pw_comment)
			free(pwd->pw_comment);
		if (pwd->pw_dir)
			free(pwd->pw_dir);
		if (pwd->pw_shell)
			free(pwd->pw_shell);
		free(pwd);
	}
}
savenisgrent(p)
struct group *p;
{
	register struct group *gr;
	register struct nislist *nis; 
	register char **q, **n;
	int grcnt;

	nis = (struct nislist *)calloc(1,sizeof(struct nislist));

	if (nis == NULL)
		return(1);

	gr = &nis->grent;
	gr->gr_name = strdup(p->gr_name);
	if (p->gr_passwd)
		gr->gr_passwd = strdup(p->gr_passwd);
	gr->gr_gid = p->gr_gid;
	q = p->gr_mem;
	grcnt = 0;
	while (*q++) {
		grcnt++;
	}
	gr->gr_mem = n = (char **)calloc(grcnt+1, sizeof(char *));
	for (q=p->gr_mem; *q; q++){
		*n++ = strdup(*q);
	}
	*n = NULL;

	if (nisgrents) {
		if (nisgrents->tail) {
			nisgrents->tail->next = nis;
		} else{
			nisgrents->next = nis;
		}
		nisgrents->tail = nis;
	} else {
		nisgrents = nis;
	}
	return(0);
}
putnisgrents(fp)
FILE *fp;
{
	register struct nislist *nis, *tmp; 
	register struct group *g;
	register char **q, *n;

	for (nis = nisgrents; nis;){
		g = &nis->grent;
		putgrent(&nis->grent, fp);
		free(g->gr_name);
		if (g->gr_passwd)
			free(g->gr_passwd);
		q= g->gr_mem;
		while (*q){
			n = *q++;
			free(n);
		}
		tmp = nis;
		nis = nis->next;
		free(tmp);
	}
}
/*
 * the static version of getpwnam() in libgen does not
 * return NIS entries, so we need a version that does
 */
struct passwd *
nis_getpwnam(name)
	char	*name;
{
	register char *p;
	FILE *fp;
	static char line[BUFSIZ];
	static struct passwd pwd;
	
	if ((fp = fopen("/etc/passwd", "r")) == NULL)
		return(NULL);

	/*
	 * clean up data from previous calls.
	 */
	memset((char *)&pwd, '\0', sizeof(struct passwd));

	for (;;) {
		if (fgets(line, BUFSIZ, fp) == NULL){
			fclose(fp);
			return(NULL);
		}

		if (p = strchr(line, '\n'))
			*p = '\0';

		if ((p = strchr(line, ':')) == NULL) {
			if (strcmp(name , line) == 0) {
				pwd.pw_name = line;
				fclose(fp);
				return(&pwd);
			}
			continue;
		}
		*p++ = '\0';

		if (strcmp(name, line)) {
			if (nisname(name)){
				if (strcmp(name+1, line))
					continue;
			} else {
				continue;
			}
		}

		pwd.pw_name = line;
		pwd.pw_passwd = p;
		if (p = strchr(p, ':')){
			*p++ = '\0';
			pwd.pw_uid = atoi(p);
		} 
		if (p = strchr(p, ':')){
			*p++ = '\0';
			pwd.pw_gid = atoi(p);
		}
		if (p = strchr(p, ':')){
			*p++ = '\0';
			pwd.pw_comment = p;
			pwd.pw_gecos = p;
		}
		if (p = strchr(p, ':')){
			*p++ = '\0';
			pwd.pw_dir = p;
		}
		if (p = strchr(p, ':')){
			*p++ = '\0';
			pwd.pw_shell = p;
		}
		if (p = strchr(pwd.pw_passwd, ',')) {
			*p++ = '\0';
			 pwd.pw_age = p;
		}
		fclose(fp);
		return(&pwd);
	}
}
static FILE *pwf;
void
nis_setpwent()
{
	if (pwf == NULL)
		pwf = fopen("/etc/passwd", "r");
	else
		rewind(pwf);

	return;
}
int
nis_endpwent()
{
	if (pwf)
		fclose(pwf);
	pwf = NULL;
}
struct passwd *
nis_getpwent()
{
	register char *p;
	FILE *fp;
	int ret;
	static char line[BUFSIZ];
	static struct passwd pwd;
	
	if (pwf == NULL && ((pwf = fopen("/etc/passwd", "r")) == NULL))
		return(NULL);

	memset((char *)&pwd, '\0', sizeof(struct passwd));
	for (;;) {
		if (fgets(line, BUFSIZ, pwf) == NULL){
			return(NULL);
		}

		if (p = strchr(line, '\n'))
			*p = '\0';

		if ((p = strchr(line, ':')) == NULL) {
			if (*line == '+' || *line == '-') {
				pwd.pw_name = line;
				return(&pwd);
			}
			continue;
		}
		*p++ = '\0';

		pwd.pw_name = line;
		pwd.pw_passwd = p;
		if (p = strchr(p, ':')){
			*p++ = '\0';
			pwd.pw_uid = atoi(p);
		} 
		if (p = strchr(p, ':')){
			*p++ = '\0';
			pwd.pw_gid = atoi(p);
		}
		if (p = strchr(p, ':')){
			*p++ = '\0';
			pwd.pw_comment = p;
			pwd.pw_gecos = p;
		}
		if (p = strchr(p, ':')){
			*p++ = '\0';
			pwd.pw_dir = p;
		}
		if (p = strchr(p, ':')){
			*p++ = '\0';
			pwd.pw_shell = p;
		}
		if (p = strchr(pwd.pw_passwd, ',')) {
			*p++ = '\0';
			 pwd.pw_age = p;
		}
		return(&pwd);
	}
}
#define MAXGRP 200

static char *
grskip(g, c)
char *g;
char c;
{
	while (*g != '\0' && *g != c)
		++g;
	if (*g != '\0')
		*g++ = '\0';
	return(g);
}
struct group *
nis_getgrnam(group)
	char *group;
{
	register char *g;
	FILE *fp;
	static char line[BUFSIZ];
	static struct group grp;
	static char *gr_mem[MAXGRP];
	char **q;

	if ((fp = fopen("/etc/group", "r")) == NULL)
		return(NULL);

	memset((char *)&grp, '\0', sizeof(struct group));

	for (;;) {
		if ((g = fgets(line, BUFSIZ, fp)) == NULL) {
			fclose(fp);
			return(NULL);
		}
		if (strchr(line, ':') == NULL) {
			if (g = (char *)strchr(line, '\n'))
				*g = '\0';
			if (strcmp(group, line) == 0) {
				grp.gr_name = line;
				fclose(fp);
				return(&grp);
			}
			continue;
		}

		grp.gr_name = line;
		g = grskip(g, ':'); 

		if (strcmp(group, line)) {
			if (nisname(group)) {
				if (strcmp(group+1, line))
					continue;
			} else {
				continue;
			}
		}

		grp.gr_passwd = g;
		grp.gr_gid = atoi(g = grskip(g, ':'));
		g = grskip(g, ':');
		(void)grskip(g, '\n');
		grp.gr_mem = q = gr_mem;
		while (*g != '\0') {
			if (q < &gr_mem[MAXGRP-1])
				*q++ = g;
			g = grskip(g, ',');
		}
		*q = NULL;
		fclose(fp);
		return(&grp);
	}
}

struct group *
nis_fgetgrent(fp)
	FILE *fp;
{
	register char *g;
	static char line[BUFSIZ];
	static struct group grp;
	static char *gr_mem[MAXGRP];
	char **q;

	memset((char *)&grp, '\0', sizeof(struct group));

	for (;;) {
		if ((g = fgets(line, BUFSIZ, fp)) == NULL) {
			return(NULL);
		}
		if (strchr(line, ':') == NULL) {
			if (*line == '+' || *line == '-') {
				if (g = (char *)strchr(line, '\n'))
					*g = '\0';
				grp.gr_name = line;
				return(&grp);
			}
			continue;
		}

		grp.gr_name = line;
		grp.gr_passwd = g = grskip(g, ':'); 
		grp.gr_gid = atoi(g = grskip(g, ':'));
		g = grskip(g, ':');
		(void)grskip(g, '\n');
		grp.gr_mem = q = gr_mem;
		while (*g != '\0') {
			if (q < &gr_mem[MAXGRP-1])
				*q++ = g;
			g = grskip(g, ',');
		}
		*q = NULL;
		return(&grp);
	}
}
