#pragma ident "@(#)elfops.c	29.1"
#pragma ident "$Header$"

/*
 *
 *      Copyright (C) The Santa Cruz Operation, 1993-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libelf.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/systeminfo.h>
#include <sys/utsname.h>
#include "common.h"

/* 
 * BL9: we always want to see the driver version to encourage people to use
 *      the MDI version.  If you want to go back to the old behaviour,
 *      uncomment the following line:
 *      #define MAGIC "__NDCFG_SHOWTYPE" 
 * BL15.1: restored to previous method
 */
#define MAGIC "__NDCFG_SHOWTYPE" 

/* the name of the section we add */
#define NDNOTE ".ndnote"

/* numeric type of this note.  While the ELF specification defines the general 
 * .note/PT_NOTE/SHT_NOTE format it doesn't define the specific numbers
 * for "type".  We choose 1
 */
#define NDNOTETYPE 1

/* the name that identifies who created this note.  See the ELF spec. */
#define NDNOTENAME "SCO Network Driver Group"
#define NDNOTENAMESIZE 28 /*(strlen(NDNOTENAME)|4)-must be mod 4 for padding*/

/* the machine dependent fields have a size limit of SYS_NMLN (257) bytes.  
 * however I don't really want to preallocate that many bytes for each
 * field in the .ndnote section when none of them need that many bytes
 * so I'll lower it to 10
 */
#define FIELDSIZE 10    /* really SYS_NMLN */

/* name of the variable in the Driver.o we look for 
 * if a driver declares this as
 *  static char _ndversion[]="1.2.3beta";
 * or, in an SCCS environment,
 *  static char _ndversion[]="% I %";    should suffice for most purposes.
 *  except that there isn't a space between '% I' and 'I %' in the real thing.
 * If found then we add to .driverversion.   Note it must be static so that 
 * it doesn't cause multiply defined symbols at relink if there are other net 
 * drivers in the system at the same time that also use this scheme
 * and also declare a _ndversion[] variable!
 * Note that while the routines in this file accept any .o file as stampable,
 * bcfgops.c will only check for the version in a Driver.o file.  As a result,
 * _ndversion cannot be defined in the Space.c file.
 */
#define NDVERSIONSYMBOL "_ndversion" 

#pragma pack(1)

/* this is the literal .ndnote section contents.  The names are straight
 * from the ELF spec.
 */
struct ndnote {
   u_int namesz;   /* actual size: strlen(NDNOTENAME)+1 for null */
   u_int descsz;   /* sizeof(desc) */
   u_int type;     /* NDNOTETYPE */
   char name[NDNOTENAMESIZE]; /* size here must be mod 4 for padding */
   struct {
      /* here's the content of the .ndnote section we care about */
      char string[NDNOTEMAXSTRINGSIZE+1]; /* add a byte for null termination */
      time_t date;  /* when the .ndnote section was added */
      char version[FIELDSIZE];
      char release[FIELDSIZE];
      char architecture[FIELDSIZE];
      char hw_provider[FIELDSIZE];
      char hw_serial[FIELDSIZE];
      char machine[FIELDSIZE];
      /* put other stuff in here as we add more fields to section. */
   } desc;
};
#pragma pack()

typedef struct ndnote ndnote_t;
extern u_int iflag;


/* this ugly hack is necessary because sysinfo(2) (aka systeminfo)
 * doesn't exist in /usr/lib/libc.so* but does exist in /usr/ccs/lib/libc*
 * as weakly defined causing:
 * - linker to work properly (i.e. we get an executable)
 * - rtld to choke since it can't resolve it in the runtime directory
 * we just define our own sysinfo here.   
 *  define WORKING_LIBC_SYSINFO someday to avoid this mess.
 * note that sysinfo(2) even has a man page...
 * 
 * Gemini BL8v2e: sysinfo properly exists in libc.so.1 now so define
 * WORKING_LIBC_SYSINFO.  Keep code around for compiling on earlier releases
 */
#define WORKING_LIBC_SYSINFO
#ifdef WORKING_LIBC_SYSINFO
int
mysysinfo(int command, char *buf, size_t count)
{
   return(sysinfo(command, buf, count));
} 
#else

#define SYSINFO 139   /* from syscall.h, sysent.c */

__asm int 
mysysinfo(int command, char *buf, size_t count)
{
%mem command, buf, count;

   pushl count
   pushl buf
   pushl command
   pushl $0xdeadbeef    / doesn't matter
   movl  $SYSINFO,%eax
   lcall $0x7,$0
   addl  $0x10,%esp
   movl  $0,%eax        / "success"
   / we should check carry bit and set if set to move -1 into %eax and
   / set errno appropriately.  
}

