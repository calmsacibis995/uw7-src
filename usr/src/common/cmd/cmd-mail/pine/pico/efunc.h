/*
 * $Id$
 *
 * Program:	Pine's composer and pico's function declarations
 *
 *
 * Michael Seibel
 * Networks and Distributed Computing
 * Computing and Communications
 * University of Washington
 * Administration Builiding, AG-44
 * Seattle, Washington, 98195, USA
 * Internet: mikes@cac.washington.edu
 *
 * Please address all bugs and comments to "pine-bugs@cac.washington.edu"
 *
 *
 * Pine and Pico are registered trademarks of the University of Washington.
 * No commercial use of these trademarks may be made without prior written
 * permission of the University of Washington.
 * 
 * Pine, Pico, and Pilot software and its included text are Copyright
 * 1989-1996 by the University of Washington.
 * 
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this distribution.
 *
 */
/*	EFUNC.H:	MicroEMACS function declarations and names

		This file list all the C code functions used by MicroEMACS
	and the names to use to bind keys to them. To add functions,
	declare it here in both the extern function list and the name
	binding table.

	Update History:

	Daniel Lawrence
*/

#ifndef	EFUNC_H
#define	EFUNC_H


/*	External function declarations		*/
#ifdef	ANSI
/* attach.c */
extern	int AskAttach(char *, char *, char *);
extern	int SyncAttach(void);
extern	int intag(char *, int);
extern	char *prettysz(long);
extern  int AttachError(void);

/* basic.c */
extern	int gotobol(int, int);
extern	int backchar(int, int);
extern	int gotoeol(int, int);
extern	int forwchar(int, int);
extern	int gotoline(int, int);
extern	int gotobob(int, int);
extern	int gotoeob(int, int);
extern	int forwline(int, int);
extern	int backline(int, int);
extern	int gotobop(int, int);
extern	int gotoeop(int, int);
extern	int forwpage(int, int);
extern	int backpage(int, int);
extern  int scrollupline(int, int);
extern  int scrolldownline(int, int);
extern  int scrollto(int, int);
extern	int setmark(int, int);
extern	int swapmark(int, int);
extern	int setimark(int, int);
extern	int swapimark(int, int);
extern	int mousepress(int, int);

/* bind.c */
extern	int whelp(int, int);
extern	int wscrollw(int, int, char **,int);
extern	int normal(int, int (*)[2],int);
extern	int rebindfunc(int (*)(int, int),int (*)(int, int));
extern	int bindtokey(int c, int (*)(int, int));

/* browse.c */
extern	int FileBrowse(char *, char *, char *, int);
extern	int ResizeBrowser(void);
extern	int set_browser_title(char *);

/* buffer.c */
extern	int anycb(void);
extern	struct BUFFER *bfind(char *, int, int);
extern	int bclear(struct BUFFER *);
extern	int packbuf(char **, int *, int);
extern	int readbuf(char **);

/* composer.c */
extern	int InitMailHeader(struct pico_struct *);
extern	int ResizeHeader(void);
extern	int HeaderEditor(int, int);
extern	void PaintHeader(int, int);
extern	int ArrangeHeader(void);
extern	int ToggleHeader(int);
extern	int HeaderLen(void);
extern	int UpdateHeader(void);
extern	int entry_line(int, int);
extern	int call_builder(struct headerentry *);
extern	int ShowPrompt(void);
extern	int packheader(void);
extern	int zotheader(void);
extern	void display_for_send(void);

/* display.c */
extern	int vtinit(void);
extern	int vttidy(void);
extern	int update(void);
extern	int modeline(struct WINDOW *);
extern	int movecursor(int, int);
extern	int clearcursor(void);
extern	int mlerase(void);
extern	int mlyesno(char *, int);
extern	int mlreply(char *, char *, int, int, EXTRAKEYS *);
extern	int mlreplyd(char *, char *, int, int, EXTRAKEYS *);
extern	void emlwrite(char *, void *);
extern	int mlwrite(char *, void *);
extern	int scrolldown(struct WINDOW *, int, int);
extern	int scrollup(struct WINDOW *, int, int);
extern	int pprints(int, int);
extern	int doton(int *, unsigned int *);
extern	int resize_pico(int, int);
extern	int zotdisplay(void);
extern	int pputc(int, int);
extern	int pputs(char *, int);
extern	int peeol(void);
extern	CELL *pscr(int, int);
extern	int pclear(int, int);
extern	int pinsert(CELL);
extern	int pdel(void);
extern	void wstripe(int, int, char *, int);
extern	int wkeyhelp(KEYMENU *);

