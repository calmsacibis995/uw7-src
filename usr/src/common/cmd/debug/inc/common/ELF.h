#ifndef _ELF_h
#define _ELF_h

#ident	"@(#)debugger:inc/common/ELF.h	1.2"

#include "Iaddr.h"
#include "Object.h"
#include "Machine.h"
#include <sys/types.h>
#include <sys/elf.h>


// maintain information about an ELF object
// and provide access to needed sections

class Seginfo;

class ELF : public Object {
	int		phdrnum;
	Elf_Phdr	*phdr;
	int		read_section(Sect_type, Elf_Shdr *, int noread);
	int		map_sections(Sect_type, Elf_Shdr *, Sect_type, 
				Elf_Shdr *);
public:
			ELF( int fdobj, dev_t, ino_t, time_t );
			~ELF() { delete phdr ;}
	Seginfo		*get_seginfo(int &count, int &isshared);
	int		find_symbol( const char *name, Iaddr &addr );
	int		get_phdr(int &num, Elf_Phdr *&);
	void		reset_phdr(Elf_Phdr *, int num);
};

#endif
