#ident	"@(#)gencat:gencat.c	1.2.9.3"
#include <dirent.h>
#include <stdio.h>
#include <ctype.h>
#include <nl_types.h>
#include <locale.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#define _XOPEN_SOURCE 1
#include <widec.h>
#include "gencat.h"

/*
 * gencat : take a message source file and produce a message catalog.
 * usage : gencat [-lmXf <format>] catfile [msgfile]
 * if msgfile is not specified, read from standard input.
 * Several message files can be specified
 */
char msg_buf[NL_TEXTMAX];
struct cat_set *sets = 0;
int curset = 1;
int is_set_1 = 1;
int no_output = 0;
int coff = 0;
int list = 0;
int cat_format = 0;
int show_flag = 0;
char catname[MAXNAMLEN];

static int posix;
static char posix_var[] = "POSIX2";

FILE *tempfile;
FILE *fd_temp;

const char
  nomem[] = ":12:Out of memory: %s\n",
  badread[] = ":13:Cannot read %s: %s\n",
  badreadtmp[] = ":14:Cannot read temporary file: %s\n",
  badwritetmp[] = ":15:Cannot write in temporary file: %s\n",
  badwrite[] = ":16:Cannot write %s: %s\n",
  badcreate[] = ":17:Cannot create %s: %s\n",
  badopen[] = ":18:Cannot open %s: %s\n",
  badseektmp[] = ":19:seek() failed in temporary file: %s\n",
  badsetnr[] = ":20:Invalid set number %d -- Ignored\n",
  badmsgnr[] = ":21:Invalid message number %d -- Ignored\n";

main(argc, argv)
  int argc;
  char **argv;
{
  FILE *fd;
  register int i;
  int cat_exists;
  int opt;
  extern int optind;

  (void)setlocale(LC_ALL, "");
  (void)setcat("uxmesg");
  (void)setlabel("UX:gencat");

  if (getenv(posix_var))
	posix = 1;
  else
	posix = 0;
  /*
   * No arguments at all : give usage
   */
  if (argc == 1)
    usage(1);
    
  /*
   * Create tempfile
   */
  if ((tempfile = tmpfile()) == NULL){
    pfmt(stderr, MM_ERROR, ":22:tempfile() failed: %s\n",
      strerror(errno));
    exit(1);
  }
  
  /*
   * test for mkmsgs format
   */
  while ((opt = getopt(argc, argv, "lmXf:")) != EOF){
    switch (opt) {
      case 'l' :
	list = 1;
	break;
      case 'm' :
      case 'X' :
	if (cat_format != 0)
	  usage(0);
	cat_format = MALLOC_FORMAT;
	break;
      case 'f' :
	if (cat_format != 0)
	  usage(0);

	if (strcmp(optarg, "m") == 0)
	  cat_format = MALLOC_FORMAT;
	else if (strcmp(optarg, "SVR4") == 0)
	  cat_format = SVR_FORMAT;
	else if (strcmp(optarg, "sco") == 0)
	  cat_format = SCO_FORMAT;
	else if (strcmp(optarg, "ISC") == 0)
	  cat_format = ISC_FORMAT;
	else
	  usage(0);
	break;

      default :
	usage(0);

    }
  }
      
  if (optind == argc)
    usage(1);

  /*
   *  Check for environment variable to set default output mode if
   *  no mode specified earlier
   */
  if (cat_format == 0) {
	if (getenv("SCOMPAT") != NULL)
	    cat_format = SCO_FORMAT;
	else if (posix)
	    cat_format = MALLOC_FORMAT;
  }

  /*
   * Check catfile name : if file exists, read it.
   */
  strcpy(catname, argv[optind]);
  
  /*
   * use an existing catalog
   */
  if (strcmp(catname,"-") == 0) {
	if (cat_format != MALLOC_FORMAT) {
		pfmt(stderr,MM_ERROR,
	":206:Output to stdout only supported with MALLOC format catalogues\n");
		pfmt(stderr,MM_ACTION, ":207:Use -m option\n");
		exit(1);
	}
  } else if (cat_build(catname))
    cat_exists = 1;
  else
    cat_exists = 0;

  /*
   * Open msgfile(s) or use stdin and call handling proc
   */
  if (optind == argc - 1)
    cat_msg_build("stdin", stdin);
  else {
    for (i = optind + 1; i < argc ; i++){
      if (strcmp(argv[i], "-") == 0) 
	cat_msg_build("stdin", stdin);
      else {
        if ((fd = fopen(argv[i], "r")) == 0){
          /*
           * Cannot open file for reading
           */
          pfmt(stderr, MM_ERROR, badopen, argv[i], strerror(errno));
          continue;
        }
        if (!posix && argc - optind > 2)
          pfmt(stdout, MM_INFO, ":23:%s:\n", argv[i]);
        cat_msg_build(argv[i], fd);
        fclose(fd);
     }
    }
  }

  if (strcmp(catname,"-") == 0) {
      if (no_output) 
	      pfmt(stderr, MM_ERROR, ":208:catalogue not created\n");
      else 
	      cat_dump("stdout",stdout);
  } else if (!no_output){
    /*
     * Work is done. Time now to open catfile (for writing) and to dump
     * the result
     */

	
    if (cat_format == MALLOC_FORMAT) {
	  if ((fd = fopen(catname,"w")) == NULL) {
	    pfmt(stderr, MM_ERROR, badcreate, catname, strerror(errno));
	    exit(1);
	  }
	  cat_dump(catname, fd);
	  (void)fclose(fd);
    } else if (cat_format == SCO_FORMAT)
      cat_sco_dump(catname);
    else if (cat_format == ISC_FORMAT)
      cat_isc_dump(catname);
    else
      cat_mmp_dump(catname);
  } else
    if (cat_exists)
      pfmt(stderr, MM_ERROR, ":24:%s not updated\n", catname);
    else
      pfmt(stderr, MM_ERROR, ":25:%s not created\n", catname);

  exit(0);
}

