#ident	"@(#)libelf:i386/cofftab.h	1.2"


void	_elf_coff386_flg _((Elf *, Info *, unsigned));
int	_elf_coff386_opt _((Elf *, Info *, char *, size_t));
int	_elf_coff386_rel _((Elf *, Info *, Elf_Scn *, Elf_Scn *, Elf_Data *));
void	_elf_coff386_shdr _((Elf *, Info *, Elf32_Shdr *, SCNHDR *));
int	_elf_coff386_note _((Elf *, Info *));