#endif   /* WORKING_LIBC_SYSINFO */


/* fills in all of the .desc fields in ndnote.  Could also use POSIX getconf */
void
fillinnote(ndnote_t *ndnote, char *string, u_int errcode)
{
   /* could also add /etc/config/licensekeys but I suspect that will
    * be disappearing in the future. 
    */

   if (mysysinfo(SI_VERSION, ndnote->desc.version, FIELDSIZE) < 0) {
      error(errcode,"fillinnote: mysysinfo failed");
      return;
   }
   if (mysysinfo(SI_RELEASE, ndnote->desc.release, FIELDSIZE) < 0) {
      error(errcode,"fillinnote: mysysinfo failed");
      return;
   }
   if (mysysinfo(SI_ARCHITECTURE, ndnote->desc.architecture, FIELDSIZE) < 0) {
      error(errcode,"fillinnote: mysysinfo failed");
      return;
   }
   if (mysysinfo(SI_HW_PROVIDER, ndnote->desc.hw_provider, FIELDSIZE) < 0) {
      error(errcode,"fillinnote: mysysinfo failed");
      return;
   }
   if (mysysinfo(SI_HW_SERIAL, ndnote->desc.hw_serial, FIELDSIZE) < 0) {
      error(errcode,"fillinnote: mysysinfo failed");
      return;
   }
   if (mysysinfo(SI_MACHINE, ndnote->desc.machine, FIELDSIZE) < 0) {
      error(errcode,"fillinnote: mysysinfo failed");
      return;
   }

   ndnote->desc.date=time(0);
   memset(ndnote->desc.string, '\0', NDNOTEMAXSTRINGSIZE+1);
   strncpy(ndnote->desc.string, string, NDNOTEMAXSTRINGSIZE);
   return;
}


/* operations for cmd */
#define CMD_STAMP    0
#define CMD_GETSTAMP 1


int fd;
Elf *elf;
char *freethis=(char *)-1;
int err;

/* returns -1 if error
 * 0 for success - not found
 * 1 for success - found
 *
 * we can only call error() in this routine when we encounter an ELF
 * problem since it's acceptable for a Driver.o to not have a .ndnote section
 *
 * the doopenbeginend is used do for performance improvements and
 * determines if we should call open/elf_begin/elf_end in this routine
 * since elf_begin _should_ use mmap doing this twice isn't terribly
 * expensive but we keep it open do avoid this anyway.
 */
