#ident	"@(#)proto.h	1.3"
#ident	"$Header$"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1993 Lachman Technology, Inc.
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
 *
 */
/*      SCCS IDENTIFICATION        */
#ifdef __STDC__
# define	P(s) s
#else
# define P(s) ()
#endif


/* builtin.c */
int num P((char *str , int *res , int base ));
struct val *Open P((struct finst *fi , struct cmd *c , int argc , struct val *argv ));
struct val *Link P((struct finst *fi , struct cmd *c , int argc , struct val *argv ));
struct val *Push P((struct finst *fi , struct cmd *c , int argc , struct val *argv ));
struct val *Sifname P((struct finst *fi , struct cmd *c , int argc , struct val *argv ));
struct val *Vifname P((struct finst *fi , struct cmd *c , int argc , struct val *argv ));
struct val *Unitsel P((struct finst *fi , struct cmd *c , int argc , struct val *argv ));
struct val *Initqp P((struct finst *fi , struct cmd *c , int argc , struct val *argv ));
struct val *Dlattach P((struct finst *fi , struct cmd *c , int argc , struct val *argv ));
struct val *Strcat P((struct finst *fi , struct cmd *c , int argc , struct val *argv ));
struct val *Sifaddr P((struct finst *fi , struct cmd *c , int argc , struct val *argv ));
struct val *Dlbind P((struct finst *fi , struct cmd *c , int argc , struct val *argv ));
struct val *Dlsubsbind P((struct finst *fi , struct cmd *c , int argc , struct val *argv ));
struct val *Noexit P((struct finst *fi , struct cmd *c , int argc , struct val *argv ));
struct val *Sifhdr P((struct finst *fi , struct cmd *c , int argc , struct val *argv ));
struct val *binit P((void ));


/* exec.c */
int xerr P((struct finst *fi , ...));
int showval P((struct val *v ));
struct val *makeargv P((struct finst *fi , struct cmd *c ));
int chkargs P((struct finst *fi , struct cmd *c , struct bfunc *bf , int argc , struct val *argv ));
int docmd P((struct finst *fi , struct cmd *c , struct val *rval ));
struct val *userfunc P((struct func *f , int argc , struct val *argv ));

/* main.c */
void error P((int flags , ...));
void catch P((int signo ));

/* parse.c */
char *xmalloc P((int n ));
char *savestr P((char *str ));
struct fntab *findfunc P((char *name ));
struct fntab *deffunc P((char *name , ...));
int gettok P((int eofok ));
int syntax P((char *msg ));
struct var *findvar P((struct func *f , char *name ));
struct var *defvar P((struct func *f , char *name ));
int parsecmd P((struct func *f ));
int parsefunc P((void ));
int parse P((void ));

#undef P
