#ident	"@(#)libelf:common/elf_str.awk	1.1"

# Convert error description file to C.
# Input has the following form (fields separated by tabs):
#
#	12345678901234567890...
#	# comment
#	#ident	"string"
#
#	MAJOR_ENUM	MAJOR_DATA	Error prefix
#		MINOR_ENUM	message text
#
# Example
#
#	
#	BUG	bug	Internal error
#		BUG1	Goofed up data structure
#		BUG2	Messed up pointer
#
#	FMT	fmt	Format error
#		...

BEGIN	{
		major = "_0"
		FS = "\t"
		MAJOR_ENUM = "_0"
		MAJOR_DATA = "_1"
		MAJOR_NUMB = "_2"
		MAJOR_PREF = "_3"
		MAJOR_MINOR = "_4"
		MINOR_ENUM = "_0"
		MINOR_TEXT = "_1"
	}


/^[A-Za-z]/	{
		++major
		info[major MAJOR_PREF] = $3
		info[major MAJOR_NUMB] = 0
	}

/^	[A-Za-z]/ {
		j = ++info[major MAJOR_NUMB];
		info[major MAJOR_MINOR j MINOR_TEXT] = $3
	}

END	{
		printf "ELF error 0\n"
		printf "Unknown ELF error\n"
		for (j = 1; j <= major; ++j)
		{
			x = j MAJOR_PREF
			printf "%s: reason unknown\n", info[x]
			for (k = 1; k <= info[j MAJOR_NUMB]; ++k)
			{
				printf "%s: %s\n",\
					info[j MAJOR_PREF],\
					info[j MAJOR_MINOR k MINOR_TEXT] 
			}
		}

	}

