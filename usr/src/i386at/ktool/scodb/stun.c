#ident	"@(#)ktool:i386at/ktool/scodb/stun.c	1.1"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

#include	<stdio.h>
#include	<storclass.h>
#include	<syms.h>
#include	<ctype.h>
#include	<sys/scodb/stunv.h>

int mode = MD_ALL;
struct vari *varilist = 0;
int nvaris = 0;
struct stun *stunlist = 0;
int nstuns = 0;

int s_data, s_text;
#define		DATA()	(s_data = 1, s_text = 0)
#define		TEXT()	(s_data = 0, s_text = 1)

/*
*	get an input line from /lib/comp
*/
struct line *
getline(fp)
	FILE *fp;
{
	char bf[256];
	int nd = 0;
	register char *s;
	register char *o;
	char *x;
	static struct line l;

memset(&l, 0, sizeof(l));
	l.l_tag[0] = '\0';
	for (;;) {
		if (!fgets(bf, sizeof bf - 1, fp)) {
			return 0;	/* EOF */
		}
		for (s = bf;*s;s++)
			if (*s == '.')
				break;
		if (*s++ != '.')
			continue;
		if (s[0] == 'd'	&&
		    s[1] == 'a' &&
		    s[2] == 't' &&
		    s[3] == 'a'  ) {
			DATA();
			continue;
		}
		if (s[0] == 't'	&&
		    s[1] == 'e' &&
		    s[2] == 'x' &&
		    s[3] == 't'  ) {
			TEXT();
			continue;
		}
		if (!(		s[0]	== 'd'	&&
				s[1]	== 'e'	&&
				s[2]	== 'f'	&&
				s[3]	== '\t'	 ))
			continue;
		/* a .def */
		s += 4;
		o = l.l_name;
		while (*s != ';')
			*o++ = *s++;
		*o = '\0';
		l.l_size = 0;

		/* other fields */
		for (;;) {
			while (*s++ != '.')
				;
			if (*s == 'd') {
				if (s[1] == 'i'	&&
				    s[2] == 'm'	&&
				    s[3] == '\t' ) {
					s += 4;
					for (;;) {
						l.l_dim[nd++] = strtol(s, &x, 10);
						s = x;
						if (*s == ';')
							break;
						++s;
					}
				}
			}
			else if (*s == 's') {
				if (s[1] == 'c'	&&
				    s[2] == 'l'	&&
				    s[3] == '\t' ) {
					s += 4;
					l.l_sclass = atoi(s);
				}
				else if (s[1] == 'i'	&&
					 s[2] == 'z'	&&
					 s[3] == 'e'	&&
					 s[4] == '\t'	 ) {
					s += 5;
					l.l_size = atoi(s);
				}
			}
			else if (*s == 't') {
				if (s[1] == 'a'	&&
				    s[2] == 'g'	&&
				    s[3] == '\t' ) {
					s += 4;
					o = l.l_tag;
					while (*s != ';')
						*o++ = *s++;
					*o = '\0';
				}
				else if (s[1] == 'y'	&&
					 s[2] == 'p'	&&
					 s[3] == 'e'	&&
					 s[4] == '\t'	 ) {
						s += 5;
						l.l_type = strtol(s, 0, 8);
				}
			}
			else if (*s == 'v') {
				if (s[1] == 'a'	&&
				    s[2] == 'l'	&&
				    s[3] == '\t' ) {
					s += 4;
					if (isdigit(*s))
						l.l_offset = atoi(s);
				}
			}
			while (*s && *s++ != ';')
				;
			if (!*s)
				break;
		}
		break;
	}
	while (nd < NDIM)
		l.l_dim[nd++] = 0;
fprintf(stderr, "%s %x %x %x %x %x %x %x %x %s\n",
	l.l_name, l.l_sclass & 0xff, l.l_size, l.l_offset, l.l_type,
	l.l_dim[0], l.l_dim[1], l.l_dim[2], l.l_dim[3], l.l_tag);
	return &l;
}

/*
*	process all input on this file
*/
procinp(fp)
	FILE *fp;
{
	struct line *l, *getline();

	while (l = getline(fp)) {
		if (l->l_sclass == C_STRTAG || l->l_sclass == C_UNTAG) {
			if ((mode & MD_STRUCTS) == 0) {
				while (l = getline(fp))
					if (l->l_sclass == C_EOS)
						break;
				continue;
			}
			stun(fp, l);
		}
		else if (l->l_sclass == C_EXT) {
			if ((mode & MD_VARIABLES) == 0)
				continue;
			vari(l);
		}
	}
}

