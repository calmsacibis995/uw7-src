#ident	"@(#)rtld:i386/rtsetup.c	1.50"

/*
 * 386 specific setup routine - relocate rtld's symbols, setup its
 * environment, map in loadable sections of a.out (if kernel didn't do
 * it).
 *
 * takes address of rtld's dynamic structure,
 * and a pointer to the argument list starting at argc.  This list is
 * organized as follows: argc, argv[0], argv[1], ..., 0, envp[0], envp[1],
 * ..., 0, auxv[0], auxv[1], ..., 0 if errors occur, send process SIGKILL -
 * otherwise return a.out's entry point to the bootstrap routine 
 */

#include "machdep.h"
#include "rtld.h"
#include "externs.h"
#include "paths.h"
#include <sys/types.h>
#include <elf.h>
#include <dlfcn.h>
#include <sys/elf_386.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/sysconfig.h>
#include <elfid.h>

static rt_map	ld_map;  /* link map for rtld */
static rt_map	aout_map;  /* link map for a.out */
static void 	setup_symbols	ARGS((int fphw, char **envp, char **argv));

/* set _fp_hw according to what floating-point hardware is available. */
extern int	_fp_hw; 

#if !defined(GEMINI_ON_OSR5) && !defined(GEMINI_ON_UW2)
#include <sys/fcntl.h>
#include <sys/sysi86.h>

/* struct used to pass info to rtld boot code */
struct {
	ulong_t r_alt_entry;
	ulong_t r_old_base;
	ulong_t r_old_size;
	ulong_t r_alt_base;
} _rt_boot_info;

/* open and map in alternate dynamic linker; reset auxv
 * entry for dynamic linker base address; invoke 
 * sysi86(SI86GETFEATURES...) to inform kernel that we
 * are running an OSR5 binary; return dynamic linker entry point
 */

static rt_map *
map_alternate_rtld(base_auxv_entry, rtld_memsize)
auxv_t	*base_auxv_entry;
ulong_t	rtld_memsize;
{
	rt_map		*lm;
	int		fd;
	object_id	obj_id;
	ulong_t		alt_entry;
	static const char	alt_name[] = ALTERNATE_LIBCSO_NAME;

	DPRINTF(LIST,(2, "map_alternate_rtld(0x%x, %d)\n", base_auxv_entry, rtld_memsize));

	if (rtld_memsize == 0)
	{
		_rt_lasterr("%s: %s: cannot determine size of dynamic linker\n",
			(char*)_rt_name, _rt_proc_name);
		return 0;
	}

	if ((fd = _rtopen(alt_name, O_RDONLY)) == -1) 
	{
		_rt_lasterr("%s: %s: cannot open %s",
			(char*)_rt_name, _rt_proc_name, alt_name);
		return 0;
	}
	obj_id.n_name = (char *)alt_name;
	obj_id.n_fd = fd;
	obj_id.n_lm = 0;
	if ((lm = _rt_map_so(&obj_id, &alt_entry, 0, 1)) == 0)
		return 0;
	_rt_boot_info.r_alt_entry = alt_entry;
	_rt_boot_info.r_old_base = (ulong_t)base_auxv_entry->a_un.a_ptr;
	_rt_boot_info.r_old_size = rtld_memsize;
	_rt_boot_info.r_alt_base = ADDR(lm);
	base_auxv_entry->a_un.a_ptr = (char *)ADDR(lm);

	/* notify debuggers of change */
	_r_debug.r_state = RT_ALTERNATE_RTLD; 
	_r_debug_state();

	/* should return EINVAL, but we don't care */
	(void)_rtsysi86(SI86GETFEATURES, 0, 0);

	return(lm);
}
#endif	/* !OSR5 and !UW2 */

static Elf32_Ehdr *
check_for_ehdr(ulong_t addr)
{
	Elf32_Ehdr	*ehdr = (Elf32_Ehdr *)addr;

        /* check ELF identifier and that program headers
	 * follow ehdr
	 */
        if (ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
		ehdr->e_ident[EI_MAG1] == ELFMAG1 &&
		ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
		ehdr->e_ident[EI_MAG3] == ELFMAG3 &&
		(ehdr->e_ehsize == ehdr->e_phoff))
		return ehdr;
	return 0;
}