int
elfop(int doopenbeginend, int cmd, char *filename, char *string, u_int errcode)
{
   int loop;
   Elf32_Ehdr h,*ehdr;
   Elf_Data *data,*strtabdata;
   Elf_Scn *scn,*strtabscn;
   Elf32_Shdr *shdr,*strtabshdr;
   Elf32_Phdr *phdr;
   unsigned int ndx;
   int found=0;
   char *name;
   size_t foundscnindex;
   int returncode=1;  /* assume we will find it */

   if ((cmd == CMD_STAMP) && (strlen(string) > NDNOTEMAXSTRINGSIZE)) {
      error(errcode,"can't stamp string -- too long");
      return(-1);
   }
   if (doopenbeginend) {
      (void) elf_version(EV_NONE);
      if (elf_version(EV_CURRENT) == EV_NONE) {
         /* library out of date -- "shouldn't happen" */
         err=elf_errno();
         error(errcode,"libelf is out of date - %d=%s",
                 err,elf_errmsg(err));
         goto donefail;
      }
      fd=open(filename, (cmd == CMD_GETSTAMP) ? O_RDONLY : O_RDWR);
      if (fd == -1) {
         error(errcode,"elfop: problem opening file %s errno=%d",
                       filename,errno);
         goto donefail;
      }
   
      if (read(fd,&h,sizeof(h)) != sizeof(h)) {
         error(errcode,"elfop: problem reading elf header");
         goto donefail;
      }

      if ((h.e_ident[EI_MAG0] != ELFMAG0) ||
          (h.e_ident[EI_MAG1] != ELFMAG1) ||
          (h.e_ident[EI_MAG2] != ELFMAG2) ||
          (h.e_ident[EI_MAG3] != ELFMAG3) ||
          (h.e_ident[EI_CLASS] != ELFCLASS32) || /* must have 32 bit routines */
          (h.e_type != ET_REL) || /* only .o files not executables */
          (h.e_shentsize * h.e_shnum == 0) ||
          (h.e_shstrndx == SHN_UNDEF)) {
         error(errcode,"%s: invalid ELF file or unstampable ELF file",filename);
         goto donefail;
      }

      lseek(fd,0,0);
      /* we only have one "activation" of our elf routine so we 
       * know that a call to elf_end will terminate the elf descriptor
       * since the activation count will be zero
       * otherwise we need to call elf_end multiple times -- once for each
       * open descriptor -- and check elf_end return value.
       */
      elf=elf_begin(fd,(cmd == CMD_GETSTAMP) ? ELF_C_READ:ELF_C_RDWR,(Elf *)0);

      /* we could do an elf_cntl(elf, ELF_C_FDREAD) here if cmd is CMD_STAMP
       * which should allow a later creat() before elf_update but we don't
       */

      if (elf == (Elf *)0) {
         err=elf_errno();
         error(errcode,"elf_begin returned failure - %d=%s",
                 err,elf_errmsg(err));
         goto donefail;
      }
   }

   if (elf_kind(elf) != ELF_K_ELF) {   /* could be COFF or archive */
      error(errcode,"%s isn't a single stampable ELF file");
      goto donefail;
   }
  
   /* start off by getting ehdr.  required by libelf routines */
   ehdr=elf32_getehdr(elf);
   if (ehdr == 0) {
      err=elf_errno();
      error(errcode,"elf32_getehdr returned failure - %d=%s",
              err,elf_errmsg(err));
      goto donefail;
   }

   /* program header is optional and not normally found in
    * .o files, so this might fail.  we don't care.
    */
   phdr=elf32_getphdr(elf);

   /* now get .shstrtab information */
   strtabscn=elf_getscn(elf, (size_t) ehdr->e_shstrndx);
   if (strtabscn == 0) {
      err=elf_errno();
      error(errcode,"elf_getscn for .shstrtab returned failure - %d=%s",
              err,elf_errmsg(err));
      goto donefail;
   }
   strtabshdr=elf32_getshdr(strtabscn);
   if (strtabshdr == 0) {
      err=elf_errno();
      error(errcode,"elf32_getshdr for .shstrtab returned failure - %d=%s",
              err,elf_errmsg(err));
      goto donefail;
   }

   if (strtabshdr->sh_type != SHT_STRTAB) {
      /* not a string table -- shouldn't happen since we used e_shstrndx! */
      error(errcode,"e_shstrndx doesn't point to STRTAB!");
      goto donefail;
   }

   strtabdata=0;
   strtabdata=elf_getdata(strtabscn, strtabdata);
   if (strtabdata == 0) {
      err=elf_errno();
      error(errcode,"elf_getdata for .shstrtab returned failure - %d=%s",
              err,elf_errmsg(err));
      goto donefail;
   }

   if (strtabdata->d_size == 0) {
      /* empty string table */
      error(errcode,"empty string table in .shstrtab!");
      goto donefail;
   }

   /* too bad we can have weird offsets into .shstrtab, because we would
    * much rather parse through it directly instead of walking through
    * all sections to get the name.  this is the official way though
    */
   scn=0;
   while ((scn=elf_nextscn(elf, scn)) != 0) {
      shdr=elf32_getshdr(scn);
      if (shdr == 0) {
         err=elf_errno();
         error(errcode,"elf32_getshdr returned failure - %d=%s",
                 err,elf_errmsg(err));
         goto donefail;
      }

      name=elf_strptr(elf, ehdr->e_shstrndx, (size_t) shdr->sh_name);

      if (strcmp(name,NDNOTE) == 0) {
         found=1;
         foundscnindex=elf_ndxscn(scn);  /* will succeed since scn != 0 */
         break;
      }
   }  /* processed all sections */

   if (!found) {    
      /* no .ndnote section found in .o file */
      Elf_Data *addon,*newnote;

      if (cmd == CMD_GETSTAMP) {
         returncode=0;  /* success -- but not found */
         goto donesuccess;
      }
 
      if (cmd != CMD_STAMP) {
         error(errcode,"unknown command!");
         goto donefail;
      }

      /* add .ndnote on to the end of .shstrtab */
      addon=elf_newdata(strtabscn);
      if (addon == 0) {
         err=elf_errno();
         error(errcode,"elf_newdata for addon returned failure - %d=%s",
                 err,elf_errmsg(err));
         goto donefail;
      }
      addon->d_buf=NDNOTE;  /* do not call strdup(NDNOTE) here - 
                             * space would not be free()d at elf_end time
                             */
      addon->d_type=ELF_T_BYTE;  /* since type is SHT_STRTAB */
      addon->d_size=strlen(NDNOTE) + 1;  /* add on the null! */
      addon->d_off=strtabdata->d_size;   /* 0-based - correct  */
      addon->d_align=1;   /* 1 byte alignment */
      addon->d_version=EV_CURRENT;
      
      /* create new .ndnote section */
      scn=elf_newscn(elf);
      if (scn == 0) {
         err=elf_errno();
         error(errcode,"elf_newscn returned failure - %d=%s",
                 err,elf_errmsg(err));
         goto donefail;
      }
      shdr=elf32_getshdr(scn);
      if (shdr == 0) {
         err=elf_errno();
         error(errcode,"elf_getshdr returned failure - %d=%s",
                 err,elf_errmsg(err));
         goto donefail;
      }
      shdr->sh_name=strtabdata->d_size;
      shdr->sh_type = SHT_NOTE;
      shdr->sh_flags = 0;   /* maybe SHF_ALLOC? */
      shdr->sh_addr = 0;  /* doesn't appear in memory image of process */
      /* shdr->sh_offset =  don't have to set it */
      /* shdr->sh_size = don't have to set it */
      shdr->sh_link = SHN_UNDEF;
      shdr->sh_info = 0;
      shdr->sh_addralign = 4;  /* may not need to set this */
      shdr->sh_entsize = 0;   /* no fixed size entries here */

      /* here's the contents of our new .ndnote section */
      newnote=elf_newdata(scn);   /* add on to just created scn */
      if (newnote == 0) {
         err=elf_errno();
         error(errcode,"elf_newdata for newnote returned failure - %d=%s",
                 err,elf_errmsg(err));
         goto donefail;
      }
      /* d_buf is not free()ed by libelf when you call elf_end so we
       * must remember to do it ourself.  Hence the variable freethis.
       */
      newnote->d_buf=(ndnote_t *)malloc(sizeof(ndnote_t));
      if (newnote->d_buf == 0) {
         error(errcode,"malloc for newnote failed");
         goto donefail;
      }
      freethis=(char *)newnote->d_buf;/* to free up this space after elf_end */
      ((ndnote_t *)newnote->d_buf)->namesz = strlen(NDNOTENAME)+1;
      ((ndnote_t *)newnote->d_buf)->descsz = sizeof(((ndnote_t *)
                                                      newnote->d_buf)->desc);
      ((ndnote_t *)newnote->d_buf)->type = NDNOTETYPE;
      memset(((ndnote_t *)newnote->d_buf)->name, '\0', NDNOTENAMESIZE);
      strcpy(((ndnote_t *)newnote->d_buf)->name,NDNOTENAME);


      fillinnote(((ndnote_t *)newnote->d_buf),string,errcode);

      newnote->d_type=ELF_T_BYTE;   /* since section type is SHT_NOTE */
      newnote->d_size = sizeof(ndnote_t);
      newnote->d_off=0;   /* start of section since new section */
      newnote->d_align=4;  /* notes are 4 byte aligned */
      newnote->d_version=EV_CURRENT;
   } else {
      /* .ndnote section was found in .o file.  -- either
       * a) fill in contents if cmd is CMD_GETSTAMP
       * b) overwrite current contents if cmd is CMD_STAMP 
       *    While ELF allows multiple sections with same name, we don't.
       * IN ALL CASES WE MUST GOTO DONESUCCESS TO FREE UP MEMORY
       * remember that foundscnindex points to the index containing .ndnote
       */
      Elf_Scn *notescn;
      Elf_Data *notedata;
      ndnote_t *ndnote;

      notescn=elf_getscn(elf, foundscnindex);
      if (notescn == 0) {
         error(errcode,"couldn't get found .ndnote section");
         goto donefail;
      }
      
      notedata=elf_getdata(notescn, 0);
      if (notedata == 0) {
         error(errcode,"couldn't get found .ndnote section data");
         goto donefail;
      }

      /* ensure the .ndnote is "valid" for our respective version */
      if (notedata->d_size < sizeof(ndnote_t)) {
         error(errcode,".ndnote section in %s is corrupt",filename);
         /* we don't take corrective action because
          * - .o file might have been opened O_RDONLY/ELF_C_READ
          * - too much bother (can't free up old section easily)
          */
         goto donefail;
      }

      /* we assume that if notedata->d_size > sizeof(ndnote_t) that
       * it is still backwards compatible
       * we know that the size is at least sizeof nodnote_t to get here
       */
      ndnote=(ndnote_t *)notedata->d_buf;      
      if ((ndnote->namesz != strlen(NDNOTENAME)+1) || 
          (ndnote->descsz != sizeof(ndnote->desc)) ||
          (ndnote->type   != NDNOTETYPE) ||
          (memcmp(NDNOTENAME,ndnote->name,strlen(NDNOTENAME)) != 0)) {
         /* we don't take corrective action because
          * - .o file might have been opened O_RDONLY/ELF_C_READ
          * - don't know what to do (is it a newer .ndnote format?)
          */
         error(errcode,"unknown .ndnote format");
         goto donefail;
      }

      if (cmd == CMD_GETSTAMP) {
         strncpy(string,ndnote->desc.string,NDNOTEMAXSTRINGSIZE);
         if (iflag) {  /* not set if in tcl mode so Pstdout is ok here */
            Pstdout("timestamp is at %s",ctime(&ndnote->desc.date));
            Pstdout("version      = '%s'\n",ndnote->desc.version);
            Pstdout("release      = '%s'\n",ndnote->desc.release);
            Pstdout("architecture = '%s'\n",ndnote->desc.architecture);
            Pstdout("hw_provider  = '%s'\n",ndnote->desc.hw_provider);
            Pstdout("hw_serial    = '%s'\n",ndnote->desc.hw_serial);
            Pstdout("machine      = '%s'\n",ndnote->desc.machine);
         }
      } else if (cmd == CMD_STAMP) {

         fillinnote(ndnote, string,errcode);

      } else {
         error(errcode,"elfop: Unknown cmd %d",cmd);
         goto donefail;
      }
   }

donesuccess:
   /* update our on-disk filename.o if we need to */
   if (cmd == CMD_STAMP) {
      /* close (creat(filename, 0644)); causes errors from libelf 
       * could avoid by call to elf_cntl(elf, ELF_C_FDREAD) but we don't
       */
      if (elf_update(elf, ELF_C_WRITE) == -1) {
         err=elf_errno();
         error(errcode,"elf_update returned failure - %d=%s",
                 err,elf_errmsg(err));
         goto donefail;
      }
   }
   if (doopenbeginend) {
      if (elf_end(elf) != 0) {
         err=elf_errno();
         error(errcode,"elf_end in donesuccess returned failure - %d=%s",
                 err,elf_errmsg(err));
         goto donefail;
      }
      /* since elf_end doesn't free up Elf_Data d_buf section we must do it
       * ourself if we created a new .ndnote section
       */
      if (freethis != (char *)-1) free(freethis);
      close(fd);
   }
   return(returncode);
donefail:
   /* free up libelf memory if we called elf_begin */
   if (doopenbeginend) {
      if ((elf != (Elf *)-1) && (elf_end(elf) != 0)) {
         err=elf_errno();
         error(errcode,"elf_end in donefail returned failure - %d=%s",
                 err,elf_errmsg(err));
         /* we drop through to possibly calling free()/close() and we
          * still return(-1)
          */
      }
      /* since elf_end doesn't free up Elf_Data d_buf section we must do it
       * ourself if we created a new .ndnote section
       */
      if (freethis != (char *)-1) free(freethis);
      /* close our fd if we called open */
      if (fd != -1) close(fd);
   }
   return(-1);
}


