#ident	"@(#)debugger:gui.d/i386/CoreInfo.h	1.3"

#include <filehdr.h>

class ELFcoreInfo
{
#ifndef GEMINI_ON_OSR5
	char	*note_data;	// note section data
	int	note_sz;	// note section data size
#endif
	char	*psargs;	// PRPSINFO entry within the note section
	char 	*obj_name;	// name of object that created this core

public:
		ELFcoreInfo(int fd, Elf_Ehdr *ehdr);
		~ELFcoreInfo();
	char	*get_obj_name();
};
