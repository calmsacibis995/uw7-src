#ident	"@(#)gencat:cat_build.c	1.1.7.5"

#include <dirent.h>
#include <locale.h>
#include <stdio.h>
#include "nl_types.h"
#include <malloc.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include "gencat.h"

extern int list;
extern int cat_format;
extern FILE *tempfile;
extern char msg_buf[];
extern const char nomem[], badread[], badwritetmp[];

struct cat_set *sets;
static void strcpy_conv();
/*
 * Read a catalog and build the internal structure
 */
cat_build(catname)
  char *catname;
{
  FILE *fd;
  long magic;
  
    
  /*
   * Test for mkmsgs
   * or old style malloc
   */

  if ((fd = fopen(catname, "r")) == 0){
    /*
     * does not exist
     */
    return 0;
  }

  if (fread(&magic, sizeof(long), 1, fd) != 1){
    pfmt(stderr, MM_ERROR, badread, catname, gettxt(":205", "empty or bad catalog file"));
    exit(1);
  }
  /*  The SCO header uses a short for the magic number, so we need to
   *  mask out what we need
   */
  if ((magic & 0xFFFF) == M_MFMAG)
  {
    cat_sco_build(fd);

    if (cat_format == 0)
      cat_format = SCO_FORMAT;
  }
  else if (magic == CAT_MAGIC) 
  {
    cat_malloc_build(fd,catname);

    if (cat_format == 0)
      cat_format = MALLOC_FORMAT;
  }
  else if (magic == ISC_MAGIC)
  {
    cat_isc_build(fd);

    if (cat_format == 0)
      cat_format = ISC_FORMAT;
  }
  else 
    cat_mmp_build(fd,catname);

  fclose(fd);
  
  return 1;
}

static
cat_malloc_build (fd,catname)
  char  *catname;
  FILE *fd;
{
  struct cat_hdr hdr;
  struct cat_set *cur_set, *set_last = NULL;
  struct cat_msg *cur_msg, *msg_last = NULL;
  char *data;
  struct cat_set_hdr set_hdr;
  struct cat_msg_hdr msg_hdr;
  register int i;

    
  /*
   * Read malloc file header
   */
  rewind(fd);
  if (fread((char *)&hdr, sizeof(struct cat_hdr), 1, fd) != 1){
    pfmt(stderr, MM_ERROR, badread, catname, strerror(errno));
    exit(1);
  }
  /*
   * Read sets headers
   */
  for (i = 0 ; i < hdr.hdr_set_nr ; i++){
    struct cat_set *new;
    if ((new = (struct cat_set *)malloc(sizeof(struct cat_set))) == 0){
      pfmt(stderr, MM_ERROR, nomem, strerror(errno));
      exit(1);
    }
    if (fread((char *)&set_hdr, sizeof(struct cat_set_hdr), 1, fd) != 1){
      pfmt(stderr, MM_ERROR, badread, catname, strerror(errno));
      exit(1);
    }
    new->set_nr = set_hdr.shdr_set_nr;
    new->set_msg_nr = set_hdr.shdr_msg_nr;
    new->set_next = 0;
    if (sets == 0)
      sets = new;
    else
      set_last->set_next = new;
    set_last = new;
  }

  /*
   * Read messages headers
   */
  for (cur_set = sets ; cur_set != 0 ; cur_set = cur_set->set_next){
    cur_set->set_msg = 0;
    for (i = 0 ; i < cur_set->set_msg_nr ; i++){
      struct cat_msg *new;

      if ((new = (struct cat_msg *)malloc(sizeof(struct cat_msg))) == 0){
        pfmt(stderr, MM_ERROR, nomem, strerror(errno));
        exit(1);
      }
      if (fread((char *)&msg_hdr, sizeof(struct cat_msg_hdr), 1, fd) != 1){
        pfmt(stderr, MM_ERROR, badread, catname, strerror(errno));
        exit(1);
      }
      new->msg_nr = msg_hdr.msg_nr;
      new->msg_len = msg_hdr.msg_len;
      new->msg_next = 0;
      if (cur_set->set_msg == 0)
        cur_set->set_msg = new;
      else
        msg_last->msg_next = new;
      msg_last = new;
    }
  }
  
  /*
   * Read messages.
   */
  for (cur_set = sets ; cur_set != 0 ; cur_set = cur_set->set_next){
    for (cur_msg = cur_set->set_msg ;cur_msg!= 0;cur_msg= cur_msg->msg_next){

      if (fread(msg_buf, 1, cur_msg->msg_len, fd) != cur_msg->msg_len ){
        pfmt(stderr, MM_ERROR, badread, catname, strerror(errno));
        exit(1);
      }

      /*
       * Put message in the temp file and keep offset
       */
      cur_msg->msg_off = ftell(tempfile);

      if (list)
	pfmt(stdout, MM_INFO, 
		":1:Set %d,Message %d,Offset %d,Length %d\n%.*s\n*\n",
		cur_set->set_nr, cur_msg->msg_nr, cur_msg->msg_off,
		cur_msg->msg_len, cur_msg->msg_len, msg_buf);

      if (fwrite(msg_buf, 1, cur_msg->msg_len, tempfile) != cur_msg->msg_len){
        pfmt(stderr, MM_ERROR, badwritetmp, strerror(errno));
        exit(1);
      }
    }
  }
}

