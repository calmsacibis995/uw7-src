#ident	"@(#)debugger:catalog.d/common/GMsg.awk	1.6"

# This awk script reads the Message table description file GMsg.awk.in
# and creates three new files:
#
# gui_msg.h
#	enum declaration of the types of messages
#	There is one entry for each non-comment line in Msg.awk.in
#
# GMtable.c
#	The initialization code for the message table,
#	one line per Message type.  Each line looks like:
#		{catalog number, 0, format string}, /* Gui_msg_id */
#
# GMcatalog
#	Message text, one per line.  This is the input to mkmsgs,
#	which transforms it into the gui-specific message catalog, debug.gui
#
# We implement a simple conditional inclusion mechanism;
# it supports #define foo; #ifdef foo; #ifndef foo; 
# #else; #endif; #undef foo
# The values for foo off by default;  we read a file
# called ../../awk.cond at startup to process any
# outside defines.
#
# The format for entries in GMsg.awk.in is:
# msg_id	message text	parameter description	extra comments

# !!! NOTE: the field separator is a single tab !!!
# !!! Messages may NOT contain embedded tabs (\t is okay) !!!

# The first two fields are required and specify the
# msg id as used in the gui_msg enumeration in the gui
# and the printf format string for the message.
# The third and fourth fields appear as comment lines
# in the mkmsgs message catalog we generate (GMcatalog).
# The third field contains a description of each parameter
# in a message.  The fourth field, if present, is printed
# as an extra line of commentary.  Missing fields should
# be replaced by a single "-".

BEGIN {
	cur_index = 0
	modes[cur_index] = "on"
	exit_status = 0
	FS="\t"
	f_tab_c = "GMtable.c"
	f_msg_h = "gui_msg.h"
	f_cat = "GMcatalog"

	# print the necesary header information to each file
	print "/* file produced by ../../catalog.d/common/GMsg.awk */\n" >f_msg_h
	print "#ifndef _GUI_MSG_H"	>f_msg_h
	print "#define _GUI_MSG_H\n"	>f_msg_h
	print "enum Gui_msg_id\n{"		>f_msg_h
	print "\tGM_none = 0,"	>f_msg_h

	print "/* file produced by ../../catalog.d/common/GMsg.awk\n */"  >f_tab_c
	print "#include \"gui_msg.h\"" > f_tab_c
	print "struct GM_tab gmtable[] = {" > f_tab_c
	print "{ 0, 0 },\t/* GM_none */" >f_tab_c

	# ------ WARNING -------
	# Messages in an existing catalog cannot be modified or removed,
	# because we have no control over the translated catalogs,
	# also, calls to gettxt have hard-coded numbers in them.
	# Messages MUST stay in the same order.
	next_num = 1			# next catalog entry
}

# main loop
# the command line in the makefile is
# 	awk -f GMsg.awk GMsg.awk.in

{
	# The only lines we are interested in are the ones that
	# start with GM_ or GE_
	# or the conditional lines #ifdef, #define, #else, #endif,
	# #undef, #ifndef
	if ($1 == "#define")
	{
		if (modes[cur_index] == "on")
			defines[$2] = 1
		next
	}
	else if ($1 == "#undef")
	{
		if (modes[cur_index] == "on")
			defines[$2] = 0
		next
	}
	else if ($1 == "#ifdef")
	{
		if (modes[cur_index] == "on")
		{
			cur_index++
			if (defines[$2] == 1)
			{
				modes[cur_index] = "on"
			}
			else
			{
				modes[cur_index] = "off"
			}
		}
		else
		{
			cur_index++
			modes[cur_index] = "skip"
		}
		next
	}
	else if ($1 == "#ifndef")
	{
		if (modes[cur_index] == "on")
		{
			cur_index++
			if (defines[$2] == 0)
			{
				modes[cur_index] = "on"
			}
			else
			{
				modes[cur_index] = "off"
			}
		}
		else
		{
			cur_index++
			modes[cur_index] = "skip"
		}
		next
	}
	else if ($1 == "#else")
	{
		if (cur_index <= 0)
		{
			print("Error: #else with no matching #ifdef")
			exit_status = 1
			exit exit_status
		}
		if (modes[cur_index] == "on")
		{
			# ifdef was true - now skip #else part
			modes[cur_index] = "off"
		}
		else if (modes[cur_index] == "off")
		{
			# ifdef was false - now process #else part
			modes[cur_index] = "on"
		}
		next
	}
	else if ($1 == "#endif")
	{
		if (cur_index <= 0)
		{
			print("Error: #endif with no matching #ifdef")
			exit_status = 1
			exit exit_status
		}
		cur_index--
		next
	}
	else if (modes[cur_index] != "on")
		next

	if (substr($1, 1, 3) != "GM_" && substr($1, 1, 3) != "GE_")
		next

	msg_num = next_num++
	printf "\t%s,\n", $1 >f_msg_h
	printf "{ %d, 0, \t\"", msg_num >f_tab_c
	mid = $1
	fstring = $2

	if ($3 != "-")
		printf "%%# %d;WIDTH=0;%s\n", msg_num, $3 > f_cat
	else
		printf "%%# %d;WIDTH=0;\n", msg_num > f_cat

	if ($4 != "-")
		printf "%%# %s\n", $4 > f_cat

	printf "%s\"},\t/* %s */\n", fstring, mid > f_tab_c
	print fstring > f_cat
}

END {
	if (exit_status != 0)
		exit exit_status
	if (cur_index > 0)
	{
		print("Error: #ifdef with no matching #endif")
		exit 1
	}
	# finish off the message table files
	print "\tGM_last\n};\n"					>f_msg_h
	print "struct GM_tab\n{"				>f_msg_h
	print "\tshort catindex;"					>f_msg_h
	print "\tshort set_local;"					>f_msg_h
	print "\tconst char *string;"				>f_msg_h
	print "};\n"						>f_msg_h
	print "extern struct GM_tab gmtable[];\n"		>f_msg_h
	print "const char *gm_format(enum Gui_msg_id);\n"	>f_msg_h
	print "#endif // _GUI_MSG_H"				>f_msg_h

	print "{0}\t/* GM_last */\n};" >f_tab_c
}
