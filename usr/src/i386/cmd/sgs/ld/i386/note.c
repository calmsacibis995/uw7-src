#ident	"@(#)ld:i386/note.c	1.2"

#include <unistd.h>
#include <ctype.h>
#include "globals.h"
#include "elfid.h"

#ifndef NO_OSR5_SUPPORT
enum platform_type {
	platform_unknown,		/* not yet determined */
	platform_open_server,		/* all OpenServer ELF */
	platform_force_open_server,	/* input may be mixed;
					 * output OpenServer ELF
					 */
	platform_iabi,			/* all Intel ABI ELF */
	platform_force_iabi,		/* input may be mixed;
					 * output Intel ABI ELF
					 */
};

static enum platform_type platform = platform_unknown;
static Insect	*open_server_note_sect;
static unsigned char	assert_udk_binary;

/* create output relocation section */
static void
build_open_server_note_sect()
{
	register Insect	*nsect;
	Word		*wp;

	nsect = NEWZERO(Insect);
	nsect->is_shdr = NEWZERO(Shdr);
	nsect->is_shdr->sh_type = SHT_NOTE;
	nsect->is_shdr->sh_size = NT_ELFID_SZ;
	nsect->is_shdr->sh_addralign = 0;
	nsect->is_shdr->sh_entsize = 0;
	nsect->is_name = (char*)mymalloc(sizeof(".note"));
	strcpy(nsect->is_name, ".note");
	nsect->is_rawbits = NEWZERO(Elf_Data);
	nsect->is_rawbits->d_buf = (char *)mymalloc(NT_ELFID_SZ);
	nsect->is_rawbits->d_type = ELF_T_BYTE;
	nsect->is_rawbits->d_size = NT_ELFID_SZ;
	nsect->is_rawbits->d_align = 0;
	nsect->is_rawbits->d_version = libver;
	wp = (Word *)nsect->is_rawbits->d_buf;
	*wp++ = NT_NAME_SZ;
	*wp++ = ELFID_DESC_SZ;
	*wp++ = NT_ELFID;
	(void)memset((void *)wp, '\0', NT_NAME_PAD);
	(void)strncpy((void *)wp, NT_NAME, NT_NAME_SZ);
	wp = (Word *)((char *)wp + NT_NAME_PAD);
	*wp++ = ELFID_MK_VERS(ELFID_MAJOR, ELFID_MINOR);
	*wp++ = 0;	/* source - fill in later */
	*wp = 0;

	place_section(nsect);
	open_server_note_sect = nsect;
}

static void
set_note_source_type(source)
Word source;
{
	Word	*wp;
	if (!open_server_note_sect)
		return;
	wp = (Word *)open_server_note_sect->is_rawbits->d_buf;
	wp += 3; /* name_sz, desc_size, id */
	wp = (Word *)((char *)wp + NT_NAME_PAD); /* name */
	wp++;	/* version */
	*wp |= (source & ELFID_SRC_MASK);
}

/* check_note checks an input note section
 * to see if it is the special note section
 * denoting an OSR5 ELF or converted COFF file.
 * If it is, check_note returns TRUE, otherwise FALSE.
 */
Boolean
check_note(scn, shdr, data)
Elf_Scn	*scn;
Shdr	*shdr;
Elf_Data *data;
{
	Word	*wp;
	Boolean	coff = FALSE;
	Word	version;
	Word	source;

	if (shdr->sh_size != NT_ELFID_SZ || data->d_size != NT_ELFID_SZ)
		return FALSE;
	wp = (Word *)data->d_buf;
	if (*wp++ != NT_NAME_SZ ||
		*wp++ != ELFID_DESC_SZ ||
		*wp++ != NT_ELFID )
		return FALSE;
	if (strncmp((char *)wp, NT_NAME, NT_NAME_SZ) != 0)
		return FALSE;
	wp = (Word *)((char *)wp + NT_NAME_PAD);
	version = *wp++;
	if ((ELFID_GET_MAJOR(version) > ELFID_MAJOR)
		|| (ELFID_GET_MINOR(version) > ELFID_MINOR))
		return FALSE;
	source = *wp & ELFID_SRC_MASK;
	/* we have an OSR5 note section - check this against
	 * desired platform type
	 */
	switch(platform)
	{
	case platform_unknown:
		if (assert_udk_binary)
			lderror(MSG_FATAL,
				gettxt(":1648","the -f udk flag cannot be asserted for an OpenServer binary"));
		platform = platform_open_server;
		break;
	case platform_open_server:
	case platform_force_open_server:
		break;
	case platform_iabi:
		lderror(MSG_FATAL,
			gettxt(":1615","cannot link OpenServer object into Intel iABI target"));
	case platform_force_iabi:
		return TRUE;
	}
	if (!open_server_note_sect)
		build_open_server_note_sect();
	set_note_source_type(source);
	return TRUE;
}

/* no OpenServer note section found; take
 * appropriate actions.
 */
void
no_note_found()
{
	switch(platform)
	{
	case platform_unknown:
		platform = platform_iabi;
		break;
	case platform_iabi:
	case platform_force_iabi:
		break;
	case platform_force_open_server:
		if (!open_server_note_sect)
			build_open_server_note_sect();
		set_note_source_type(ELFID_SRC_IABI_ELF);
		break;
	case platform_open_server:
		lderror(MSG_FATAL,
			gettxt(":1614","cannot link Intel iABI object into OpenServer target"),
			cur_infile_ptr->fl_name);
	}
}

#define MAX_FORCE_NAME	20

int
force_platform(opt)
CONST char *opt;
{
	char	name[MAX_FORCE_NAME];
	char	*str;
	int	i;

	str = name;
	for(i = 0; *opt && i < MAX_FORCE_NAME; i++)
	{
		*str++ = toupper(*opt++);
	}
	*str = 0;
	str = name;
	while(*str == ' ')	
		str++;
	if ((strcmp(str, "OSRVR") == 0) ||
		(strcmp(str, "OPENSERVER") == 0) ||
		(strcmp(str, "OPEN_SERVER") == 0) ||
		(strcmp(str, "OSR5") == 0))
	{
		if (assert_udk_binary)
			lderror(MSG_FATAL,
				gettxt(":1648","the -f udk flag cannot be asserted for an OpenServer binary"));
		platform = platform_force_open_server;
	}
	else if ((strcmp(str, "IABI") == 0) ||
		(strcmp(str, "ABI") == 0))
	{
		platform = platform_force_iabi;
	}
	else if (strcmp(str, "UDK") == 0)
	{
		if (platform == platform_force_open_server)
			lderror(MSG_FATAL,
				gettxt(":1648","the -f udk flag cannot be asserted for an OpenServer binary"));
		assert_udk_binary = TRUE;
		ehdr_flags = UDK_FIX_FLAG;
	}
	else
	{
		return 0;
	}
	return 1;
}
#else
/*ARGSUSED*/
Boolean
check_note(scn, shdr, data)
Elf_Scn	*scn;
Shdr	*shdr;
Elf_Data *data;
{
	return FALSE;
}

/*ARGSUSED*/
int 
force_platform(opt)
const char *opt;
{
	return 1;
}
void
no_note_found()
{
	return;
}
#endif