usage(complain)
int complain;
{
  if (complain)
    pfmt(stderr, MM_ERROR, ":26:Incorrect usage\n");
  pfmt(stderr, MM_ACTION, ":193:Usage: gencat [-mXlf <format> ] catfile [msgfile ...]\n");
  exit(1);
}

char *linebuf;

/*
 * Scan message file and complete the tables
 */
cat_msg_build(filename, fd)
  char *filename;
  FILE *fd;
{
  register int i, j;
  char quotechar = 0;
  char c;
  int msg_nr = 0;
  int msg_len;
  int offset, buflen;

  /*
   * always a set NL_SETD
   */
  curset=NL_SETD;
  add_set(curset);
  
  if ((linebuf = (char *)malloc(BUFSIZ)) == 0) {
	pfmt(stderr, MM_ERROR, ":209:Cannot allocate sufficient memory\n");
	exit(1);
  }
  buflen = BUFSIZ;
  while (fgets(linebuf, buflen, fd) != 0){
    while ((offset = strlen(linebuf)) == buflen - 1 &&
      linebuf[offset-1] != '\n' ) {
	buflen += BUFSIZ - 1;
	if ((linebuf = (char *)realloc(linebuf, buflen)) == 0) {
	  pfmt(stderr, MM_ERROR, ":209:Cannot allocate sufficient memory\n");
	  exit(1);
	}
	if (fgets(linebuf+offset, BUFSIZ, fd) == 0)
	  break;
    }

    if ((i = skip_blanks(linebuf, 0)) == -1)
      continue;

    if (linebuf[i] == '$'){
      i += 1;
      /*
       * Handle commands or comments
       */
      if (strncmp(&linebuf[i], CMD_SET, CMD_SET_LEN) == 0){
        i += CMD_SET_LEN;

        /*
         * Change current set
         */
        if ((i = skip_blanks(linebuf, i)) == -1){
	  pfmt(stderr, MM_WARNING, ":28:Incomplete set command -- Ignored\n");
          continue;
        }
        add_set(atoi(&linebuf[i]));
        continue;
      }
      if (strncmp(&linebuf[i], CMD_DELSET, CMD_DELSET_LEN) == 0){
  i += CMD_DELSET_LEN;

        /*
         * Delete named set
         */
        if ((i = skip_blanks(linebuf, i)) == -1){
          pfmt(stderr, MM_WARNING, ":29:Incomplete delset command -- Ignored\n")
          ;
          continue;
        }
        del_set(atoi(&linebuf[i]));
        continue;
      }
      if (strncmp(&linebuf[i], CMD_QUOTE, CMD_QUOTE_LEN) == 0){
        i += CMD_QUOTE_LEN;

        /*
         * Change quote character
         */
        if ((i = skip_blanks(linebuf, i)) == -1)
          quotechar = 0;
        else
          quotechar = linebuf[i];
        continue;
      }
      /*
       * Everything else is a comment
       */
      continue;
    }
    if (isdigit(linebuf[i])){
      msg_nr = 0;
      /*
       * A message line
       */
      while(isdigit(c = linebuf[i])){
        msg_nr *= 10;
        msg_nr += c - '0';
        i++;
      }
      j = i;
      if (linebuf[i] == '\n')
        del_msg(curset, msg_nr);
      else
      if ((i = skip_blanks(linebuf, i)) == -1)
	add_msg(curset,msg_nr,1,"");
      else {
        if ((msg_len = msg_conv(filename, curset, msg_nr, fd, linebuf + j,
        BUFSIZ, msg_buf, NL_TEXTMAX, quotechar)) == -1){
    no_output = 1;
          continue;
  }
        add_msg(curset, msg_nr, msg_len + 1, msg_buf);
      }
      continue;
    }
    
    /*
     * Everything else is unexpected
     */
    pfmt(stderr, MM_WARNING, ":30:Unexpected line -- Skipped\n");
  }
}