/* file.c */
extern	int fileread(int, int);
extern	int insfile(int, int);
extern	int readin(char *, int);
extern	int filewrite(int, int);
extern	int filesave(int, int);
extern	int writeout(char *);
extern	char *writetmp(int, int);
extern	int filename(int, int);
extern	int in_oper_tree(char *);

/* fileio.c */
extern	int ffropen(char *);
extern	int ffputline(CELL *, int);
extern	int ffgetline(char *, int);

/* line.c */
extern	struct LINE *lalloc(int);
extern	int lfree(struct LINE *);
extern	int lchange(int);
extern	int linsert(int, int);
extern	int geninsert(LINE **, short *, LINE *, int, int, int, long *);
extern	int lnewline(void);
extern	int ldelete(long, int);
extern	int kdelete(void);
extern	int kinsert(int);
extern	int kremove(int);
extern	int ksize(void);
extern	int fdelete(void);
extern	int finsert(int);
extern	int fremove(int);

/* osdep.c */
extern	int ttopen(void);
extern	int ttresize();
extern	int ttclose(void);
extern	int ttisslow(void);
extern	int ttputc(int);
extern	int ttflush(void);
extern	int ttgetc(void);
extern	int ttgetwinsz(int *, int *);
extern	int GetKey(void);
extern	int alt_editor(int, int);
extern	int picosigs(void);
#ifdef	JOB_CONTROL
extern	int bktoshell(void);
#endif
extern	int fallowc(int);
extern	int fexist(char *, char *, long *);
extern	int isdir(char *, long *);
extern	char *gethomedir(int *);
extern	int homeless(char *);
extern	char *errstr(int);
extern	char *getfnames(char *, char *, int *, char *);
extern	void fioperr(int, char *);
extern	int fixpath(char *, int);
extern	int compresspath(char *, char *, int);
extern	void tmpname(char *);
extern	void makename(char *, char *);
extern	int copy(char *, char *);
extern	int ffwopen(char *);
extern	int ffclose(void);
extern	int ffelbowroom(char *);
extern	FILE *P_open(char *);
extern	int P_close(FILE *);
extern	int worthit(int *);
extern	int o_insert(char);
extern	int o_delete(void);
extern	int pico_new_mail(void);
extern	int time_to_check(void);
extern	int sstrcasecmp(const void *, const void *);
extern	int chkptinit(char *, int);
#ifdef	MOUSE
extern	unsigned long pico_mouse(unsigned, int, int);
#endif

/* pico.c */
extern	int pico(struct pico_struct *);
extern	int edinit(char *);
extern	int execute(int, int, int);
extern	int quickexit(int, int);
extern	int abort_composer(int, int);
extern	int suspend_composer(int, int);
extern	int wquit(int, int);
extern	int ctrlg(int, int);
extern	int rdonly(void);
extern	int pico_help(char **, char *, int);
extern	int zotedit(void);

/* random.c */
extern	int showcpos(int, int);
extern	int tab(int, int);
extern	int newline(int, int);
extern	int forwdel(int, int);
extern	int backdel(int, int);
extern	int killtext(int, int);
extern	int yank(int, int);

/* region.c */
extern	int killregion(int, int);
extern	int deleteregion(int, int);
extern	int markregion(int);

/* search.c */
extern	int forwsearch(int, int);
extern	int readpattern(char *);
extern	int forscan(int *, char *, int);

/* spell.c */
#ifdef	SPELLER
extern	int spell(int, int);
#endif

/* window.c */
extern	int refresh(int, int);

/* word.c */
extern	int wrapword(void);
extern	int backword(int, int);
extern	int forwword(int, int);
extern	int fillpara(int, int);
extern	int inword(void);

#else
/* attach.c */
extern	int AskAttach();
extern	int SyncAttach();
extern	int intag();
extern	char *prettysz();
extern  int AttachError();

/* basic.c */
extern	int gotobol();
extern	int backchar();
extern	int gotoeol();
extern	int forwchar();
extern	int gotoline();
extern	int gotobob();
extern	int gotoeob();
extern	int forwline();
extern	int backline();
extern	int gotobop();
extern	int gotoeop();
extern	int forwpage();
extern	int backpage();
extern  int scrollto();
extern	int setmark();
extern	int swapmark();
extern	int setimark();
extern	int swapimark();
extern	int mousepress();