ulong_t   
_rt_setup(ld_dyn, args_p)
Elf32_Dyn	*ld_dyn;
char		**args_p;
{
	Elf32_Dyn	*dyn[DT_MAXPOSTAGS], *ld;
	Elf32_Dyn	interface[3];
	Elf32_Dyn	lreturn[DT_MAXNEGTAGS+1];
	CONST char	*envdirs = 0;
	int		i; 
	int		bmode = RTLD_LAZY;
	char		**envp;
#ifdef GEMINI_ON_OSR5
	struct osr5_stat32	sbuf;
#else
	struct stat	sbuf;
#endif
	int		argc;
	char		**save_argv;
	int		fd = -1;
	auxv_t		*auxv;
	char		*pname;
	int		relent;
	int		phsize;
	int		phnum;
	int		flags;
	int		pagsz = 0;
	Elf32_Phdr	*phdr;
	Elf32_Phdr	*rtld_text_phdr = 0;
	Elf32_Ehdr	*ehdr;
	Elf32_Sym	*sym;
	rt_map		*def_lm;
	/* interpreter's device and inode number */
	dev_t		intp_device = (dev_t)-1;  
	ino_t		intp_inode = (ino_t)-1;	
	int		use_ld_lib_path = -1; 
				/* use LD_LIBRARY_PATH if non-zero */
	ulong_t		entry;
	ulong_t		rtld_memsize = 0;
	register ulong_t off, reladdr, rend;
	char		*runpath = 0;
	char		*rtld_name;
	ulong_t		ld_base;
#ifndef GEMINI_ON_OSR5
	auxv_t		*base_auxv_entry = 0;
#endif

	object_id	obj_id;
	int		fphw = -1;  	/* type of Floating Point hardware -
				 values in sys/fp.h 
				-1 means that the startup code must 
				call sysi86() */

	/* traverse argument list and get values of interest */
	/* we save some values in locals that we will later copy
	 * to static/globals because we cannot access static
	 * symbols until we process interpreter's relocations
	 */
	argc = (int)*args_p++;
	save_argv = args_p;	/* points to &argv[0] */
	pname = *args_p;	/* set local to process name for error 
				 * messages */
	args_p += argc;		/* skip argv[0] ... argv[n] */
	args_p++;		/* and 0 at end of list */
	envp = args_p;		/* get the environment pointer */
	while (*args_p++)
		;		/* skip envp[0] ... envp[n] */
				/* and 0 at end of list */

	/* search the aux. vector for the information passed by exec */
	auxv = (auxv_t *)args_p;
	for (; auxv->a_type != AT_NULL; auxv++) 
	{
		switch(auxv->a_type) 
		{
		case AT_EXECFD:
			/* this is the old exec that passes a file descriptor */
			fd = auxv->a_un.a_val;
			break;
		case AT_FLAGS:
			/* required */
			/* processor flags (MAU available, etc) */
			flags = auxv->a_un.a_val;
			break;
		case AT_PAGESZ:
			/* system page size */
			pagsz = auxv->a_un.a_val;
			break;
		case AT_PHDR:
			/* required, if no AT_EXECFD */
			/* address of the segment table for a.out */
			phdr = (Elf32_Phdr *)auxv->a_un.a_ptr;
			break;
		case AT_PHENT:
			/* required, if no AT_EXECFD */
			/* size of each segment header for a.out */
			phsize = auxv->a_un.a_val;
			break;
		case AT_PHNUM:
			/* required, if no AT_EXECFD */
			/* number of program headers for a.out */
			phnum = auxv->a_un.a_val;
			break;
		case AT_BASE:
			/* required */
			/* rtld base address */
			ld_base = auxv->a_un.a_val;
#if !defined(GEMINI_ON_OSR5) && !defined(GEMINI_ON_UW2)
			base_auxv_entry = auxv;
#endif
			break;
		case AT_ENTRY:
			/* required */
			/* entry point for the a.out */
			entry = auxv->a_un.a_val;
			break;
                case AT_LIBPATH:
                        /* non-zero if process can use LD_LIBRARY_PATH
			 * for searching */
                        use_ld_lib_path = (auxv->a_un.a_val != 0);
                        break;
                case AT_FPHW:
                        /* floating-point hardware type */
                        fphw = auxv->a_un.a_val;
                        break;
		case AT_INTP_DEVICE:
                        /* interpreter's device number */
                        intp_device = (dev_t)auxv->a_un.a_val;
                        break;
                case AT_INTP_INODE:
                        /* interpreter's inode number */
                        intp_inode = (ino_t)auxv->a_un.a_val;
                        break;
		}
	}

	/* store pointers to each item in rtld's dynamic structure 
	 * dyn[tag] points to the dynamic section entry with d_tag
	 * == tag
	 */
	for (i = 0; i < DT_MAXPOSTAGS; i++)
		dyn[i] = (Elf32_Dyn *)0;

	ld_dyn = NEWPTR(Elf32_Dyn, ld_dyn, ld_base);
	for (ld = ld_dyn; ld->d_tag != DT_NULL; ld++)
	{
		if (ld->d_tag >= DT_MAXPOSTAGS)
			continue;
		dyn[ld->d_tag] = ld;
	}
	
	/* relocate all symbols in rtld */
	reladdr = ((ulong_t)(dyn[DT_REL]->d_un.d_ptr) + ld_base);

	rend = reladdr + dyn[DT_RELSZ]->d_un.d_val;
	relent = dyn[DT_RELENT]->d_un.d_val;

	for (; reladdr < rend; reladdr += relent)
	{
	/*
	 * insert value calculated at reference point.
	 * We only deal with R_386_RELATIVE relocations here.
	 * These should be sufficient for rtld's own purposes, because
	 * of the way we were linked (ld -r -Bsymbolic).  
	 * Other relocations are handled if rtld is loaded as a library.
	 */
		if (ELF32_R_TYPE(((Elf32_Rel *)reladdr)->r_info) 
			!= R_386_RELATIVE)
			continue;

		off = (ulong_t)((Elf32_Rel *)reladdr)->r_offset
			+ ld_base;

		/* THIS CODE WILL BREAK ON MACHINES THAT REQUIRE 
		 * DATA TO BE ALIGNED
		 */
		*((ulong_t *) off) += ld_base;	
	}

	/* set global to process name for error messages */
	_rt_proc_name = pname;

	_rt_flags = flags;

	/* look for environment strings */
	envdirs = _rt_readenv((CONST char **)envp, &bmode );

	
#ifndef GEMINI_ON_OSR5
	if (!pagsz)
		pagsz = _rtsysconfig(_CONFIG_PAGESIZE);
#endif
	_rt_syspagsz = pagsz;

	DPRINTF(LIST,(2, "rtld: _rt_setup: base:0x%x, dyn: 0x%x \n",ld_base,(ulong_t)ld_dyn));

	/* see if we can access rtld's program headers;
	 * if so, search for space at end of bss so we
	 * can use it to initialize space allocator
	 */
	if ((ehdr = check_for_ehdr(ld_base)) != 0)
	{
		int		j;
    		Elf32_Phdr	*rphdr = NEXT_PHDR(ld_base, ehdr->e_phoff);
		Elf32_Phdr	*last_rphdr, *first_rphdr = 0;
		for(j = ehdr->e_phnum; j > 0; j--,
			rphdr = NEXT_PHDR(rphdr, ehdr->e_phentsize))
		{
			int	pflags;
			if (rphdr->p_type != PT_LOAD)
				continue;
			if (!first_rphdr)
				first_rphdr = rphdr;
			pflags = (rphdr->p_flags & (PF_R|PF_W|PF_X));
			if (!rtld_text_phdr && (pflags == (PF_R|PF_X)))
				rtld_text_phdr = rphdr;
			last_rphdr = rphdr;
			if ((rphdr->p_memsz != rphdr->p_filesz) &&
				(pflags & PF_W))
			{
				ulong_t	addr = rphdr->p_vaddr +
					ld_base;
				addr += rphdr->p_memsz;
				_rtmkspace((char *)addr, 
					PROUND(addr) - addr);
			}
		}
		rtld_memsize = (last_rphdr->p_vaddr + 
			last_rphdr->p_memsz) - 
			STRUNC(first_rphdr->p_vaddr);
	}
	 
	/* map in the file, if exec has not already done so.
	 * If it has, just create a new link map structure for the a.out
	 */
	if (fd != -1) 
	{
		obj_id.n_name = 0;
		obj_id.n_fd = fd;
		obj_id.n_lm = 0;
		if ((_rt_map_head = _rt_map_so(&obj_id, &entry, 
			&runpath, 0)) == 0)
		{
			_rt_fatal_error();
		}
	}
	else 
	{
		Elf32_Phdr	*pptr;
		Elf32_Phdr	*firstptr = 0;
		Elf32_Phdr	*lastptr;
		Elf32_Phdr	*text_phdr = 0;
		Elf32_Dyn	*mld;
		ulong_t		base_addr;
#if !defined(GEMINI_ON_OSR5) && !defined(GEMINI_ON_UW2)
		Elf32_Phdr	*notep = 0;
#endif

		/* extract the needed information from the segment 
		 * headers 
		 */
		for (i = 0, pptr = phdr; i < phnum; i++,
			pptr = NEXT_PHDR(pptr, phsize))
		{
			if (pptr->p_type == PT_LOAD) 
			{
				if (!firstptr)
					firstptr = pptr;
				if (!text_phdr &&
					(((pptr->p_flags & (PF_R|PF_W|PF_X))
						== (PF_R|PF_X))))
					text_phdr = pptr;
				lastptr = pptr;
			}
			else if (pptr->p_type == PT_DYNAMIC)
				mld = (Elf32_Dyn *)(pptr->p_vaddr);
#if !defined(GEMINI_ON_OSR5) && !defined(GEMINI_ON_UW2)
			else if ((pptr->p_type == PT_NOTE) &&
				(pptr->p_filesz == NT_ELFID_SZ)) 
			{
				/* special note section denoting
				 * an OpenServer ELF binary
				 */
				notep = pptr;
			}
#endif
		}
#if !defined(GEMINI_ON_OSR5) && !defined(GEMINI_ON_UW2)
		ehdr = check_for_ehdr((ulong_t)phdr - 
			sizeof(Elf32_Ehdr));
		if (!(ehdr && (ehdr->e_flags == UDK_FIX_FLAG)) &&
			(notep ||
			(ehdr && (ehdr->e_flags == OSR5_FIX_FLAG))))
		{
			/* An OpenServer 5 binary - 
			 * map in alternate dynamic linker
			 * and invoke its bootstrapper
			 */
			if (map_alternate_rtld(base_auxv_entry,
				rtld_memsize) == 0)
			{
				_rt_fatal_error();
			}
			return 0;
		}
#endif	/* !OSR5 and !UW2 */
		obj_id.n_name = 0;
		obj_id.n_lm = &aout_map;
		base_addr = STRUNC(firstptr->p_vaddr);
		if ((_rt_map_head = _rt_new_lm(&obj_id, mld, base_addr,
			(lastptr->p_vaddr + lastptr->p_memsz) - base_addr,
			phdr, phnum, phsize, text_phdr,
			&runpath)) == 0)
		{
			_rt_fatal_error();
		}
#if !defined(GEMINI_ON_OSR5) && !defined(GEMINI_ON_UW2)
		/* Make one more check for OSR5 binaries - see if
		 * the a.out calls _init_features_vector.
		 */
		{
			if ((sym = _rt_lookup("_init_features_vector",
				_rt_map_head, 0, 0, &def_lm,
				LOOKUP_SPEC)) != 0)
			{
				/* An OpenServer 5 binary - 
				 * map in alternate dynamic linker
				 * and invoke its bootstrapper
				 */
				if (map_alternate_rtld(base_auxv_entry,
					rtld_memsize) == 0)
				{
					_rt_fatal_error();
				}
				return 0;
			}
		}
#endif
		if (TEST_FLAG(_rt_map_head, RT_TEXTREL) &&
			(_rt_set_protect(_rt_map_head, PROT_WRITE) == 0)) 
		{
			_rt_fatal_error();
		}
	}

	/* _rt_map_head and _rt_map_tail point to head and tail of
	 * rt_map list
	 */
	_rt_map_tail = _rt_map_head;

	/* initialize debugger information structure 
	 * some parts of this structure were initialized
	 * statically
	 */
	_r_debug.r_map = (struct link_map *)_rt_map_head;
	_r_debug.r_ldbase = ld_base;

	/* create a rt_map structure for rtld */
	/* we copy the name here rather than just setting a pointer
	 * to it so that it will appear in the data segment and
	 * thus in any core file
	 */
	rtld_name = (char *)dyn[DT_STRTAB]->d_un.d_ptr + ld_base +
		dyn[DT_SONAME]->d_un.d_val;
#ifdef GEMINI_ON_OSR5
	if ((obj_id.n_name = _rtmalloc(_rtstrlen(rtld_name) + 
		ALT_PREFIX_LEN + 1)) != 0)
	{
		_rtstrcpy(obj_id.n_name, ALT_PREFIX);
		_rtstrcpy(obj_id.n_name + ALT_PREFIX_LEN, rtld_name);
	}
	else
	{
		_rt_fatal_error();
	}
#else
	if ((obj_id.n_name = _rtmalloc(_rtstrlen(rtld_name) + 1)) 
		!= 0)
		_rtstrcpy(obj_id.n_name, rtld_name);
	else
		obj_id.n_name = rtld_name;
#endif

	obj_id.n_lm = &ld_map;
	/* get device and inode for rtld for later comparisons */
	if (intp_device == (dev_t)-1 || intp_inode == (ino_t)-1) 
	{
		if (_rtstat(obj_id.n_name, &sbuf) == -1) 
		{
			_rt_lasterr("cannot stat %s\n", obj_id.n_name);
			_rt_fatal_error();
		}
		obj_id.n_dev = sbuf.st_dev;
		obj_id.n_ino = sbuf.st_ino;
	}
	else 
	{  
		/* use what was passed in by auxv */
		obj_id.n_dev = intp_device;
		obj_id.n_ino = intp_inode;
	}
	
	_rtld_map = _rt_new_lm(&obj_id, ld_dyn, ld_base, 
		rtld_memsize, 0, 0, 0, rtld_text_phdr, 0);

	SET_FLAG(_rtld_map, RT_NODELETE);

	DPRINTF(LIST,(2, "RTLD: name is %s\n",NAME(_rtld_map)));
	DPRINTF(LIST,(2, "RTLD: device is %d\n",DEV(_rtld_map)));
	DPRINTF(LIST,(2, "RTLD: inode is %d\n",INO(_rtld_map)));
	DPRINTF(LIST,(2, "RTLD: libpath is %d\n",use_ld_lib_path));

	/* set up directory search path */
	if (!_rt_setpath(envdirs, runpath, use_ld_lib_path)) 
	{
		_rt_fatal_error();
	}

	/* setup for call to _rtld */
	interface[0].d_tag = DT_MODE;
	interface[0].d_un.d_val = bmode;
	interface[1].d_tag = DT_GROUP;
	interface[1].d_un.d_val = 0;
	interface[2].d_tag = DT_NULL;

	if (_rtld(interface, lreturn) != 0) 
	{
		_rt_fatal_error();
	}

	/*
	** Do some global setup before calling the init section of
	** each loaded object
	*/
	setup_symbols(fphw, envp, save_argv);

	if (!_rt_build_init_list(_rt_map_head, &_rt_fini_list))
	{
		_rt_fatal_error();
	}

	/* change list if needed to get right order for
	 * system libraries
	 */
	_rt_special_order_inits();

	_r_debug.r_state = RT_SYMBOLS_AVAILABLE; 
	_r_debug_state();

	/* check for existence of _rt_pre_init and invoke, if found */
	/* _rt_pre_init is defined and used by various crts 
	 */
	if ((sym = _rt_lookup("_rt_pre_init",  0, _rt_map_head, 0,
		&def_lm, LOOKUP_NORM)) != 0)
	{
		void	(*pre_init)();
		pre_init = (void (*)())sym->st_value;
		if (NAME(def_lm))
			pre_init = (void (*)())((ulong_t)pre_init +
				ADDR(def_lm));
		(*pre_init)();
	}
	
	_rt_call_init(&_rt_fini_list);

	if (_rt_event)
	{
		/* alert profilers */
		(*_rt_event)((ulong_t)&_r_debug);
	}
	_rt_close_devzero();
	return(entry);
}

