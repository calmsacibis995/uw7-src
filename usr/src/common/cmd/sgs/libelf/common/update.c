#ident	"@(#)libelf:common/update.c	1.20.5.3"


#ifdef __STDC__
	#pragma weak	elf_update = _elf_update
#endif


#include "syn.h"

#ifdef MMAP_IS_AVAIL
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/siginfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#endif

#include "libelf.h"
#include "decl.h"
#include "error.h"
#include <stdio.h>

/* Output file update
 *	These functions walk an Elf structure, update its information,
 *	and optionally write the output file.  Because the application
 *	may control of the output file layout, two upd_... routines
 *	exist.  They're similar but too different to merge cleanly.
 *
 *	The library defines a "dirty" bit to force parts of the file
 *	to be written on update.  These routines ignore the dirty bit
 *	and do everything.  A minimal update routine might be useful
 *	someday.
 */


static size_t	upd_lib	_((Elf *));
static size_t	upd_usr	_((Elf *));
static size_t	wrt	_((Elf *, size_t, unsigned));
static size_t   _elf_outsync    _((int, char *, size_t, unsigned));

#ifdef MMAP_IS_AVAIL
/*  This data is needed for the SIGBUS handler.  The SIGBUS handler
 *  is engaged when creating output files with elf_update().  The
 *  reason for impleneting a SIGBUS handler in libelf is due to the
 *  following problem.  Even if mmap creates the image of the file in 
 *  memory and succeeds there may not be enough room on the file system
 *  to hold the output file which can cause SIGBUS errors in an application.
 *  mmap does not allocate disk blocks for the file or attempt to determine 
 *  whether there are enough available.  As a result, when an application, 
 *  such as ld, attempts to write a section of the output file to this memory, 
 *  the system fails to get a disk block and the application receives a SIGBUS 
 *  signal and dies.
 *  
 *  Instead of having the application dump core with a SIGBUS error,
 *  libelf will provide a handler to allow a more friendlier interface.  
 *  The right solution is to fix mmap!
 */ 

static struct info {
	jmp_buf 	env;
	size_t		start;
	size_t		length;
	struct  	sigaction old_sig;
} map_info;

static void sigbus_handler	_((int, siginfo_t *, ucontext_t *));
#endif

off_t
elf_update(elf, cmd)
	Elf		*elf;
	Elf_Cmd		cmd;
{
	size_t		sz;
	unsigned	u;

	if (elf == 0)
		return -1;
	switch (cmd)
	{
	default:
		_elf_err = EREQ_UPDATE;
		return -1;

	case ELF_C_WRITE:
	case ELF_C_IMPURE_WRITE:
		if (elf->ed_myflags & EDF_COFFAOUT)
		{
			_elf_err = EREQ_COFFAOUT;
			return -1;
		}
		if ((elf->ed_myflags & EDF_WRITE) == 0)
		{
			_elf_err = EREQ_UPDWRT;
			return -1;
		}
		/*FALLTHRU*/
	case ELF_C_NULL:
		break;
	}
	if (elf->ed_ehdr == 0)
	{
		_elf_err = ESEQ_EHDR;
		return -1;
	}
	if ((u = elf->ed_ehdr->e_version) > EV_CURRENT)
	{
		_elf_err = EREQ_VER;
		return -1;
	}
	if (u == EV_NONE)
		elf->ed_ehdr->e_version = EV_CURRENT;
	if ((u = elf->ed_ehdr->e_ident[EI_DATA]) == ELFDATANONE)
	{
		if (_elf_encode == ELFDATANONE)
		{
			_elf_err = EREQ_ENCODE;
			return -1;
		}
		elf->ed_ehdr->e_ident[EI_DATA] = (Byte)_elf_encode;
	}
	u = 0x1;
	if (elf->ed_uflags & ELF_F_LAYOUT)
	{
		sz = upd_usr(elf);
		u = 0;
	}
	else
		sz = upd_lib(elf);
	if (sz != 0)
	{
		if (cmd == ELF_C_IMPURE_WRITE)
			u |= 0x2;
		else if (cmd != ELF_C_WRITE)
			return sz;
		if ((sz = wrt(elf, sz, u)) != 0)
			return sz;
	}
	return -1;
}