/*
*	return a vari, or at least let us know the last vari
*	before where this one should go, if not found.
*	(*lv == 0 if new one should go at head of list).
*/
struct vari *
find_vari(nm, lv)
	struct vari **lv;
{
	int r;
	register struct vari *v, *l;

	l = 0;
	for (v = varilist;v;l = v, v = v->va_next) {
		r = strcmp(nm, v->va_name);
		if (r == 0) {
			if (lv)
				*lv = l;
			return v;
		}
		if (r < 0)
			break;
	}
	if (lv)
		*lv = l;
	return 0;
}

/*
*	return a stun, or at least let us know the last stun
*	before where this one should go, if not found.
*	(*lv == 0 if new one should go at head of list).
*/
struct stun *
find_stun(nm, ls)
	struct stun **ls;
{
	int r;
	register struct stun *s, *l;

	l = 0;
	for (s = stunlist;s;l = s, s = s->st_next) {
		r = strcmp(nm, s->st_name);
		if (r == 0) {
			if (ls)
				*ls = l;
			return s;
		}
		if (r < 0)
			break;
	}
	if (ls)
		*ls = l;
	return 0;
}

/*
*	start a stun (if p, use p otherwise allocate new)
*/
struct stun *
start_stun(l, p)
	struct line *l;
	struct stun *p;
{
	struct stun *s, *ls, *find_stun();
	char *malloc();

	s = find_stun(l->l_name, &ls);
	if (s)
		return 0;
	++nstuns;
	if (p)
		s = p;
	else {
		s = (struct stun *)malloc(sizeof(*s));
		s->st_nmel = 0;
		s->st_stels = 0;
		strcpy(s->st_name, l->l_name);
		s->st_size = l->l_size;
		if (l->l_type == T_STRUCT)
			s->st_flags = SUF_STRUCT;
		else
			s->st_flags = SUF_UNION;
	}
	if (ls) {
		s->st_next = ls->st_next;
		ls->st_next = s;
	}
	else {
		s->st_next = stunlist;
		stunlist = s;
	}
	return s;
}

/*
*	line is a variable
*/
vari(l)
	struct line *l;
{
	int i;
	struct vari *v, *lv;
	struct stun *find_stun();
	char *malloc();

	if (l->l_name[0] == '.')	/* a fake */
		return;
	v = find_vari(l->l_name, &lv);
	if (v)
		return;
	++nvaris;
	v = (struct vari *)malloc(sizeof(*v));
	strcpy(v->va_name, l->l_name);
	v->va_type = l->l_type;
	v->va_size = l->l_size;
	for (i = 0;i < NDIM;i++)
		v->va_dim[i] = l->l_dim[i];
	switch (l->l_type & N_BTMASK) {
		case T_STRUCT:
		case T_UNION:
			strcpy(v->va_tag, l->l_tag);
			break;

		default:
			v->va_tag[0] = '\0';
	}
	if (lv) {
		v->va_next = lv->va_next;
		lv->va_next = v;
	}
	else {
		v->va_next = varilist;
		varilist = v;
	}
}

free_stun(s)
	struct stun *s;
{
	struct stel *e, *n;

	--nstuns;
	e = s->st_stels;
	while (e) {
		n = e->sl_next;
		free(e);
		e = n;
	}
	free(s);
}

/*
*	line is a stun
*/
stun(fp, l)
	FILE *fp;
	struct line *l;
{
	struct stun *s = 0, *start_stun();

	if (l->l_name[0] != '.')	/* not a fake */
		s = start_stun(l, 0);
	while (l = getline(fp)) {
		if (l->l_sclass == C_EOS)
			break;
		if (!s)
			continue;
		stel(s, l);
	}
}

/*
*	line is a stel
*/
stel(s, l)
	struct stun *s;
	struct line *l;
{
	int i;
	struct stel *e;
	struct stun *find_stun();
	static struct stel *le;
	char *malloc();