/* returns 0 for success else -1 for error */
int
stamp(char *filename, char *string, u_int errcode)
{
   int status;

   string++;   /* when we get it from lexer 1st char is space since
                * we enter state WANTSPACE immediately after we have
                * read the filename and not the first char of the string
                * so we that space as part of our string
                */

   if (*filename != '/') {
      error(errcode,"stamp: must give absolute path to file %s",filename);
      return(-1);
   }

   if (strlen(string) > NDNOTEMAXSTRINGSIZE) {
      error(errcode,"stamp: string to stamp must be less than %d chars",
            NDNOTEMAXSTRINGSIZE);
		return(-1);
   }

   fd=-1;
   elf=(Elf *)-1;
   freethis=(char *)-1;

   status=elfop(1, CMD_STAMP,filename,string,errcode);

   if (status == -1) {
      /* we don't have to call elf_end/free/close here for failure since we 
       * called elfop with doopenbeginend of 1  -- elfop() did this for us
       */
      error(errcode,"stamp: elfop failed");
      return(-1);
   }

   /* we don't have to call elf_end/free/close here for success since we 
    * called elfop with doopenbeginend of 1  -- elfop() did this for us
    */
   StartList(2,"STATUS",10);
   AddToList(1,"success");
   EndList();
   
   return(0);

}


