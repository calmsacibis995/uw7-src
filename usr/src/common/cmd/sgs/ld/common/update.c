#ident	"@(#)ld:common/update.c	1.32"

#include	<unistd.h>
#include	"globals.h"

/* elf_update assigned offsets to output sections and individual
 * data buffers; we now need to assign virtual addresses and
 * fill in segment descriptors based on those offsets.
 * The task is complicated by the fact that segments can either
 * grow up from lower addresses or down from higher addresses.
 * We break the task into 3 parts:
 * 1st, figure out the sizes of all segments.
 * 2nd, figure out beginning virtual addresses for loadable segments.
 * 3rd, assign individual section addresses based on segment addresses.
 */

/* returns number of segments in segment list */
static int
calculate_sizes_and_offsets()
{
	Listnode	*np1, *np2;
	Sg_desc		*sgp;
	Os_desc		*osp;
	Phdr		*phdr;
	int		nsegs = 0;

	for(LIST_TRAVERSE(&seg_list, np1, sgp)) {
		Off	offPHDR; /* offset of first loadable seg */
		Off	cur_fileoff;
		Off	cur_memoff = 0;

		nsegs++;
		phdr = &(sgp->sg_phdr);
		switch(phdr->p_type) {
		case PT_PHDR:
			if (dmode && !Gflag) {
				phdr->p_offset = outfile_ehdr->e_phoff;
				phdr->p_filesz = phdr->p_memsz = 
					sizePHDR;
				offPHDR = phdr->p_offset;
				continue;
			}
			break;
		case PT_INTERP:
			sgp->sg_phdr.p_memsz = 0;
			if (interp_sect != NULL) {
				sgp->sg_phdr.p_offset = 
					interp_sect->is_outsect_ptr->
					os_shdr->sh_offset;
				sgp->sg_phdr.p_filesz = 
					interp_sect->is_outsect_ptr->
					os_shdr->sh_size;
			}
			continue;
		case PT_DYNAMIC:
			if (dmode)
				continue;
			break;
		case PT_SHLIB:
			continue;
		default:
			break;
		}

		if (phdr->p_align == 0) {
			/* alignment not set in mapfile */
			phdr->p_align = CHUNK_SIZE;
		}

		if (sgp->sg_osectlist.head == NULL) {
			continue;
		}

		/* get segment offset from offset of first 
		 * outsection in segment 
		 */

		osp = (Os_desc*)(sgp->sg_osectlist.head->data);
		if (dmode && !Gflag && (phdr->p_type == PT_LOAD) && 
			offPHDR) {
			/* if first loadable segment, 
			 * adjust by size of program headers 
			 */
			phdr->p_offset = offPHDR;
			offPHDR = (Off)0;
		} else {
			phdr->p_offset = osp->os_shdr->sh_offset;
		}

		cur_fileoff = cur_memoff = phdr->p_offset;
		if (cur_fileoff != osp->os_shdr->sh_offset)
			/* phdr included */
			cur_memoff += osp->os_shdr->sh_offset 
				- cur_fileoff;
		for(LIST_TRAVERSE(&(sgp->sg_osectlist),np2,osp)) {
			Boolean	is_alloc, is_nobits;
	
			is_alloc = (dmode || aflag) &&
				(osp->os_shdr->sh_flags & SHF_ALLOC);
			is_nobits = osp->os_shdr->sh_type == SHT_NOBITS;
			cur_fileoff = osp->os_shdr->sh_offset;
				/* elf_update has done alignment */
			if (!is_nobits)
				cur_fileoff += osp->os_shdr->sh_size;
			if (is_alloc) {
				cur_memoff = ROUND(cur_memoff, 
					osp->os_shdr->sh_addralign);
				cur_memoff += osp->os_shdr->sh_size;
			}
		}
		phdr->p_filesz = cur_fileoff - phdr->p_offset;
		phdr->p_memsz = cur_memoff - phdr->p_offset;
		if (sgp->sg_sizesym != NULL) {
			sgp->sg_sizesym->ls_syment->st_value = 
				phdr->p_memsz ? phdr->p_memsz :
				phdr->p_filesz;
		}
		if (sgp->sg_flags & (1 << SGA_LENGTH)) {
			if (sgp->sg_length < phdr->p_memsz)
				lderror(MSG_FATAL, gettxt(":1144",
				"segment %s calculated size larger than user-defined size"),
					sgp->sg_name);
		}
	}
	return nsegs;
}