/*
 * Skip blanks in a line buffer
 */
skip_blanks(linebuf, i)
  char *linebuf;
  int i;
{
static wctype_t blank = 0;

  if (blank == 0)
	blank = wctype("blank");

  while (linebuf[i] && iswctype(linebuf[i], blank))
    i++;
  if (!linebuf[i] || linebuf[i] == '\n')
    return -1;
  return i;
}

/*
 * Dump the internal structure into the catfile.
 */
cat_dump(char *catalog, FILE *fd)
{
  FILE *f_shdr, *f_mhdr, *f_msg;
  long o_shdr, o_mhdr, o_msg;
  struct cat_set *p_sets;
  struct cat_msg *p_msg;
  struct cat_hdr hdr;
  struct cat_set_hdr shdr;
  struct cat_msg_hdr mhdr;
  int msg_len;
  int nmsg = 0;
  
  if ((f_shdr = tmpfile()) == NULL || (f_mhdr = tmpfile()) == NULL ||
      (f_msg = tmpfile()) == NULL){
    pfmt(stderr, MM_ERROR, ":31:Cannot create temporary file: %s\n",
      strerror(errno));
    exit(1);
  }
  o_shdr = 0;
  o_mhdr = 0;
  o_msg = 0;
  
  p_sets = sets;
  hdr.hdr_set_nr = 0;
  hdr.hdr_magic = CAT_MAGIC;
  while (p_sets != 0){
    hdr.hdr_set_nr++;
    p_msg = p_sets->set_msg;
    
    /*
     * Keep offset in shdr temp file to mark the set's begin
     */
    shdr.shdr_msg = nmsg;
    shdr.shdr_msg_nr = 0;
    shdr.shdr_set_nr = p_sets->set_nr;
    while (p_msg != 0){
      shdr.shdr_msg_nr++;
      nmsg++;
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
      
      /*
       * Put it in the messages temp file and keep offset
       */
      mhdr.msg_ptr = (int)ftell(f_msg);
      mhdr.msg_len = msg_len;
      mhdr.msg_nr  = p_msg->msg_nr;
      if (fwrite(msg_buf, 1, msg_len, f_msg) != msg_len){
        pfmt(stderr, MM_ERROR, badwritetmp, strerror(errno));
        exit(1);
      }

      /*
       * Put message header
       */
      if (fwrite((char *)&mhdr, sizeof(mhdr), 1, f_mhdr) != 1){
        pfmt(stderr, MM_ERROR, badwritetmp, strerror(errno));
        ;
        exit(1);
      }
      p_msg = p_msg->msg_next;
    }
    
    /*
     * Put set hdr
     */
    if (fwrite((char *)&shdr, sizeof(shdr), 1, f_shdr) != 1){
      pfmt(stderr, MM_ERROR, badwritetmp, strerror(errno));
      exit(1);
    }
    p_sets = p_sets->set_next;
  }
  
  /*
   * Fill file header
   */
  hdr.hdr_mem = ftell(f_shdr) + ftell(f_mhdr) + ftell(f_msg);
  hdr.hdr_off_msg_hdr = ftell(f_shdr);
  hdr.hdr_off_msg = hdr.hdr_off_msg_hdr + ftell(f_mhdr);

  /*
   * Generate catfile
   */
  if (fwrite((char *)&hdr, sizeof (hdr), 1, fd) != 1){
    pfmt(stderr, MM_ERROR, badwrite, catalog, strerror(errno));
    exit(1);
  }
  
  copy(fd, f_shdr);
  copy(fd, f_mhdr);
  copy(fd, f_msg);
  fclose(f_shdr);
  fclose(f_mhdr);
  fclose(f_msg);
}

static char copybuf[BUFSIZ];
static
copy(dst, src)
  FILE *dst, *src;
{
  int n;
  
  rewind(src);
  
  while ((n = fread(copybuf, 1, BUFSIZ, src)) > 0){
    if (fwrite(copybuf, 1, n, dst) != n){
      pfmt(stderr, MM_ERROR, badwrite, catname, strerror(errno));
      exit(1);
    }
  }
}

void
fatal(err_str)
char *err_str;
{
  pfmt(stderr, MM_ERROR, err_str);
  exit(1);
}