/*
 * Called just before the startup _init() functions.
 * Can be overridden by the application.  The profiling
 * "crt"s define their own version to start profiling
 * in time to gather information on the _init() code.
 * We need a null definition here to force the symbol
 * to be exported from a.outs.
 */
void
_rt_pre_init()
{
	/* do nothing */
}

static void
setup_a_symbol(name, value)
CONST char	*name;
ulong_t		value;
{
	Elf32_Sym	*sym;
	rt_map		*lm;
	ulong_t		addr;

	if ((sym = _rt_lookup(name,  0, _rt_map_head, 0, &lm,
		LOOKUP_NORM)) != 0)
	{
		addr = sym->st_value;
		if (NAME(lm))
			addr += ADDR(lm);
		*(ulong_t *)addr = value;
	}
}

/* setup values of certain important symbols */
static void
setup_symbols(fphw, envp, argv)
int	fphw;
char	**envp;
char	**argv;
{
	Elf32_Sym	*sym;
	rt_map		*lm;
	rt_map		*save_lm;
	ulong_t		addr;

	/* _nd is used by sbrk  - set its value to
	* the a.out's notion of the program break
	*/
	if ((sym = _rt_lookup("_end", _rt_map_head, 0, 0, &lm,
		LOOKUP_NORM)) != 0)
		_nd = sym->st_value;

	/* _fp_hw is used by certain floating-point routines */
	setup_a_symbol("_fp_hw", (ulong_t)fphw);

	/* _environ is used by setenv, getenv */
	setup_a_symbol("_environ", (ulong_t)envp);

	/* ___Argv is used by monitor and can be used
	 * by other routines needing access to argv
	 */
	setup_a_symbol("___Argv", (ulong_t)argv);

	/* If the symbol .rtld.event is in the .dynsym symbol table
	 * of the a.out, then this means that it was compiled for
	 * profiling, prof.  A pointer to a function is set so that
	 * each time rtld updates the link_map it announces the
	 * change by calling the pointed-to function.  The definition
	 * of _rt_event in in libprof.a.
	 */
	if ((sym = _rt_lookup(".rtld.event", _rt_map_head, 
		0, 0, &lm, LOOKUP_NORM)) != 0)
	{
		void(**x)();
		x = (void(**)())sym->st_value;
		_rt_event = *x;
		if (!TEXTSTART(_rtld_map))
		{
			/* profiler needs size of text -
			 * make sure we set it for rtld, if
			 * we did not pick it up before
			 */
			if ((sym = _rt_lookup("_etext", _rtld_map, 
				0, 0, &lm, LOOKUP_NORM)) != 0)
			{
				TEXTSTART(_rtld_map) = ADDR(_rtld_map);
				TEXTSIZE(_rtld_map) = sym->st_value;
			}
		} 
	}

	/* support old a.outs that did not have copy relocations
	 * for __ctype - if we saw such a copy relocation,
	 * reloc code would have cleared CTL_COPY_CTYPE
	 */
	if (CONTROL(CTL_COPY_CTYPE))
	{
		Elf32_Sym	*aout_sym;
		Elf32_Sym	*rtld_sym;
		rt_map		*lm1, *lm2;
		CONST		char *ctname = "__ctype";

		if ((aout_sym = _rt_lookup(ctname, _rt_map_head, 
			0, 0, &lm1, LOOKUP_NORM)) != 0
			&& (rtld_sym = _rt_lookup(ctname, _rtld_map, 
			(rt_map *)NEXT(_rt_map_head), 
			(rt_map *)NEXT(_rt_map_head),
			&lm2, LOOKUP_NORM)) != 0)
		{
			if (aout_sym->st_size != rtld_sym->st_size) 
			{
				_rt_lasterr("%s: size mismatch (%d and %d) for %s\n",
					_rt_name, aout_sym->st_size, 
					rtld_sym->st_size, ctname);
					_rt_fatal_error();
			}
			_rt_memcpy((char *)aout_sym->st_value,
				NEWPTR(char, ADDR(lm2), rtld_sym->st_value),
				aout_sym->st_size);
		}
	}

	/* Call _rt_setaddr to redirect GOT entries for certain
	 * symbols linked with -Bsymbolic in rtld itself.
	 * First, unlink the link map for rtld, so it isn't 
	 * searched unnecessaily.
	 * We need only search objects loaded before rtld, since
	 * we know all these symbols are defined in rtld.
	 */
	save_lm = 0;
	for (lm = _rt_map_head; lm; lm = (rt_map *)NEXT(lm)) 
	{
		if (NEXT(lm) == (struct link_map *)_rtld_map)
		{
			save_lm = lm;
			break;
		}
	}
                        
	if (save_lm)
		NEXT(save_lm) = 0;

	/* set up errno */
	_rt_setaddr();

	/* reconnect the list */
	if (save_lm)
		NEXT(save_lm) = (struct link_map *)_rtld_map;
}