/* try and find the special symbol in NDVERSIONSYMBOL if defined inside
 * the .o file.  in order to get here the global Elf and fd have already
 * been set properly.
 * Sigh, we can't go the dlopen/dlsym/dlclose route because they only
 * work on shared objects where ehdr->e_type == ET_DYN
 * for our Driver.o files e_type is ET_REL so we must go the long
 * route.
 *
 * returns -1 if error
 * 0 if success but symbol not found.   nothing copied to buffer
 * 1 if success and symbol was found in .o file.  up to buffersize
 * bytes of the contents of that symbol were copied into buffer 
 */
int
getsymbol(char *buffer, u_int buffersize, u_int errcode)
{
   Elf_Data *data,*variabledata;
   Elf32_Ehdr *ehdr;
   Elf_Scn *scn,*symtabscn,*variablescn;
   Elf32_Shdr *shdr,*strtabshdr;
   char *name,*variablename;
   size_t strtabscnindex;
   Elf32_Sym *sym;
   int loop,numsyms;

   /* elf should already be defined and file open to get here ! */
   if (elf == (Elf *)-1) {
      error(errcode,"getsymbol: elf is -1!");
      return(-1);
   }

   ehdr=elf32_getehdr(elf); /* already checked for failure earlier */
   scn=0;
   symtabscn=0;
   strtabscnindex=0;
   /* first, go through all sections looking for the symbol table */
   /* ASSUMES:  only one .symtab and only one .strtab section */
   while ((scn=elf_nextscn(elf, scn)) != 0) {
      shdr=elf32_getshdr(scn);
      if (shdr == 0) {
         err=elf_errno();
         error(errcode,"getsymbol: elf32_getshdr returned failure - %d=%s",
                 err,elf_errmsg(err));
         goto getsymbolfail;
      }

      name=elf_strptr(elf, ehdr->e_shstrndx, (size_t) shdr->sh_name);

      if (strcmp(name,".symtab") == 0) {
         symtabscn=scn;
      } else
      if (strcmp(name,".strtab") == 0) {
         strtabscnindex=elf_ndxscn(scn);  /* will succeed since scn != 0 */
      }
   }  /* processed all sections */
   if ((symtabscn == 0) || (strtabscnindex == 0)) {
      notice("couldn't find .symtab or .strtab section in .o file");
      return(0);
   }
   /* next, go through .symtab looking for our variable */
   /* ASSUMES:  elf_getdata will return all of .symtab in one Elf_Data struct */
   data=elf_getdata(symtabscn,0);
   if (data == 0) {
      error(errcode,"getsymbol: elf_getdata for .symtab returned null");
      goto getsymbolfail;
   } 
   numsyms=data->d_size / sizeof(Elf32_Sym);

   if (numsyms == 0) {
      notice("getsymbol: .o file symbol table size has 0 entries");
      return(0);
   }

   for(loop=0;loop<numsyms;loop++) {
      sym=((Elf32_Sym *)data->d_buf)+loop;
      variablename=elf_strptr(elf, strtabscnindex, (size_t) sym->st_name);
      if (strcmp(variablename,NDVERSIONSYMBOL) == 0) {
         if ((sym->st_size > 0) &&   /* must have a size if we define it */
             (ELF32_ST_BIND(sym->st_info) == STB_LOCAL) &&  /* i.e. static */
             (ELF32_ST_TYPE(sym->st_info) == STT_OBJECT) && /* no functions! */
             (sym->st_shndx != SHN_UNDEF)) {
            variablescn=elf_getscn(elf, sym->st_shndx);
            /* ASSUMES:  elf_getdata will return all of section in one struct*/
            variabledata=elf_getdata(variablescn, 0);
            strncpy(buffer,((char *)variabledata->d_buf)+(sym->st_value),
                    buffersize);
            return(1);
         } else {
            /* they have some symbol called ndversion but the symbol is:
             * - not defined (externed and perhaps defined in space.c - not 
             *   acceptable) OR
             * - not static (will cause multiply defined relink errors) OR
             * - not a variable (maybe a function named ndversion?) OR
             * - in an undefined section
             * In any of these cases, we flag the driver as bad.  This will
             * prevent the driver from being installed and encourage
             * vendors to use ndversion properly.  Note they can still
             * override this check by having a bogus definition of ndversion
             * but the driver has a .ndnote section which takes precedence
             * over the ndversion[] symbol.  This can still cause relink
             * errors since this routine will never be run since the .o had
             * a .ndnote section.
             */
            error(errcode,
                  "getsymbol: %s not defined as static char %s[]=\"1.2.3.4\";",
                  NDVERSIONSYMBOL,NDVERSIONSYMBOL);
            goto getsymbolfail;
         }
      }
   }
   return(0);  /* we searched through all variables but didn't find it */
   /* NOTREACHED */
getsymbolfail:
   return(-1);
}