/* calculate segment virtual addresses - go through list
 * twice; first go forward through list and get all "up"
 * segments; then go backward and get all down segments
 */
static void
calculate_virtual_addresses(int nsegs)
{
	Addr		cur_vaddr;
	Addr		cur_paddr;
	Phdr		*phdr;
	Listnode	*np1;
	Sg_desc		*sgp;
	extern	char	*Mflag;
	Sg_desc		**list;
	Sg_desc		**lptr;
	Sg_desc		*first_exec = NULL;
	Sg_desc		*prev_seg;
	Os_desc		*osp;

	/* build a list in reverse order */
	list = (Sg_desc **)mymalloc(sizeof(Sg_desc *) * (nsegs+1));
	lptr = &list[nsegs];
	*lptr = NULL;

	if ((dmode && Gflag) || (!dmode && !aflag))
		cur_vaddr = 0;
	else
		cur_vaddr = firstseg_origin;
	cur_paddr = 0;

	for(LIST_TRAVERSE(&seg_list, np1, sgp)) {
		lptr--;
		*lptr = sgp;

		phdr = &(sgp->sg_phdr);

		switch(phdr->p_type) {
		case PT_PHDR:
			if (dmode && !Gflag) {
				phdr->p_vaddr = 
					cur_vaddr + phdr->p_offset;
				continue;
			}
		case PT_INTERP:
			sgp->sg_phdr.p_vaddr = 0;
			continue;

		case PT_DYNAMIC:
			if (dmode)
				continue;
			break;
		case PT_SHLIB:
			continue;
		default:
			break;
		}

		if ( sgp->sg_osectlist.head == NULL) {
			continue;
		}

		if ((phdr->p_flags & PF_X) && !first_exec)
			first_exec = sgp;

		if (sgp->sg_downseg)
			continue;

		if (sgp->sg_flags & (1 << SGA_VADDR)) {
			/* virtual addr assigned in map file */
			if (cur_vaddr != 0 && cur_vaddr 
				!= firstseg_origin &&
				cur_vaddr > phdr->p_vaddr)
				lderror(MSG_WARNING,
					gettxt(":1143",
					"user defined segment addresses overlap; previous segment overlaps segment named: %s, current address = %#x, segment address = %#x"),
					sgp->sg_name,
					(unsigned long)cur_vaddr,
					(unsigned long)phdr->p_vaddr);
			cur_vaddr = phdr->p_vaddr;
		} else {
			/* virtual addresses and file offsets are
			 * congruent modulo the alignment
			 */
			cur_vaddr = ROUND(cur_vaddr, phdr->p_align); 
			if (!(sgp->sg_flags & (1 << SGA_XALIGN)))
				cur_vaddr += 
					phdr->p_offset % phdr->p_align;
			phdr->p_vaddr = cur_vaddr;
		}
		
		if (sgp->sg_flags & (1 << SGA_PADDR)) {
			if (cur_paddr != 0 && cur_paddr > phdr->p_paddr)
				lderror(MSG_WARNING, gettxt(":1143",
				"user defined segment addresses overlap; previous segment overlaps segment named: %s, current address = %#x, segment address = %#x"),
					sgp->sg_name,
					(unsigned long)cur_paddr,
					(unsigned long)phdr->p_paddr);
			cur_paddr = phdr->p_paddr;
		} else if (Mflag) {
			cur_paddr = ROUND(cur_paddr, phdr->p_align); 
			if (!(sgp->sg_flags & (1 << SGA_XALIGN)))
				cur_paddr += 
					phdr->p_offset % phdr->p_align;
			phdr->p_paddr = cur_paddr;
		}
		cur_vaddr += phdr->p_memsz;
		cur_paddr += phdr->p_memsz;
	}
	/* now go backward and get down segments */
	cur_vaddr = cur_paddr = (Addr)~0;

	sgp = NULL;
	for(lptr = list; *lptr; lptr++) {
		prev_seg = sgp;
		sgp = *lptr;

		phdr = &(sgp->sg_phdr);
		if ((phdr->p_type != PT_LOAD) ||
			(sgp->sg_osectlist.head == NULL)) {
			continue;
		}

		if (!sgp->sg_downseg) {
			if (prev_seg && prev_seg->sg_downseg) {
				if ((cur_vaddr != (Addr)~0) &&
					(cur_vaddr < (phdr->p_vaddr +
						phdr->p_memsz)))
					lderror(MSG_WARNING, gettxt(":1143",
					"user defined segment addresses overlap; previous segment overlaps segment named: %s, current address = %#x, segment address = %#x"),
					sgp->sg_name,
					(unsigned long)cur_vaddr,
					(unsigned long)phdr->p_vaddr);
				if ((cur_paddr != (Addr)~0) &&
					(cur_paddr < (phdr->p_paddr +
						phdr->p_memsz)))
					lderror(MSG_WARNING, gettxt(":1143",
					"user defined segment addresses overlap; previous segment overlaps segment named: %s, current address = %#x, segment address = %#x"),
					sgp->sg_name,
					(unsigned long)cur_paddr,
					(unsigned long)phdr->p_paddr);
			}
			cur_vaddr = phdr->p_vaddr;
			cur_paddr = phdr->p_paddr;
			continue;
		}

		osp = (Os_desc*)(sgp->sg_osectlist.head->data);

		if (sgp->sg_flags & (1 << SGA_VADDR)) {
			if ((cur_vaddr != (Addr)~0) &&
				(phdr->p_vaddr >= cur_vaddr))
				lderror(MSG_WARNING, gettxt(":1143",
				"user defined segment addresses overlap; previous segment overlaps segment named: %s, current address = %#x, segment address = %#x"),
					sgp->sg_name,
					(unsigned long)cur_vaddr,
					(unsigned long)phdr->p_vaddr);
			cur_vaddr = phdr->p_vaddr - phdr->p_memsz;
			cur_vaddr = TRUNCATE(cur_vaddr, 
				osp->os_shdr->sh_addralign);
			phdr->p_vaddr = cur_vaddr;
		} else {
			cur_vaddr = TRUNCATE(cur_vaddr, phdr->p_align)
				- phdr->p_memsz;
			if (sgp->sg_flags & (1 << SGA_XALIGN))
				cur_vaddr = TRUNCATE(cur_vaddr, 
					osp->os_shdr->sh_addralign);
			else {
				Addr	t;
				t = TRUNCATE(cur_vaddr, phdr->p_align) +
					(phdr->p_offset%phdr->p_align);
				if (t > cur_vaddr)
					t -= phdr->p_align;
				cur_vaddr = t;
			}
			phdr->p_vaddr = cur_vaddr;
		}

		if (sgp->sg_flags & (1 << SGA_PADDR)) {
			if ((cur_paddr != (Addr)~0) &&
				(phdr->p_paddr >= cur_paddr))
				lderror(MSG_WARNING,gettxt(":1143",
				"user defined segment addresses overlap; previous segment overlaps segment named: %s, current address = %#x, segment address = %#x"),
					sgp->sg_name,
					(unsigned long)cur_paddr,
					(unsigned long)phdr->p_paddr);
			cur_paddr = phdr->p_paddr - phdr->p_memsz;
			cur_paddr = TRUNCATE(cur_paddr, 
				osp->os_shdr->sh_addralign);
			phdr->p_paddr = cur_paddr;
		} else if (Mflag) {
			cur_paddr = TRUNCATE(cur_paddr, phdr->p_align)
				- phdr->p_memsz;
			if (sgp->sg_flags & (1 << SGA_XALIGN))
				cur_paddr = TRUNCATE(cur_paddr, 
					osp->os_shdr->sh_addralign);
			else {
				Addr	t;
				t = TRUNCATE(cur_paddr, phdr->p_align) +
					(phdr->p_offset%phdr->p_align);
				if (t > cur_paddr)
					t -= phdr->p_align;
				cur_paddr = t;
			}
			phdr->p_paddr = cur_paddr;
		}
	}
	free(list);

	/* save address of first executable segment for default
	 * use in build_ophdr() 
	 */
	if (first_exec)
		firstexec_seg = first_exec->sg_phdr.p_vaddr;
}