static size_t
upd_lib(elf)
	Elf		*elf;
{
	size_t		hi;
	Elf_Scn		*s;
	register size_t	sz;
	Elf32_Ehdr	*eh = elf->ed_ehdr;
	unsigned	ver = eh->e_version;

	/*	Ehdr and Phdr table go first
	 */

	{
		register char	*p = (char *)eh->e_ident;

		p[EI_MAG0] = ELFMAG0;
		p[EI_MAG1] = ELFMAG1;
		p[EI_MAG2] = ELFMAG2;
		p[EI_MAG3] = ELFMAG3;
		p[EI_CLASS] = ELFCLASS32;
		p[EI_VERSION] = (Byte)ver;
		hi = elf32_fsize(ELF_T_EHDR, 1, ver);
		eh->e_ehsize = (Elf32_Half)hi;
		if (eh->e_phnum != 0)
		{
			eh->e_phentsize = elf32_fsize(ELF_T_PHDR, 1, ver);
			eh->e_phoff = hi;
			hi += eh->e_phentsize * eh->e_phnum;
		}
		else
		{
			eh->e_phoff = 0;
			eh->e_phentsize = 0;
		}
	}

	/*	Loop through sections, skipping index zero.
	 *	Compute section size before changing hi.
	 *	Allow null buffers for NOBITS.
	 */

	if ((s = elf->ed_hdscn) == 0)
		eh->e_shnum = 0;
	else
	{
		eh->e_shnum = 1;
		*s->s_shdr = _elf_snode_init.sb_shdr;
		s = s->s_next;
	}
	for (; s != 0; s = s->s_next)
	{
		register Dnode	*d;
		register size_t	fsz, j;

		++eh->e_shnum;
		if (s->s_shdr->sh_type == SHT_NULL)
		{
			*s->s_shdr = _elf_snode_init.sb_shdr;
			continue;
		}
		s->s_shdr->sh_addralign = 1;
		if ((sz = _elf32_entsz(s->s_shdr->sh_type, ver)) != 0)
			s->s_shdr->sh_entsize = sz;
		sz = 0;
		for (d = s->s_hdnode; d != 0; d = d->db_next)
		{
			if ((fsz = elf32_fsize(d->db_data.d_type, 1, ver)) == 0)
				return 0;
			j = _elf32_msize(d->db_data.d_type, ver);
			fsz *= d->db_data.d_size / j;
			d->db_osz = fsz;
			if ((j = d->db_data.d_align) > 1)
			{
				if (j > s->s_shdr->sh_addralign)
					s->s_shdr->sh_addralign = j;
				if (sz % j != 0)
					sz += j - sz % j;
			}
			d->db_data.d_off = sz;
			sz += fsz;
		}
		s->s_shdr->sh_size = sz;
		j = s->s_shdr->sh_addralign;
		if ((fsz = hi % j) != 0)
			hi += j - fsz;
		s->s_shdr->sh_offset = hi;
		if (s->s_shdr->sh_type != SHT_NOBITS)
			hi += sz;
	}

	/*	Shdr table last
	 */

	if (eh->e_shnum != 0)
	{
		if (hi % ELF32_FSZ_WORD != 0)
			hi += ELF32_FSZ_WORD - hi % ELF32_FSZ_WORD;
		eh->e_shoff = hi;
		eh->e_shentsize = elf32_fsize(ELF_T_SHDR, 1, ver);
		hi += eh->e_shentsize * eh->e_shnum;
	}
	else
	{
		eh->e_shoff = 0;
		eh->e_shentsize = 0;
	}
	return hi;
}