/* return -1 if error 
 * 0 if success but no version string found--don't print out contents of buffer
 * 1 if success and some version string found -- contents of buffer are valid
 * we only pay attention to buffer and buffersize if dolist is 0
 * on return, if success, wherefound is set to STAMP_FOUND_IN_NDNOTE or 
 * STAMP_FOUND_IN_SYMBOL or STAMP_NOT_FOUND
 */
int
getstamp(int dolist, char *filename, char *buffer, u_int buffersize, 
         u_int errcode, int *wherefound)
{
   Elf32_Ehdr h;
   int cmd;
   char tmp[NDNOTEMAXSTRINGSIZE];  /* must be this big since we use 
                                    * strncpy in elfop()
                                    */
   int status;

   if (*filename != '/') {
      error(errcode,"getstamp: must give absolute path to file %s",filename);
      return(-1);
   }

   if ((dolist == 0) && (buffersize > NDNOTEMAXSTRINGSIZE)) {
      notice("lowering buffersize from %d to %d",
             buffersize,NDNOTEMAXSTRINGSIZE);
      buffersize=NDNOTEMAXSTRINGSIZE;
   }

   fd=-1;
   elf=(Elf *)-1;
   freethis=(char *)-1;
   cmd=CMD_GETSTAMP;

   /* since we call elfop w/ doopenbeginend of zero we must do the work here */

   /**********************BEGIN OF CODE SNIPPIT BASED ON ELFOP() **************/
      (void) elf_version(EV_NONE);
      if (elf_version(EV_CURRENT) == EV_NONE) {
         /* library out of date -- "shouldn't happen" */
         err=elf_errno();
         error(errcode,"getstamp: libelf is out of date - %d=%s",
                 err,elf_errmsg(err));
         goto getstampfail;
      }
      fd=open(filename, (cmd == CMD_GETSTAMP) ? O_RDONLY : O_RDWR);
      if (fd == -1) {
         error(errcode,"getstamp: problem opening file %s errno=%d",
                       filename,errno);
         goto getstampfail;
      }

      if (read(fd,&h,sizeof(h)) != sizeof(h)) {
         error(errcode,"getstamp: problem reading elf header");
         goto getstampfail;
      }

      if ((h.e_ident[EI_MAG0] != ELFMAG0) ||
          (h.e_ident[EI_MAG1] != ELFMAG1) ||
          (h.e_ident[EI_MAG2] != ELFMAG2) ||
          (h.e_ident[EI_MAG3] != ELFMAG3) ||
          (h.e_ident[EI_CLASS] != ELFCLASS32) || /* must have 32 bit routines */
          (h.e_type != ET_REL) || /* only .o files not executables */
          (h.e_shentsize * h.e_shnum == 0) ||
          (h.e_shstrndx == SHN_UNDEF)) {
         error(errcode,"getstamp: %s: invalid ELF file or invalid ELF "
                       "that couldn't contain an ELF stamp",filename);
         goto getstampfail;
      }

      lseek(fd,0,0);
      /* we only have one "activation" of our elf routine so we
       * know that a call to elf_end will terminate the elf descriptor
       * since the activation count will be zero
       * otherwise we need to call elf_end multiple times -- once for each
       * open descriptor -- and check elf_end return value.
       */
      elf=elf_begin(fd,(cmd == CMD_GETSTAMP) ? ELF_C_READ:ELF_C_RDWR,(Elf *)0);

      /* we could do an elf_cntl(elf, ELF_C_FDREAD) here if cmd is CMD_STAMP
       * which should allow a later creat() before elf_update but we don't
       */

      if (elf == (Elf *)0) {
         err=elf_errno();
         error(errcode,"getstamp: elf_begin returned failure - %d=%s",
                 err,elf_errmsg(err));
         goto getstampfail;
      }
   /**********************END OF CODE SNIPPIT BASED ON ELFOP() **************/
   status=elfop(0, CMD_GETSTAMP,filename,tmp,errcode);

   /* NOTE:we must call elf_end/free/close for both success and failure cases */

   if (status == 1) {
      /* we found our string in the .ndata section of the .o file
       * so copy our returned string into the user's buffer 
       */
      if (dolist) {
         StartList(2,"VERSIONSTRING",78);
         AddToList(1,tmp);
         EndList();
      } else {  /* not interactive mode so do the strncpy */
         if (strlen(tmp) > buffersize) { /* user supplied too small of buffer */
            notice("getstamp: chopping returned version string '%s' "
                   "at %d chars", tmp,buffersize);
         }
         strncpy(buffer,tmp,buffersize);
      }

      /* now call elf_end/free/close since elfop didn't do it for us */

      if (elf_end(elf) != 0) {
         err=elf_errno();
         error(errcode,
                 "getstamp: elf_end in donesuccess returned failure - %d=%s",
                 err,elf_errmsg(err));
         goto getstampfail;
      }
      /* since elf_end doesn't free up Elf_Data d_buf section we must do it
       * ourself if we created a new .ndnote section
       */
      if (freethis != (char *)-1) free(freethis);
      close(fd);
      *wherefound=STAMP_FOUND_IN_NDNOTE;
      return(1);   /* success -- a version string found */
   } else
   if (status == 0) {
      /* no .ndnote section found in .o file so 
       * now try to find the special ndversion[] symbol in NDVERSIONSYMBOL
       */
      int retvalue;

      *wherefound=STAMP_NOT_FOUND;

      if ((retvalue=getsymbol(tmp, NDNOTEMAXSTRINGSIZE, errcode)) == -1) {
         error(errcode,"getstamp: getsymbol returned -1");
         goto getstampfail;
      }

      if (retvalue == 1) {
         *wherefound=STAMP_FOUND_IN_SYMBOL;
         if (dolist == 1) {
            StartList(2,"VERSIONSTRING",78);
            AddToList(1,tmp);
            EndList();
         } else {
            if (strlen(tmp) > buffersize) { /* too small of buffer */
               notice("getstamp: chopping returned symbol version string '%s' "
                         "at %d chars",tmp,buffersize);
            }
            strncpy(buffer,tmp,buffersize);
         }
      }

      /* now call elf_end/free/close since elfop didn't do it for us */
      if (elf_end(elf) != 0) {
         err=elf_errno();
         error(errcode,
                 "getstamp: elf_end in donesuccess returned failure - %d=%s",
                 err,elf_errmsg(err));
         goto getstampfail;
      }
      /* since elf_end doesn't free up Elf_Data d_buf section we must do it
       * ourself if we created a new .ndnote section
       */
      if (freethis != (char *)-1) free(freethis);
      close(fd);
      return(retvalue);  /* return either 0 or 1 */
   } else {

      error(errcode,"getstamp: elfop returned %d",status);
      /* now call elf_end/free/close since elfop didn't do it for us */

getstampfail:
      *wherefound=STAMP_NOT_FOUND;
      if ((elf != (Elf *)-1) && (elf_end(elf) != 0)) {
         err=elf_errno();
         error(errcode,
                 "getstamp: elf_end in getstampfail returned failure - %d=%s",
                 err,elf_errmsg(err));
         /* we drop through to possibly calling free()/close() and we
          * still return(-1)
          */
      }
      /* since elf_end doesn't free up Elf_Data d_buf section we must do it
       * ourself if we created a new .ndnote section
       */
      if (freethis != (char *)-1) free(freethis);
      /* close our fd if we called open */
      if (fd != -1) close(fd);
      return(-1);
   }
   /* NOTREACHED */
}