static
cat_mmp_build (fd,catname)
  FILE *fd;
  char  *catname;
{
  struct cat_set *cur_set, *set_last = NULL;
  struct cat_msg *cur_msg, *msg_last = NULL;
  struct m_cat_set set;
  register int i;
  register int j;
  register int k;
  int no_sets;
  int msg_ptr;
  int mfd;
  caddr_t addr;
  char *p;
  struct cat_set *new;
  char message_file[MAXNAMLEN];
  char temp_text[NL_TEXTMAX];
  struct stat sb;

  /*
   * get the number of sets
   * of a set file
   */
  rewind(fd);
  if (fread(&no_sets, 1, sizeof(int), fd) != sizeof(int) ){
    pfmt(stderr, MM_ERROR, ":2:%s: Cannot get number of sets\n", catname);
    exit(1);
  }
  
  /*  Open the message file for reading  */
  sprintf(message_file, "%s.m", catname);

  if ((mfd = open(message_file, O_RDONLY)) < 0) {
    pfmt(stderr, MM_ERROR, ":171:Cannot open message file %s:%s\n", 
      message_file, strerror(errno));
    exit(1);
  }

  if (fstat(mfd, &sb) < 0)
  {
    pfmt(stderr, MM_ERROR, ":172:Could not stat message file: %s\n",
      strerror(errno));
    exit(1);
  }

  addr = mmap(0, sb.st_size, PROT_READ, MAP_SHARED, mfd, 0);
  close(mfd);

  if (addr == (caddr_t)-1)
  {
    pfmt(stderr, MM_ERROR, ":173:Cannot map message file: %s\n", strerror(errno));
    exit(1);
  }

  for(i=1;i<=no_sets;i++) {
    if (fread(&set,1,sizeof(struct m_cat_set),fd) != sizeof(struct m_cat_set) ){
      pfmt(stderr, MM_ERROR, ":4:%s: Cannot get set information\n", catname);
      munmap(addr, sb.st_size);
      exit(1);
    }
    if ((new = (struct cat_set *)malloc(sizeof(struct cat_set))) == 0){
      pfmt(stderr, MM_ERROR, nomem, strerror(errno));
      munmap(addr, sb.st_size);
      exit(1);
    }
    if (set.first_msg == 0)
      continue;
    new->set_nr = i;
    new->set_next = 0;
    if (sets == 0)
      sets = new;
    else
      set_last->set_next = new;
    set_last = new;

    /*
     * create message headers
     */
    new->set_msg = 0;
    for(j=1, k=set.first_msg; j <=set.last_msg && k != 0; k++, j++) {
        msg_ptr = *((int *)(addr + (k * sizeof(int))));
        p = addr + msg_ptr;
        strcpy_conv(temp_text, p);

	if (strcmp(temp_text,DFLT_MSG)) {
          struct cat_msg *new_msg;
          if ((new_msg=(struct cat_msg *)malloc(sizeof(struct cat_msg))) == 0){
            pfmt(stderr, MM_ERROR, nomem, strerror(errno));
	    munmap(addr, sb.st_size);
            exit(1);
	  }
          if (new->set_msg == 0) 
	    new->set_msg = new_msg;
	  else
	    msg_last->msg_next = new_msg;
	  new_msg->msg_nr = j;
	  new_msg->msg_len = strlen(temp_text)+1;
	  new_msg->msg_next = 0;
	  msg_last = new_msg;
	  /*
	   * write message to tempfile
	   */
          new_msg->msg_off = ftell(tempfile);
	  if (list)
	    pfmt(stdout, MM_INFO, ":5:Set %d,Message %d,Text %s\n*\n", i, j, 
	    	temp_text);
          if(fwrite(temp_text,1,new_msg->msg_len,tempfile)!=new_msg->msg_len){
            pfmt(stderr, MM_ERROR, badwritetmp, strerror(errno));
	    munmap(addr, sb.st_size);
            exit(1);
          }
	}
    }
  }
  munmap(addr, sb.st_size);
}

/* 
 * mkmsgs cant handle the new line
 */
static void
strcpy_conv ( a , b )
char *a,*b;
{

  while (*b) {

    switch (*b) {

      case '\n' :
        *a++ = '\\';
        *a++ = 'n';
	b++;
        break;

      case '\\' :
        *a++ = '\\';
        *a++ = '\\';
	b++;
        break;

      default :
	*a++ = *b++;
    }
  }
  *a = '\0';
}