static size_t
upd_usr(elf)
	Elf		*elf;
{
	size_t		hi;
	Elf_Scn		*s;
	register size_t	sz;
	Elf32_Ehdr	*eh = elf->ed_ehdr;
	unsigned	ver = eh->e_version;

	/*	Ehdr and Phdr table go first
	 */

	{
		register char	*p = (char *)eh->e_ident;

		p[EI_MAG0] = ELFMAG0;
		p[EI_MAG1] = ELFMAG1;
		p[EI_MAG2] = ELFMAG2;
		p[EI_MAG3] = ELFMAG3;
		p[EI_CLASS] = ELFCLASS32;
		p[EI_VERSION] = (Byte)ver;
		hi = elf32_fsize(ELF_T_EHDR, 1, ver);
		eh->e_ehsize = (Elf32_Half)hi;

		/*	If phnum is zero, phoff "should" be zero too,
		 *	but the application is responsible for it.
		 *	Allow a non-zero value here and update the
		 *	hi water mark accordingly.
		 */

		if (eh->e_phnum != 0)
			eh->e_phentsize = elf32_fsize(ELF_T_PHDR, 1, ver);
		else
			eh->e_phentsize = 0;
		if ((sz = eh->e_phoff + eh->e_phentsize * eh->e_phnum) > hi)
			hi = sz;
	}

	/*	Loop through sections, skipping index zero.
	 *	Compute section size before changing hi.
	 *	Allow null buffers for NOBITS.
	 */

	if ((s = elf->ed_hdscn) == 0)
		eh->e_shnum = 0;
	else
	{
		eh->e_shnum = 1;
		*s->s_shdr = _elf_snode_init.sb_shdr;
		s = s->s_next;
	}
	for (; s != 0; s = s->s_next)
	{
		register Dnode	*d;
		register size_t	fsz, j;

		++eh->e_shnum;
		sz = 0;
		for (d = s->s_hdnode; d != 0; d = d->db_next)
		{
			if ((fsz = elf32_fsize(d->db_data.d_type, 1, ver)) == 0)
				return 0;
			j = _elf32_msize(d->db_data.d_type, ver);
			fsz *= d->db_data.d_size / j;
			d->db_osz = fsz;
			if ((s->s_shdr->sh_type != SHT_NOBITS) &&
			((j = d->db_data.d_off + d->db_osz) > sz))
				sz = j;
		}
		if (s->s_shdr->sh_size < sz)
		{
			_elf_err = EFMT_SCNSZ;
			return 0;
		}
		if (s->s_shdr->sh_type != SHT_NOBITS
		&& hi < s->s_shdr->sh_offset + s->s_shdr->sh_size)
			hi = s->s_shdr->sh_offset + s->s_shdr->sh_size;
	}

	/*	Shdr table last.  Comment above for phnum/phoff applies here.
	 */

	if (eh->e_shnum != 0)
		eh->e_shentsize = elf32_fsize(ELF_T_SHDR, 1, ver);
	else
		eh->e_shentsize = 0;
	if ((sz = eh->e_shoff + eh->e_shentsize * eh->e_shnum) > hi)
		hi = sz;
	return hi;
}

typedef struct
{
	Elf_Data	data;
	char		*image;
	char		*buf;
	size_t		bufsz;
	size_t		cur;
	unsigned	encode;
	unsigned	flags;
	int		fd;
} Elf_Dest;

static Okay	trans	_((Elf_Dest *, const Elf_Data *, size_t));

