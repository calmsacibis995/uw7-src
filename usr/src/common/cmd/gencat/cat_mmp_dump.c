#ident	"@(#)gencat:cat_mmp_dump.c	1.1.7.4"
#ident  "$Header$"
#include <dirent.h>
#include <stdio.h>
#include <ctype.h>
#include <nl_types.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include "gencat.h"

extern int list;
extern FILE *tempfile;
extern struct cat_set *sets;
static char *bldtmp();
extern char msg_buf[];
extern const char nomem[], badwritetmp[], badwrite[], badopen[], badcreate[], badseektmp[], badreadtmp[];
/*
 * dump a memory mapped gettxt library.
 */
void
cat_mmp_dump(catalog)
char *catalog;
{
  FILE *f_shdr;
  FILE *f_msg;
  struct cat_set *p_sets;
  struct cat_msg *p_msg;
  struct m_cat_set set;
  int status;
  int msg_len;
  int nmsg = 1;
  int i;
  int no_sets = 0;
  char *tmp_file;
  char message_file[MAXNAMLEN];
  

  f_shdr = fopen(catalog,"w+");
  if (f_shdr == NULL) {
    pfmt(stderr, MM_ERROR, badopen, catalog, strerror(errno));
    exit(1);
  }
  if (fwrite((char *)&no_sets, sizeof (no_sets) , 1, f_shdr) != 1){
    pfmt(stderr, MM_ERROR, badwrite, catalog, strerror(errno));
    exit(1);
  }
  /*
   * create a tempory for messages
   */
  tmp_file = bldtmp();
  if (tmp_file == (char *)NULL) {
    pfmt(stderr, MM_ERROR, nomem, strerror(errno));
    exit(1);
  }
  f_msg = fopen(tmp_file,"w+");
  if (f_msg == NULL) {
    pfmt(stderr, MM_ERROR, badcreate, tmp_file, strerror(errno));
    exit(1);
  }
  /* 
   * for all the sets
   */
  p_sets = sets;
  nmsg = 1;
  while (p_sets != 0){
    no_sets++;
    /*
     * if set holes then
     * fill them
     */
    set.first_msg = 0;
    set.last_msg = 0;

    while (no_sets != p_sets->set_nr) {
      if (fwrite((char *)&set, sizeof (struct m_cat_set) , 1, f_shdr) != 1){
        pfmt(stderr, MM_ERROR, badwritetmp, strerror(errno));
        exit(1);
      }
      no_sets++;
    }
    p_msg = p_sets->set_msg;
    
    /*
     * Keep offset in shdr temp file to mark the set's begin
     */
    if (p_msg) {
      set.last_msg = 0;
      set.first_msg = nmsg;
      while (p_msg != 0){
        msg_len = p_msg->msg_len;
        /*
         * Get message from main temp file
         */
        if (fseek(tempfile, p_msg->msg_off, 0) != 0){
          pfmt(stderr, MM_ERROR, badseektmp, strerror(errno));
          exit(1);
        }
        if (fread(msg_buf, 1, msg_len, tempfile) != msg_len){
          pfmt(stderr, MM_ERROR, badreadtmp, strerror(errno));
          exit(1);
        }
	msg_buf[msg_len-1] = '\n';

	/*  Output message info if list switched on  */
	if (list)
	  pfmt(stdout, MM_INFO,
	    ":1:Set %d,Message %d,Offset %d,Length %d\n%.*s\n*\n",
	    p_sets->set_nr, p_msg->msg_nr, p_msg->msg_off,
	    p_msg->msg_len, p_msg->msg_len, msg_buf);

        /*
         * Put it in the messages temp file and keep offset
         */
        for (i=set.last_msg+1;i <p_msg->msg_nr;i++,nmsg++) {
          if (fwrite(DFLT_MSG, 1, strlen(DFLT_MSG), f_msg) != strlen(DFLT_MSG)){
            pfmt(stderr, MM_ERROR, badwritetmp, strerror(errno));
            exit(1);
          }
          if (fwrite("\n", 1, 1, f_msg) != 1 ) {
            pfmt(stderr, MM_ERROR, badwritetmp, strerror(errno));
            exit(1);
          }
	}
        set.last_msg  = p_msg->msg_nr;
        if (fwrite(msg_buf, 1, msg_len, f_msg) != msg_len){
          pfmt(stderr, MM_ERROR, badwritetmp, strerror(errno));
          exit(1);
        }
        nmsg++;
        p_msg = p_msg->msg_next;
      }
    } else {
      set.first_msg = 0;
      set.last_msg = 0;
    }
    
    /*
     * Put set hdr into set temp file
     */
    if (fwrite((char *)&set, sizeof (struct m_cat_set) , 1, f_shdr) != 1){
      pfmt(stderr, MM_ERROR, badwritetmp, strerror(errno));
      exit(1);
    }
    p_sets = p_sets->set_next;
  }
  
  /*
   * seek to begining of  file
   * and then write total sets
   */
  if (fseek(f_shdr, 0 , 0) != 0){
    pfmt(stderr, MM_ERROR, ":9:seek() failed in %s: %s\n", catalog,
      strerror(errno));
    exit(1);
  }
  if (fwrite((char *)&no_sets, sizeof (no_sets) , 1, f_shdr) != 1){
    pfmt(stderr, MM_ERROR, badwrite, catalog, strerror(errno));
    exit(1);
  }

  fclose(tempfile);
  fclose(f_shdr);
  fclose(f_msg);

  sprintf(message_file,"%s%s",catalog,M_EXTENSION);
  if (fork() == 0) {
	
    execlp(BIN_MKMSGS,BIN_MKMSGS,"-o",tmp_file,message_file,(char*)NULL);
    pfmt(stderr, MM_ERROR, ":10:Cannot execute %s: %s\n", BIN_MKMSGS,
      strerror(errno));
    exit(1);

  }
  wait(&status);
  unlink(tmp_file);
  if (status) {
    pfmt(stderr, MM_ERROR, ":11:%s failed\n", BIN_MKMSGS);
    exit(1);
  }
}

static 
char *
bldtmp()
{

  char *getenv();
  static char buf[MAXNAMLEN];
  char *tmp;
  char *mktemp();

  tmp = getenv("TMPDIR");
  if (tmp==(char *)NULL || *tmp == '\0')
    tmp="/tmp";

  sprintf(buf,"%s/%s",tmp,mktemp("gencat.XXXXXX"));
  return buf;
}
