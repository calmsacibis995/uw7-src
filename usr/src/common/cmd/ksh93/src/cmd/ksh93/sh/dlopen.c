#ident	"@(#)ksh93:src/cmd/ksh93/sh/dlopen.c	1.1"
#pragma prototyped
/*
 * provide dlopen/dlsym/dlerror interface
 *
 *   David Korn
 *   AT&T Bell Laboratories
 *   Room 2B-102
 *   Murray Hill, N. J. 07974
 *   Tel. x7975
 *   research!dgk
 *
 */

#include	<ast.h>
#include	<error.h>
#include	<nval.h>
#include	"FEATURE/dynamic"

#if	defined(_hdr_dlfcn) && defined(_lib_dlopen)
    int __STUB_dlopen;
#else
#   ifdef _hdr_dl /* HP-UX style interface */
#	include	<dl.h>
#	ifndef BIND_FIRST
#	    define BIND_FIRST	0x4
#	endif /* BIND_FIRST */
#	ifndef BIND_NOSTART
#	    define BIND_NOSTART	0x10
#	endif /* BIND_NOSTART */
	void *dlopen(const char *path,int mode)
	{
		if(mode)
			mode = (BIND_IMMEDIATE|BIND_FIRST|BIND_NOSTART);
		return((void*)shl_load(path,mode,0L));
	}
	void *dlsym(void *handle, const char *name)
	{
		long addr;
		shl_findsym(&handle,name,TYPE_PROCEDURE,&addr);
		return((void*)addr);
	}
	char *dlerror(void)
	{
		return(fmterror(errno));
	}
#   else  /* try rs6000 style */
#	if  defined(_sys_ldr) && defined(_lib_loadbind)
#	    include <sys/ldr.h>
#	    include <xcoff.h>
	    /* xcoff module header */
	    struct hdr
	    {
		struct filehdr f;
		struct aouthdr a;
		struct scnhdr s[1];
	    };
	    static struct ld_info *ld_info;
	    static unsigned int ld_info_size=1024;
	    static void *last_module;
	    void *dlopen(const char *path,int mode)
	    {
		return((void*)load((char*)path,mode,getenv("LIBPATH")));
	    }
	    static int getquery(void)
	    {
		if(!ld_info)
			ld_info=malloc(ld_info_size);
		while(1)
		{
			if(!ld_info)
				return(1);
			if(loadquery(L_GETINFO,ld_info,ld_info_size)==0)
				return(0);
			if(errno != ENOMEM)
				return(1);
			ld_info = realloc(ld_info,ld_info_size*=2);
		}
 	    }
	    /* find the loaded module whose data area contains the
	     * address passed in.  Remember that procedure pointers
	     * are implemented as pointers to descriptors in the
	     * data area of the module defining the procedure
	     */
	    static struct ld_info *getinfo(void *module)
	    {
	    	struct ld_info *info=ld_info;
		register int n=1;
		if (!ld_info || module != last_module)
		{
			last_module = module;
			if(getquery())
				return((struct ld_info *)0);
			info=ld_info;
		}
		while(n)
		{
			if((char*)(info->ldinfo_dataorg) <= (char*)module &&
				(char*)module <= ((char*)(info->ldinfo_dataorg)
				+ (unsigned)(info->ldinfo_datasize)))
				return(info);
			if(n=info->ldinfo_next)
				info = (void*)((char*)info +n);
		}
		return((struct ld_info *)0);
	    }
	    static char *getloc(struct hdr *hdr,char * data,char *name)
	    {
		struct ldhdr *ldhdr;
		struct ldsym *ldsym;
		ulong datareloc,textreloc;
		int i;
		/* data is relocated by the difference between
		 * its virtual origin and where it was
		 * actually placed
		 */
		/*N.B. o_sndata etc. are one based */
		datareloc = (ulong)data - hdr->s[hdr->a.o_sndata-1].s_vaddr;
		/*hdr is address of header, not text, so add text s_scnptr */
		textreloc = (ulong)hdr + hdr->s[hdr->a.o_sntext-1].s_scnptr
			- hdr->s[hdr->a.o_sntext-1].s_vaddr;
		ldhdr = (void*)((char*)hdr+ hdr->s[hdr->a.o_snloader-1].s_scnptr);
		ldsym = (void*) (ldhdr+1);
		/* search the exports symbols */
		for(i=0; i < ldhdr->l_nsyms;ldsym++,i++)
		{
			char *symname,symbuf[9];
			char *loc;
			/* the symbol name representation is a nuisance since
			 * 8 character names appear in l_name but may
			 * not be null terminated.  This code works around
			 * that by brute force
			 */
			if (ldsym->l_zeroes)
			{
				symname = symbuf;
				memcpy(symbuf,ldsym->l_name,8);
				symbuf[8] = 0;
			}
			else
				symname = (void*)(ldsym->l_offset + (ulong)ldhdr + ldhdr->l_stoff);
			if(strcmp(symname,name))
				continue;
			loc = (char*)ldsym->l_value;
			if  ((ldsym->l_scnum==hdr->a.o_sndata) ||
				(ldsym->l_scnum==hdr->a.o_snbss))
				loc += datareloc;
			else if  (ldsym->l_scnum==hdr->a.o_sntext)
				loc += textreloc;
			return(loc);
		}
		return((char*)0);
	    }
	    void *dlsym(void *handle, const char *name)
	    {
		struct ld_info *info;
		if(info=getinfo(handle))
			return(getloc(info->ldinfo_textorg,info->ldinfo_dataorg,(char*)name));
		return((void*)0);
	    }
	    char *dlerror(void)
	    {
		return(fmterror(errno));
	    }
#	else
		void *dlopen(const char *path,int mode)
		{
			return((void*)0);
		}
		void *dlsym(void *handle, const char *name)
		{
			return((void*)0);
		}
		char *dlerror(void)
		{
			return("dynamic linking not supported");
		}
#	endif /* _sys_ldr */
#   endif /* _hdr_dl */
#endif /* _hdr_dlfcn */