/* sets the .driverversion string in bcfgfile[] 
 * we call strdup on the text so you will have to free() it up later on
 * returns -1 for error
 * returns 0 for success
 */
int
GetDriverVersionInfo(int bcfgindex,char *filename)
{
   char tmp[NDNOTEMAXSTRINGSIZE];
   char string[NDNOTEMAXSTRINGSIZE];
   char *type;
   int status,wherefound;

   status=getstamp(0, filename, tmp, NDNOTEMAXSTRINGSIZE,BADELF, &wherefound);
   if (status == -1) {
      bcfgfile[bcfgindex].driverversion=NULL;
      return(-1);
   }
   type=StringListPrint(bcfgindex,N_TYPE);
   if (type == NULL) {
      /* shouldn't happen, and BADELF isn't correct, but easy to ID it */
      error(BADELF, "GetDriverVersionInfo: TYPE not defined in bcfg");
      return(-1);
   }
   if (status == 0) {
      /* no version found in .o in .nddata or ndversion[] 
       */
#ifdef MAGIC 
      if (getenv(MAGIC) == NULL)
#else
      if (0)
#endif
      {
         bcfgfile[bcfgindex].driverversion=strdup(""); /* since we free */
      } else {
         snprintf(string,NDNOTEMAXSTRINGSIZE,"(%s)",type);
         bcfgfile[bcfgindex].driverversion=strdup(string); /* since on stack */
      }
      free(type);
      return(0);
   }
   /* there is a version string in .o file in either ndversion[] or .nddata
    * section to get here 
    */
#ifdef MAGIC 
   if (getenv(MAGIC) == NULL)
#else
   if (0)
#endif
   {
      snprintf(string,NDNOTEMAXSTRINGSIZE,"(%s%s)",
                                         /* used to say "version " here */
                                         /*              vvv  */
                   wherefound == STAMP_FOUND_IN_SYMBOL ? "" : "", tmp);
   } else {
      snprintf(string,NDNOTEMAXSTRINGSIZE,"(%s-%s%s)",
             type, wherefound == STAMP_FOUND_IN_SYMBOL ? "" : "", tmp);
                                         /*              ^^^  */
                                         /* used to say "version " here */
   }
   bcfgfile[bcfgindex].driverversion=strdup(string); /* string on stack */
   free(type);
   return(0);
}