static void
assign_section_addresses()
{
	Addr		cur_vaddr;
	Off		cur_offset;
	Listnode	*np1, *np2, *np3;
	Sg_desc		*sgp;
	Insect		*isp;
	Phdr		*phdr;
	Os_desc		*osp;
	Sg_desc		*dynseg = NULL;
	Sg_desc		*shlibseg = NULL;

	for(LIST_TRAVERSE(&seg_list, np1, sgp)) {
		phdr = &(sgp->sg_phdr);
		if (sgp->sg_basesym)
			sgp->sg_basesym->ls_syment->st_value = 
				phdr->p_vaddr;

		switch(phdr->p_type) {
		case PT_PHDR:
			continue;
		case PT_DYNAMIC:
			if (dmode) {
				dynseg = sgp;
				continue;
			}
			break;
		case PT_SHLIB:
			shlibseg = sgp;
			continue;
		default:
			break;
		}

		if (sgp->sg_osectlist.head == NULL) {
			continue;
		}

		cur_vaddr = phdr->p_vaddr;

		osp = (Os_desc *)(sgp->sg_osectlist.head->data);
		if (phdr->p_offset != osp->os_shdr->sh_offset)
			/* phdr included */
			cur_vaddr += (osp->os_shdr->sh_offset - 
				phdr->p_offset);

		for(LIST_TRAVERSE(&(sgp->sg_osectlist),np2,osp)) {
			Boolean		is_alloc, is_nobits;
			Addr		bss_vaddr;
	
			is_alloc = (dmode || aflag) &&
				(osp->os_shdr->sh_flags & SHF_ALLOC);
			is_nobits = osp->os_shdr->sh_type == SHT_NOBITS;
			if (is_alloc) {
				cur_vaddr = ROUND(cur_vaddr,
					osp->os_shdr->sh_addralign);
				osp->os_shdr->sh_addr = cur_vaddr;
				if (is_nobits)
					bss_vaddr = cur_vaddr;
			}
			cur_offset = osp->os_shdr->sh_offset;
				/* elf_update has done alignment */

			for(LIST_TRAVERSE(&(osp->os_insects),np3,isp)) {
				Elf_Data	*data;

				data = isp->is_outdata;
				isp->is_newFOffset = data->d_off +
					cur_offset;
				isp->is_displ = data->d_off;
				if (!is_alloc) 
					continue;
				if (is_nobits) {
					bss_vaddr = ROUND(bss_vaddr,
						isp->is_shdr->sh_addralign);
					isp->is_newVAddr = bss_vaddr;
					bss_vaddr += 
						isp->is_shdr->sh_size;
				} else
					isp->is_newVAddr = 
						cur_vaddr + data->d_off;
			}
			cur_vaddr += osp->os_shdr->sh_size;
		}
		if (phdr->p_type == PT_NOTE) {
			sgp->sg_phdr.p_align = 0;
			sgp->sg_phdr.p_memsz = 0;
			sgp->sg_phdr.p_vaddr = 0;
			sgp->sg_phdr.p_paddr = 0;
		}
	}
	if ((dynseg != NULL) && (dynamic_sect != NULL)) {
		dynseg->sg_phdr.p_vaddr = dynamic_sect->is_newVAddr;
		dynseg->sg_phdr.p_offset = dynamic_sect->is_newFOffset;
		dynseg->sg_phdr.p_filesz =
			dynamic_sect->is_rawbits->d_size;
		dynseg->sg_phdr.p_flags = PF_R | PF_W | PF_X;
	}

	if (shlibseg->sg_osectlist.head != NULL) {
		shlibseg->sg_phdr.p_offset = ((Os_desc *)shlibseg->sg_osectlist.head->data)->os_shdr->sh_offset;
		shlibseg->sg_phdr.p_filesz = ((Os_desc *)shlibseg->sg_osectlist.head->data)->os_shdr->sh_size;
	}
}

void
set_off_addr()
{
	int	nsegs;

	nsegs = calculate_sizes_and_offsets();
	calculate_virtual_addresses(nsegs);
	assign_section_addresses();
}