/* bind.c */
extern	int whelp();
extern	int wscrollw();
extern	int normalize_cmd();
extern	int rebindfunc();
extern	int bindtokey();

/* browse.c */
extern	int FileBrowse();
extern	int ResizeBrowser();
extern	int set_browser_title();

/* buffer.c */
extern	int anycb();
extern	struct BUFFER *bfind();
extern	int bclear();
extern	int packbuf();
extern	int readbuf();

/* composer.c */
extern	int InitMailHeader();
extern	int ResizeHeader();
extern	int HeaderEditor();
extern	void PaintHeader();
extern	int ArrangeHeader();
extern	int ToggleHeader();
extern	int HeaderLen();
extern	int UpdateHeader();
extern	int entry_line();
extern	int call_builder();
extern	int ShowPrompt();
extern	int packheader();
extern	int zotheader();
extern	void display_for_send();

/* display.c */
extern	int vtinit();
extern	int vttidy();
extern	int update();
extern	int modeline();
extern	int movecursor();
extern	int clearcursor();
extern	int mlerase();
extern	int mlyesno();
extern	int mlreply();
extern	int mlreplyd();
extern	void emlwrite();
extern	int mlwrite();
extern	int scrolldown();
extern	int scrollup();
extern	int pprints();
extern	int doton();
extern	int resize_pico();
extern	int zotdisplay();
extern	int pputc();
extern	int pputs();
extern	int peeol();
extern	CELL *pscr();
extern	int pclear();
extern	int pinsert();
extern	int pdel();
extern	void wstripe();
extern	int wkeyhelp();

/* file.c */
extern	int fileread();
extern	int insfile();
extern	int readin();
extern	int filewrite();
extern	int filesave();
extern	int writeout();
extern	char *writetmp();
extern	int filename();
extern	int in_oper_tree();

/* fileio.c */
extern	int ffropen();
extern	int ffputline();
extern	int ffgetline();

/* line.c */
extern	struct LINE *lalloc();
extern	int lfree();
extern	int lchange();
extern	int linsert();
extern	int geninsert();
extern	int lnewline();
extern	int ldelete();
extern	int kdelete();
extern	int kinsert();
extern	int kremove();
extern	int ksize();
extern	int fdelete();
extern	int finsert();
extern	int fremove();

/* osdep.c */
extern	int ttopen();
extern	int ttresize();
extern	int ttclose();
extern	int ttisslow();
extern	int ttputc();
extern	int ttflush();
extern	int ttgetc();
extern	int ttgetwinsz();
extern	int GetKey();
extern	int alt_editor();
extern	int picosigs();
#ifdef	JOB_CONTROL
extern	int bktoshell();
#endif
extern	int fallowc();
extern	int fexist();
extern	int isdir();
extern	char *gethomedir();
extern	int homeless();
extern	char *errstr();
extern	char *getfnames();
extern	void fioperr();
extern	int fixpath();
extern	int compresspath();
extern	void tmpname();
extern	void makename();
extern	int copy();
extern	int ffwopen();
extern	int ffclose();
extern	int ffelbowroom();
extern	FILE *P_open();
extern	int P_close();
extern	int worthit();
extern	int o_insert();
extern	int o_delete();
extern	int pico_new_mail();
extern	int time_to_check();
extern	int sstrcasecmp();
extern	int chkptinit();
#ifdef	MOUSE
extern	unsigned long pico_mouse();
#endif

/* pico.c */
extern	int pico();
extern	int edinit();
extern	int execute();
extern	int quickexit();
extern	int abort_composer();
extern	int suspend_composer();
extern	int wquit();
extern	int ctrlg();
extern	int rdonly();
extern	int pico_help();
extern	int zotedit();

/* random.c */
extern	int showcpos();
extern	int tab();
extern	int newline();
extern	int forwdel();
extern	int backdel();
extern	int killtext();
extern	int yank();

/* region.c */
extern	int killregion();
extern	int deleteregion();
extern	int markregion();

/* search.c */
extern	int forwsearch();
extern	int readpattern();
extern	int forscan();

/* spell.c */
#ifdef	SPELLER
extern	int spell();
#endif

/* window.c */
extern	int refresh();

/* word.c */
extern	int wrapword();
extern	int backword();
extern	int forwword();
extern	int fillpara();
extern	int inword();

#endif	/* ANSI */
#endif	/* EFUNC_H */