static size_t
wrt(elf, outsz, flags)
	Elf		*elf;
	size_t		outsz;
	unsigned	flags;
{
	Elf_Data	src;
	Elf_Dest	dst;
	unsigned	mapflag;
	size_t		off;
	Elf_Scn		*s;
	Elf32_Ehdr	*eh = elf->ed_ehdr;

	/*	Two issues can cause trouble for the output file.
	 *	First, begin() with ELF_C_RDWR opens a file for both
	 *	read and write.  On the write update(), the library
	 *	has to read everything it needs before truncating
	 *	the file.  Second, using mmap for both read and write
	 *	is too tricky.  Consequently, the library disables mmap
	 *	on the read side.  Using mmap for the output saves swap
	 *	space, because that mapping is SHARED, not PRIVATE.
	 *
	 *	If the file is write-only, there can be nothing of
	 *	interest to bother with.
	 *
	 *	The following reads the entire file, which might be
	 *	more than necessary.  Better safe than sorry.
	 */

	if (elf->ed_myflags & EDF_READ
	&& _elf_vm(elf, (size_t)0, elf->ed_fsz) != OK_YES)
		return 0;

	/*	Prefer to hold the entire output file image in memory
	 *	but we try to work around the lack below if not enough
	 *	memory can be found.
	 */

	dst.buf = 0;
	mapflag = 0;

	/*  Attemp to write the output file.
	 *  On SVR4 and newer systems use mmap(2). On older systems (or on
	 *  file systems that don't support mmap), use malloc(2).  Or, if 
	 *  the mmap fails, attempt to use malloc.
	 */

#ifdef MMAP_IS_AVAIL

        if (_elf_svr4()
        && ftruncate(elf->ed_fd, (off_t)outsz) == 0
        && (dst.image = mmap((char *)0, outsz, PROT_READ+PROT_WRITE,
                        MAP_SHARED, elf->ed_fd, (off_t)0)) != (char *)-1)
        {
		struct	sigaction	psig;
		psig.sa_handler = sigbus_handler;
		psig.sa_flags = SA_SIGINFO;
		sigemptyset(&psig.sa_mask);
		sigaction(SIGBUS, &psig, &map_info.old_sig);

                mapflag = 1;
		map_info.start = (size_t)dst.image;
		map_info.length = outsz;

		if (setjmp(map_info.env) !=0){
                	mapflag = 0;
                	munmap(dst.image,outsz);
		}
        }
#endif
	if ((mapflag == 0) && ((dst.image = malloc(outsz)) == 0)){
                _elf_err = EMEM_OUT;
		dst.bufsz = 0;
		dst.cur = 1;	/* anything > 0 forces initial lseek() */
		dst.fd = elf->ed_fd;
	}

	/*	If an error occurs below, a "dirty" bit may be cleared
	 *	improperly.  To save a second pass through the file,
	 *	this code sets the dirty bit on the elf descriptor
	 *	when an error happens, assuming that will "cover" any
	 *	accidents.
	 */

	/*	Ehdr first
	 */

	src.d_version = EV_CURRENT;
	dst.data.d_version = eh->e_version;
	dst.encode = eh->e_ident[EI_DATA];
	dst.flags = flags;

	src.d_buf = (Elf_Void *)eh;
	src.d_type = ELF_T_EHDR;
	src.d_size = sizeof(Elf32_Ehdr);
	dst.data.d_size = eh->e_ehsize;
	if (trans(&dst, &src, (size_t)0) != OK_YES)
		goto bad;
	elf->ed_ehflags &= ~ELF_F_DIRTY;

	/*	Phdr table if one exists
	 */

	if (eh->e_phnum != 0)
	{
		/*	Unlike other library data, phdr table is 
		 *	in the user version.  Change src buffer
		 *	version here, fix it after translation.
		 */

		src.d_buf = (Elf_Void *)elf->ed_phdr;
		src.d_type = ELF_T_PHDR;
		src.d_size = elf->ed_phdrsz;
		src.d_version = _elf_work;
		dst.data.d_size = eh->e_phnum * eh->e_phentsize;
		if (trans(&dst, &src, eh->e_phoff) != OK_YES)
			goto bad;
		elf->ed_phflags &= ~ELF_F_DIRTY;
		src.d_version = EV_CURRENT;
	}

	/*	Loop through sections
	 */

	for (s = elf->ed_hdscn; s != 0; s = s->s_next)
	{
		register Dnode	*d, *prevd;

		/*	Just "clean" DIRTY flag for "empty" sections.  Even if
		 *	NOBITS needs padding, the next thing in the
		 *	file will provide it.  (And if this NOBITS is
		 *	the last thing in the file, no padding needed.)
		 */

		off = s->s_shdr->sh_offset;
		d = s->s_hdnode;
		if (s->s_shdr->sh_type == SHT_NOBITS
		|| s->s_shdr->sh_type == SHT_NULL)
		{
			for (; d != 0; d = d->db_next)
				d->db_uflags &= ~ELF_F_DIRTY;
			continue;
		}
		for (prevd = 0; d != 0; prevd = d, d = d->db_next)
		{
			d->db_uflags &= ~ELF_F_DIRTY;
			if ((d->db_myflags & DBF_READY) == 0
			&& elf_getdata(s, &prevd->db_data) != &d->db_data)
				goto bad;
			dst.data.d_size = d->db_osz;
			if (trans(&dst, &d->db_data,
				off + d->db_data.d_off) != OK_YES)
			{
				goto bad;
			}
		}
	}

	/*	Shdr table last
	 */

	src.d_type = ELF_T_SHDR;
	src.d_size = sizeof(Elf32_Shdr);
	dst.data.d_size = eh->e_shentsize;
	off = eh->e_shoff;
	for (s = elf->ed_hdscn; s != 0; s = s->s_next)
	{
		s->s_shflags &= ~ELF_F_DIRTY;
		s->s_uflags &= ~ELF_F_DIRTY;
		src.d_buf = (Elf_Void *)s->s_shdr;
		if (trans(&dst, &src, off) != OK_YES)
			goto bad;
		off += dst.data.d_size;
	}

	if (dst.image == 0
	|| _elf_outsync(elf->ed_fd, dst.image, outsz, mapflag) != 0)
		elf->ed_uflags &= ~ELF_F_DIRTY;
	else
	{
bad:
		outsz = 0;
		elf->ed_uflags |= ELF_F_DIRTY;
	}
	if (dst.buf != 0)
		free(dst.buf);
#ifdef MMAP_IS_AVAIL
	if (mapflag)
	sigaction(SIGBUS, &map_info.old_sig, 0); /* restore the older 
							signal handler */
#endif
	return outsz;
}

