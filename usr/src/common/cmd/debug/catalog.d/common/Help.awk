#ident	"@(#)debugger:catalog.d/common/Help.awk	1.3"

# This awk script reads the file of Command Line Interface help messages (cli.help)
# and generates a C file containing the table of help messages that
# is used by Help.C

BEGIN {
	in_help = 0
	FS = " "
}

{

	if ($1 == "++")
	{
		if (in_help)
		{
			in_help = 0
			printf "\",\n\n"
		}
		else
		{
			printf "\"\\n\\\n"
			in_help = 1
		}
	}
	else if ($1 == "##PRINT_AS_IS##")
	{
		getline
		while($1 != "##END_AS_IS##")
		{
			print $0
			getline
		}
	}
	else if (in_help)
	{
		n = split($0, a, "\\")
		newstr = a[1]
		for (i = 2; i <= n; i++)
			newstr = newstr "\\\\" a[i]

		n = split(newstr, a, "\"")
		newstr = a[1]
		for (i = 2; i <= n; i++)
			newstr = newstr "\\\"" a[i]

		n = split(newstr, a, "\t")
		newstr = a[1]
		for (i = 2; i <= n; i++)
			newstr = newstr "\\t" a[i]

		printf "%s\\n\\\n", newstr
	}
}