	e = (struct stel *)malloc(sizeof(*e));
	strcpy(e->sl_name, l->l_name);
	++s->st_nmel;
	e->sl_flags	 = 0;
	e->sl_offset	 = l->l_offset;
	e->sl_size	 = l->l_size;
	e->sl_type	 = l->l_type;
	for (i = 0;i < NDIM;i++)
		e->sl_dim[i] = l->l_dim[i];
	if (l->l_sclass == C_FIELD)
		e->sl_flags	|= SNF_FIELD;
	switch (l->l_type & N_BTMASK) {
		case T_STRUCT:
		case T_UNION:
			strcpy(e->sl_tag, l->l_tag);
			break;

		default:
			e->sl_tag[0] = '\0';
	}
	e->sl_next = 0;
	if (s->st_stels == 0)
		s->st_stels = e;
	else
		le->sl_next = e;
	le = e;
}

/*
*	dump all varis to file
*/
d_varis(fp)
	FILE *fp;
{
	register struct vari *v;

	fwrite(&nvaris, sizeof nvaris, 1, fp);
	for (v = varilist;v;v = v->va_next)
		fwrite(v, VARI_SZ, 1, fp);
}

/*
*	dump all stuns to file
*/
d_stuns(fp)
	FILE *fp;
{
	register struct stun *s;
	register struct stel *e;

	fwrite(&nstuns, sizeof nstuns, 1, fp);
	for (s = stunlist;s;s = s->st_next) {
		fwrite(s, STUN_SZ, 1, fp);
		for (e = s->st_stels;e;e = e->sl_next)
			fwrite(e, STEL_SZ, 1, fp);
	}
}

vadiff(van)
	char *van;
{
	printf("warning - ignored different declaration of variable \"%s\"\n", van);
}

/*
*	read intermediate varis in
*/
r_varis(fp)
	FILE *fp;
{
	int i, nvas;
	struct vari *v = 0, *lv, *find_vari(), *nv;
	char *malloc();

	fread(&nvas, sizeof nvas, 1, fp);
	while (nvas--) {
		if (!v)
			v = (struct vari *)malloc(sizeof(*v));
		if (fread(v, VARI_SZ, 1, fp) != 1) {
			free(v);
			break;
		}
		if (nv = find_vari(v->va_name, &lv)) {
			if (memcmp(nv, v, VARI_SZ))
				vadiff(v->va_name);
		}
		else {
			++nvaris;
			if (lv) {
				v->va_next = lv->va_next;
				lv->va_next = v;
			}
			else {
				v->va_next = varilist;
				varilist = v;
			}
			v = 0;
		}
	}
}

stdiff(stn)
	char *stn;
{
	printf("warning - ignored different declaration of stun \"%s\"\n", stn);
}

/*
*	read intermediate stuns in
*/
r_stuns(fp)
	FILE *fp;
{
	int i, r, nstu;
	struct stun *s = 0, *find_stun(), *nst;
	struct stel ses, *se, *sel;
	char *malloc();

	fread(&nstu, sizeof nstu, 1, fp);
	while (nstu--) {
		if (!s)
			s = (struct stun *)malloc(sizeof(*s));
		if ((r = fread(s, STUN_SZ, 1, fp)) != 1) {
			free(s);
			break;
		}
		if (nst = find_stun(s->st_name, 0)) {
			/* see if they're the same */
			if (memcmp(nst, s, STUN_SZ)) {
				stdiff(s->st_name);
				/* read all stels */
				for (i = 0;i < s->st_nmel;i++)
					fread(&ses, STEL_SZ, 1, fp);
				continue;
			}
			se = nst->st_stels;
			for (i = 0;i < s->st_nmel;i++) {
				fread(&ses, STEL_SZ, 1, fp);
				if (memcmp(&ses, se, STEL_SZ)) {
					stdiff(s->st_name);
					/* read rest of stels */
					while (++i < s->st_nmel)
						fread(&ses, STEL_SZ, 1, fp);
					break;
				}
				se = se->sl_next;
			}
		}
		else {
			start_stun(s->st_name, s);
			for (i = 0;i < s->st_nmel;i++) {
				se = (struct stel *)malloc(sizeof(*se));
				if (se == NULL) {
					fprintf(stderr, "Out of memory in r_stuns()\n");
					exit(1);
				}
				if (i == 0)
					s->st_stels = se;
				else
					sel->sl_next = se;
				sel = se;
				fread(se, STEL_SZ, 1, fp);
			}
			if (i == 0) {
				s->st_stels = NULL;
				fprintf(stderr, "Empty structure %s\n", s->st_name);
			} else
			if (sel)
				sel->sl_next = 0;
			s = 0;
		}
	}
}