#ifdef MMAP_IS_AVAIL

static void
sigbus_handler(sig, info_p, cntxt_p)
	int sig;
	siginfo_t *info_p;
	ucontext_t *cntxt_p;
{

        size_t	addr;

        addr = (size_t) info_p->si_addr;

	sigaction(SIGBUS, &map_info.old_sig, 0); /* restore the older
							signal handler */

        /* if value is in the mmap'ed range, then longjmp */
        if (addr >= map_info.start && addr < map_info.start + map_info.length)
                longjmp(map_info.env, 1);

        /* bus error for some other reason */
	kill(getpid(), SIGBUS);
}
#endif

static Okay
trans(dp, sp, off)
	Elf_Dest	*dp;
	const Elf_Data	*sp;
	size_t		off;
{
	/*	Get positioned, filling a forward gap if appropriate
	 */

	if (off != dp->cur)
	{
		if ((dp->flags & 0x1) && off > dp->cur)
		{
			size_t sz = off - dp->cur;

			if (dp->image != 0)
				(void)memset(dp->image + dp->cur, _elf_byte, sz);
			else
			{
				char tbuf[128], *tp;
				size_t n, tsz;

				if (sz > 128 && dp->bufsz > 128)
				{
					tp = dp->buf;
					tsz = dp->bufsz;
				}
				else
				{
					tp = tbuf;
					tsz = 128;
				}
				do
				{
					if ((n = sz) > tsz)
						n = tsz;
					(void)memset(tp, _elf_byte, n);
					if (write(dp->fd, tp, n) != n)
					{
						_elf_err = EIO_WRITE;
						return OK_NO;
					}
				} while ((sz -= n) != 0);
			}
		}
		else if (dp->image == 0)
		{
			if (lseek(dp->fd, (off_t)off, SEEK_SET) != (off_t)off)
			{
				_elf_err = EIO_SEEK;
				return OK_NO;
			}
		}
	}

	/*	Update our eventual location and reject empty blocks
	 */

	dp->cur = off + dp->data.d_size;
	if (dp->data.d_size == 0)
		return OK_YES;

	/*	Choose target buffer and translate to the file version
	 */

	if (dp->image != 0)
		dp->data.d_buf = (Elf_Void *)(dp->image + off);
	else if ((dp->flags & 0x2) && sp->d_size >= dp->data.d_size)
		dp->data.d_buf = sp->d_buf;	/*ELF_C_IMPURE_WRITE*/
	else
	{
		if (dp->bufsz < dp->data.d_size)
		{
			if (dp->buf != 0)
				free(dp->buf);
			if ((dp->buf = malloc(dp->bufsz = dp->data.d_size)) == 0)
			{
				_elf_err = EMEM_OUT;
				return OK_NO;
			}
		}
		dp->data.d_buf = (Elf_Void *)dp->buf;
	}
	if (elf32_xlatetof(&dp->data, sp, dp->encode) == 0)
		return OK_NO;

	/*	Write it out if the entire image is not in memory
	 */

	if (dp->image == 0
	&& write(dp->fd, dp->data.d_buf, dp->data.d_size) != dp->data.d_size)
	{
		_elf_err = EIO_WRITE;
		return OK_NO;
	}

	return OK_YES;
}


/*ARGSUSED*/
static size_t
_elf_outsync(fd, p, sz, flag)
	int		fd;
	char		*p;
	size_t		sz;
	unsigned	flag;
{

#ifdef MMAP_IS_AVAIL
	if (flag != 0)
	{
		fd = msync(p, sz, MS_ASYNC);
		(void)munmap(p, sz);
		if (fd == 0)
			return sz;
		_elf_err = EIO_SYNC;
		return 0;
	}
#endif
	if (lseek(fd, 0L, 0) != 0
	|| write(fd, p, sz) != sz)
	{
		_elf_err = EIO_WRITE;
		sz = 0;
	}
	free(p);
	return sz;
}
